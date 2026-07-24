#include "crashhandler.hpp"
#include "../libs/files/log/logger.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <new>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <dbghelp.h>
#  pragma comment(lib, "dbghelp.lib")
#else
#  include <execinfo.h>
#  include <unistd.h>
#  include <signal.h>
#endif

namespace OpenCK {

namespace {

constexpr int kMaxFrames = 64;

void logStackTraceFromContext()
{
#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, nullptr, TRUE))
    {
        LOG_ERROR("Stack trace: SymInitialize failed");
        return;
    }

    void* stack[kMaxFrames];
    USHORT numFrames = CaptureStackBackTrace(0, kMaxFrames, stack, nullptr);

    SYMBOL_INFO* symbol = static_cast<SYMBOL_INFO*>(
        calloc(sizeof(SYMBOL_INFO) + 256, 1));
    if (!symbol)
    {
        SymCleanup(process);
        LOG_ERROR("Stack trace: calloc failed");
        return;
    }
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (USHORT i = 0; i < numFrames; ++i)
    {
        DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);
        DWORD displacement = 0;
        if (SymFromAddr(process, address, nullptr, symbol))
        {
            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(line);
            DWORD lineDisplacement = 0;
            const char* fileLine = "";
            if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line))
            {
                fileLine = line.FileName;
            }
            LOG_ERROR(QString("  #%1 0x%2 %3 (%4:%5) +0x%6")
                .arg(i)
                .arg(address, 0, 16)
                .arg(QString::fromLocal8Bit(symbol->Name))
                .arg(QString::fromLocal8Bit(fileLine))
                .arg(line.LineNumber)
                .arg(displacement, 0, 16));
        }
        else
        {
            LOG_ERROR(QString("  #%1 0x%2 (no symbol)")
                .arg(i)
                .arg(address, 0, 16));
        }
    }
    free(symbol);
    SymCleanup(process);
#else
    void* array[kMaxFrames];
    int size = backtrace(array, kMaxFrames);
    char** strings = backtrace_symbols(array, size);
    if (strings)
    {
        for (int i = 0; i < size; ++i)
        {
            LOG_ERROR(QString("  #%1 %2").arg(i).arg(QString::fromLocal8Bit(strings[i])));
        }
        free(strings);
    }
    else
    {
        LOG_ERROR("Stack trace: backtrace_symbols failed");
    }
#endif
}

void terminateHandler()
{
    std::exception_ptr ex = std::current_exception();
    if (ex)
    {
        try
        {
            std::rethrow_exception(ex);
        }
        catch (const std::exception& e)
        {
            LOG_FATAL(QString("Unhandled std::exception: %1 (%2)")
                .arg(QString::fromLocal8Bit(e.what()))
                .arg(QString::fromLocal8Bit(typeid(e).name())));
        }
        catch (...)
        {
            LOG_FATAL("Unhandled non-std exception");
        }
    }
    else
    {
        LOG_FATAL("std::terminate called (no current exception)");
    }
    writeStackTrace("terminate");
    std::_Exit(2);
}

#ifdef _WIN32
LONG WINAPI exceptionHandler(EXCEPTION_POINTERS* ex)
{
    if (ex && ex->ExceptionRecord)
    {
        const auto* rec = ex->ExceptionRecord;
        const char* kind = "UNKNOWN";
        switch (rec->ExceptionCode)
        {
            case EXCEPTION_ACCESS_VIOLATION:       kind = "ACCESS_VIOLATION";       break;
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:  kind = "ARRAY_BOUNDS_EXCEEDED";  break;
            case EXCEPTION_BREAKPOINT:             kind = "BREAKPOINT";             break;
            case EXCEPTION_DATATYPE_MISALIGNMENT:  kind = "DATATYPE_MISALIGNMENT";  break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:     kind = "FLT_DIVIDE_BY_ZERO";     break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:    kind = "ILLEGAL_INSTRUCTION";    break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:     kind = "INT_DIVIDE_BY_ZERO";     break;
            case EXCEPTION_STACK_OVERFLOW:         kind = "STACK_OVERFLOW";         break;
            case EXCEPTION_PRIV_INSTRUCTION:       kind = "PRIV_INSTRUCTION";       break;
            case EXCEPTION_GUARD_PAGE:             kind = "GUARD_PAGE";             break;
            case 0xC0000005:                       kind = "ACCESS_VIOLATION";       break;
            default: break;
        }
        LOG_FATAL(QString("Win32 exception %1 (0x%2) at address 0x%3")
            .arg(QString::fromLocal8Bit(kind))
            .arg(rec->ExceptionCode, 8, 16, QChar('0'))
            .arg(reinterpret_cast<quintptr>(rec->ExceptionAddress), 0, 16));
        if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            // The first exception parameter holds the read/write flag for
            // access violations (0=read, 1=write, 8=execute).
            const auto accessType = rec->NumberParameters >= 1
                ? rec->ExceptionInformation[0] : 0ULL;
            const auto accessAddr = rec->NumberParameters >= 2
                ? rec->ExceptionInformation[1] : 0ULL;
            const char* op = (accessType == 1) ? "write"
                            : (accessType == 8) ? "execute"
                            : "read";
            LOG_FATAL(QString("  ACCESS_VIOLATION %1 of address 0x%2")
                .arg(QString::fromLocal8Bit(op))
                .arg(accessAddr, 0, 16));
        }
    }
    else
    {
        LOG_FATAL("Win32 exception with null ExceptionRecord");
    }
    writeStackTrace("win32-exception");
    return EXCEPTION_EXECUTE_HANDLER;
}

void signalHandlerWin32(int sig)
{
    const char* name = (sig == SIGSEGV) ? "SIGSEGV"
                     : (sig == SIGABRT) ? "SIGABRT"
                     : (sig == SIGFPE)  ? "SIGFPE"
                     : (sig == SIGILL)  ? "SIGILL"
                     : "UNKNOWN";
    LOG_FATAL(QString("POSIX signal %1 on Windows").arg(QString::fromLocal8Bit(name)));
    writeStackTrace(name);
    // Best effort: the structured exception handler in the same
    // process has already attached for the vectored-exception code
    // path, so we re-raise as a SEGV-equivalent so the OS takes us
    // out. Using std::abort() would still write minidump if a
    // debugger is attached; we let the default disposition win.
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}
#else
void posixSignalHandler(int sig, siginfo_t*, void*)
{
    const char* name = (sig == SIGSEGV) ? "SIGSEGV"
                     : (sig == SIGABRT) ? "SIGABRT"
                     : (sig == SIGFPE)  ? "SIGFPE"
                     : (sig == SIGILL)  ? "SIGILL"
                     : (sig == SIGBUS)  ? "SIGBUS"
                     : "UNKNOWN";
    LOG_FATAL(QString("POSIX signal %1").arg(QString::fromLocal8Bit(name)));
    writeStackTrace(name);
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}
#endif

} // namespace

void writeStackTrace(const char* reason)
{
    if (reason)
    {
        LOG_FATAL(QString("Stack trace requested: %1").arg(QString::fromLocal8Bit(reason)));
    }
    else
    {
        LOG_FATAL("Stack trace requested");
    }
    logStackTraceFromContext();
}

void installCrashHandlers()
{
    std::set_terminate(terminateHandler);

#ifdef _WIN32
    SetUnhandledExceptionFilter(exceptionHandler);
    std::signal(SIGSEGV, signalHandlerWin32);
    std::signal(SIGABRT, signalHandlerWin32);
    std::signal(SIGFPE,  signalHandlerWin32);
    std::signal(SIGILL,  signalHandlerWin32);
#else
    struct sigaction sa{};
    sa.sa_sigaction = posixSignalHandler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE,  &sa, nullptr);
    sigaction(SIGILL,  &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);
#endif

    LOG_INFO("Crash handlers installed (terminate, structured-exception, signal)");
}

} // namespace OpenCK

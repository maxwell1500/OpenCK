#ifndef CRASH_HANDLER_HPP
#define CRASH_HANDLER_HPP

namespace OpenCK {

// Installs platform-specific signal handlers and (on Windows) a
// vectored exception handler so that a crash (SIGSEGV, SIGABRT,
// unhandled C++ exception, structured Windows exception) writes a
// stack trace to the log file before the process terminates.
//
// All handlers are best-effort and safe to call from within a
// signal context (no allocation, no Qt calls beyond the logger
// which has its own mutex).
void installCrashHandlers();

// Writes a stack trace of the current thread to the log file.
// Safe to call from main thread; on signal context it falls back
// to a minimal unbacktraceable entry.
void writeStackTrace(const char* reason);

} // namespace OpenCK

#endif // CRASH_HANDLER_HPP

#ifndef SHAREDFILEDEVICE_H
#define SHAREDFILEDEVICE_H

#include <QIODevice>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#endif

class SharedFileDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit SharedFileDevice(QObject* parent = nullptr)
        : QIODevice(parent),
          mHandle(INVALID_HANDLE_VALUE),
          mSize(0)
    {
    }

    explicit SharedFileDevice(const QString& fileName, QObject* parent = nullptr)
        : QIODevice(parent),
          mHandle(INVALID_HANDLE_VALUE),
          mSize(0),
          mFileName(fileName)
    {
    }

    ~SharedFileDevice()
    {
        if (mHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mHandle);
            mHandle = INVALID_HANDLE_VALUE;
        }
    }

    bool openFile(const QString& fileName)
    {
#ifdef _WIN32
        std::wstring wpath = fileName.toStdWString();
        mHandle = CreateFileW(wpath.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
        if (mHandle == INVALID_HANDLE_VALUE)
        {
            mHandle = INVALID_HANDLE_VALUE;
            mSize = 0;
            return false;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(mHandle, &fileSize))
        {
            CloseHandle(mHandle);
            mHandle = INVALID_HANDLE_VALUE;
            mSize = 0;
            return false;
        }

        mSize = fileSize.QuadPart;
        return true;
#else
        return false;
#endif
    }

    bool open(QIODevice::OpenMode mode) override
    {
        if (mode & QIODevice::WriteOnly)
        {
            return false;
        }

        bool result = openFile(mFileName);
        if (result)
        {
            setOpenMode(mode);
        }
        return result;
    }

    QString fileName() const
    {
        return mFileName;
    }

    qint64 pos() const override
    {
        if (mHandle == INVALID_HANDLE_VALUE)
        {
            return -1;
        }

        LARGE_INTEGER currentPos;
        currentPos.QuadPart = 0;
        LARGE_INTEGER fileOffset;
        if (SetFilePointerEx(mHandle, currentPos, &fileOffset, FILE_CURRENT))
        {
            return fileOffset.QuadPart;
        }

        return -1;
    }

    qint64 size() const override
    {
        return mSize;
    }

    bool seek(qint64 pos) override
    {
        if (mHandle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        LARGE_INTEGER fileOffset;
        fileOffset.QuadPart = pos;
        if (SetFilePointerEx(mHandle, fileOffset, nullptr, FILE_BEGIN))
        {
            return true;
        }

        return false;
    }

    void close() override
    {
        if (mHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mHandle);
            mHandle = INVALID_HANDLE_VALUE;
        }
        setOpenMode(NotOpen);
    }

protected:
    qint64 readData(char* data, qint64 maxLen) override
    {
        if (mHandle == INVALID_HANDLE_VALUE || data == nullptr || maxLen <= 0)
        {
            return -1;
        }

        DWORD bytesRead = 0;
        if (!ReadFile(mHandle, data, static_cast<DWORD>(maxLen), &bytesRead, nullptr))
        {
            return -1;
        }

        return static_cast<qint64>(bytesRead);
    }

    qint64 writeData(const char* data, qint64 maxLen) override
    {
        Q_UNUSED(data)
        Q_UNUSED(maxLen)
        return -1;
    }

    bool atEnd() const override
    {
        return pos() >= mSize;
    }

    bool isSequential() const override
    {
        return false;
    }

private:
    HANDLE mHandle;
    qint64 mSize;
    QString mFileName;
};

#endif // SHAREDFILEDEVICE_H

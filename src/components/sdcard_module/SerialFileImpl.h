#include <Arduino.h>
#include <memory>
#include <ctime>
#include <FSImpl.h>
#include <FS.h>

namespace fs {

class SerialFileImpl : public FileImpl {
private:
    String _path;
    bool _isOpen;
    size_t _fileSize;
    size_t _currentPosition;
    bool _isDirectory;

public:
    SerialFileImpl(const String& path, bool isDirectory = false)
        : _path(path), _isOpen(false), _fileSize(0), _currentPosition(0), _isDirectory(isDirectory) { }

    virtual ~SerialFileImpl() { close(); }

    virtual size_t write(const uint8_t *buf, size_t size) override {
        if (!_isOpen) return 0;

        Serial.print("WRITE ");
        Serial.print(_path);
        Serial.print(" ");
        Serial.println(size);

        // Send the data
        Serial.write(buf, size);

        if (waitForResponse("WRITE_OK")) {
            _currentPosition += size;
            _fileSize = max(_fileSize, _currentPosition);
            return size;
        }

        return 0;
    }

    virtual size_t read(uint8_t* buf, size_t size) override {
        if (!_isOpen) return 0;

        Serial.print("READ ");
        Serial.print(_path);
        Serial.print(" ");
        Serial.println(size);

        if (waitForResponse("READ_OK")) {
            size_t bytesRead = Serial.readBytes(buf, size);
            _currentPosition += bytesRead;
            return bytesRead;
        }

        return 0;
    }

    virtual void flush() override {
        if (_isOpen) {
            Serial.print("FLUSH ");
            Serial.println(_path);
            waitForResponse("FLUSH_OK");
        }
    }

    virtual bool seek(uint32_t pos, SeekMode mode) override {
        if (!_isOpen) return false;

        Serial.print("SEEK ");
        Serial.print(_path);
        Serial.print(" ");
        Serial.print(pos);
        Serial.print(" ");
        Serial.println(static_cast<int>(mode));

        if (waitForResponse("SEEK_OK")) {
            _currentPosition = pos;
            return true;
        }
        return false;   
    }

    virtual size_t position() const override {
        return _currentPosition;
    }

    virtual size_t size() const override {
        return _fileSize;
    }

    virtual bool setBufferSize(size_t size) override {
        Serial.print("SET_BUFFER ");
        Serial.print(_path);
        Serial.print(" ");
        Serial.println(size);

        return waitForResponse("BUFFER_OK");
    }

    virtual void close() override {
        if (_isOpen) {
            Serial.print("CLOSE ");
            Serial.println(_path);
            waitForResponse("CLOSE_OK");
            _isOpen = false;
        }
    }

    virtual time_t getLastWrite() override {
        Serial.print("GET_LAST_WRITE ");
        Serial.println(_path);

        if (waitForResponse("TIME_OK")) {
            return static_cast<time_t>(Serial.parseInt());
        }

        return 0;
    }

    virtual const char* path() const override {
        return _path.c_str();
    }

    virtual const char* name() const override {
        return strrchr(_path.c_str(), '/') + 1;
    }

    virtual boolean isDirectory(void) override {
        return _isDirectory;
    }

    virtual FileImplPtr openNextFile(const char* mode) override {
        Serial.print("OPEN_NEXT_FILE ");
        Serial.print(_path);
        Serial.print(" ");
        Serial.println(mode);

        if (waitForResponse("FILE_OPENED")) {
            String nextFilePath = Serial.readStringUntil('\n');
            return std::make_shared<SerialFileImpl>(nextFilePath);
        }

        return nullptr;
    }

    virtual boolean seekDir(long position) override {
        if (!_isDirectory) return false;

        Serial.print("SEEK_DIR ");
        Serial.print(_path);
        Serial.print(" ");
        Serial.println(position);

        return waitForResponse("SEEK_OK");
    }

    virtual String getNextFileName(void) override {
        Serial.print("GET_NEXT_FILE_NAME ");
        Serial.println(_path);

        if (waitForResponse("NAME_OK")) {
            return Serial.readStringUntil('\n');
        }

        return String();
    }

    virtual String getNextFileName(bool *isDir) override {
        Serial.print("GET_NEXT_FILE_NAME ");
        Serial.println(_path);

        if (waitForResponse("NAME_OK")) {
            String fileName = Serial.readStringUntil('\n');
            String dirFlag = Serial.readStringUntil('\n');
            *isDir = (dirFlag == "DIR");
            return fileName;
        }

        return String();
    }

    virtual void rewindDirectory(void) override {
        Serial.print("REWIND_DIR ");
        Serial.println(_path);
        waitForResponse("REWIND_OK");
    }

    virtual operator bool() override {
        return _isOpen;
    }

private:
    bool waitForResponse(const String& expectedResponse, unsigned long timeout = 5000) {
        unsigned long startTime = millis();
        while (millis() - startTime < timeout) {
            if (Serial.available()) {
                String response = Serial.readStringUntil('\n');
                if (response.startsWith(expectedResponse)) {
                    return true;
                }
            }
        }
        return false;
    }
};

}
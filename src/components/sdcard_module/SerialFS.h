
#include <Arduino.h>
#include <FSImpl.h>
#include <FS.h>
#include "SerialFileImpl.h"

namespace fs
{


class SerialFSImpl : public FSImpl {
public:
    SerialFSImpl() : FSImpl() {}

    // Override open function
    FileImplPtr open(const char* path, const char* mode, const bool create) override {
        // Send command to open the file
        Serial1.print("OPEN ");
        Serial1.print(path);
        Serial1.print(" ");
        Serial1.print(mode);
        Serial1.print(create ? " CREATE" : "");
        Serial1.println();

        if (waitForResponse("FILE_OPENED")) {
            return std::make_shared<SerialFileImpl>(String(path));
        }

        return nullptr;
    }

    bool exists(const char* path) override {
        Serial1.print("EXISTS ");
        Serial1.println(path);

        return waitForResponse("FILE_EXISTS");
    }

    bool remove(const char* path) override {
        Serial1.print("REMOVE ");
        Serial1.println(path);

        return waitForResponse("FILE_REMOVED");
    }

    bool rename(const char* pathFrom, const char* pathTo) override {
        Serial1.print("RENAME ");
        Serial1.print(pathFrom);
        Serial1.print(" ");
        Serial1.println(pathTo);

        return waitForResponse("FILE_RENAMED");
    }

    bool mkdir(const char *path) override {
        Serial1.print("MKDIR ");
        Serial1.println(path);

        return waitForResponse("DIR_CREATED");
    }

    bool rmdir(const char *path) override {
        Serial1.print("RMDIR ");
        Serial1.println(path);
        return waitForResponse("DIR_REMOVED");
    }

private:
    bool waitForResponse(const String& expectedResponse, unsigned long timeout = 5000) {
        unsigned long startTime = millis();
        while (millis() - startTime < timeout) {
            if (Serial1.available()) {
                String response = Serial1.readStringUntil('\n');
                if (response.startsWith(expectedResponse)) {
                    return true;
                }
            }
        }
        return false;
    }
};

class SerialFS : public FS {
public:
    SerialFS() : FS(FSImplPtr(new SerialFSImpl())) {}
};

}
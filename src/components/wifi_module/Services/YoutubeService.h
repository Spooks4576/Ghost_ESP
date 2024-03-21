#ifndef YOUTUBE_SERVICE_H
#define YOUTUBE_SERVICE_H

#include "IRemoteService.h"
#include <ArduinoJson.h>

class RID {
private:
    uint32_t number;
    
public:
    RID() {
        reset();
    }

    void reset() {
        number = random(10000, 99999);
    }

    uint32_t next() {
        return ++number;
    }
};

class YouTubeService : public IRemoteService {
public:
    void BindSessionID(Device& device) override;
    void sendCommand(const String& command, const String& videoId, const Device& device) override;
    String getToken(const String& id) override;

    RID rid;
    String generateUUID();
    String extractJSON(const String& Response);
    String zx();
    const char* ServerAddress = "www.youtube.com";
    const int port = 443;
    const char* BindEndpoint = "/api/lounge/bc/bind";
    const char* GetLoungeTokenEndpoint = "/api/lounge/pairing/get_lounge_token_batch";
};

#endif // YOUTUBE_SERVICE_H
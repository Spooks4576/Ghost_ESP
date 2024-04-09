
#include "esptinyusb.h"

#pragma once

#ifdef CFG_TUD_MIDI

class MIDIusb : public EspTinyUSB
{
public:
    MIDIusb();
    bool begin(char* str = nullptr);
    int available(void);
    int peek(void) { return -1; }
    int read(void) { return -1; }
    size_t read(uint8_t *buffer, size_t size) { return 0; }
    void flush(void) { return; }
    size_t write(uint8_t) { return 0; }
    size_t write(const uint8_t *buffer, size_t size) { return 0; }

    bool setSong(uint8_t* song, size_t len);
    void playSong();

    void noteON(uint8_t note, uint8_t velocity, uint8_t channel = 0);
    void noteOFF(uint8_t note, uint8_t velocity = 0, uint8_t channel = 0);
    void polyKey(uint8_t note, uint8_t pressure, uint8_t channel = 0);
    void controlChange(uint8_t controller, uint8_t value, uint8_t channel = 0);
    void programChange(uint8_t program, uint8_t channel = 0);
    void channelPresure(uint8_t presure, uint8_t channel = 0);
    void pitchChange(uint16_t value, uint8_t channel = 0);
    void setBaseEP(uint8_t);

private:
    uint8_t* _song;
    size_t _len;
    uint8_t _EPNUM_MIDI;
};
#endif

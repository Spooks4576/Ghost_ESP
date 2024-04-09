#include "midiusb.h"
#define EPNUM_MIDI 0x05
#if CFG_TUD_MIDI

MIDIusb::MIDIusb()
{
    enableMIDI = true;
    _EPNUM_MIDI = EPNUM_MIDI;
}

void MIDIusb::setBaseEP(uint8_t ep)
{
    _EPNUM_MIDI = ep;
}

bool MIDIusb::begin(char* str)
{
    // Interface number, string index, EP Out & EP In address, EP size
    uint8_t midi[] = {TUD_MIDI_DESCRIPTOR(1, 8, _EPNUM_MIDI, (uint8_t)(0x80 | _EPNUM_MIDI), 64)};
    memcpy(&desc_configuration[total], midi, sizeof(midi));
    total += sizeof(midi);
    count += 2;
    return EspTinyUSB::begin(str, 8);
}

void MIDIusb::noteON(uint8_t note, uint8_t velocity, uint8_t channel)
{
    log_v("ON: %d\n", note);
    uint8_t buf[] = {0x90, note, velocity};
    tud_midi_stream_write(channel, buf, 3);
}

void MIDIusb::noteOFF(uint8_t note, uint8_t velocity, uint8_t channel)
{
    log_v("OFF: %d\n", note);
    uint8_t buf[] = {0x80, note, velocity};
    tud_midi_stream_write(channel, buf, 3);
}

void MIDIusb::polyKey(uint8_t note, uint8_t pressure, uint8_t channel)
{
    uint8_t buf[] = {0xa0, note, pressure};
    tud_midi_stream_write(channel, buf, 3);
}

void MIDIusb::controlChange(uint8_t controller, uint8_t value, uint8_t channel)
{
    uint8_t buf[] = {0xb0, controller, value};
    tud_midi_stream_write(channel, buf, 3);
}

void MIDIusb::programChange(uint8_t program, uint8_t channel)
{
}

void MIDIusb::channelPresure(uint8_t presure, uint8_t channel)
{
}

void MIDIusb::pitchChange(uint16_t value, uint8_t channel)
{
}

typedef struct
{
    uint8_t type[4];
    uint32_t length;
    uint16_t format;
    uint16_t tracks;
    uint16_t division;
} mthd_t;

typedef struct
{
    uint8_t type[4];
    uint8_t *data;
} mtrk_t;

bool MIDIusb::setSong(uint8_t *song, size_t len)
{
    _len = len - 36;
    mthd_t mthd;
    mtrk_t mtrk;
    if (strstr((char*)song, "MThd") != NULL)
    {
        _song = (uint8_t*)strstr((char*)song, "MThd");
        memcpy(&mthd, _song, sizeof(mthd));
        log_v("MThd format => %d\n", mthd.format);
        if(mthd.format > 0) return false;
    }

    if (strstr((char*)song, "MT") != NULL)
    {
        _song = (uint8_t*)strstr((char*)_song, "MTrk");
        Serial.println("MTrk");
    }
    do{
        song++;
    }while(song[0] != 0x90);
    if(song == NULL) return false;
    song--;

    log_v("song => %x, i = %x\n", *(song), *song);
    _song = (uint8_t*)calloc(_len, 1);
    memcpy(_song, song, _len);
    return true;
}

uint8_t parseMeta(uint8_t* _song, size_t i)
{
    uint8_t n = 0;
    if (_song[i + 1] == 0x0){
        n = 3;
    }else if (_song[i + 1] == 0x03){
        Serial.println(_song[i + 3]);    
        n = _song[i+2] + 3;
    } else if (_song[i + 1] <= 0x07){
        n = _song[i+2] + 3;
    } else if (_song[i + 1] == 0x20){ // channel prefix
        n = 4;
    } else if (_song[i + 1] == 0x2f){ // end track
        n = 3;
    } else if (_song[i + 1] == 0x51){ //
        n = 3 + 3;
    } else if (_song[i + 1] == 0x54){
        n = 3 + 5;
    } else if (_song[i + 1] == 0x58){
        n = 3 + 4;
    } else if (_song[i + 1] == 0x59){
        n = 3 + 2;
    } else if (_song[i + 1] == 0x7f){
        n = _song[i+2] + 3;
    }

    log_v("meta => %x, n = %x\n", _song[i+1], n - 1);

    return n - 1;
}

void MIDIusb::playSong()
{
    uint8_t delta;
    uint8_t _byte;
    if(_song == nullptr) return;
    log_v("play");

    for (size_t i = 0; i < _len; i++)
    {
        delta = _song[i];
        _byte = _song[++i];
        log_v("delta => %x, _byte = %x\n", delta, _byte);
        if(delta) delay(delta * 8);
        if ((_byte & 0xff) == 0xff){ // meta event
            i += parseMeta(_song, i);
        }
        else if ((_byte & 0xe0) == 0xe0){ // pitch wheel
            i += 2;
            log_v("pitch");
        }
        else if ((_byte & 0xd0) == 0xd0){ // pressure
            i += 1;
            log_v("pressure");
        }
        else if ((_byte & 0xc0) == 0xc0){ // prog change
            i += 1;
            log_v("prog");
        }
        else if ((_byte & 0xb0) == 0xb0){ // ctrl change
            controlChange(_song[i + 1], _song[i + 2]);
            i += 2;
            log_v("ctrl");
        }
        else if ((_byte & 0xa0) == 0xa0){ // poly key
            polyKey(_song[i + 1], _song[i + 2]);
            i += 2;
            log_v("poly");
        }
        else if ((_byte & 0x90) == 0x90){ // note on
            noteON(_song[i + 1], _song[i + 2]);
            i += 2;
            log_v("on");
        }
        else if ((_byte & 0x80) == 0x80){ // note off
            noteOFF(_song[i + 1], _song[i + 2]);
            i += 2;
            log_v("off");
        } else {
            Serial.printf("unknown: %d\n", _song[i]);
        }
    }
}

void tud_midi_rx_cb(uint8_t itf)
{
    uint8_t _mid[4] = {0};
    if(tud_midi_packet_read(_mid)) {
      for (size_t i = 0; i < 4; i++)
      {
        Serial.printf("%02x ", _mid[i]);      
      }
      Serial.println();
    } else log_e("failed to receive");
}

int MIDIusb::available()
{
    return tud_midi_available();
}

#endif

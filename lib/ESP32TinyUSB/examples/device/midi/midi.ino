/**
 * Simple example MIDI class
 * author: chegewara
 */
#include "midiusb.h"
#include "song.h"
// #ifdef CFG_TUD_MIDI

MIDIusb midi;

void setup() {
    Serial.begin(115200);
    midi.begin();
    delay(1000);
  if(midi.setSong(song, song_len)) {
    Serial.println("we can play now");
    midi.playSong();
  }
}


// Variable that holds the current position in the sequence.
uint32_t note_pos = 0;

// Store example melody as an array of note values
uint8_t note_sequence[] =
{
  74,78,81,86,90,93,98,102,57,61,66,69,73,78,81,85,88,92,97,100,97,92,88,85,81,78,
  74,69,66,62,57,62,66,69,74,78,81,86,90,93,97,102,97,93,90,85,81,78,73,68,64,61,
  56,61,64,68,74,78,81,86,90,93,98,102
};

void midi_task(void)
{
  static uint32_t start_ms = 0;

  // send note every 1000 ms
  if (millis() - start_ms < 286) return; // not enough time
  start_ms += 286;

  // Previous positions in the note sequence.
  int previous = note_pos - 1;

  // If we currently are at position 0, set the
  // previous position to the last note in the sequence.
  if (previous < 0) previous = sizeof(note_sequence) - 1;

  // Send Note On for current position at full velocity (127) on channel 1.
  midi.noteON(note_sequence[note_pos], 127);

  // Send Note Off for previous note.
  midi.noteOFF(note_sequence[previous]);

  // Increment position
  note_pos++;

  // If we are at the end of the sequence, start over.
  if (note_pos >= sizeof(note_sequence)) note_pos = 0;
}

void loop() {
  // midi_task();
  // midi.playSong();
}

// #endif

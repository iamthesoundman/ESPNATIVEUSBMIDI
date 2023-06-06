/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

//MAKE SURE IF USING ARDUINO IDE TO SET USB MODE TO TINYUSB (Tools->USB Mode)

/* This sketch is enumerated as USB MIDI device.
 * !!USES ESPNATIVEUSBMIDI implementation
 * Following library is required
 * - MIDI Library by Forty Seven Effects
 *   https://github.com/FortySevenEffects/arduino_midi_library
 */
#include <Arduino.h>
#include <USB.h>
#include <ESPNATIVEUSBMIDI.h>
#include <MIDI.h>

ESPNATIVEUSBMIDI espnativeusbmidi;

MIDI_CREATE_INSTANCE(ESPNATIVEUSBMIDI,espnativeusbmidi,MIDI)

// Variable that holds the current position in the sequence.
uint32_t position = 0;

// Store example melody as an array of note values
byte note_sequence[] = {
        74,78,81,86,90,93,98,102,57,61,66,69,73,78,81,85,88,92,97,100,97,92,88,85,81,78,
        74,69,66,62,57,62,66,69,74,78,81,86,90,93,97,102,97,93,90,85,81,78,73,68,64,61,
        56,61,64,68,74,78,81,86,90,93,98,102
};


void handleNoteOn(byte channel, byte pitch, byte velocity);

void handleNoteOff(byte channel, byte pitch, byte velocity);

void setup() {
    espnativeusbmidi.begin();

    //Change the USB device name
    USB.productName("ESP Native MIDI");
    USB.begin();

    // Initialize MIDI, and listen to all MIDI channels
    // This will also call usb_midi's begin()
    MIDI.begin(MIDI_CHANNEL_OMNI);

    // Attach the handleNoteOn function to the MIDI Library. It will
    // be called whenever the Bluefruit receives MIDI Note On messages.
    MIDI.setHandleNoteOn(handleNoteOn);

    // Do the same for MIDI Note Off messages.
    MIDI.setHandleNoteOff(handleNoteOff);

    Serial.begin(115200);
}

void loop() {
    static uint32_t start_ms = 0;
    if ( millis() - start_ms > 266)
    {
        start_ms += 266;

        // Setup variables for the current and previous
        // positions in the note sequence.
        int previous = position - 1;

        // If we currently are at position 0, set the
        // previous position to the last note in the sequence.
        if (previous < 0) {
            previous = sizeof(note_sequence) - 1;
        }

        // Send Note On for current position at full velocity (127) on channel 1.
        MIDI.sendNoteOn(note_sequence[position], 127, 1);

        // Send Note Off for previous note.
        MIDI.sendNoteOff(note_sequence[previous], 0, 1);

        // Increment position
        position++;

        // If we are at the end of the sequence, start over.
        if (position >= sizeof(note_sequence)) {
            position = 0;
        }
    }

    // read any new MIDI messages
    MIDI.read();
}
void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Log when a note is pressed.
    Serial.print("Note on: channel = ");
    Serial.print(channel);

    Serial.print(" pitch = ");
    Serial.print(pitch);

    Serial.print(" velocity = ");
    Serial.println(velocity);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    // Log when a note is released.
    Serial.print("Note off: channel = ");
    Serial.print(channel);

    Serial.print(" pitch = ");
    Serial.print(pitch);

    Serial.print(" velocity = ");
    Serial.println(velocity);
}
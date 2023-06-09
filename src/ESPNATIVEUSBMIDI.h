//
// Created by jbars on 5/26/2023.
// Based on https://github.com/adafruit/Adafruit_TinyUSB_Arduino/blob/master/src/arduino/midi/Adafruit_USBD_MIDI.h
//


#pragma once
#include "sdkconfig.h"

#if CONFIG_TINYUSB_MIDI_ENABLED

#include "esp_event.h"
#include "class/midi/midi.h"
#include "class/midi/midi_device.h"

enum MidiMessageCodes : uint8_t {
    NoteOff = 0x80,
    NoteOn = 0x90,
};

class ESPNATIVEUSBMIDI {
public:
    ESPNATIVEUSBMIDI(void);

    bool begin(void);
    // for MIDI library
    bool begin(uint32_t baud) {
        (void)baud;
        return begin();
    }
    void end(void);
    virtual int available(void);
    virtual int read(void);
    virtual size_t write(uint8_t b);

    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 1);
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel = 1);


    // from Adafruit_USBD_Interface
    virtual uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf,
                                            uint16_t bufsize);

    // internal use only
    uint16_t makeItfDesc(uint8_t itfnum, uint8_t *buf, uint16_t bufsize,
                         uint8_t ep_in, uint8_t ep_out);

private:
    uint8_t _n_cables;
    uint8_t _cable_name_strid[16];
};


#endif /* CONFIG_TINYUSB_HID_ENABLED */

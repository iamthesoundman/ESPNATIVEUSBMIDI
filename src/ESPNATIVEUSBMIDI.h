//
// Created by jbars on 5/26/2023.
//


#pragma once

#if CONFIG_TINYUSB_MIDI_ENABLED
#include "esp_event.h"
#include "class/midi/midi.h"
#include "class/midi/midi_device.h"

// Used by the included TinyUSB drivers
enum {

};
#define ESP_MIDI_PROTOCOL_MODE_BOOT          0x00      // Boot Protocol Mode
#define ESP_MIDI_PROTOCOL_MODE_REPORT        0x01      // Report Protocol Mode

enum MidiCodes: uint8_t
{
    InvalidType           = 0x00,    ///< For notifying errors
    NoteOff               = 0x80,    ///< Channel Message - Note Off
    NoteOn                = 0x90,    ///< Channel Message - Note On
    AfterTouchPoly        = 0xA0,    ///< Channel Message - Polyphonic AfterTouch
    ControlChange         = 0xB0,    ///< Channel Message - Control Change / Channel Mode
    ProgramChange         = 0xC0,    ///< Channel Message - Program Change
    AfterTouchChannel     = 0xD0,    ///< Channel Message - Channel (monophonic) AfterTouch
    PitchBend             = 0xE0,    ///< Channel Message - Pitch Bend
    SystemExclusive       = 0xF0,    ///< System Exclusive
    SystemExclusiveStart  = SystemExclusive,   ///< System Exclusive Start
    TimeCodeQuarterFrame  = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
    SongPosition          = 0xF2,    ///< System Common - Song Position Pointer
    SongSelect            = 0xF3,    ///< System Common - Song Select
    Undefined_F4          = 0xF4,
    Undefined_F5          = 0xF5,
    TuneRequest           = 0xF6,    ///< System Common - Tune Request
    SystemExclusiveEnd    = 0xF7,    ///< System Exclusive End
    Clock                 = 0xF8,    ///< System Real Time - Timing Clock
    Undefined_F9          = 0xF9,
    Tick                  = Undefined_F9, ///< System Real Time - Timing Tick (1 tick = 10 milliseconds)
    Start                 = 0xFA,    ///< System Real Time - Start
    Continue              = 0xFB,    ///< System Real Time - Continue
    Stop                  = 0xFC,    ///< System Real Time - Stop
    Undefined_FD          = 0xFD,
    ActiveSensing         = 0xFE,    ///< System Real Time - Active Sensing
    SystemReset           = 0xFF,    ///< System Real Time - System Reset
};


ESP_EVENT_DECLARE_BASE(ARDUINO_ESP_USB_MIDI_EVENTS);

typedef enum {
    ARDUINO_ESP_USB_MIDI_ANY_EVENT = ESP_EVENT_ANY_ID,
    ARDUINO_ESP_USB_MIDI_SET_PROTOCOL_EVENT = 0,
    ARDUINO_ESP_USB_MIDI_SET_IDLE_EVENT,
    ARDUINO_ESP_USB_MIDI_MAX_EVENT,
} arduino_esp_usb_midi_event_t;

typedef struct {
    uint8_t instance;
    union {
        struct {
                uint8_t protocol;
        } set_protocol;
        struct {
                uint8_t idle_rate;
        } set_idle;
    };
} arduino_esp_usb_midi_event_data_t;
typedef enum {
    ESP_HID_USAGE_GENERIC  = 0,
    ESP_HID_USAGE_KEYBOARD = 1,
    ESP_HID_USAGE_MOUSE    = 2,
    ESP_HID_USAGE_JOYSTICK = 4,
    ESP_HID_USAGE_GAMEPAD  = 8,
    ESP_HID_USAGE_TABLET   = 16,
    ESP_HID_USAGE_CCONTROL = 32,
    ESP_HID_USAGE_VENDOR   = 64
} esp_midi_usage_t;

typedef struct {
    uint8_t map_index;              /*!< HID report map index */
    uint8_t report_id;              /*!< HID report id */
    uint8_t report_type;            /*!< HID report type */
    uint8_t protocol_mode;          /*!< HID protocol mode */
    esp_midi_usage_t usage;          /*!< HID usage type */
    uint16_t value_len;             /*!< HID report length in bytes */
} esp_midi_report_item_t;
typedef struct {
    esp_midi_usage_t usage;              /*!< Dominant HID usage. (keyboard > mouse > joystick > gamepad > generic) */
    uint16_t appearance;                /*!< Calculated HID Appearance based on the dominant usage */
    uint8_t reports_len;                /*!< Number of reports discovered in the report map */
    esp_midi_report_item_t *reports;     /*!< Reports discovered in the report map */
} esp_midi_report_map_t;

esp_midi_report_map_t *esp_midi_parse_report_map(const uint8_t *midi_rm, size_t midi_rm_len);

void esp_midi_free_report_map(esp_midi_report_map_t *map);

class ESPUSBMIDIDevice
{
public:
    virtual uint16_t _onGetDescriptor(uint8_t* buffer){return 0;}
    virtual uint16_t _onGetFeature(uint8_t report_id, uint8_t* buffer, uint16_t len){return 0;}
    virtual void _onSetFeature(uint8_t report_id, const uint8_t* buffer, uint16_t len){}
    virtual void _onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len){}
};

class ESPNATIVEUSBMIDI {
public:
    ESPNATIVEUSBMIDI(void);
    void begin(void);
    void end(void);
    bool ready(void);

    virtual size_t write(uint8_t b);

    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel= 1);
    void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 1);

    void run_task(void);

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

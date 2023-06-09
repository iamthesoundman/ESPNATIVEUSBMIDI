//
// Created by jbars on 5/26/2023.
//https://github.com/adafruit/Adafruit_TinyUSB_Arduino/blob/master/src/arduino/midi/Adafruit_USBD_MIDI.cpp
//

#include "ESPNATIVEUSBMIDI.h"

#if CONFIG_TINYUSB_MIDI_ENABLED
#include "esp32-hal-tinyusb.h"
#include "USB.h"

#define EPOUT 0x00
#define EPIN 0x80
#define EPSIZE 64


static bool tinyusb_midi_device_is_initialized = false;
static bool tinyusb_midi_is_initialized = false;


static ESPNATIVEUSBMIDI *_midi_dev = NULL;

static uint16_t tusb_midi_load_descriptor(uint8_t * dst, uint8_t * itf)
{
    if(tinyusb_midi_is_initialized){
        return 0;
    }
    tinyusb_midi_is_initialized = true;

    // uint8_t str_index = tinyusb_add_string_descriptor("TinyUSB HID");

    uint8_t ep_in = tinyusb_get_free_in_endpoint();
    uint8_t ep_out = tinyusb_get_free_out_endpoint();
    TU_VERIFY(ep_in && ep_out);
    ep_in |= 0x80;

    uint16_t desc_len = _midi_dev->getInterfaceDescriptor(0, NULL, 0);

    desc_len = _midi_dev->makeItfDesc(*itf, dst, desc_len, ep_in, ep_out);

    *itf += 2;
    return desc_len;
}

ESPNATIVEUSBMIDI::ESPNATIVEUSBMIDI(){
    if(!tinyusb_midi_device_is_initialized){
        tinyusb_midi_device_is_initialized = true;
        _n_cables = 1;
        _midi_dev = this;
        uint16_t const desc_len = getInterfaceDescriptor(0, NULL, 0);
        tinyusb_enable_interface(USB_INTERFACE_MIDI, desc_len, tusb_midi_load_descriptor);
    }
}

uint16_t ESPNATIVEUSBMIDI::getInterfaceDescriptor(uint8_t itfnum,
                                                  uint8_t *buf,
                                                  uint16_t bufsize) {
    return makeItfDesc(itfnum, buf, bufsize, EPIN, EPOUT);
}

uint16_t ESPNATIVEUSBMIDI::makeItfDesc(uint8_t itfnum, uint8_t *buf,
                                       uint16_t bufsize, uint8_t ep_in,
                                       uint8_t ep_out) {
    uint16_t const desc_len = TUD_MIDI_DESC_HEAD_LEN +
                              TUD_MIDI_DESC_JACK_LEN * _n_cables +
                              2 * TUD_MIDI_DESC_EP_LEN(_n_cables);

    // null buf is for length only
    if (!buf) {
        return desc_len;
    }

    if (bufsize < desc_len) {
        return 0;
    }

    uint16_t len = 0;

    // Header
    {
        uint8_t desc[] = {TUD_MIDI_DESC_HEAD(itfnum, 0, _n_cables)};
        memcpy(buf + len, desc, sizeof(desc));
        len += sizeof(desc);
    }

    // Jack
    for (uint8_t i = 1; i <= _n_cables; i++) {
        uint8_t jack[] = {TUD_MIDI_DESC_JACK_DESC(i, _cable_name_strid[i - 1])};
        memcpy(buf + len, jack, sizeof(jack));
        len += sizeof(jack);
    }

    // Endpoint OUT + jack mapping
    {
        uint8_t desc[] = {TUD_MIDI_DESC_EP(ep_out, EPSIZE, _n_cables)};
        memcpy(buf + len, desc, sizeof(desc));
        len += sizeof(desc);
    }

    for (uint8_t i = 1; i <= _n_cables; i++) {
        uint8_t jack[] = {TUD_MIDI_JACKID_IN_EMB(i)};
        memcpy(buf + len, jack, sizeof(jack));
        len += sizeof(jack);
    }

    // Endpoint IN + jack mapping
    {
        uint8_t desc[] = {TUD_MIDI_DESC_EP(ep_in, EPSIZE, _n_cables)};
        memcpy(buf + len, desc, sizeof(desc));
        len += sizeof(desc);
    }

    for (uint8_t i = 1; i <= _n_cables; i++) {
        uint8_t jack[] = {TUD_MIDI_JACKID_OUT_EMB(i)};
        memcpy(buf + len, jack, sizeof(jack));
        len += sizeof(jack);
    }

    if (len != desc_len) {
        // TODO should throw an error message
        return 0;
    }

    return desc_len;
}


bool ESPNATIVEUSBMIDI::begin(void) {
    _midi_dev = this;
    return true;
}

int ESPNATIVEUSBMIDI::available(void) { return tud_midi_available(); }


void ESPNATIVEUSBMIDI::end(){

}

int ESPNATIVEUSBMIDI::read(void) {
    uint8_t ch;
    return tud_midi_stream_read(&ch, 1) ? (int)ch : (-1);
}

size_t ESPNATIVEUSBMIDI::write(uint8_t b) {
    return tud_midi_stream_write(0, &b, 1);
}

void ESPNATIVEUSBMIDI::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel){
    uint8_t note_on[3] = { (uint8_t)(NoteOn|(channel - 1)), note, velocity };
    write(note_on[0]);
    write(note_on[1]);
    write(note_on[2]);
}
void ESPNATIVEUSBMIDI::sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel){
    uint8_t note_off[3] = {(uint8_t)(NoteOff | (channel - 1)), note, velocity };
    write(note_off[0]);
    write(note_off[1]);
    write(note_off[2]);
}
#endif /* CONFIG_TINYUSB_MIDI_ENABLED */

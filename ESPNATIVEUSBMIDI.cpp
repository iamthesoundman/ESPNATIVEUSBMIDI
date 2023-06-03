//
// Created by jbars on 5/26/2023.
//


#if CONFIG_TINYUSB_MIDI_ENABLED
#include "ESPNATIVEUSBMIDI.h"
#include "esp32-hal-tinyusb.h"
#include "USB.h"

enum
{
    ITF_NUM_MIDI = 0,
    ITF_NUM_MIDI_STREAMING,
    ITF_NUM_TOTAL
};

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

void ESPNATIVEUSBMIDI::begin(){
    _midi_dev = this;
}

void ESPNATIVEUSBMIDI::end(){

}

size_t ESPNATIVEUSBMIDI::write(uint8_t b) {
    return tud_midi_stream_write(0, &b, 1);
}

void ESPNATIVEUSBMIDI::noteOn(uint8_t note, uint8_t velocity, uint8_t channel){
    uint8_t note_on[3] = { static_cast<uint8_t>(NoteOn|(channel - 1)), note, velocity };
    tud_midi_stream_write(0, note_on, 3);
}
void ESPNATIVEUSBMIDI::noteOff(uint8_t note, uint8_t velocity, uint8_t channel){
    uint8_t note_off[3] = {static_cast<uint8_t>(NoteOff | (channel - 1)), note, velocity };
    tud_midi_stream_write(0, note_off, 3);
}

void ESPNATIVEUSBMIDI::run_task() {

    uint8_t packet[4];
    while ( tud_midi_available() ) tud_midi_packet_read(packet);
}
#endif /* CONFIG_TINYUSB_MIDI_ENABLED */



#if false
#include "ESPUSBMIDI.h"

#include "esp32-hal-tinyusb.h"
#include "USB.h"


#define USB_MIDI_DEVICES_MAX 1

enum
{
    ITF_NUM_MIDI = 0,
    ITF_NUM_MIDI_STREAMING,
    ITF_NUM_TOTAL
};
#define EPNUM_MIDI_OUT   0x01
#define EPNUM_MIDI_IN   0x01

ESP_EVENT_DEFINE_BASE(ARDUINO_ESP_USB_MIDI_EVENTS);
esp_err_t arduino_usb_event_post(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_size, TickType_t ticks_to_wait);
esp_err_t arduino_usb_event_handler_register_with(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg);

typedef struct {
    ESPUSBMIDIDevice * device;
    uint8_t reports_num;
    uint8_t * report_ids;
} tinyusb_midi_device_t;

static tinyusb_midi_device_t tinyusb_midi_devices[USB_MIDI_DEVICES_MAX];

static uint8_t tinyusb_midi_devices_num = 0;
static bool tinyusb_midi_devices_is_initialized = false;
static xSemaphoreHandle tinyusb_midi_device_input_sem = NULL;
static xSemaphoreHandle tinyusb_midi_device_input_mutex = NULL;

static bool tinyusb_midi_is_initialized = false;
static uint8_t tinyusb_loaded_midi_devices_num = 0;
static uint16_t tinyusb_midi_device_descriptor_len = 0;
static uint8_t * tinyusb_midi_device_descriptor = NULL;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
static const char * tinyusb_midi_device_report_types[4] = {"INVALID", "INPUT", "OUTPUT", "FEATURE"};
#endif

static bool tinyusb_enable_midi_device(uint16_t descriptor_len, ESPUSBMIDIDevice * device){
    if(tinyusb_midi_is_initialized){
        log_e("TinyUSB MIDI has already started! Device not enabled");
        return false;
    }
    if(tinyusb_loaded_midi_devices_num >= USB_MIDI_DEVICES_MAX){
        log_e("Maximum devices already enabled! Device not enabled");
        return false;
    }
    tinyusb_midi_device_descriptor_len += descriptor_len;
    tinyusb_midi_devices[tinyusb_loaded_midi_devices_num++].device = device;

    log_d("Device[%u] len: %u", tinyusb_loaded_midi_devices_num-1, descriptor_len);
    return true;
}

ESPUSBMIDIDevice * tinyusb_get_device_by_report_id(uint8_t report_id){
    for(uint8_t i=0; i<tinyusb_loaded_midi_devices_num; i++){
        tinyusb_midi_device_t * device = &tinyusb_midi_devices[i];
        if(device->device && device->reports_num){
            for(uint8_t r=0; r<device->reports_num; r++){
                if(report_id == device->report_ids[r]){
                    return device->device;
                }
            }
        }
    }
    return NULL;
}

static uint16_t tinyusb_on_get_feature(uint8_t report_id, uint8_t* buffer, uint16_t reqlen){
    ESPUSBMIDIDevice * device = tinyusb_get_device_by_report_id(report_id);
    if(device){
        return device->_onGetFeature(report_id, buffer, reqlen);
    }
    return 0;
}

static bool tinyusb_on_set_feature(uint8_t report_id, const uint8_t* buffer, uint16_t reqlen){
    ESPUSBMIDIDevice * device = tinyusb_get_device_by_report_id(report_id);
    if(device){
        device->_onSetFeature(report_id, buffer, reqlen);
        return true;
    }
    return false;
}

static bool tinyusb_on_set_output(uint8_t report_id, const uint8_t* buffer, uint16_t reqlen){
    ESPUSBMIDIDevice * device = tinyusb_get_device_by_report_id(report_id);
    if(device){
        device->_onOutput(report_id, buffer, reqlen);
        return true;
    }
    return false;
}

static uint16_t tinyusb_on_add_descriptor(uint8_t device_index, uint8_t * dst){
    uint16_t res = 0;
    uint8_t report_id = 0, reports_num = 0;
    tinyusb_midi_device_t * device = &tinyusb_midi_devices[device_index];
    if(device->device){
        res = device->device->_onGetDescriptor(dst);
        if(res){

            esp_midi_report_map_t *midi_report_map = esp_midi_parse_report_map(dst, res);
            if(midi_report_map){
                if(device->report_ids){
                    free(device->report_ids);
                }
                device->reports_num = midi_report_map->reports_len;
                device->report_ids = (uint8_t*)malloc(device->reports_num);
                memset(device->report_ids, 0, device->reports_num);
                reports_num = device->reports_num;

                for(uint8_t i=0; i<device->reports_num; i++){
                    if(midi_report_map->reports[i].protocol_mode == ESP_MIDI_PROTOCOL_MODE_REPORT){
                        report_id = midi_report_map->reports[i].report_id;
                        for(uint8_t r=0; r<device->reports_num; r++){
                            if(!report_id){
                                //todo: handle better when device has no report ID set
                                break;
                            } else if(report_id == device->report_ids[r]){
                                //already added
                                reports_num--;
                                break;
                            } else if(!device->report_ids[r]){
                                //empty slot
                                device->report_ids[r] = report_id;
                                break;
                            }
                        }
                    } else {
                        reports_num--;
                    }
                }
                device->reports_num = reports_num;
                esp_midi_free_report_map(midi_report_map);
            }

        }
    }
    return res;
}


static bool tinyusb_load_enabled_midi_devices(){
    if(tinyusb_midi_device_descriptor != NULL){
        return true;
    }
    tinyusb_midi_device_descriptor = (uint8_t *)malloc(tinyusb_midi_device_descriptor_len);
    if (tinyusb_midi_device_descriptor == NULL) {
        log_e("MIDI Descriptor Malloc Failed");
        return false;
    }
    uint8_t * dst = tinyusb_midi_device_descriptor;

    for(uint8_t i=0; i<tinyusb_loaded_midi_devices_num; i++){
        uint16_t len = tinyusb_on_add_descriptor(i, dst);
        if (!len) {
            break;
        } else {
            dst += len;
        }
    }

    esp_midi_report_map_t *midi_report_map = esp_midi_parse_report_map(tinyusb_midi_device_descriptor, tinyusb_midi_device_descriptor_len);
    if(midi_report_map){
        log_d("Loaded MIDI Desriptor with the following reports:");
        for(uint8_t i=0; i<midi_report_map->reports_len; i++){
            if(midi_report_map->reports[i].protocol_mode == ESP_MIDI_PROTOCOL_MODE_REPORT){
                log_d("  ID: %3u, Type: %7s, Size: %2u, Usage: %8s",
                      midi_report_map->reports[i].report_id,
                      esp_midi_report_type_str(midi_report_map->reports[i].report_type),
                      midi_report_map->reports[i].value_len,
                      esp_midi_usage_str(midi_report_map->reports[i].usage)
                );
            }
        }
        esp_midi_free_report_map(midi_report_map);
    } else {
        log_e("Failed to parse the midi report descriptor!");
        return false;
    }

    return true;
}

extern "C" uint16_t tusb_midi_load_descriptor(uint8_t * dst, uint8_t * itf)
{
    if(tinyusb_midi_is_initialized){
        return 0;
    }
    tinyusb_midi_is_initialized = true;

    uint8_t str_index = tinyusb_add_string_descriptor("TinyUSB MIDI");
    uint8_t ep_in = tinyusb_get_free_in_endpoint();
    TU_VERIFY (ep_in != 0);
    uint8_t ep_out = tinyusb_get_free_out_endpoint();
    TU_VERIFY (ep_out != 0);
    uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
            // MIDI Input & Output descriptor
            // Interface number, string index, EP Out & EP In address, EP size
            TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0, EPNUM_MIDI_OUT, (0x80 | EPNUM_MIDI_IN), 64)
    };
    *itf+=1;
    memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
    return TUD_MIDI_DESC_LEN;
}




// Invoked when received GET MIDI REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const * tud_midi_descriptor_report_cb(uint8_t instance){
    log_v("instance: %u", instance);
    if(!tinyusb_load_enabled_midi_devices()){
        return NULL;
    }
    return tinyusb_midi_device_descriptor;
}

// Invoked when received SET_PROTOCOL request
// protocol is either MIDI_PROTOCOL_BOOT (0) or MIDI_PROTOCOL_REPORT (1)
void tud_midi_set_protocol_cb(uint8_t instance, uint8_t protocol){
    log_v("instance: %u, protocol:%u", instance, protocol);
    arduino_esp_usb_midi_event_data_t p;
    p.instance = instance;
    p.set_protocol.protocol = protocol;
    arduino_usb_event_post(ARDUINO_ESP_USB_MIDI_EVENTS, ARDUINO_ESP_USB_MIDI_SET_PROTOCOL_EVENT, &p, sizeof(arduino_esp_usb_midi_event_data_t), portMAX_DELAY);
}

// Invoked when received SET_IDLE request. return false will stall the request
// - Idle Rate = 0 : only send report if there is changes, i.e skip duplication
// - Idle Rate > 0 : skip duplication, but send at least 1 report every idle rate (in unit of 4 ms).
bool tud_midi_set_idle_cb(uint8_t instance, uint8_t idle_rate){
    log_v("instance: %u, idle_rate:%u", instance, idle_rate);
    arduino_esp_usb_midi_event_data_t p;
    p.instance = instance;
    p.set_idle.idle_rate = idle_rate;
    arduino_usb_event_post(ARDUINO_ESP_USB_MIDI_EVENTS, ARDUINO_ESP_USB_MIDI_SET_IDLE_EVENT, &p, sizeof(arduino_esp_usb_midi_event_data_t), portMAX_DELAY);
    return true;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
/*uint16_t tud_midi_get_report_cb(uint8_t instance, uint8_t report_id, midi_report_type_t report_type, uint8_t* buffer, uint16_t reqlen){
    uint16_t res = tinyusb_on_get_feature(report_id, buffer, reqlen);
    if(!res){
        log_d("instance: %u, report_id: %u, report_type: %s, reqlen: %u", instance, report_id, tinyusb_midi_device_report_types[report_type], reqlen);
    }
    return res;
}*/

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
/*void tud_midi_set_report_cb(uint8_t instance, uint8_t report_id, midi_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize){
    if(!report_id && !report_type){
        if(!tinyusb_on_set_output(0, buffer, bufsize) && !tinyusb_on_set_output(buffer[0], buffer+1, bufsize-1)){
            log_d("instance: %u, report_id: %u, report_type: %s, bufsize: %u", instance, buffer[0], tinyusb_midi_device_report_types[MIDI_REPORT_TYPE_OUTPUT], bufsize-1);
        }
    } else {
        if(!tinyusb_on_set_feature(report_id, buffer, bufsize)){
            log_d("instance: %u, report_id: %u, report_type: %s, bufsize: %u", instance, report_id, tinyusb_midi_device_report_types[report_type], bufsize);
        }
    }
}*/

ESPUSBMIDI::ESPUSBMIDI(){
    if(!tinyusb_midi_devices_is_initialized){
        tinyusb_midi_devices_is_initialized = true;
        for(uint8_t i=0; i < USB_MIDI_DEVICES_MAX; i++){
            memset(&tinyusb_midi_devices[i], 0, sizeof(tinyusb_midi_device_t));
        }
        tinyusb_midi_devices_num = 0;
        tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, tusb_midi_load_descriptor);
    }
}

void ESPUSBMIDI::begin(){
    if(tinyusb_midi_device_input_sem == NULL){
        tinyusb_midi_device_input_sem = xSemaphoreCreateBinary();
    }
    if(tinyusb_midi_device_input_mutex == NULL){
        tinyusb_midi_device_input_mutex = xSemaphoreCreateMutex();
    }
}

void ESPUSBMIDI::end(){
    if (tinyusb_midi_device_input_sem != NULL) {
        vSemaphoreDelete(tinyusb_midi_device_input_sem);
        tinyusb_midi_device_input_sem = NULL;
    }
    if (tinyusb_midi_device_input_mutex != NULL) {
        vSemaphoreDelete(tinyusb_midi_device_input_mutex);
        tinyusb_midi_device_input_mutex = NULL;
    }
}

/*bool ESPUSBMIDI::ready(void){
    return tud_midi_n_ready(0);
}*/

// TinyUSB is in the process of changing the type of the last argument to
// tud_midi_report_complete_cb(), so extract the type from the version of TinyUSB that we're
// compiled with.
template <class F> struct ArgType;

template <class R, class T1, class T2, class T3>
struct ArgType<R(*)(T1, T2, T3)> {
    typedef T1 type1;
    typedef T2 type2;
    typedef T3 type3;
};

//typedef ArgType<decltype(&tud_midi_report_complete_cb)>::type3 tud_midi_report_complete_cb_len_t;

/*void tud_midi_report_complete_cb(uint8_t instance, uint8_t const* report, tud_midi_report_complete_cb_len_t len){
    if (tinyusb_midi_device_input_sem) {
        xSemaphoreGive(tinyusb_midi_device_input_sem);
    }
}*/

bool ESPUSBMIDI::SendReport(uint8_t id, const void* data, size_t len, uint32_t timeout_ms){
    if(!tinyusb_midi_device_input_sem || !tinyusb_midi_device_input_mutex){
        log_e("TX Semaphore is NULL. You must call ESPUSBMIDI::begin() before you can send reports");
        return false;
    }

    if(xSemaphoreTake(tinyusb_midi_device_input_mutex, timeout_ms / portTICK_PERIOD_MS) != pdTRUE){
        log_e("report %u mutex failed", id);
        return false;
    }

    bool res = ready();
    if(!res){
        log_e("not ready");
    } else {
        // The semaphore may be given if the last SendReport() timed out waiting for the report to
        // be sent. Or, tud_midi_report_complete_cb() may be called an extra time, causing the
        // semaphore to be given. In these cases, take the semaphore to clear its state so that
        // we can wait for it to be given after calling tud_midi_n_report().
        xSemaphoreTake(tinyusb_midi_device_input_sem, 0);

        //res = tud_midi_n_report(0, id, data, len);
        if(!res){
            log_e("report %u failed", id);
        } else {
            if(xSemaphoreTake(tinyusb_midi_device_input_sem, timeout_ms / portTICK_PERIOD_MS) != pdTRUE){
                log_e("report %u wait failed", id);
                res = false;
            }
        }
    }

    xSemaphoreGive(tinyusb_midi_device_input_mutex);
    return res;
}

bool ESPUSBMIDI::addDevice(ESPUSBMIDIDevice * device, uint16_t descriptor_len){
    if(device && tinyusb_loaded_midi_devices_num < USB_MIDI_DEVICES_MAX){
        if(!tinyusb_enable_midi_device(descriptor_len, device)){
            return false;
        }
        return true;
    }
    return false;
}

void ESPUSBMIDI::onEvent(esp_event_handler_t callback){
    onEvent(ARDUINO_ESP_USB_MIDI_ANY_EVENT, callback);
}
void ESPUSBMIDI::onEvent(arduino_esp_usb_midi_event_t event, esp_event_handler_t callback){
    arduino_usb_event_handler_register_with(ARDUINO_ESP_USB_MIDI_EVENTS, event, callback, this);
}

#endif /* CONFIG_TINYUSB_MIDI_ENABLED */

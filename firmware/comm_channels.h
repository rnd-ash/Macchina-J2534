#ifndef COMM_CHANNELS_H_
#define COMM_CHANNELS_H_

#include "MACCHINA_CONFIG.h"

#include "comm.h"
#include "custom_can.h"
#include "j2534_mini.h"


bool debug_send_frame(CAN_FRAME &f);
void debug_read_frame(CAN_FRAME &f);
bool debug_send_frame_force(CAN_FRAME &f);

class Channel {
    public:
        virtual void wakeup(uint8_t type, uint8_t* request, uint8_t request_len);
        virtual bool setup(int id, int protocol, int baud, int flags);
        virtual void addFilter(int type, int filter_id, char* mask, char* pattern, char* flowcontrol, int mask_len, int pattern_len, int flowcontrol_len);
        virtual void removeFilter(int id);
        virtual void sendMsg(uint32_t tx_flags, char* data, int data_size, bool respond);
        virtual void destroy();
        virtual void update();
        virtual void ioctl_get(uint32_t id);
        virtual void ioctl_set(uint32_t id, uint32_t value);
    protected:
        int channel_id;
};

#if defined(CFG_MACCHINA_M2)

class Iso9141Channel : public Channel {
    public:
        void wakeup(uint8_t type, uint8_t* request, uint8_t request_len);
        bool setup(int id, int protocol, int baud, int flags);
        void addFilter(int type, int filter_id, char* mask, char* pattern, char* flowcontrol, int mask_len, int pattern_len, int flowcontrol_len);
        void removeFilter(int id);
        void destroy();
        void sendMsg(uint32_t tx_flags, char* data, int data_size, bool respond);
        void update();
        void ioctl_get(uint32_t id);
        void ioctl_set(uint32_t id, uint32_t value);
    private:
        int baud = 0;
        void set_port(bool state);
        void set_line(bool state);
        void write_cs(uint8_t* buffer, uint8_t len);
        void write_data(uint8_t* buf, uint8_t buf_len, bool do_checksum);
        HardwareSerial* obdSerial = nullptr;
        bool loopback = false;
        bool used_mailboxes[7] = {false};
        bool blocking_filters[7] = {false};
        bool masks[7] = {false};
        uint32_t patterns[7] = {0x00};
        uint32_t p1_min = 0; // Default 0
        uint32_t p1_max = 0; // Default 20
        uint32_t p2_min = 0; // Default 25
        uint32_t p2_max = 0; // Default 50
        uint32_t p3_min = 0; // Default 55
        uint32_t p3_max = 0; // Default 5000
        uint32_t p4_min = 0; // Default 5
        uint32_t p4_max = 0; // Default 20

        uint32_t w1 = 0;
        uint32_t w2 = 0;
        uint32_t w3 = 0;
        uint32_t w4 = 0;
        uint32_t w5 = 0;

        uint32_t tidle = 0;
        uint32_t tinl = 0;
        uint32_t twup = 0;

        uint32_t parity = 0;

};

#endif

#define MAX_CAN_BUFFER_SIZE 16
struct CanRingBuffer {
    CAN_FRAME* buffer[MAX_CAN_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
};

class CanChannel : public Channel {
    public:
        void wakeup(uint8_t type, uint8_t* request, uint8_t request_len){};
        bool setup(int id, int protocol, int baud, int flags);
        void addFilter(int type, int filter_id, char* mask, char* pattern, char* flowcontrol, int mask_len, int pattern_len, int flowcontrol_len);
        void removeFilter(int id);
        void destroy();
        void sendMsg(uint32_t tx_flags, char* data, int data_size, bool respond);
        void update();
        void ioctl_get(uint32_t id);
        void ioctl_set(uint32_t id, uint32_t value);
    private:
        bool loopback = false;
        bool isExtended = false;
        CAN_FRAME f;
        bool used_mailboxes[7] = {false};
        bool blocking_filters[7] = {false};
        bool masks[7] = {false};
        uint32_t patterns[7] = {0x00};
};

struct isoPayload {
    char payload[5120]; // 4096 for payload, 4 for CANID
    int payloadSize;
    int payloadPos;
};

class ISO15765Channel : public Channel {
    public:
        void wakeup(uint8_t type, uint8_t* request, uint8_t request_len){};
        bool setup(int id, int protocol, int baud, int flags);
        void addFilter(int type, int filter_id, char* mask, char* pattern, char* flowcontrol, int mask_len, int pattern_len, int flowcontrol_len);
        void removeFilter(int id);
        void destroy();
        void sendMsg(uint32_t tx_flags, char* data, int data_size, bool respond);
        void update();
        void ioctl_get(uint32_t id);
        void ioctl_set(uint32_t id, uint32_t value);
    private:
        void rx_single_frame(CAN_FRAME *read);
        void rx_multi_frame(CAN_FRAME *read, int filter_id);
        void tx_multi_frame();
        void send_ff_indication(CAN_FRAME *read, int filter_id);
        void handle_fc(CAN_FRAME *read, int filter_id);
        CAN_FRAME f;
        bool used_mailboxes[MAILBOX_COUNT] = {false};
        uint32_t flowcontrol_ids[MAILBOX_COUNT] = {0x00};
        uint32_t mask_ids[MAILBOX_COUNT] = {0x00};
        uint32_t pattern_ids[MAILBOX_COUNT] = {0x00};
        bool use29bitCid = false;
        bool extAddressingChannel = false;
        bool extAddressingPayload = false;
        bool isSending = false;
        bool isReceiving = false;


        uint8_t tx_id = 0;
        bool respond_after_send = false;

        isoPayload rxPayload; // For receiving
        isoPayload txPayload; // For sending
        uint16_t block_size;
        uint16_t sep_time;
        uint16_t rx_frame_count;
        uint16_t block_size_tx = 0;
        uint16_t sep_time_tx = 0;
        uint16_t tx_frames_sent = 0;
        uint8_t tx_pci = 0x20;
        unsigned long next_send_time;
        bool clear_to_send = false;
};

#endif
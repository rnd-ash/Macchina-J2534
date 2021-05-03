#include "custom_can.h"
#include "comm.h"

// 7 RxQueues
CustomCan::rxQueue rxQueues[7];

void CustomCan::__delete_check_rx_ring(int i) {
    rxQueues[i].head = 0;
    rxQueues[i].tail = 0;
    Can0.removeCallback(i);
}

void CustomCan::__create_check_rx_ring(int i) {
    rxQueues[i].head = 0;
    rxQueues[i].tail = 0;
    // Register callback for hardware interrupt
    switch (i)
    {
    case 0:
        Can0.setCallback(i, CustomCan::__callback_mb0);
        break;
    case 1:
        Can0.setCallback(i, CustomCan::__callback_mb1);
        break;
    case 2:
        Can0.setCallback(i, CustomCan::__callback_mb2);
        break;
    case 3:
        Can0.setCallback(i, CustomCan::__callback_mb3);
        break;
    case 4:
        Can0.setCallback(i, CustomCan::__callback_mb4);
        break;
    case 5:
        Can0.setCallback(i, CustomCan::__callback_mb5);
        break;
    case 6:
        Can0.setCallback(i, CustomCan::__callback_mb6);
        break;
    default:
        break;
    }
}

bool CustomCan::enableCanBus(int baud) {
#if defined(CFG_MACCHINA_A0) || defined(CFG_MACCHINA_ESP32_TEST)
    pinMode(GPIO_NUM_21, OUTPUT);
    digitalWrite(GPIO_NUM_21, LOW); // Fix for TJA1042 (Production Macchina A0). Set HSC_S pin low
    if (Can0.begin(baud*2, 255) == 0) {
        return false;
    }
#else
    // Begin bus
    if (Can0.init(baud) == 0) {
        return false;
    }
#endif
    
    // Block all traffic
    for (int i = 0; i < 7; i++) {
        Can0.setRXFilter(i, 0xFFFF, 0x0000, false);
        // In case rxQueue is still there, delete it
        __delete_check_rx_ring(i);
    }
    // No software queues created in this method
    return true;
}

void CustomCan::disableCanBus() {
    Can0.disable();
    // Block all traffic
    for (int i = 0; i < 7; i++) {
        Can0.setRXFilter(i, 0xFFFF, 0x0000, false);
        // In case rxQueue is still there, delete it
        __delete_check_rx_ring(i);
    }
}

void CustomCan::__rx_queue_push_frame(rxQueue &r, CAN_FRAME &f) {
    uint8_t nextEntry = (r.head + 1) % MAX_RX_QUEUE;
    // Queue is full, data is lost
    if (nextEntry == r.tail) return;
    memcpy((void *)&r.buffer[r.head], (void *)&f, sizeof(CAN_FRAME));
    r.head = nextEntry;
}

bool CustomCan::__rx_queue_pop_frame(rxQueue &r, CAN_FRAME &f) {
    // No frames in ring buffer
    if (r.head == r.tail)  return false;
    memcpy((void *)&f, (void *)&r.buffer[r.tail], sizeof(CAN_FRAME));
    r.tail = (r.tail + 1) % MAX_RX_QUEUE;
    return true;
}

void CustomCan::enableCanFilter(int id, uint32_t pattern, uint32_t mask, bool isExtended) {
    if (id < 0 || id >= 7) return; // Invalid mailbox ID

    // Set pattern and mask on the specified mailbox
    Can0.setRXFilter(id, pattern, mask, isExtended);
    // Delete any old buffer if it for some reason exists
    __delete_check_rx_ring(id);
    // Create our new ring
    __create_check_rx_ring(id);
    // Now register the callback so that frames get pushed to our mailbox
}

void CustomCan::disableCanFilter(int id) {
    if (id < 0 || id >= 7) return; // Invalid mailbox ID
    Can0.setRXFilter(id, 0xFFFF, 0x0000, false);
    __delete_check_rx_ring(id);
}

bool CustomCan::receiveFrame(int mailbox_id, CAN_FRAME *f) {
    if (mailbox_id < 0 || mailbox_id >= 7) return false; // Invalid malbox ID
    return __rx_queue_pop_frame(rxQueues[mailbox_id], *f);
}

bool CustomCan::sendFrame(CAN_FRAME *cf) {
    Can0.sendFrame(*cf);
}

void CustomCan::clearMailboxQueue(int mailbox_id) {
    if (mailbox_id < 0 || mailbox_id >= 7) return; // Invalid malbox ID
    rxQueues[mailbox_id].head = 0;
    rxQueues[mailbox_id].tail = 0;
}

void CustomCan::__callback_mb0(CAN_FRAME *f) { __rx_queue_push_frame(rxQueues[0], *f); }
void CustomCan::__callback_mb1(CAN_FRAME *f) { __rx_queue_push_frame(rxQueues[1], *f); }
void CustomCan::__callback_mb2(CAN_FRAME *f) { __rx_queue_push_frame(rxQueues[2], *f); }
void CustomCan::__callback_mb3(CAN_FRAME *f) { __rx_queue_push_frame(rxQueues[3], *f); }
void CustomCan::__callback_mb4(CAN_FRAME *f) { __rx_queue_push_frame(rxQueues[4], *f); }
void CustomCan::__callback_mb5(CAN_FRAME *f) { __rx_queue_push_frame(rxQueues[5], *f); }
void CustomCan::__callback_mb6(CAN_FRAME *f) { __rx_queue_push_frame(rxQueues[6], *f); }
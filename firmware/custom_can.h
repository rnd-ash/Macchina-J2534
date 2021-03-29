#ifndef CUSTOM_CAN_H_
#define CUSTOM_CAN_H_

#include <stdint.h>
#include "MACCHINA_CONFIG.h"

#if defined(CFG_MACCHINA_M2)
#include "due_can.h"
#endif
#if defined(CFG_MACCHINA_A0) || defined(CFG_MACCHINA_ESP32_TEST)
#include <esp32_can.h>
#endif

namespace CustomCan {

    // Each mailbox has a rxMailbox of 8 frames
    #define MAX_RX_QUEUE 8
    struct rxQueue {
        volatile CAN_FRAME buffer[MAX_RX_QUEUE];
        uint8_t head;
        uint8_t tail;
    };

    /**
     * Sets up the CAN0 interface on the M2, and pre-configures all the mailboxes
     * to block all traffic
     * 
     * @param baud Bus speed to initialize the CAN0 controller with
     * 
     * @returns Boolean indicating if CAN was setup successfully
     */
    bool enableCanBus(int baud);

    /**
     * Deletes one of the mailboxes Rx ring buffer
     * @param i Mailbox ID to delete its ring buffer
     */
    void __delete_check_rx_ring(int i);

    /**
     * Creates a new Rx ring buffer for a CAN mailbox
     * If the ring buffer is already setup for the target mailbox,
     * it is simply cleared of any data
     * 
     * @param i Mailbox ID to set up a new Rx ring buffer
     */
    void __create_check_rx_ring(int i);

    /**
     * Called on mailbox interrupt. This function will attempt to push
     * a new CAN Frame onto the mailboxes' Rx ring buffer. If the ring
     * buffer is full, then the incoming data is simply discarded
     * 
     * @param r Rx Queue to push the frame to (This is also the mailbox ID who triggered the interrupt)
     * @param f CAN Frame object to push to the Rx ring buffer
     */
    void __rx_queue_push_frame(rxQueue &r, CAN_FRAME &f);

    /**
     * Called by receiveFrame to pop a frame from a mailboxes ring buffer.
     * @param r - Ring buffer to pop a frame from (Same as mailbox ID)
     * @param f - CAN Frame object to read into if popping a frame
     * 
     * @returns Bool indicating if the pop operation managed to read data
     */
    bool __rx_queue_pop_frame(rxQueue &r, CAN_FRAME &f);


    // Callback functions that are ran if a frame is sent to a mailbox within an interrupt
    // Only registered when a rxFilter is set for the mailbox

    // Callback function for mailbox 0
    void __callback_mb0(CAN_FRAME *f);
    // Callback function for mailbox 1
    void __callback_mb1(CAN_FRAME *f);
    // Callback function for mailbox 2
    void __callback_mb2(CAN_FRAME *f);
    // Callback function for mailbox 3
    void __callback_mb3(CAN_FRAME *f);
    // Callback function for mailbox 4
    void __callback_mb4(CAN_FRAME *f);
    // Callback function for mailbox 5
    void __callback_mb5(CAN_FRAME *f);
    // Callback function for mailbox 6
    void __callback_mb6(CAN_FRAME *f);

    /**
     * Disables the Can0 interface
     */
    void disableCanBus();

    /**
     * Disables a CAN mailbox filter
     * @param id Mailbox ID to disable 
     */
    void disableCanFilter(int id);

    /**
     * Enables a CAN mailbox with a specified filter
     * @param id Mailbox ID (0-6)
     * @param pattern Pattern for CAN ID
     * @param mask Mask for CAN ID
     * @param isExtended Boolean indicating if the mailbox should be configured for Extended CAN or not
     */
    void enableCanFilter(int id, uint32_t pattern, uint32_t mask, bool isExtended);

    /**
     * Transmits a CAN Frame to the vehicles CAN Network using the CAN0 interface on the M2
     */
    bool sendFrame(CAN_FRAME *cf);

    /**
     * Attempts to read a frame from one of the pre-configured mailboxes queues on the CAN0 interface
     * @param mailbox_id mailbox ID (0-6) to grab a frame from
     * @param f Pointer to CAN Frame to read into if data is in the mailbox queue
     * 
     * @returns Boolean indicating if read was successful or not
     */
    bool receiveFrame(int mailbox_id, CAN_FRAME *f);

    /**
     * Clears a mailboxes Rx ring buffer queue
     * @param mailbox_id the mailbox ID to clear
     */
    void clearMailboxQueue(int mailbox_id);
}

#endif
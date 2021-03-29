#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "comm.h"
#include "j2534_mini.h"
#include "comm_channels.h"

#define CAN_CHANNEL_ID 0
#define KLINE_CHANNEL_ID 1
#define J1850_CHANNEL_ID 2
#define SCI_CHANNEL_ID 3

void setup_channel(COMM_MSG* msg);
void remove_channel(COMM_MSG *msg);
void channel_loop();
void delete_channel(Channel*& ptr);
void add_channel_filter(COMM_MSG* msg);
void del_channel_filter(COMM_MSG* msg);
void send_data(COMM_MSG *msg);

void ioctl_get(COMM_MSG *msg);
void ioctl_set(COMM_MSG *msg);

void create_can_channel(int id, int protocol, int baud, int flags);

/**
 * This function is ran when disconnect is called.
 * This removes all channels, returning the M2
 * back to its idle state
 */
void reset_all_channels();




#endif

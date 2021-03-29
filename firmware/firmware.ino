#include "comm.h"
#include "j2534_mini.h"
#include "channel.h"

#include "pt_device.h"
#include "MACCHINA_CONFIG.h"

CAN_FRAME input;

void setup() {
  PT_DEVICE->init_device();
  PCCOMM::init();
}

COMM_MSG msg = {0x00};

void send_v_batt() {
  int batt_mv = PT_DEVICE->read_batt_mv();
  PCCOMM::respond_ok(MSG_READ_BATT, (uint8_t*)(&batt_mv), 4);
}

bool isConnected = false;
void set_status_led(uint8_t status) {
    // Clear no matter what!
    reset_all_channels();
    PCCOMM::reset();
    if (status == 0x00) {
      PT_DEVICE->set_status_led(false);
      // TODO Reset M2 to default state when we disconnect
    } else {
      PT_DEVICE->set_status_led(true);
    }
}

void get_fw_version(COMM_MSG *msg) {
  const char* fw_str = PT_DEVICE->get_firmware_version();
  PCCOMM::respond_ok(MSG_GET_FW_VERSION, (uint8_t*)fw_str, strlen(fw_str));
}

#ifdef FW_TEST
unsigned long lastPing = millis();
#endif

void loop() {
  if (PCCOMM::read_message(&msg)) {
    switch (msg.msg_type)
    {
#ifdef FW_TEST
    case MSG_TEST:
      PCCOMM::send_message(&msg); // Test Message type - Should just loop back response
      break;
#endif
    case MSG_STATUS:
      set_status_led(msg.args[0]);
      break;
    case MSG_READ_BATT:
      send_v_batt();
      break;
    case MSG_OPEN_CHANNEL:
      setup_channel(&msg);
      break;
    case MSG_SET_CHAN_FILT:
      add_channel_filter(&msg);
      break;
    case MSG_REM_CHAN_FILT:
      del_channel_filter(&msg);
      break;
    case MSG_TX_CHAN_DATA:
      send_data(&msg);
      break;
    case MSG_CLOSE_CHANNEL:
      remove_channel(&msg);
      break;
    case MSG_IOCTL_SET:
      ioctl_set(&msg);
      break;
    case MSG_IOCTL_GET:
      ioctl_get(&msg);
      break;
    case MSG_GET_FW_VERSION:
      get_fw_version(&msg);
      break;
    default:
      break;
    }
  }
  channel_loop();

#if defined(CFG_MACCHINA_A0)
  //vTaskDelay(pdMS_TO_TICKS(2));
#endif
  #ifdef FW_TEST
  if (millis() - lastPing > 1000 && isConnected) {
    lastPing = millis();
    char buf[12];
    sprintf(buf, "PING %d", sizeof(CAN_FRAME));
    PCCOMM::log_message(buf);
  }
  #endif
}
 
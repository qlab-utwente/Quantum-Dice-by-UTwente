#ifndef ESPNOWHELPERS_H_
#define ESPNOWHELPERS_H_

#include "esp_wifi.h"
#include <esp_now.h>
#include <WiFi.h>

//#define WATCHDOGTIME 1000  //time between watchdog

// Define the struct_message structure
// Structure example to send and receive data
// Must match the sender structure
typedef struct struct_message {
  int message_type;  // 1. Watchdog; 2. Measured data
  DiceNumbers diceNumberUp;
  UpSide upSide;
  MeasuredAxises measureAxis;
} struct_message;

// Declare global variables
extern struct_message myData;
extern struct_message myDataRcv;

extern bool isDeviceA;
extern bool isDeviceB;

extern esp_now_peer_info_t peerInfo;
extern uint8_t *broadcastAddress;

extern int rssi;
void setBroadcastAdress();
void ESPinit();
bool sendMeasurements(DiceNumbers diceNumberUp, UpSide upSide, MeasuredAxises measureAxis);

// void entanglementConfirm();
bool sendMessage();
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type);

#endif /* ESPNOWHELPERS_H_ */
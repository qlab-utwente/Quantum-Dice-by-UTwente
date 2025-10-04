#include "Arduino.h"
#include "diceConfig.h"
#include "defines.h"
#include "Screenfunctions.h"
#include "StateMachine.h"
#include "ScreenStateDefs.h"
#include "ESPNowHelpers.h"

// Define and initialize the global variables
struct_message myData;
struct_message myDataRcv;

bool isDeviceA = false;
bool isDeviceB = false;

int rssi = 0;

esp_now_peer_info_t peerInfo = {};  // Initialize with an empty struct

uint8_t *broadcastAddress = nullptr;  // Initialize as null pointer

// void setBroadcastAdress() {
//   // Determine role based on #define
// #ifdef ROLE_A
//   debugln("This device is A");
//   broadcastAddress = macB;  // Communicate with ESP32 B
// #elif defined(ROLE_B)
//   debugln("This device is B");
//   broadcastAddress = macA;  // Communicate with ESP32 A
// #else
//   debugln("Role not defined! Define ROLE_A or ROLE_B.");
//   while (true) {}  // Halt execution
// #endif
// }

void readMacAddress() {
  debug("MAC Address is : ");
  debugln(WiFi.macAddress());
}

void ESPinit() {
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Determine role based on MAC address
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  if (memcmp(mac, deviceA_mac, 6) == 0) {
    isDeviceA = true;
    broadcastAddress = deviceB_mac;
    debugln("This device is Device A");
  } else if (memcmp(mac, deviceB_mac, 6) == 0) {
    isDeviceB = true;
    broadcastAddress = deviceA_mac;
    debugln("This device is Device B");
  }

  // Add peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
  esp_wifi_set_promiscuous(true);

  readMacAddress();
}

bool sendMeasurements(DiceNumbers diceNumberUp, UpSide upSide, MeasuredAxises measureAxis) {
  //debugln("Send Measurements");
  myData.message_type = 2;  //1. watch dog; 2. measured data
  myData.diceNumberUp = diceNumberUp;
  myData.upSide = upSide;
  myData.measureAxis = measureAxis;
  //  switch (measureAxis) {
  //   case MeasuredAxises::XAXIS:
  //     debugln("X-axis self");
  //     break;
  //   case MeasuredAxises::YAXIS:
  //     debugln("Y-axis self");
  //     break;
  //   case MeasuredAxises::ZAXIS:
  //     debugln("Z-axis self");
  //     break;
  //   case MeasuredAxises::ALL:
  //     debugln("All-axis self");
  //     break;
  // }
  return sendMessage();
}

bool sendMessage() {  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  return (result == ESP_OK);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myDataRcv, incomingData, sizeof(myData));

  if (myDataRcv.message_type == 2) {  //measurement data
    //debugln("measurement data received and processed");
    diceNumberSister = myDataRcv.diceNumberUp;
    upSideSister = myDataRcv.upSide;
    measureAxisSister = myDataRcv.measureAxis;

/*
    switch (measureAxisSister) {
      case MeasuredAxises::XAXIS:
        debugln("X-axis sister");
        break;
      case MeasuredAxises::YAXIS:
        debugln("Y-axis sister");
        break;
      case MeasuredAxises::ZAXIS:
        debugln("Z-axis sister");
        break;
      case MeasuredAxises::ALL:
        debugln("All-axis sister");
        break;
    }

    switch (diceNumberSister) {
      case DiceNumbers::NONE:
        debugln("received NONE");
        break;
      case DiceNumbers::ONE:
        debugln("received ONE");
        break;
      case DiceNumbers::TWO:
        debugln("received TWO");
        break;
      case DiceNumbers::THREE:
        debugln("received THREE");
        break;
      case DiceNumbers::FOUR:
        debugln("received FOUR");
        break;
      case DiceNumbers::FIVE:
        debugln("received FIVE");
        break;
      case DiceNumbers::SIX:
        debugln("received SIX");
        break;
    }
*/
  } else {
    debugln("message received but ignored");
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  static bool prevStatus = false;
  if (status != prevStatus) {
    debug("Last Packet Send Status: ");
    if (status == ESP_NOW_SEND_SUCCESS) {
      debugln("Delivery Success");
    } else {
      debugln("Delivery Fail");
      diceNumberSister = DiceNumbers::NONE;
      upSideSister = UpSide::NONE;
    }
    //    debugln(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    prevStatus = status;
  }
};

typedef struct {
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  for (size_t i = 0; i < 6; i++) {
    if (hdr->addr2[i] != broadcastAddress[i]) {
      return;
    }
    rssi = ppkt->rx_ctrl.rssi;
  }

  if (rssi > RSSILIMIT && rssi < -1) {
    //debug("RSSI: ");
    //debugln(rssi);
  }
}

#ifndef ESPNOWSENSOR_H_
#define ESPNOWSENSOR_H_

#define ESPNOW_WIFI_CHANNEL 6

#include <sys/_stdint.h>
#include <assert.h>
#include "esp_wifi.h"
#include <esp_now.h>
#include <WiFi.h>
#include "defines.h"
#include "Queue.h"

template <typename T>
class EspNowSensor
{
private:
  static EspNowSensor *instance;

private:
  void init(uint8_t *rssiCompareAddr);

  void addPeer(uint8_t *addr);

  void printMacAddress();
  void getMacAddress(uint8_t *addr);

  bool isCloseBy();

  bool send(T message, uint8_t *target);
  bool poll(T* message);

  void onDataRecv(const esp_now_recv_info_t *mac, const unsigned char*incomingData, int len);
  void onDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);  // CHANGED
  void promiscuousRxCb(void *buf, wifi_promiscuous_pkt_type_t type);

private:
  static void OnDataRecv(const esp_now_recv_info_t *mac, const unsigned char*incomingData, int len)
  {
    assert(instance);
    instance->onDataRecv(mac, incomingData, len);
  }

  static void OnDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)  // CHANGED
  {
    assert(instance);
    instance->onDataSend(tx_info, status);  // CHANGED
  }

  static void PromiscuousRxCb(void *buf, wifi_promiscuous_pkt_type_t type)
  {
    assert(instance);
    instance->promiscuousRxCb(buf, type);
  }
public:
  static void Init(uint8_t *rssiCompareAddr)
  {
    // Should only be initialized once
    assert(!instance);
    instance = new EspNowSensor<T>();
    instance->init(rssiCompareAddr);
  }

  static void AddPeer(uint8_t *addr)
  {
    assert(instance);
    instance->addPeer(addr);
  }
  static void PrintMacAddress()
  {
    assert(instance);
    instance->printMacAddress();
  }

  static void GetMacAddress(uint8_t *addr)
  {
    assert(instance);
    instance->getMacAddress(addr);
  }

  static bool IsCloseBy()
  {
    assert(instance);
    return instance->isCloseBy();
  }

  static bool Send(T message, uint8_t *target)
  {
    assert(instance);
    return instance->send(message, target);
  }

  static bool Poll(T* message)
  {
    assert(instance);
    return instance->poll(message);
  }
  
private:
  Queue<T> _messageQueue;

  uint8_t *_rssiCmpAddr;
  int _rssi;
};

// ================================================================================
// =============================== IMPLEMENTATION =================================
// ================================================================================

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

template<typename T>
EspNowSensor<T> *EspNowSensor<T>::instance = 0;

template<typename T>
void EspNowSensor<T>::init(uint8_t *rssiCompareAddr)
{
  _rssiCmpAddr = (uint8_t*)malloc(6 * sizeof(uint8_t));
  memcpy(_rssiCmpAddr, rssiCompareAddr, 6 * sizeof(uint8_t));

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  delay(1000);  // Give WiFi time to initialize

  esp_err_t result = esp_now_init();
  if (result != ESP_OK) {
    Serial.print("Error initializing ESP-NOW, error code: ");
    Serial.println(result);
    return;
  }

  // Register callback functions
  esp_now_register_send_cb(EspNowSensor<T>::OnDataSend);
  esp_now_register_recv_cb(EspNowSensor<T>::OnDataRecv);

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(EspNowSensor<T>::PromiscuousRxCb);
  esp_wifi_set_promiscuous(true);

  Serial.println("ESP-NOW initialized successfully!");

  printMacAddress();
}

template<typename T>
void EspNowSensor<T>::addPeer(uint8_t *addr)
{
  // Register peers (both sister and brother devices)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  memcpy(peerInfo.peer_addr, addr, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add brother peer");
  }
}

template<typename T>
void EspNowSensor<T>::printMacAddress()
{
  uint8_t addr[6];
  getMacAddress(addr);
  debug("MAC Address is : ");
  for (uint8_t i = 0; i < 6; i++) {
    Serial.print(addr[i] < 0x10 ? "0" : "");
    Serial.print(addr[i], HEX);
    if (i != 5) {
      debug(":");
    }
  }
  debug("\n");
}

template<typename T>
void EspNowSensor<T>::getMacAddress(uint8_t *addr)
{
  WiFi.macAddress(addr);
}

template<typename T>
bool EspNowSensor<T>::isCloseBy()
{
  //debugln(_rssi);
  return (_rssi > RSSILIMIT && _rssi < -1);
}

template<typename T>
bool EspNowSensor<T>::send(T message, uint8_t *target)
{
  T* toSend = (T*)malloc(sizeof(T));
  if (!toSend)
    return false;

  memcpy(toSend, &message, sizeof(T));
  
  esp_err_t result = esp_now_send(target, (uint8_t *)toSend, sizeof(T));
  return (result == ESP_OK);
}

template<typename T>
bool EspNowSensor<T>::poll(T* message)
{
  if (_messageQueue.isEmpty())
    return false;

  *message = _messageQueue.pop();
  return true;
}

template<typename T>
void EspNowSensor<T>::onDataRecv(const esp_now_recv_info_t *mac, const unsigned char*incomingData, int len)
{
  T message;
  memcpy(&message, incomingData, sizeof(T));
  _messageQueue.push(message);
}

template<typename T>
void EspNowSensor<T>::onDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)  // CHANGED
{
  static bool prevStatus = false;
  if (status != prevStatus) {
    debug("Last Packet Send Status: ");
    debugln(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    prevStatus = status;
  }
  
  // Optional: You can now access additional information if needed:
  // tx_info->des_addr - destination MAC address
  // tx_info->src_addr - source MAC address  
  // tx_info->tx_status - transmission status (alternative to 'status' parameter)
}

template<typename T>
void EspNowSensor<T>::promiscuousRxCb(void *buf, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  for (size_t i = 0; i < 6; i++) {
    if (hdr->addr2[i] != _rssiCmpAddr[i]) {
      return;
    }

    _rssi = ppkt->rx_ctrl.rssi;
  }
}

#endif /* ESPNOWSENSOR_H_ */
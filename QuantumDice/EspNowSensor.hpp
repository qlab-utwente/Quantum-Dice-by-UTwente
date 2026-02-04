#ifndef ESPNOWSENSOR_H_
#define ESPNOWSENSOR_H_

#define ESPNOW_WIFI_CHANNEL 6

#include "defines.hpp"
#include "Queue.hpp"

#include <esp_now.h>
#include <sys/_stdint.h>
#include <WiFi.h>

template<typename T>
struct temp {
    T message;
    uint8_t source[6];
    int32_t rssi;
};

template<typename T> class EspNowSensor {
    private:
        static EspNowSensor *instance;

        void init();

        void addPeer(uint8_t *addr);

        void printMacAddress();
        void getMacAddress(uint8_t *addr);

        auto send(T message, uint8_t *target) -> bool;
        auto poll(T *message, uint8_t *source, int32_t *rssi) -> bool;

        void onDataRecv(const esp_now_recv_info_t *mac, const unsigned char *incomingData, int len);
        void onDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);

        static void OnDataRecv(const esp_now_recv_info_t *mac, const unsigned char *incomingData,
                int len) {
            assert(instance);
            instance->onDataRecv(mac, incomingData, len);
        }

        static void OnDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
            assert(instance);
            instance->onDataSend(tx_info, status);
        }

    public:
        static void Init() {
            // Should only be initialized once
            assert(!instance);
            instance = new EspNowSensor<T>();
            instance->init();
        }

        static void AddPeer(uint8_t *addr) {
            assert(instance);
            instance->addPeer(addr);
        }

        static void PrintMacAddress() {
            assert(instance);
            instance->printMacAddress();
        }

        static void GetMacAddress(uint8_t *addr) {
            assert(instance);
            instance->getMacAddress(addr);
        }

        static auto Send(T message, uint8_t *target) -> bool {
            assert(instance);
            return instance->send(message, target);
        }

        static auto Poll(T *message, uint8_t *source, int32_t *rssi) -> bool {
            assert(instance);
            return instance->poll(message, source, rssi);
        }

    private:
        Queue<struct temp<T>> _messageQueue;
};

// ================================================================================
// =============================== IMPLEMENTATION
// =================================
// ================================================================================

using wifi_ieee80211_mac_hdr_t = struct {
    unsigned frame_ctrl  : 16;
    unsigned duration_id : 16;
    uint8_t  addr1[6]; /* receiver address */
    uint8_t  addr2[6]; /* sender address */
    uint8_t  addr3[6]; /* filtering address */
    unsigned sequence_ctrl : 16;
    uint8_t  addr4[6]; /* optional */
};

using wifi_ieee80211_packet_t = struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t                  payload[0]; /* network data ended with 4 bytes csum (CRC32) */
};

template<typename T> EspNowSensor<T> *EspNowSensor<T>::instance = 0;

template<typename T> void EspNowSensor<T>::init() {

    // Initialize the Wi-Fi module
    WiFiClass::mode(WIFI_STA);
    delay(1000); // Give WiFi time to initialize

    esp_err_t result = esp_now_init();
    if (result != ESP_OK) {
        Serial.print("Error initializing ESP-NOW, error code: ");
        Serial.println(result);
        return;
    }

    // Register callback functions
    esp_now_register_send_cb(EspNowSensor<T>::OnDataSend);
    esp_now_register_recv_cb(EspNowSensor<T>::OnDataRecv);

    uint8_t broadcast[6];
    memset((char *) broadcast, 0xFF, 6);
    this->addPeer(broadcast);

    Serial.println("ESP-NOW initialized successfully!");

    printMacAddress();
}

template<typename T> void EspNowSensor<T>::addPeer(uint8_t *addr) {
    // Register peers (both sister and brother devices)
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    memcpy((void *) peerInfo.peer_addr, addr, 6);
    const auto exists = esp_now_is_peer_exist(addr);
    if (exists) {
        Serial.println("Peer already exists - skipping add");
        return;
    }
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add brother peer");
    }
}

template<typename T> void EspNowSensor<T>::printMacAddress() {
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

template<typename T> void EspNowSensor<T>::getMacAddress(uint8_t *addr) {
    WiFi.macAddress(addr);
}

template<typename T> auto EspNowSensor<T>::send(T message, uint8_t *target) -> bool {
    T *toSend = (T *) malloc(sizeof(T));
    if (toSend == nullptr) {
        return false;
    }

    memcpy(toSend, &message, sizeof(T));

    esp_err_t result = esp_now_send(target, (uint8_t *)toSend, sizeof(T));
    return result == ESP_OK;
}

template<typename T> auto EspNowSensor<T>::poll(T *message, uint8_t *source, int32_t *rssi) -> bool {
    if (_messageQueue.isEmpty()) {
        return false;
    }

    struct temp<T> _temp = _messageQueue.pop();
    memcpy(message, &_temp.message, sizeof(T));
    memcpy(source, _temp.source, 6);
    *rssi = _temp.rssi;
    return true;
}

template<typename T>
void EspNowSensor<T>::onDataRecv(const esp_now_recv_info_t *mac, const unsigned char *incomingData,
        int  /*len*/) {
    struct temp<T> _temp;
    memcpy(&_temp.message, incomingData, sizeof(T));
    memcpy(_temp.source, mac->src_addr, 6);
    _temp.rssi = mac->rx_ctrl->rssi;
    _messageQueue.push(_temp);
}

template<typename T>
void EspNowSensor<T>::onDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    static bool prevStatus = false;
    if (status != static_cast<int>(prevStatus)) {
        debug("Last Packet Send Status: ");
        debugln(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
        prevStatus = (status != 0U);
    }
}

#endif /* ESPNOWSENSOR_H_ */

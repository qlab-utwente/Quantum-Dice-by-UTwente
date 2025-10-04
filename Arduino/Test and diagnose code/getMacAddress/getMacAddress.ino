#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Initialize WiFi to get the MAC address
  WiFi.mode(WIFI_STA);
  delay(1000);
  String macStr = WiFi.macAddress();

  // Print the original MAC address
  Serial.print("Original MAC Address: ");
  Serial.println(macStr);

  // Convert to hexadecimal list
  uint8_t macList[6];
  parseMacAddress(macStr, macList);

  // Print the MAC address as a list
  Serial.print("inline uint8_t deviceX_mac[6] = { ");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    Serial.print(macList[i], HEX);
    if (i < 5) Serial.print(", ");
  }
  Serial.println(" };");
  Serial.println("Copy line  ^ into diceConfig.h and replace X with A or B");
}

void loop() {}

// Function to parse MAC address string into a list of bytes
void parseMacAddress(String macStr, uint8_t* macList) {
  int j = 0;
  for (int i = 0; i < macStr.length(); i += 3) {
    macList[j] = strtol(macStr.substring(i, i + 2).c_str(), NULL, 16);
    j++;
  }
}

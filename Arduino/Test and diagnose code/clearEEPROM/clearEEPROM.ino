/*
 * ESP32-S3 EEPROM Clear Sketch
 * This sketch clears all EEPROM data by writing 0xFF to each address
 * ESP32-S3 uses flash memory to emulate EEPROM functionality
 */

#include <EEPROM.h>

#define EEPROM_SIZE 4096  // Define EEPROM size (4KB max for ESP32)

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32-S3 EEPROM Clear Utility");
  Serial.println("==============================");
  
  // Initialize EEPROM with defined size
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM!");
    return;
  }
  
  Serial.print("Clearing EEPROM (");
  Serial.print(EEPROM_SIZE);
  Serial.println(" bytes)...");
  
  // Clear all EEPROM data
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0xFF);  // Write 0xFF (255) to each address
    
    // Print progress every 256 bytes
    if (i % 256 == 0) {
      Serial.print("Progress: ");
      Serial.print((i * 100) / EEPROM_SIZE);
      Serial.println("%");
    }
  }
  
  // Commit changes to flash memory
  if (EEPROM.commit()) {
    Serial.println("EEPROM cleared successfully!");
  } else {
    Serial.println("Failed to commit EEPROM changes!");
  }
  
  // Verify the clear operation
  Serial.println("Verifying EEPROM clear...");
  bool clearSuccess = true;
  
  for (int i = 0; i < EEPROM_SIZE; i++) {
    if (EEPROM.read(i) != 0xFF) {
      Serial.print("Verification failed at address: ");
      Serial.println(i);
      clearSuccess = false;
      break;
    }
  }
  
  if (clearSuccess) {
    Serial.println("EEPROM clear verified successfully!");
  } else {
    Serial.println("EEPROM clear verification failed!");
  }
  
  Serial.println("==============================");
  Serial.println("Process complete.");
}

void loop() {
  // Nothing to do in loop
  delay(1000);
}
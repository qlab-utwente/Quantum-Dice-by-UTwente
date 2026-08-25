#ifndef DICE_CONFIG_MANAGER_H
#define DICE_CONFIG_MANAGER_H

#include <Arduino.h>
#include <array>

constexpr uint8_t MAX_ENTANGLEMENT_COLORS = 8;

// Configuration structure
struct DiceConfig {
    String   diceId;   // "TEST1", "BART1", etc.
    uint16_t x_background; // Display background colors
    uint16_t y_background;
    uint16_t z_background;
    std::array<uint16_t, MAX_ENTANGLEMENT_COLORS> entang_colors;    // Available entanglement colors (RGB565)
    uint8_t  entang_colors_count; // Number of colors in array
    uint16_t colorFlashTimeout;   // Color flash duration in milliseconds
    int8_t   rssiLimit;           // RSSI limit for entanglement detection
    float    tumbleConstant;      // Number of tumbles to detect tumbling
    uint32_t deepSleepTimeout;    // Deep sleep timeout in milliseconds
    uint8_t  checksum;            // Simple checksum for validation
};

class DiceConfigManager {
  public:
    // Constructor
    DiceConfigManager();

    // Initialize LittleFS and load config
    auto begin() -> bool;

    // Load configuration from file
    auto load() -> bool;
    auto load(const String &filename) -> bool;

    // Get/Set configuration
    auto getConfig() -> DiceConfig &;

    // Get last error message
    auto getLastError() -> String {
        return _lastError;
    }

    // Get the currently loaded config filename
    auto getConfigPath() -> String {
        return _configPath;
    }

    void setConfigPath(const String &path) {
        _configPath = path;
    }

  private:
    DiceConfig _config;
    String     _configPath = "/config.txt";
    String     _lastError  = "";

    // Internal parsing functions
    static auto parseBool(String str) -> bool;
    static void trim(String &str);
    static void calculateChecksum(DiceConfig &config);
    static auto validateChecksum(const DiceConfig &config) -> bool;
    void        setError(String error);

    // Default configuration values
    void initDefaultConfig();
};

// ============================================================================
// GLOBAL CONFIG ACCESS
// ============================================================================
// Global config instance - accessible from all libraries
extern DiceConfig currentConfig;

// Helper functions for global config management
auto loadGlobalConfig() -> bool;
void printGlobalConfig();

// Auto-detection helper
auto findConfigFile(String &foundPath, size_t maxLen) -> bool;

// Auto-initialization function (formats LittleFS and creates default config if needed)
auto ensureLittleFSAndConfig() -> bool;

// Internal helper (exposed for advanced use)
auto createDefaultConfigFile() -> bool;

#endif // DICE_CONFIG_MANAGER_H

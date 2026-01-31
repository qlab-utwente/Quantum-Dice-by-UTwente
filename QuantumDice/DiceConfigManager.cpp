#include "DiceConfigManager.hpp"
#include "FS.h"
#include "WString.h"
#include "defines.hpp"

#include <LittleFS.h>
#include <WiFi.h>
#include <cstdlib>

// Constructor
DiceConfigManager::DiceConfigManager() {
    initDefaultConfig();
}

// Initialize LittleFS and optionally load config
auto DiceConfigManager::begin() -> bool {
    if (!LittleFS.begin(false, "/littlefs", 10, "littlefs")) {
        setError("LittleFS mount failed");
        return false;
    }

    // Try to load existing config, otherwise use defaults
    if (!load()) {
        debugln("Config file not loaded, using defaults");
        initDefaultConfig();
        return true; // Not a critical error
    }

    return true;
}

// Load configuration from file
auto DiceConfigManager::load() -> bool {
    return load(_configPath);
}

auto DiceConfigManager::load(const String &filename) -> bool {
    File file = LittleFS.open(filename, FILE_READ, false);
    if (!file) {
        errorf("Failed to open config file: %s\n", filename.c_str());
        return false;
    }

    int  lineNum = 0;
    bool success = true;

    while (file.available()) {
        String strLine = file.readStringUntil('\n');
        size_t len     = strLine.length();
        lineNum++;

        // Remove carriage return if present
        if (len > 0 && strLine[len - 1] == '\r') {
            strLine.remove(len - 1);
        }

        trim(strLine);

        // Skip empty lines and comments
        if (strLine[0] == '\0' || strLine[0] == '#') {
            continue;
        }

        // Find the '=' separator
        int separatorIndex = strLine.indexOf('=');
        if (separatorIndex == -1) {
            warnf("Line %d: Invalid format (no '=')\n", lineNum);
            continue;
        }

        // Split into key and value
        String key = strLine.substring(0, separatorIndex);
        String value = strLine.substring(separatorIndex + 1);

        trim(key);

        // Strip inline comments from value (everything after #)
        int commentIndex = value.indexOf('#');
        if (commentIndex != -1) {
            value = value.substring(0, commentIndex);
        }

        trim(value);

        // Parse based on key
        if (key == "diceId") {
            _config.diceId = value;
        } else if (key == "x_background") {
            _config.x_background = (uint16_t) strtoul(value.c_str(), nullptr, 0);
        } else if (key == "y_background") {
            _config.y_background = (uint16_t) strtoul(value.c_str(), nullptr, 0);
        } else if (key == "z_background") {
            _config.z_background = (uint16_t) strtoul(value.c_str(), nullptr, 0);
        } else if (key == "entang_colors") {
            // Parse comma-separated color values
            _config.entang_colors_count = 0;
            String token = strtok(value.begin(), ",");
            while (token != nullptr && _config.entang_colors_count < MAX_ENTANGLEMENT_COLORS) {
                trim(token);
                _config.entang_colors[_config.entang_colors_count]
                  = (uint16_t) strtoul(token.c_str(), nullptr, 0);
                _config.entang_colors_count++;
                token = strtok(nullptr, ",");
            }
        } else if (key == "colorFlashTimeout") {
            _config.colorFlashTimeout = (uint16_t) strtoul(value.c_str(), nullptr, 0);
        } else if (key == "rssiLimit") {
            _config.rssiLimit = (int8_t) strtol(value.c_str(), nullptr, 0);
        } else if (key == "isSMD") {
            _config.isSMD = parseBool(value);
        } else if (key == "isNano") {
            _config.isNano = parseBool(value);
        } else if (key == "tumbleConstant") {
            _config.tumbleConstant = (float) strtod(value.c_str(), nullptr);
        } else if (key == "deepSleepTimeout") {
            _config.deepSleepTimeout = strtoul(value.c_str(), nullptr, 0);
        } else if (key == "checksum") {
            _config.checksum = (uint8_t)strtol(value.c_str(), nullptr, 0);
        } else {
            warnf("Line %d: Unknown key '%s'\n", lineNum, key.c_str());
        }
    }

    file.close();

    // Validate checksum if not 0
    if (_config.checksum != 0) {
        if (!validateChecksum(_config)) {
            setError("Checksum validation failed");
            debugln("Warning: Checksum validation failed!");
            success = false;
        }
    }

    if (success) {
        debugln("Config loaded successfully");
    }

    return success;
}

// Get configuration
auto DiceConfigManager::getConfig() -> DiceConfig & {
    return _config;
}

auto DiceConfigManager::parseBool(String str) -> bool {
    return strcasecmp(str.c_str(), "true") == 0 || strcmp(str.c_str(), "1") == 0;
}

void DiceConfigManager::trim(String &str) {
    // Trim leading space
    int start = 0;
    while (start < (int)str.length() && isspace((unsigned char)str[start]) != 0) {
        start++;
    }

    str = str.substring(start);

    // Trim trailing space
    int end = str.length() - 1;
    while (end >= 0 && isspace((unsigned char)str[end]) != 0) {
        end--;
    }
    str = str.substring(0, end + 1);
}

void DiceConfigManager::calculateChecksum(DiceConfig &config) {
    auto   *ptr = (uint8_t *) &config;
    uint8_t sum = 0;

    // XOR all bytes except the checksum field itself
    for (size_t i = 0; i < sizeof(DiceConfig) - 1; i++) {
        sum ^= ptr[i];
    }

    config.checksum = sum;
}

auto DiceConfigManager::validateChecksum(const DiceConfig &config) -> bool {
    DiceConfig temp             = config;
    uint8_t    originalChecksum = temp.checksum;
    calculateChecksum(temp);
    return temp.checksum == originalChecksum;
}

void DiceConfigManager::setError(String error) {
    _lastError = error;
    debugf("Error: %s\n", error.c_str());
}

void DiceConfigManager::initDefaultConfig() {
    // Default values
    _config.diceId = "DEFAULT";

    // Default colors (RGB565)
    _config.x_background = 0x0; // Black
    _config.y_background = 0x0; // Black
    _config.z_background = 0x0; // Black

    // Default entanglement colors: Yellow, Green, Cyan, Magenta
    _config.entang_colors[0]    = 0xFFE0; // Yellow
    _config.entang_colors[1]    = 0x07E0; // Green
    _config.entang_colors[2]    = 0x07FF; // Cyan
    _config.entang_colors[3]    = 0xF81F; // Magenta
    _config.entang_colors_count = 4;

    // Default color flash timeout (250ms)
    _config.colorFlashTimeout = 250;

    // Default RSSI
    _config.rssiLimit = -35;

    // Default hardware config
    _config.isSMD  = true;
    _config.isNano = false;

    _config.tumbleConstant = 45;

    // Default operational parameters
    _config.deepSleepTimeout = 300000; // 5 minutes

    // Checksum (will be calculated on save)
    _config.checksum = 0;
}

// ============================================================================
// GLOBAL CONFIG ACCESS IMPLEMENTATION
// ============================================================================

// Define the global config instance
DiceConfig currentConfig;

// Internal static config manager for global access
static DiceConfigManager globalConfigManager;

auto loadGlobalConfig() -> bool {
    if (!globalConfigManager.begin()) {
        errorln("Failed to load global config!");
        return false;
    }

    currentConfig = globalConfigManager.getConfig();
    return true;
}

void printGlobalConfig() {
    infoln("=== Global Configuration ===");
    infof("Dice ID: %s\n", currentConfig.diceId.c_str());
    infof("X Background: 0x%04X (%u)\n", currentConfig.x_background, currentConfig.x_background);
    infof("Y Background: 0x%04X (%u)\n", currentConfig.y_background, currentConfig.y_background);
    infof("Z Background: 0x%04X (%u)\n", currentConfig.z_background, currentConfig.z_background);
    String colours = "";
    for (uint8_t i = 0; i < currentConfig.entang_colors_count; i++) {
        colours += String(currentConfig.entang_colors[i], HEX);
        if (i < currentConfig.entang_colors_count - 1) {
            colours += ", ";
        }
    }
    infof("Entangle Colors (%d): %s\n", currentConfig.entang_colors_count, colours.c_str());
    infof("Color Flash Timeout: %d ms\n", currentConfig.colorFlashTimeout);
    infof("RSSI Limit: %d dBm\n", currentConfig.rssiLimit);
    infof("Is SMD: %s\n", currentConfig.isSMD ? "true" : "false");
    infof("Is Nano: %s\n", currentConfig.isNano ? "true" : "false");
    infof("Tumble Constant: %d\n", currentConfig.tumbleConstant);
    infof("Deep Sleep Timeout: %u ms\n", currentConfig.deepSleepTimeout);
    infof("Checksum: 0x%02X\n", currentConfig.checksum);
    infoln("============================");
}

// ============================================================================
// AUTO-INITIALIZATION AND VALIDATION FUNCTIONS
// ============================================================================

auto findConfigFile(String &foundPath, size_t maxLen) -> bool {
    File root = LittleFS.open("/");
    if (!root) {
        errorln("Failed to open root directory");
        return false;
    }

    if (!root.isDirectory()) {
        errorln("Root is not a directory");
        return false;
    }

    String firstMatch = "";

    File file = root.openNextFile();
    while (file) {
        String filename = String(file.name());

        // Check if filename matches pattern: *_config.txt
        if (filename.endsWith("_config.txt")) {
            debugln("Found config file: " + filename);

            // Store first match
            if (firstMatch == "") {
                firstMatch = filename;
            } else {
                if (firstMatch == "/DEFAULT_config.txt") {
                    warnln("Default config found earlier, preferring non-default");
                    firstMatch = filename; // Prefer non-default over default
                    break;
                }

                warnln("Multiple config files found, using: " + firstMatch);
                break;
            }
        }

        file = root.openNextFile();
    }

    root.close();

    if (firstMatch == "") {
        errorln("No files matching *_config.txt pattern found");
        return false;
    }

    // Ensure path starts with /
    if (firstMatch[0] != '/') {
        foundPath = "/";
        foundPath.concat(firstMatch);
    } else {
        foundPath = firstMatch;
    }
    return true;
}

auto ensureLittleFSAndConfig() -> bool {
    // Step 1: Ensure LittleFS is mounted (format if needed)
    debugln("Mounting LittleFS...");

    if (!LittleFS.begin(false, "/littlefs", 10, "littlefs")) {
        warnln("✗ Mount failed - formatting filesystem...");

        if (LittleFS.begin(true, "/littlefs", 10, "littlefs")) {  // true = format if needed
            warnln("✓ LittleFS formatted and mounted successfully!");
        } else {
            errorln("Failed to mount/format LittleFS!");
            return false;
        }
    }


    debugln("LittleFS mounted successfully");
    debugf("Total: %u bytes, Used: %u bytes\n", LittleFS.totalBytes(), LittleFS.usedBytes());

    // Step 2: Check if any config file exists
    String foundConfigPath = "";

    findConfigFile(foundConfigPath, 256);

    // Step 3: If no config exists, create a default one
    if (foundConfigPath == "") {
        debugln("No config file found, creating default config.txt...");

        if (!createDefaultConfigFile()) {
            infoln("Failed to create default config!");
            return false;
        }
    }

    // Step 4: Load the config into currentConfig
    globalConfigManager.setConfigPath(foundConfigPath);
    return loadGlobalConfig();
}

auto createDefaultConfigFile() -> bool {
    const char* filename = "/DEFAULT_config.txt";
    File file = LittleFS.open(filename, "w");
    if (!file) {
        debugf("Failed to create file: %s\n", filename);
        return false;
    }

    file.println("diceId=DEFAULT");
    file.println("x_background=0");
    file.println("y_background=0");
    file.println("z_background=0");
    file.println("entang_colors=65504,2016,2047,63519");
    file.println("colorFlashTimeout=250");
    file.println("rssiLimit=-35");
    file.println("isSMD=true");
    file.println("isNano=false");
    file.println("tumbleConstant=45");
    file.println("deepSleepTimeout=300000");
    file.println("checksum=0");

    file.close();

    debugf("Created default config file: %s\n", filename);

    return true;
}

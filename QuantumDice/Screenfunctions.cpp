#include "Screenfunctions.hpp"

#include "defines.hpp"
#include "DiceConfigManager.hpp"
#include "handyHelpers.hpp"
#include "ImageLibrary/ImageLibrary.hpp"
#include "ScreenStateDefs.hpp"

#include <Adafruit_GC9A01A.h>
#include <algorithm>
#include <Arduino.h>

// Global TFT object - will be initialized dynamically
Adafruit_GC9A01A tft(-1, -1, -1); // Temporary pins, will be reinitialized

// Global canvas declarations
static GFXcanvas16 backgroundCanvas(240, 240);
static GFXcanvas16 imageCanvas(240, 240);
static GFXcanvas16 staticCanvas(240, 100);

screenselections selectScreen;

void selectScreens(uint8_t binaryCode) {
    // Use hwPins from handyHelpers
    for (int i = 0; i < 6; i++) {
        if ((hwPins.screenAddress[binaryCode] & (1 << i)) != 0) {
            digitalWrite(hwPins.screen_cs[i], LOW); // Activate device
        } else {
            digitalWrite(hwPins.screen_cs[i], HIGH); // Deactivate device
        }
    }
}

void initDisplays() {
    Serial.println("Initializing displays...");

    // Initialize CS pins for all screens
    for (int i = 0; i < 6; i++) {
        pinMode(hwPins.screen_cs[i], OUTPUT);
        digitalWrite(hwPins.screen_cs[i], HIGH); // Start with all deactivated
    }

    // Reinitialize TFT with correct pins from configuration
    tft = Adafruit_GC9A01A(hwPins.tft_cs, hwPins.tft_dc, hwPins.tft_rst);

    selectScreens(ALL); // Select all screens
    delay(500);
    tft.begin(); // Initialize the display
    delay(1000);
    tft.fillScreen(GC9A01A_BLACK);

    selectScreens(XXYY);
    tft.setRotation(1);

    selectScreens(ZZ);
    tft.setRotation(2);

    selectScreens(NO_ONE); // Deactivate all screens
    delay(100);

    Serial.println("Displays initialized successfully!");
}

void blankScreen(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
}

// Function to blend colors with transparency
auto blendColor(uint16_t foreground, uint16_t background, float alpha) -> uint16_t {
    // Constrain alpha between 0 and 1
    alpha = constrain(alpha, 0.0, 1.0);

    // Extract color components
    uint8_t fR = (foreground >> 11) & 0x1F;
    uint8_t fG = (foreground >> 5) & 0x3F;
    uint8_t fB = foreground & 0x1F;

    uint8_t bR = (background >> 11) & 0x1F;
    uint8_t bG = (background >> 5) & 0x3F;
    uint8_t bB = background & 0x1F;

    // Blend color components
    uint8_t rR = ((fR * alpha) + (bR * (1 - alpha)));
    uint8_t rG = ((fG * alpha) + (bG * (1 - alpha)));
    uint8_t rB = ((fB * alpha) + (bB * (1 - alpha)));

    // Reconstruct 16-bit color
    return ((rR & 0x1F) << 11) | ((rG & 0x3F) << 5) | (rB & 0x1F);
}

// Function to draw dot on dice with transparency
void drawDot(int x, int y, float alpha, uint16_t color, uint16_t bgColor) {
    uint16_t blendedColor = blendColor(color, bgColor, alpha);
    tft.fillCircle(x, y, DOT_RADIUS, blendedColor);
}

void displayImageWithBackground(const unsigned short *image, uint8_t screens) {
    // Select the appropriate screens
    selectScreens(screens);

    // Choose background color based on screen - use config values
    uint16_t backgroundColor = 0;
    switch (screens) {
        case XX: backgroundColor = currentConfig.x_background; break;
        case YY: backgroundColor = currentConfig.y_background; break;
        case ZZ: backgroundColor = currentConfig.z_background; break;
        default: backgroundColor = 0x0000; // Black
    }

    // Fill background canvas with selected color
    backgroundCanvas.fillScreen(backgroundColor);

    // Clear image canvas
    imageCanvas.fillScreen(0x0000); // Transparent black

    // Draw image on image canvas
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Calculate pixel index
            uint32_t pixelIndex = (y * WIDTH) + x;

            // Read pixel color from PROGMEM
            uint16_t pixelColor = pgm_read_word(&image[pixelIndex]);

            // Draw non-transparent pixels
            if (pixelColor != 0x0000) {
                imageCanvas.drawPixel(x, y, pixelColor);
            }
        }
    }

    // Overlay image canvas onto background canvas
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Get pixel from image canvas
            uint16_t imagePixel = imageCanvas.getPixel(x, y);

            // If pixel is not transparent, draw it on background canvas
            if (imagePixel != 0x0000) {
                backgroundCanvas.drawPixel(x, y, imagePixel);
            }
        }
    }

    // Push final canvas to display
    tft.drawRGBBitmap(0, 0, backgroundCanvas.getBuffer(), WIDTH, HEIGHT);
}

void displayCircle(uint8_t screens) {
    displayImageWithBackground(circle, screens);
    debug("Circle on screen: ");
    debugln(screens);
}

void displayCross(uint8_t screens) {
    displayImageWithBackground(cross, screens);
    debug("Cross on screen: ");
    debugln(screens);
}

void displayCrossCircle(uint8_t screens) {
    displayImageWithBackground(crossCircle, screens);
    debug("CrossCircle on screen: ");
    debugln(screens);
}

void displayEinstein(uint8_t screens) {
    displayImageWithBackground(God_does_not_play_dice, screens);
    debug("Einstein on screen: ");
    debugln(screens);
}

void displayEntangled(uint8_t screens) {
    displayImageWithBackground(entangled, screens);
    debug("entangled on screen: ");
    debugln(screens);
}

void displayLowBattery(uint8_t screens) {
    selectScreens(screens);
    // Clear the screen
    tft.fillScreen(GC9A01A_BLACK);

    // Set text color
    tft.setTextColor(GC9A01A_RED);

    // First line configuration
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextSize(1);

    drawStringCentered(tft, "Low Battery", (tft.height() / 2) + 9);
}

void displayNewDie(uint8_t screens) {
    displayImageWithBackground(new_die, screens);
    debug("Reset Ok on screen: ");
    debugln(screens);
}

void displayQLab(uint8_t screens) {
    displayImageWithBackground(quantum_labs_twente_RGB, screens);
    debug("Qlab logo on screen: ");
    debugln(screens);
}

void displayUTlogo(uint8_t screens) {
    displayImageWithBackground(UTwente_logo, screens);
    debug("UTwente logo on screen: ");
    debugln(screens);
}

void displayQRcode(uint8_t screens) {
    displayImageWithBackground(QRCode, screens);
    debug("QR code on screen: ");
    debugln(screens);
}

void displayN1(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;

    drawDot(centerX, centerY);
}

void displayN2(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int offset  = DOT_OFFSET;

    drawDot(centerX - offset, centerY + offset);
    drawDot(centerX + offset, centerY - offset);
}

void displayN3(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int offset  = DOT_OFFSET;

    drawDot(centerX - offset, centerY + offset);
    drawDot(centerX, centerY);
    drawDot(centerX + offset, centerY - offset);
}

void displayN4(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int offset  = DOT_OFFSET;

    drawDot(centerX - offset, centerY - offset);
    drawDot(centerX + offset, centerY - offset);
    drawDot(centerX - offset, centerY + offset);
    drawDot(centerX + offset, centerY + offset);
}

void displayN5(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int offset  = DOT_OFFSET;

    drawDot(centerX - offset, centerY - offset);
    drawDot(centerX + offset, centerY - offset);
    drawDot(centerX, centerY, 1.0);
    drawDot(centerX - offset, centerY + offset);
    drawDot(centerX + offset, centerY + offset);
}

void displayN6(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int offset  = DOT_OFFSET;

    drawDot(centerX - offset, centerY - offset);
    drawDot(centerX + offset, centerY - offset);
    drawDot(centerX - offset, centerY);
    drawDot(centerX + offset, centerY);
    drawDot(centerX - offset, centerY + offset);
    drawDot(centerX + offset, centerY + offset);
}

void displayMix1to6(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int offset  = DOT_OFFSET;

    drawDot(centerX - offset, centerY - offset, (3 * 0.16) + 0.2);
    drawDot(centerX + offset, centerY - offset, (5 * 0.16) + 0.2);
    drawDot(centerX - offset, centerY, (1 * 0.16) + 0.2);
    drawDot(centerX + offset, centerY, 1 * 0.2);
    drawDot(centerX - offset, centerY + offset, (5 * 0.16) + 0.2);
    drawDot(centerX + offset, centerY + offset, (3 * 0.16) + 0.2);
    drawDot(centerX, centerY, 3 * 0.2);
}

void displayMix1to6_entangled(uint8_t screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int offset  = DOT_OFFSET;

    // Determine color to show
    uint16_t color;
    if (showColors) {
        // Show colors enabled - always show the entanglement color
        color = entanglement_color_self;
    } else {
        // Show colors disabled - check if we're in flash mode
        if (flashColor) {
            // Flash active - show actual color
            color = entanglement_color_self;
        } else {
            // Flash inactive - show white
            color = 0xFFFF;
        }
    }

    drawDot(centerX - offset, centerY - offset, (3 * 0.16) + 0.2, color);
    drawDot(centerX + offset, centerY - offset, (5 * 0.16) + 0.2, color);
    drawDot(centerX - offset, centerY, (1 * 0.16) + 0.2, color);
    drawDot(centerX + offset, centerY, (1 * 0.16) + 0.2, color);
    drawDot(centerX - offset, centerY + offset, (5 * 0.16) + 0.2, color);
    drawDot(centerX + offset, centerY + offset, (3 * 0.16) + 0.2, color);
    drawDot(centerX, centerY, (3 * 0.16) + 0.2, color);
}

void printChar(uint8_t screens, char *letters, uint16_t fontcolor, uint16_t bckcolor, int x,
               int y) {
    selectScreens(screens);
    tft.fillScreen(bckcolor);
    tft.setTextColor(fontcolor);
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextSize(1);
    tft.setCursor(x, y);
    tft.print(letters);
}

void drawStringCentered(Adafruit_GFX &gfx, const String &text, int16_t y) {
    // Variables to store text bounds
    int16_t  x1 = 0;
    int16_t  y1 = 0;
    uint16_t w  = 0;
    uint16_t h  = 0;

    // Get the text bounds
    gfx.getTextBounds(text, 0, y, &x1, &y1, &w, &h);

    // Calculate centered x-position
    int16_t x = (gfx.width() - w) / 2;

    // Set cursor
    gfx.setCursor(x, y);

    // Draw the text
    gfx.print(text);
}

void voltageIndicator(uint8_t screens) {
    char bufferV[10];
    char bufferPerc[10];
    selectScreens(screens);

    // Use hwPins.adc_pin from configuration
    double voltage = analogReadMilliVolts(hwPins.adc_pin) / 1000.0 * 2.0;
    double percentage
      = mapFloat((float)voltage, MINBATERYVOLTAGE, MAXBATERYVOLTAGE, 0.0, 100.0, true);
    percentage = std::min<double>(percentage, 100.0);
    dtostrf(voltage, 3, 2, (char *)bufferV);
    strcat((char *)bufferV, "V");
    dtostrf(percentage, 3, 0, (char *)bufferPerc);
    strcat((char *)bufferPerc, "%");

    // Clear the canvas
    staticCanvas.fillScreen(GC9A01A_BLACK);

    // Set text properties
    (percentage > 20) ? staticCanvas.setTextColor(GC9A01A_WHITE)
                      : staticCanvas.setTextColor(GC9A01A_RED);
    staticCanvas.setTextSize(1);
    staticCanvas.setFont(&FreeSans18pt7b);

    // Draw centered text
    int16_t  x1 = 0;
    int16_t  y1 = 0;
    uint16_t w  = 0;
    uint16_t h  = 0;

    // Center voltage text
    staticCanvas.getTextBounds((char *)bufferV, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (staticCanvas.width() - w) / 2;
    staticCanvas.setCursor(x, 30);
    staticCanvas.print((char *)bufferV);

    // Center percentage text
    staticCanvas.getTextBounds((char *)bufferPerc, 0, 0, &x1, &y1, &w, &h);
    x = (staticCanvas.width() - w) / 2;
    staticCanvas.setCursor(x, 70);
    staticCanvas.print((char *)bufferPerc);

    // Draw a horizontal line
    staticCanvas.drawFastHLine(0, 40, 5, GC9A01A_RED);

    // Draw the canvas to the TFT
    tft.drawRGBBitmap(0, 140, staticCanvas.getBuffer(), staticCanvas.width(),
                      staticCanvas.height());
}

void welcomeInfo(uint8_t screens) {
    char displayText1[10];
    char displayText2[20];
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextSize(1);
    tft.setFont(&FreeSans18pt7b);

    strcpy((char *)displayText1, "FW");
    strcat((char *)displayText1, VERSION);
    drawStringCentered(tft, (char *)displayText1, 62);

    // Use DICE_ID from config
    strcpy((char *)displayText2, (char *)currentConfig.diceId.c_str());
    drawStringCentered(tft, (char *)displayText2, 104);
}

void showConfigMode(uint8_t screens) {
    selectScreens(screens);
    // Clear the screen
    tft.fillScreen(GC9A01A_BLACK);

    // Set text color
    tft.setTextColor(GC9A01A_RED);

    // First line configuration
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextSize(1);

    drawStringCentered(tft, "Setup Mode", (tft.height() / 2) + 9);
}

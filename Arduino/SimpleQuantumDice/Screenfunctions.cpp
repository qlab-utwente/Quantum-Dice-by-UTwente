#include "Arduino.h"
#include "Version.h"
#include "diceConfig.h"
#include "IMUhelpers.h"
#include "handyHelpers.h"
#include "defines.h"
#include "ScreenStateDefs.h"
#include "ImageLibrary/ImageLibrary.h"
#include "ESPNowHelpers.h"
#include "Screenfunctions.h"

// Global TFT object definition
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

// Global canvas declarations
static GFXcanvas16 backgroundCanvas(240, 240);
static GFXcanvas16 imageCanvas(240, 240);
static GFXcanvas16 staticCanvas(240, 100);

screenselections selectScreen;

// Define CS pins for each device
//const int csPins[6] = { 4, 5, 6, 7, 15, 16 };  //ESP32-DEV Rink
const int csPins[6] = { 5, 6, 7, 8, 9, 10 };  //NANO ESP32
//const int csPins[6] = {2,3,4,5,6,7}; //NANO ESP32 arduino number pin


#if defined(HDR)
uint8_t screenAdress[] = {
  //singles
  0b00001000,  //x0
  0b00000010,  //x1
  0b00000100,  //y0
  0b00010000,  //y1
  0b00100000,  //z0
  0b00000001,  //z1
  // doubles
  0b00001010,  //xx
  0b00010100,  //yy
  0b00100001,  //zz
  //quarters
  0b00011110,
  0b00101011,
  0b00110101,
  //triples + / -
  0b00101100,  //x0y0z0
  0b00010011,  //x1y1z1
  //others
  0b00111111,
  0b00000000,
};
#endif

#if defined(SMD)
uint8_t screenAdress[] = {
  //singles
  0b00000100,  //x0
  0b00010000,  //x1
  0b00001000,  //y0
  0b00000010,  //y1
  0b00100000,  //z0
  0b00000001,  //z1
  // doubles
  0b00010100,  //xx
  0b00001010,  //yy
  0b00100001,  //zz
  //quarters
  0b00011110,
  0b00101011,
  0b00110101,
  //triples + / -
  0b00101100,  //x0y0z0
  0b00010011,  //x1y1z1
  //others
  0b00111111,
  0b00000000,
};
#endif

void initDisplays() {
  // parallel_set_outputs();
  // delay(1000);

  for (int i = 0; i < 6; i++) {
    pinMode(csPins[i], OUTPUT);
  }

  selectScreens(ALL);  //alles laag
  delay(500);
  tft.begin();  // Initialise the display
  delay(1000);
  tft.fillScreen(GC9A01A_BLACK);

  selectScreens(XXYY);
  tft.setRotation(1);

  selectScreens(ZZ);
  tft.setRotation(2);

  selectScreens(NO_ONE);  //no active screens
  delay(100);
  //  tft.fillScreen(BLACK);
  //  selectScreens(NONE);  //no active screens
}

void selectScreens(uint8_t binaryCode) {
  //Serial.println(screenAdress[binaryCode], BIN);
  //  parallel_write(screenAdress[i] ^ 63);
  for (int i = 0; i < 6; i++) {
    if (screenAdress[binaryCode] & (1 << i)) {
      digitalWrite(csPins[i], LOW);  // Activate device
    } else {
      digitalWrite(csPins[i], HIGH);  // Deactivate device
    }
  }
}

/* void parallel_set_inputs(void) {
  REG_WRITE(GPIO_ENABLE_W1TC_REG, 0xFF << PARALLEL_0);
}

void parallel_set_outputs(void) {
  REG_WRITE(GPIO_ENABLE_W1TS_REG, 0xFF << PARALLEL_0);
}

uint8_t parallel_read(void) {
  uint32_t input = REG_READ(GPIO_IN_REG);

  return (input >> PARALLEL_0);
}

void parallel_write(uint8_t value) {
  uint32_t output =
    (REG_READ(GPIO_OUT_REG) & ~(0xFF << PARALLEL_0)) | (((uint32_t)value) << PARALLEL_0);

  REG_WRITE(GPIO_OUT_REG, output);
} */

// Function to blend colors with transparency
uint16_t blendColor(uint16_t foreground, uint16_t background, float alpha) {
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
  uint8_t rR = (fR * alpha + bR * (1 - alpha));
  uint8_t rG = (fG * alpha + bG * (1 - alpha));
  uint8_t rB = (fB * alpha + bB * (1 - alpha));

  // Reconstruct 16-bit color
  return ((rR & 0x1F) << 11) | ((rG & 0x3F) << 5) | (rB & 0x1F);
}

// Function to draw dot on dice with transparency
void drawDot(int x, int y, float alpha, uint16_t color, uint16_t bgColor) {
  uint16_t blendedColor = blendColor(color, bgColor, alpha);
  tft.fillCircle(x, y, DOT_RADIUS, blendedColor);
}


void displayImageWithBackground(const unsigned short* image, uint8_t screens) {
  // Select the appropriate screens
  selectScreens(screens);

  // Choose background color based on screen
  uint16_t backgroundColor;
  switch (screens) {
    case XX:
      backgroundColor = X_BACKGROUND;
      break;
    case YY:
      backgroundColor = Y_BACKGROUND;
      break;
    case ZZ:
      backgroundColor = Z_BACKGROUND;
      break;
    default:
      backgroundColor = 0x0000;  // Black
  }

  // Fill background canvas with selected color
  backgroundCanvas.fillScreen(backgroundColor);

  // Clear image canvas
  imageCanvas.fillScreen(0x0000);  // Transparent black

  // Draw image on image canvas
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      // Calculate pixel index
      uint32_t pixelIndex = y * WIDTH + x;

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

void displayN1(uint8_t screens) {
  selectScreens(screens);
  tft.fillScreen(GC9A01A_BLACK);
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;

  drawDot(centerX, centerY);  //
}

void displayN2(uint8_t screens) {
  selectScreens(screens);
  tft.fillScreen(GC9A01A_BLACK);
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int offset = DOT_OFFSET;

  drawDot(centerX - offset, centerY + offset);
  drawDot(centerX + offset, centerY - offset);
}

void displayN3(uint8_t screens) {
  selectScreens(screens);
  tft.fillScreen(GC9A01A_BLACK);
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int offset = DOT_OFFSET;

  drawDot(centerX - offset, centerY + offset);
  drawDot(centerX, centerY);
  drawDot(centerX + offset, centerY - offset);
}

void displayN4(uint8_t screens) {
  selectScreens(screens);
  tft.fillScreen(GC9A01A_BLACK);
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int offset = DOT_OFFSET;

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
  int offset = DOT_OFFSET;

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
  int offset = DOT_OFFSET;

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
  int offset = DOT_OFFSET;

  drawDot(centerX - offset, centerY - offset, 3 * 0.2);
  drawDot(centerX + offset, centerY - offset, 5 * 0.2);
  drawDot(centerX - offset, centerY, 1 * 0.2);
  drawDot(centerX + offset, centerY, 1 * 0.2);
  drawDot(centerX - offset, centerY + offset, 5 * 0.2);
  drawDot(centerX + offset, centerY + offset, 3 * 0.2);
  drawDot(centerX, centerY, 3 * 0.2);
}

void blankScreen(uint8_t screens) {
  selectScreens(screens);
  tft.fillScreen(GC9A01A_BLACK);
}

void displayEinstein(uint8_t screens) {
  displayImageWithBackground(God_does_not_play_dice, screens);
  debug("Einstein on screen: ");
  debugln(screens);
}

void displayLowBattery(uint8_t screens) {
  selectScreens(screens);
  // Clear the screen
  tft.fillScreen(GC9A01A_BLACK);

  // Set text color
  tft.setTextColor(GC9A01A_RED);

  // First line configuration
  tft.setFont(&FreeSansBold18pt7b);  // Include the bold font
  tft.setTextSize(1);

  drawStringCentered(tft, "Low Battery", tft.height() / 2 + 9);
}

void printChar(uint8_t screens, char* letters, uint16_t fontcolor, uint16_t bckcolor, int x, int y) {  //not in use
  selectScreens(screens);
  tft.fillScreen(bckcolor);
  tft.setTextColor(fontcolor);
  tft.setFont(&FreeSansBold18pt7b);  // Include the bold font
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(letters);
}

void drawStringCentered(Adafruit_GFX& gfx, const String& text, int16_t y) {
  // Variables to store text bounds
  int16_t x1, y1;
  uint16_t w, h;

  // Get the text bounds
  gfx.getTextBounds(text, 0, y, &x1, &y1, &w, &h);

  // Calculate centered x-position
  int16_t x = (gfx.width() - w) / 2;

  // Set cursor
  // Note: For many fonts, we need to adjust the y to account for baseline
  gfx.setCursor(x, y);

  // Draw the text
  gfx.print(text);

  // Optional: Reset to default font if needed
  // tft.setFont();
}

void voltageIndicator(uint8_t screens) {
  char bufferV[10];
  char bufferPerc[10];
  selectScreens(screens);

  float voltage = analogReadMilliVolts(ADCpin) / 1000.0 * 2.0;
  float percentage = mapFloat(voltage, MINBATERYVOLTAGE, MAXBATERYVOLTAGE, 0.0, 100.0, true);
  if (percentage > 100.0) percentage = 100.0;
  dtostrf(voltage, 3, 2, bufferV);
  strcat(bufferV, "V");
  dtostrf(percentage, 3, 0, bufferPerc);
  strcat(bufferPerc, "%");

  // Clear the canvas
  staticCanvas.fillScreen(GC9A01A_BLACK);

  // Set text properties
  (percentage > 20) ? staticCanvas.setTextColor(GC9A01A_WHITE) : staticCanvas.setTextColor(GC9A01A_RED);
  staticCanvas.setTextSize(1);
  staticCanvas.setFont(&FreeSans18pt7b);

  // Draw centered text
  int16_t x1, y1;
  uint16_t w, h;

  // Center voltage text
  staticCanvas.getTextBounds(bufferV, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (staticCanvas.width() - w) / 2;
  staticCanvas.setCursor(x, 30);
  staticCanvas.print(bufferV);

  // Center percentage text
  staticCanvas.getTextBounds(bufferPerc, 0, 0, &x1, &y1, &w, &h);
  x = (staticCanvas.width() - w) / 2;
  staticCanvas.setCursor(x, 70);
  staticCanvas.print(bufferPerc);

  // Draw a horizontal line at appropriate y-position (adjusted for canvas coordinates)
  staticCanvas.drawFastHLine(0, 40, 5, GC9A01A_RED);

  // Draw the canvas to the TFT
  tft.drawRGBBitmap(0, 140, staticCanvas.getBuffer(), staticCanvas.width(), staticCanvas.height());
}

void welcomeInfo(uint8_t screens) {
  char displayText1[10];
  char displayText2[10];
  selectScreens(screens);
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextSize(1);
  tft.setFont(&FreeSans18pt7b);

  strcpy(displayText1, "V");
  strcat(displayText1, VERSION);
  drawStringCentered(tft, displayText1, 62);

  strcpy(displayText2, DICE_ID);  // Use strcpy to copy the string
  if (isDeviceA) {
    strcat(displayText2, " #A");
  }
  if (isDeviceB) {
    strcat(displayText2, " #B");
  }
  drawStringCentered(tft, displayText2, 104);
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

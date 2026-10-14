#include "Screenfunctions.hpp"

#include "defines.hpp"
#include "DiceConfigManager.hpp"
#include "ImageLibrary/ImageLibrary.hpp"
#include "ScreenStateDefs.hpp"
#include "StateMachine.hpp"

#include <Adafruit_GC9A01A.h>
#include <algorithm>
#include <Arduino.h>
#include <driver/gpio.h>
#include <cmath>

#include "Battery.hpp"

extern StateMachine stateMachine;

// Global TFT object - will be initialized dynamically
Adafruit_GC9A01A tft(-1, -1, -1); // Temporary pins, will be reinitialized

// Global canvas declarations
static GFXcanvas16 dotsCanvas(WIDTH, HEIGHT);
static GFXcanvas16 backgroundCanvas(240, 240);
static GFXcanvas16 imageCanvas(240, 240);
static GFXcanvas16 staticCanvas(240, 100);

static const gpio_config_t io_config{
	.pin_bit_mask = (1 << SCREEN_CS1) | (1 << SCREEN_CS2) | (1 << SCREEN_CS3) | (1 << SCREEN_CS4) | (1 << SCREEN_CS5) | (1 << SCREEN_CS6),
	.mode = GPIO_MODE_OUTPUT,
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE,
	.intr_type = GPIO_INTR_DISABLE
};

static void selectScreens(screenselections screens) {
	uint8_t binaryCode = static_cast<uint8_t>(screens);

	gpio_set_level(SCREEN_CS1, ((binaryCode & 0b000001) != 0) ? 0 : 1);
	gpio_set_level(SCREEN_CS2, ((binaryCode & 0b000010) != 0) ? 0 : 1);
	gpio_set_level(SCREEN_CS3, ((binaryCode & 0b000100) != 0) ? 0 : 1);
	gpio_set_level(SCREEN_CS4, ((binaryCode & 0b001000) != 0) ? 0 : 1);
	gpio_set_level(SCREEN_CS5, ((binaryCode & 0b010000) != 0) ? 0 : 1);
	gpio_set_level(SCREEN_CS6, ((binaryCode & 0b100000) != 0) ? 0 : 1);
}

// Function to blend colors with transparency
static constexpr uint16_t blendColor(const uint16_t foreground, const uint16_t background, float alpha) noexcept {
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
static void drawDot(const int x, const int y, const float alpha = 1.0F, const uint16_t color = GC9A01A_WHITE, const uint16_t backgroundColor = GC9A01A_BLACK) {
	const uint16_t blendedColor = blendColor(color, backgroundColor, alpha);
	dotsCanvas.fillCircle(x, y, DOT_RADIUS, blendedColor);
}

void initDisplays() {
    Serial.println("Initializing displays...");

    // Initialize CS pins for all screens
    gpio_config(&io_config);

    // Reinitialize TFT with correct pins from configuration
    tft = Adafruit_GC9A01A(-1, SCREEN_DC, SCREEN_RST);

    selectScreens(ALL); // Select all screens
    delay(500);
    tft.begin(40'000'000);
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

void blankScreen(screenselections screens) {
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
}

void displayImageWithBackground(const unsigned short *image, screenselections screens) {
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

void displayCircle(screenselections screens) {
    displayImageWithBackground(circle, screens);
    debug("Circle on screen: ");
    debugln(screens);
}

void displayCross(screenselections screens) {
    displayImageWithBackground(cross, screens);
    debug("Cross on screen: ");
    debugln(screens);
}

void displayCrossCircle(screenselections screens) {
    displayImageWithBackground(crossCircle, screens);
    debug("CrossCircle on screen: ");
    debugln(screens);
}

void displayEinstein(screenselections screens) {
    displayImageWithBackground(God_does_not_play_dice, screens);
    debug("Einstein on screen: ");
    debugln(screens);
}

void displayEntangled(screenselections screens) {
    displayImageWithBackground(entangled, screens);
    debug("entangled on screen: ");
    debugln(screens);
}

void displayLowBattery(screenselections screens) {
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

void displayNewDie(screenselections screens) {
    displayImageWithBackground(new_die, screens);
    debug("Reset Ok on screen: ");
    debugln(screens);
}

void displayQLab(screenselections screens) {
    displayImageWithBackground(quantum_labs_twente_RGB, screens);
    debug("Qlab logo on screen: ");
    debugln(screens);
}

void displayUTlogo(screenselections screens) {
    displayImageWithBackground(UTwente_logo, screens);
    debug("UTwente logo on screen: ");
    debugln(screens);
}

void displayQRcode(screenselections screens) {
    displayImageWithBackground(QRCode, screens);
    debug("QR code on screen: ");
    debugln(screens);
}

void displayN1(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X, CENTER_Y);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void displayN2(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y + DOT_OFFSET);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y - DOT_OFFSET);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void displayN3(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y + DOT_OFFSET);
    drawDot(CENTER_X, CENTER_Y);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y - DOT_OFFSET);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void displayN4(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y - DOT_OFFSET);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y - DOT_OFFSET);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y + DOT_OFFSET);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y + DOT_OFFSET);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void displayN5(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y - DOT_OFFSET);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y - DOT_OFFSET);
    drawDot(CENTER_X, CENTER_Y);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y + DOT_OFFSET);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y + DOT_OFFSET);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void displayN6(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y - DOT_OFFSET);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y - DOT_OFFSET);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y + DOT_OFFSET);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y + DOT_OFFSET);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void displayMix1to6(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y - DOT_OFFSET, (3 * 0.16) + 0.2);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y - DOT_OFFSET, (5 * 0.16) + 0.2);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y, (1 * 0.16) + 0.2);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y, 1 * 0.2);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y + DOT_OFFSET, (5 * 0.16) + 0.2);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y + DOT_OFFSET, (3 * 0.16) + 0.2);
    drawDot(CENTER_X, CENTER_Y, 3 * 0.2);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void displayMix1to6_entangled(screenselections screens) {
	constexpr int CENTER_X = (WIDTH / 2);
	constexpr int CENTER_Y = (HEIGHT / 2);
	const uint16_t color = stateMachine.isColorFlash() ? stateMachine.getEntanglementColor() : GC9A01A_WHITE;

	// Draw to the canvas.
    dotsCanvas.fillScreen(GC9A01A_BLACK);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y - DOT_OFFSET, (3 * 0.16) + 0.2, color);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y - DOT_OFFSET, (5 * 0.16) + 0.2, color);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y, (1 * 0.16) + 0.2, color);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y, (1 * 0.16) + 0.2, color);
    drawDot(CENTER_X - DOT_OFFSET, CENTER_Y + DOT_OFFSET, (5 * 0.16) + 0.2, color);
    drawDot(CENTER_X + DOT_OFFSET, CENTER_Y + DOT_OFFSET, (3 * 0.16) + 0.2, color);
    drawDot(CENTER_X, CENTER_Y, (3 * 0.16) + 0.2, color);

	// Select screens and draw dots to screen.
    selectScreens(screens);
	tft.drawRGBBitmap(0, 0, dotsCanvas.getBuffer(), dotsCanvas.width(), dotsCanvas.height());
}

void printChar(screenselections screens, char *letters, uint16_t fontcolor, uint16_t bckcolor, int x,
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

void voltageIndicator(screenselections screens) {
	char bufferV[10];
	char bufferPerc[10];
	selectScreens(screens);

	float batteryStateOfCharge = Battery.getStateOfCharge();
	float batteryVoltage = Battery.getVoltage();

    dtostrf(batteryVoltage, 3, 2, (char *)bufferV);
    strcat((char *)bufferV, "V");
    dtostrf(batteryStateOfCharge, 3, 0, (char *)bufferPerc);
    strcat((char *)bufferPerc, "%");

    // Clear the canvas
    staticCanvas.fillScreen(GC9A01A_BLACK);

    // Set text properties
    (batteryStateOfCharge > 20.0f) ? staticCanvas.setTextColor(GC9A01A_WHITE)
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

void welcomeInfo(screenselections screens) {
    const char *generationText = "GEN 4.0";
    char displayText2[20];
    selectScreens(screens);
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextSize(1);
    tft.setFont(&FreeSans18pt7b);

    drawStringCentered(tft, (char *)generationText, 62);

    // Use DICE_ID from config
    strcpy((char *)displayText2, (char *)currentConfig.diceId.c_str());
    drawStringCentered(tft, (char *)displayText2, 104);
}

void showConfigMode(screenselections screens) {
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

struct verticalLine {
	int16_t x;
	int16_t y;
	int16_t height;
};

static const struct verticalLine DOT_LINES[21] = {
	{ -20, -4,  9 },
	{ -19, -7,  15 },
	{ -18, -9,  19 },
	{ -17, -11, 23 },
	{ -16, -12, 25 },
	{ -15, -13, 27 },
	{ -14, -14, 29 },
	{ -13, -15, 31 },
	{ -12, -16, 33 },
	{ -11, -17, 35 },
	{ -10, -17, 35 },
	{  -9, -18, 37 },
	{  -8, -18, 37 },
	{  -7, -19, 39 },
	{  -6, -19, 39 },
	{  -5, -19, 39 },
	{  -4, -20, 41 },
	{  -3, -20, 41 },
	{  -2, -20, 41 },
	{  -1, -20, 41 },
	{   0, -20, 41 }
};
static constexpr int ANIMATE_CENTER_X = (WIDTH / 2);
static constexpr int ANIMATE_CENTER_Y = (HEIGHT / 2);
static constexpr uint16_t ANIMATE_COLOR0 = blendColor(GC9A01A_WHITE, GC9A01A_BLACK, (3.0F * 0.16F) + 0.2F);
static constexpr uint16_t ANIMATE_COLOR1 = blendColor(GC9A01A_WHITE, GC9A01A_BLACK, (5.0F * 0.16F) + 0.2F);
static constexpr uint16_t ANIMATE_COLOR2 = blendColor(GC9A01A_WHITE, GC9A01A_BLACK, (1.0F * 0.16F) + 0.2F);
static constexpr uint16_t ANIMATE_COLOR3 = blendColor(GC9A01A_WHITE, GC9A01A_BLACK, 0.2F);
static constexpr uint16_t ANIMATE_COLOR4 = blendColor(GC9A01A_WHITE, GC9A01A_BLACK, (5.0F * 0.16F) + 0.2F);
static constexpr uint16_t ANIMATE_COLOR5 = blendColor(GC9A01A_WHITE, GC9A01A_BLACK, 3.0F * 0.2F);
static constexpr float ANIMATE_FULL_ROTATION_TIME = 1.25F;
static unsigned long animateStartTime = NAN;
static float animatePreviousAngleCos = NAN;
static float animatePreviousAngleSin = NAN;

void animateRestart() {
	selectScreens(ALL);
	animateStartTime = millis();
	animatePreviousAngleCos = 1.0F;
	animatePreviousAngleSin = 0.0F;

	tft.fillScreen(GC9A01A_BLACK);
	tft.fillCircle(ANIMATE_CENTER_X - DOT_OFFSET, ANIMATE_CENTER_Y - DOT_OFFSET, DOT_RADIUS, ANIMATE_COLOR0);
	tft.fillCircle(ANIMATE_CENTER_X + DOT_OFFSET, ANIMATE_CENTER_Y - DOT_OFFSET, DOT_RADIUS, ANIMATE_COLOR1);
	tft.fillCircle(ANIMATE_CENTER_X - DOT_OFFSET, ANIMATE_CENTER_Y, DOT_RADIUS, ANIMATE_COLOR2);
	tft.fillCircle(ANIMATE_CENTER_X + DOT_OFFSET, ANIMATE_CENTER_Y, DOT_RADIUS, ANIMATE_COLOR3);
	tft.fillCircle(ANIMATE_CENTER_X - DOT_OFFSET, ANIMATE_CENTER_Y + DOT_OFFSET, DOT_RADIUS, ANIMATE_COLOR1);
	tft.fillCircle(ANIMATE_CENTER_X + DOT_OFFSET, ANIMATE_CENTER_Y + DOT_OFFSET, DOT_RADIUS, ANIMATE_COLOR0);
    tft.fillCircle(ANIMATE_CENTER_X, ANIMATE_CENTER_Y, DOT_RADIUS, ANIMATE_COLOR5);
}

static void animatePlaceDot(const int dotX, const int dotY, struct verticalLine *output) noexcept {
	static constexpr int lastIndex = 2 * DOT_RADIUS;
	for (int i = 0; i < (DOT_RADIUS + 1); i++) {
		const int inverseI = lastIndex - i;
		output[i].x = dotX + DOT_LINES[i].x;
		output[i].y = dotY + DOT_LINES[i].y;
		output[i].height = DOT_LINES[i].height;
		if (inverseI != i) {
			output[inverseI].x = dotX - DOT_LINES[i].x;
			output[inverseI].y = dotY + DOT_LINES[i].y;
			output[inverseI].height = DOT_LINES[i].height;
		}
	}
}

static void animateDrawDotMask(const struct verticalLine *dotToDraw, const struct verticalLine *mask, const uint16_t color) noexcept {
	tft.startWrite();

	// i is iterator for dotToDraw.
	// j is iterator for mask.
	int i = 0, j = 0;

	// Skip over mask lines that are to the left of the dotToDraw lines.
	while (j < (2 * DOT_RADIUS + 1)) {
		if (mask[j].x >= dotToDraw[i].x) {
			break;
		}
		j++;
	}

	// Quickly do dotToDraw lines that are to the left of mask lines.
	while (i < (2 * DOT_RADIUS + 1)) {
		if (mask[j].x <= dotToDraw[i].x) {
			break;
		}
		tft.writeFastVLine(dotToDraw[i].x, dotToDraw[i].y, dotToDraw[i].height, color);
		i++;
	}

	// Loop over the lines where there are dotToDraw lines and mask lines.
	while (i < (2 * DOT_RADIUS + 1) && (j < 2 * DOT_RADIUS + 1)) {
		const int16_t maskLow = mask[j].y;
		const int16_t maskHigh = maskLow + mask[j].height - 1;

		const int16_t dotLow = dotToDraw[i].y;
		const int16_t dotHigh = dotLow + dotToDraw[i].height - 1;

		if (maskHigh < dotLow || maskLow > dotHigh) {
			tft.writeFastVLine(dotToDraw[i].x, dotToDraw[i].y, dotToDraw[i].height, color);
		} else if (maskLow <= dotLow && maskHigh < dotHigh) {
			const int16_t y = maskHigh + 1;
			const int16_t height = dotHigh - maskHigh;
			tft.writeFastVLine(dotToDraw[i].x, y, height, color);
		} else if (maskLow > dotLow && maskHigh >= dotHigh) {
			const int16_t y = dotLow;
			const int16_t height = maskLow - y;
			tft.writeFastVLine(dotToDraw[i].x, y, height, color);
		} else if (maskLow > dotLow && maskHigh < dotHigh) {
			const int16_t y0 = dotLow;
			const int16_t height0 = maskLow - y0;

			const int16_t y1 = maskHigh + 1;
			const int16_t height1 = dotHigh - maskHigh;

			const int16_t x = dotToDraw[i].x;

			tft.writeFastVLine(x, y0, height0, color);
			tft.writeFastVLine(x, y1, height1, color);
		}

		i++;
		j++;
	}

	// Quickly do dotToDraw lines that are to the right of mask lines.
	while (i < (2 * DOT_RADIUS + 1)) {
		tft.writeFastVLine(dotToDraw[i].x, dotToDraw[i].y, dotToDraw[i].height, color);
		i++;
	}

	tft.endWrite();
}

static void animateUpdateDot(const int offsetX, const int offsetY, const uint16_t color, const float angleCos, const float angleSin) noexcept {
	const int dotX = static_cast<int>(static_cast<float>(offsetX) * angleCos - static_cast<float>(offsetY) * angleSin) + ANIMATE_CENTER_X;
	const int dotY = static_cast<int>(static_cast<float>(offsetX) * angleSin + static_cast<float>(offsetY) * angleCos) + ANIMATE_CENTER_Y;

	const int previousDotX = static_cast<int>(static_cast<float>(offsetX) * animatePreviousAngleCos - static_cast<float>(offsetY) * animatePreviousAngleSin) + ANIMATE_CENTER_X;
	const int previousDotY = static_cast<int>(static_cast<float>(offsetX) * animatePreviousAngleSin + static_cast<float>(offsetY) * animatePreviousAngleCos) + ANIMATE_CENTER_Y;

	struct verticalLine oldDot[2 * DOT_RADIUS + 1];
	animatePlaceDot(previousDotX, previousDotY, oldDot);

	struct verticalLine newDot[2 * DOT_RADIUS + 1];
	animatePlaceDot(dotX, dotY, newDot);

	animateDrawDotMask(newDot, oldDot, color);
	animateDrawDotMask(oldDot, newDot, GC9A01A_BLACK);

	/* tft.startWrite();
	for (int i = 0; i < (2 * DOT_RADIUS + 1); i++) {
		tft.writeFastVLine(oldDot[i].x, oldDot[i].y, oldDot[i].height, GC9A01A_BLACK);
	}
	for (int i = 0; i < (2 * DOT_RADIUS + 1); i++) {
		tft.writeFastVLine(newDot[i].x, newDot[i].y, newDot[i].height, color);
	}
	tft.endWrite(); */
}

void animateUpdate() {
	const unsigned long now = millis();
	const float timeElapsed = static_cast<float>(now - animateStartTime) / 1000.0F;
	const float angle = fmod(TWO_PI * timeElapsed / ANIMATE_FULL_ROTATION_TIME, TWO_PI);

	const float angleCos = cosf(angle);
	const float angleSin = sinf(angle);

	animateUpdateDot(-DOT_OFFSET, -DOT_OFFSET, ANIMATE_COLOR0, angleCos, angleSin);
	animateUpdateDot( DOT_OFFSET, -DOT_OFFSET, ANIMATE_COLOR1, angleCos, angleSin);
	animateUpdateDot(-DOT_OFFSET,  0, ANIMATE_COLOR2, angleCos, angleSin);
	animateUpdateDot( DOT_OFFSET,  0, ANIMATE_COLOR3, angleCos, angleSin);
	animateUpdateDot(-DOT_OFFSET,  DOT_OFFSET, ANIMATE_COLOR1, angleCos, angleSin);
	animateUpdateDot( DOT_OFFSET,  DOT_OFFSET, ANIMATE_COLOR0, angleCos, angleSin);

	animatePreviousAngleCos = angleCos;
	animatePreviousAngleSin = angleSin;
	const float timeToRender = static_cast<float>(millis() - now) / 1000.0F;
	debugf("Time to render: %.2f\n", timeToRender);
}

#ifndef SCREENFUNCTIONS_H_
#define SCREENFUNCTIONS_H_
#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansOblique12pt7b.h>
#include <SPI.h>
#include <cstdint>
#include <driver/gpio.h>

#define HEIGHT 240
#define WIDTH 240

// Dot radius definition
#define DOT_RADIUS 20
#define DOT_OFFSET 60

constexpr int8_t SCREEN_DC = GPIO_NUM_47;
constexpr int8_t SCREEN_RST = GPIO_NUM_48;

constexpr gpio_num_t SCREEN_CS1 = GPIO_NUM_4;
constexpr gpio_num_t SCREEN_CS2 = GPIO_NUM_5;
constexpr gpio_num_t SCREEN_CS3 = GPIO_NUM_6;
constexpr gpio_num_t SCREEN_CS4 = GPIO_NUM_7;
constexpr gpio_num_t SCREEN_CS5 = GPIO_NUM_15;
constexpr gpio_num_t SCREEN_CS6 = GPIO_NUM_16;

// Screen selection enum (moved from conditional compilation)
enum screenselections : uint8_t {
    X0 = 0b00000100,
    X1 = 0b00010000,
    Y0 = 0b00001000,
    Y1 = 0b00000010,
    Z0 = 0b00100000,
    Z1 = 0b00000001,
    XX = X0 | X1,
    YY = Y0 | Y1,
    ZZ = Z0 | Z1,
    XXYY = XX | YY,
    XXZZ = XX | ZZ,
    YYZZ = YY | ZZ,
    ODD = X0 | Y0 | Z0,
    EVEN = X1 | Y1 | Z1,
    ALL = ODD | EVEN,
    NO_ONE = 0
};

auto blendColor(uint16_t foreground, uint16_t background, float alpha) -> uint16_t;
void drawDot(int x, int y, float alpha = 1.0, uint16_t color = GC9A01A_WHITE,
             uint16_t bgColor = GC9A01A_BLACK);
void displayImageWithBackground(const unsigned short *image, screenselections screens);
void initDisplays();
void blankScreen(screenselections screens);
void displayCircle(screenselections screens);
void displayCross(screenselections screens);
void displayCrossCircle(screenselections screens);
void displayEinstein(screenselections screens);
void displayEntangled(screenselections screens);
void displayLowBattery(screenselections screens);
void displayNewDie(screenselections screens);
void displayQLab(screenselections screens);
void displayUTlogo(screenselections screens);
void displayQRcode(screenselections screens);
void display1to6(screenselections screens);
void display1to6_ent(screenselections screens);
void displayN1(screenselections screens);
void displayN2(screenselections screens);
void displayN3(screenselections screens);
void displayN4(screenselections screens);
void displayN5(screenselections screens);
void displayN6(screenselections screens);
void displayMix1to6(screenselections screens);
void displayMix1to6_entangled(screenselections screens);
void printChar(screenselections screens, char *letters, uint16_t fontcolor, uint16_t bckcolor, int x, int y);
void voltageIndicator(screenselections screens);
void welcomeInfo(screenselections screens);
void drawStringCentered(Adafruit_GFX &gfx, const String &text, int16_t y);
void showConfigMode(screenselections screens);

#endif /* SCREENFUNCTIONS_H_ */

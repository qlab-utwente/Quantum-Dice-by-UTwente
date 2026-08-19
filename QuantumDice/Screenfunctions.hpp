#ifndef SCREENFUNCTIONS_H_
#define SCREENFUNCTIONS_H_
#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansOblique12pt7b.h>
#include <SPI.h>

#define HEIGHT 240
#define WIDTH 240

// Dot radius definition
#define DOT_RADIUS 20
#define DOT_OFFSET 60

// Screen selection enum (moved from conditional compilation)
enum screenselections : uint8_t {
    X0,
    X1,
    Y0,
    Y1,
    Z0,
    Z1,
    XX,
    YY,
    ZZ,
    XXYY,
    XXZZ,
    YYZZ,
    ODD,
    EVEN,
    ALL,
    NO_ONE
};

extern screenselections selectScreen;
extern int              dotDia;

void selectScreens(uint8_t binaryCode);
auto blendColor(uint16_t foreground, uint16_t background, float alpha) -> uint16_t;
void drawDot(int x, int y, float alpha = 1.0, uint16_t color = GC9A01A_WHITE,
             uint16_t bgColor = GC9A01A_BLACK);
void displayImageWithBackground(const unsigned short *image, uint8_t screens);
void initDisplays();
void blankScreen(uint8_t screens);
void displayCircle(uint8_t screens);
void displayCross(uint8_t screens);
void displayCrossCircle(uint8_t screens);
void displayEinstein(uint8_t screens);
void displayEntangled(uint8_t screens);
void displayLowBattery(uint8_t screens);
void displayNewDie(uint8_t screens);
void displayQLab(uint8_t screens);
void displayUTlogo(uint8_t screens);
void displayQRcode(uint8_t screens);
void display1to6(uint8_t screens);
void display1to6_ent(uint8_t screens);
void displayN1(uint8_t screens);
void displayN2(uint8_t screens);
void displayN3(uint8_t screens);
void displayN4(uint8_t screens);
void displayN5(uint8_t screens);
void displayN6(uint8_t screens);
void displayMix1to6(uint8_t screens);
void displayMix1to6_entangled(uint8_t screens);
void printChar(uint8_t screens, char *letters, uint16_t fontcolor, uint16_t bckcolor, int x, int y);
void voltageIndicator(uint8_t screens);
void welcomeInfo(uint8_t screens);
void drawStringCentered(Adafruit_GFX &gfx, const String &text, int16_t y);
void showConfigMode(uint8_t screens);

#endif /* SCREENFUNCTIONS_H_ */

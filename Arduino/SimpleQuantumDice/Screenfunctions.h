#ifndef SCREENFUNCTIONS_H_
#define SCREENFUNCTIONS_H_
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <Fonts/FreeSans18pt7b.h> //quote
#include <Fonts/FreeSansBold18pt7b.h> // Include the bold font
#include <Fonts/FreeSansOblique12pt7b.h> //signature

// Pin definitions
#define TFT_CS 21         // To display chip-select pin
#define TFT_RST 4         // To display reset pin
#define TFT_DC 2          // To display data/command pin

//#define PARALLEL_0 5  //addresss
#define NUMBER_SCREENS 6

#define HEIGHT 240
#define WIDTH 240

// Dot radius definition
#define DOT_RADIUS 20
#define DOT_OFFSET 60

// Create display object
//extern Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
//extern Adafruit_GC9A01A tft;

extern uint8_t screenAdress[];
enum screenselections { X0,
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


void selectScreens(uint8_t i);
void parallel_set_inputs(void);
void parallel_set_outputs(void);
uint8_t parallel_read(void);
void parallel_write(uint8_t value);

void displayImageWithBackground(const unsigned short* image, uint8_t screens);
uint16_t blendColor(uint16_t foreground, uint16_t background, float alpha);
void drawDot(int x, int y, float alpha = 1.0, uint16_t color = GC9A01A_WHITE, uint16_t bgColor = GC9A01A_BLACK);
void initDisplays();
void displayEinstein(uint8_t screens);
void displayN1(uint8_t screens);
void displayN2(uint8_t screens);
void displayN3(uint8_t screens);
void displayN4(uint8_t screens);
void displayN5(uint8_t screens);
void displayN6(uint8_t screens);
void displayQLab(uint8_t screens);
void displayUTlogo(uint8_t screens);
void displayMix1to6(uint8_t screens);
void displayLowBattery(uint8_t screens);
void blankScreen(uint8_t screens);
void printChar(uint8_t screens, char* letters, uint16_t fontcolor, uint16_t bckcolor, int x, int y);
void voltageIndicator(uint8_t screens);
void welcomeInfo(uint8_t screens);
void drawStringCentered(Adafruit_GFX& gfx, const String& text, int16_t y);


#endif /* SCREENFUNCTIONS_H_ */
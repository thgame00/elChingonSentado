/*************************************************************************************************************
 * El Chingòn sentado - include LED lights to the game using two ESP8266 (LOLIN (WEMOS) D1 R2 & mini)
 * 
 * -> reed switches below all glasses to check which glasses are still there
 * -> LEDs below all glasses to only light up the glasses that are still in the game
 * -> different light modes depending on switch status and glass patterns
 * 
 *    Glass/Switch pattern:
 *          0
 *        1  2
 *       3  4  5 
 *     6  7  8  9
 *************************************************************************************************************
 * IMPORTANT: 
 * To reduce NeoPixel burnout risk, add:
 *  - 1000 uF capacitor parallel to power source
 *  - 300 - 500 Ohm resistor between D1 output and first pixel's data input
 * Minimize distance between Arduino and first pixel.  
 * Avoid connecting on a live circuit...if you must, connect GND first.
 *************************************************************************************************************
 *
 * Pull Up resistors for all input pins! -> Needed with port expander MCP23017
 *
 *************************************************************************************************************/

#include <Adafruit_NeoPixel.h>  // for controlling the LEDs
#include <MCP23017.h>           // for controlling MCP23017
#include <Wire.h>               // for controlling I2C bus

// pin definition
#define LEDData     2   // data output for LEDs (D4 Output on D1 Mini from AzDelivery)
#define reedGlass1  1   // reed below glass 1   (TX Output on D1 Mini from AzDelivery)
#define reedGlass2  3   // reed below glass 2   (RX Output on D1 Mini from AzDelivery)
#define reedGlass3  5   // reed below glass 3   (D1 Output on D1 Mini from AzDelivery)
#define reedGlass4  4   // reed below glass 4   (D2 Output on D1 Mini from AzDelivery)
#define reedGlass5  0   // reed below glass 5   (D3 Output on D1 Mini from AzDelivery)
#define reedGlass6  16  // reed below glass 6   (D0 Output on D1 Mini from AzDelivery)
#define reedGlass7  14  // reed below glass 7   (D5 Output on D1 Mini from AzDelivery)
#define reedGlass8  12  // reed below glass 8   (D6 Output on D1 Mini from AzDelivery)
#define reedGlass9  13  // reed below glass 9   (D7 Output on D1 Mini from AzDelivery)
#define reedGlass10 15  // reed below glass 10  (D8 Output on D1 Mini from AzDelivery)

// definition of constants
#define MCP_ADDR_LEFT   0x20  // I2C address of MCP that controls the left side of the board
#define MCP_ADDR_RIGHT  0x21  // I2C address of MCP that controls the right side of the board
#define NUMLEDS         10    // number of LEDs that are controlled
#define NUMREEDS        10    // number of reeds
#define BRIGHTNESS      50    // set brightness of LEDs
#define WAITTIME        500   // refresh rate for checking the glass status in ms
// state machine constants
#define INIT        0     // wait until all glasses are positioned
#define LIGHTING    1     // 
#define REMOVED     2     // 

// definition of variables
struct Glass {
  uint8_t reedPin;      // which pin is the reed switch below this glass connected to
  bool    glassStatus;  // is this glass on the board or not
  int     colors[3];    // color of the LED below this glass (R, G, B) -> [0-255]
};
struct Glass glass[NUMREEDS];
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMLEDS, LEDData, NEO_GRB + NEO_KHZ800);              // struct for controlling the LEDs
uint8_t reeds[10]     = {   reedGlass1, reedGlass2, reedGlass3, reedGlass4, reedGlass5,
                            reedGlass6, reedGlass7, reedGlass8, reedGlass9, reedGlass10};         // array to control the reed input pins
unsigned long previousMillis = 0;   // variable for counter
int stateVar = INIT; 

/* initalize variables */
void setup() {
  // initialize variables and reedPins as digital Inputs
  for(uint8_t i = 0; i < NUMREEDS; i++)
  {
    glass[i].reedPin = reeds[i];
    glass[i].glassStatus = false;
    glass[i].colors[0] = 0;
    glass[i].colors[1] = 0; 
    glass[i].colors[2] = 0;   
    pinMode(glass[i].reedPin, INPUT_PULLUP);
  }
  // initialize LEDs (also data output pin gets initialized in that step)
  strip.begin();
  strip.show();   // initialize all pixels to "off"
  Serial.begin(9600);
}

/* going into main loop */
void loop() {
  // check reed status every [WAITTIME] seconds, if not in INIT status
  unsigned long currentMillis = millis(); // return number of milliseconds since start of sketch
  if ((stateVar != INIT) && ((currentMillis - previousMillis) >= WAITTIME))
  {
    Serial.println("Call every 500 ms");
    Serial.print(digitalRead(glass[0].reedPin));
    previousMillis = currentMillis;
    checkGlasses();
    // checkPattern();
  }
  // state machine that controls the lights of the board
  switch (stateVar) 
  {
    case INIT:          // glass status of all glasses is false, wait until all glasses are on the board
      Serial.println("Case Init");
      initBoard();
      break;
    case LIGHTING:      // LEDs are enlightened, no glass removed
      // default status
      Serial.println("Case Lighting");
      break;
    case REMOVED:       // glass has been removed
      // go to INIT status, if all glasses have been removed, otherwise go back to LIGHTING
      stateVar = boardEmpty() ? INIT : LIGHTING;
      Serial.println("Case Removed");
      break;
    default:
      break;
  }
}

/* Checks, if all glasses have been added to the board, init in mexican colors */
void initBoard(void)
{
  int count = 0;
  for(uint8_t i = 0; i < NUMREEDS; i++)
  {
    if (digitalRead(glass[i].reedPin) == LOW && glass[i].glassStatus == false)
    {
      glass[i].glassStatus = true;
      if (i < 3) 
      {
        glass[i].colors[0] = 0;
        glass[i].colors[1] = 255;
        glass[i].colors[2] = 20;
        Serial.println("Initialize Glass in green");
      }
      else if (i > 2 && i < 6) 
      {
        glass[i].colors[0] = 255;
        glass[i].colors[1] = 255;
        glass[i].colors[2] = 255;
        Serial.println("Initialize Glass in white");
      }
      else if (i > 5) 
      {
        glass[i].colors[0] = 255;
        glass[i].colors[1] = 0;
        glass[i].colors[2] = 0;
        Serial.println("Initialize Glass in red");
      }
      turnOn(i);
    }
    if (glass[i].glassStatus == true) count += 1;
  }
  // if all glasses are on the board, go to status LIGHTING
  if (count == NUMREEDS) stateVar = LIGHTING;
}

/* Checks, if all glasses have been removed from the board */
bool boardEmpty(void)
{
  bool isEmpty = false;
  int count = 0;
  for(uint8_t i = 0; i < NUMREEDS; i++)
  {
    if (glass[i].glassStatus == false) count += 1;
  }
  if (count == NUMREEDS) isEmpty = true;
  return isEmpty;
}

/* Checks status of all glasses (reed switches), sets stateVar to REMOVED, if glass has been removed */
void checkGlasses(void)
{
  for(uint8_t i = 0; i < NUMREEDS; i++)
  {
    Serial.print("Glass: ");
    Serial.print(i);
    Serial.println(digitalRead(glass[i].reedPin));
    // Glass wasn't there (false), but is there now (LOW Input) -> turn on again
    if (digitalRead(glass[i].reedPin) == LOW && glass[i].glassStatus == false)
    {
      glass[i].glassStatus = true;  // put glass onto board
      turnOn(i);
    }
    // Glass was there (true), but got removed (HIGH Input)
    else if (digitalRead(glass[i].reedPin) == HIGH && glass[i].glassStatus == true)
    {
      glass[i].glassStatus = false;   // remove glass from board
      stateVar = REMOVED; // state machine to switch into REMOVED state
      turnOff(i);
    }
  }
}

/* Checks position of glasses (reed switches), sets stateVar accordingly */
void checkPattern(void)
{
  // Raute: 0, 1, 2, 4
  for(uint8_t i = 0; i < NUMREEDS; i++)
  {
    
  }
  // Turbopenis: 0, 4, 7 & 8
  // Vajizzling: 1, 2, 4
  // Dreieck: 0, 1, 2
}

/* turns on glass with given index and given color -> fading in */
void turnOn(int glassIndex)
{
  for(int k = 1; k < 101; k++) 
  {
    int R = int(0.01 * k * glass[glassIndex].colors[0]);
    int G = int(0.01 * k * glass[glassIndex].colors[1]);
    int B = int(0.01 * k * glass[glassIndex].colors[2]);
    strip.setPixelColor(glassIndex, strip.Color(R, G, B));
    strip.show();
    delay(5);
  }
}

/* turns off glass with given index, saves color as {0,0,0} */
void turnOff(int glassIndex)
{
  for(int k = 100; k > -1; k--) 
  {
    int R = int(0.01 * k * glass[glassIndex].colors[0]);
    int G = int(0.01 * k * glass[glassIndex].colors[1]);
    int B = int(0.01 * k * glass[glassIndex].colors[2]);
    strip.setPixelColor(glassIndex, strip.Color(R, G, B));
    strip.show();
    delay(5);
  }
}

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
 *       5  4  3 
 *     6  7  8  9
 *************************************************************************************************************
 * IMPORTANT: 
 * To reduce NeoPixel burnout risk, add:
 *  - 1000 uF capacitor parallel to power source
 *  - 300 - 500 Ohm resistor between D1 output and first pixel's data input
 * Minimize distance between Arduino and first pixel.  
 * Avoid connecting on a live circuit...if you must, connect GND first.
 ************************************************************************************************************/

#include <Adafruit_NeoPixel.h>  // for controlling the LEDs
#include <MCP23017.h>           // for controlling MCP23017
#include <Wire.h>               // for controlling I2C bus
#include "glasses.h"

// pin definition
/* 5 (D1) -> SCL and 4 (D2) -> SDA are reserved as I2C pins */
#define LEDDataLeft     13  // data output for LEDs on left side of board   (D7 Output on D1 Mini from AzDelivery)
#define LEDDataRight    15  // data output for LEDs on right side of board  (D8 Output on D1 Mini from AzDelivery)
#define INTERRUPT_LEFT  14  // interrupt pin for left MCP (D5 Output on D1 Mini from AzDelivery)
#define INTERRUPT_RIGHT 12  // interrupt pin for right MCP (D6 Output on D1 Mini from AzDelivery)
#define RESET_PIN_LEFT  0 	// reset pin for left MCP23017 (D3 Output on D1 Mini from AzDelivery)
#define RESET_PIN_RIGHT 2 	// reset pin for left MCP23017 (D4 Output on D1 Mini from AzDelivery)

// definition of constants
#define MCP_ADDR_LEFT   0x20  // I2C address of MCP that controls the left side of the board
#define MCP_ADDR_RIGHT  0x21  // I2C address of MCP that controls the right side of the board
#define NUMGLASSES      10    // number of glasses on the board
#define BRIGHTNESS      50    // set brightness of LEDs
#define WAITTIME        500   // refresh rate for checking the glass status in ms
// state machine constants
#define INIT        0     // stay here until all glasses are on board
#define LIGHTING    1     // stay in this case as long as init phase is over and at least one glass is still on board
#define REMOVED     2     // go to this case when at least one glass was removed

// definition of variables
Glass glass_left[NUMGLASSES];
Glass glass_right[NUMGLASSES];
Adafruit_NeoPixel strip_left = Adafruit_NeoPixel(NUMGLASSES, LEDDataLeft, NEO_GRB + NEO_KHZ800);    // object for controlling the LEDs on left side
Adafruit_NeoPixel strip_right = Adafruit_NeoPixel(NUMGLASSES, LEDDataRight, NEO_GRB + NEO_KHZ800);  // object for controlling the LEDs on right side
unsigned long previousMillis = 0;   // variable for counter
int stateVar = INIT; 

MCP23017 mcp_left = MCP23017(MCP_ADDR_LEFT, RESET_PIN_LEFT);
MCP23017 mcp_right = MCP23017(MCP_ADDR_RIGHT, RESET_PIN_RIGHT);

/* initalize variables */
void setup() {
  // initialize glasses / reedPins
  colors initColor = {0, 0, 0}; 
  // uint8_t reedPin = 0;
  for(uint8_t i = 0; i < NUMGLASSES; i++)
  {
    /* Port A: glasses 0 - 7, Port B: Glasses 8 - 9 */
    if (i < 8) {
      // set reed pin, port, status as false and color as "off"
      // reedPin = 0b00000000 | (1 >> (7-i));
      glass_left[i] = Glass(i, A, false, initColor);
      glass_right[i] = Glass(i, A, false, initColor);
    }
    else {
      // set reed pin, port, status as false and color as "off"
      // reedPin = 0b00000000 | (1 >> (15-i));
      glass_left[i] = Glass((8 - i), B, false, initColor);
      glass_right[i] = Glass((8 - i), B, false, initColor);
    }
  }
  
  // initialize LEDs (also data output pin gets initialized in that step)
  strip_left.begin();
  strip_right.begin();
  strip_left.show();  // initialize all pixels to "off"
  strip_right.show(); // initialize all pixels to "off"

  // initialize MCPs
  Wire.begin(); // "start" I2C
  // left MCP
  mcp_left.Init();
  mcp_left.setPortMode(B11111111, A, INPUT_PULLUP); // set A0 - A7 as input with pullup resistor
  mcp_left.setPortMode(B00000011, B, INPUT_PULLUP); // set B0 - B1 as input with pullup resistor
  // right MCP
  mcp_right.Init();
  mcp_right.setPortMode(B11111111, A, INPUT_PULLUP); // set A0 - A7 as input with pullup resistor
  mcp_right.setPortMode(B00000011, B, INPUT_PULLUP); // set B0 - B1 as input with pullup resistor
  Serial.begin(115200);
} /* end of setup function */

/* going into main loop */
void loop() {
  // check reed status every [WAITTIME] seconds, if not in INIT status
  unsigned long currentMillis = millis(); // return number of milliseconds since start of sketch
  if ((stateVar != INIT) && ((currentMillis - previousMillis) >= WAITTIME))
  {
    Serial.println("Call every 500 ms");
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
      // checkPattern();
      Serial.println("Case Removed");
      break;
    default:
      break;
  }
} /* end of main loop function */

/* Checks, if all glasses have been added to the board, init in mexican colors */
void initBoard(void)
{
  int count = 0;
  for(uint8_t i = 0; i < NUMGLASSES; i++)
  {
    /* init left side */
    if (mcp_left.getPin(glass_left[i].getReedPin(), glass_left[i].getPort()) == LOW && glass_left[i].getGlassStatus() == false)
    {
      glass_left[i].getGlassStatus() = true;
      if (i < 3) 
      {
        glass_left[i].setColors(0, 255, 20);
        Serial.println("Initialize Glass in green");
      }
      else if (i > 2 && i < 6) 
      {
        glass_left[i].setColors(255, 255, 255);
        Serial.println("Initialize Glass in white");
      }
      else if (i > 5) 
      {
        glass_left[i].setColors(255, 0, 0);
        Serial.println("Initialize Glass in red");
      }
      turnOn(i);
    }
    /* init right side */
    if (mcp_right.getPin(glass_right[i].getReedPin(), glass_right[i].getPort()) == LOW && glass_right[i].getGlassStatus() == false)
    {
      glass_right[i].getGlassStatus() = true;
      if (i < 3) 
      {
        glass_right[i].setColors(0, 255, 20);
        Serial.println("Initialize Glass in green");
      }
      else if (i > 2 && i < 6) 
      {
        glass_right[i].setColors(255, 255, 255);
        Serial.println("Initialize Glass in white");
      }
      else if (i > 5) 
      {
        glass_right[i].setColors(255, 0, 0);
        Serial.println("Initialize Glass in red");
      }
      turnOn(i);
    }
    if (glass_left[i].getGlassStatus() == true) count += 1;
    if (glass_right[i].getGlassStatus() == true) count += 1;
  }
  // if all glasses are on the board, go to status LIGHTING
  if (count == 2*NUMGLASSES) stateVar = LIGHTING;
}

/* Checks, if all glasses have been removed from the board */
bool boardEmpty(void)
{
  bool isEmpty = false;
  int count = 0;
  for(uint8_t i = 0; i < NUMGLASSES; i++)
  {
    if (glass[i].getGlassStatus() == false) count += 1;
  }
  if (count == NUMGLASSES) isEmpty = true;
  return isEmpty;
}

/* Checks status of all glasses (reed switches), sets stateVar to REMOVED, if glass has been removed */
void checkGlasses(void)
{
  for(uint8_t i = 0; i < NUMGLASSES; i++)
  {
    Serial.print("Glass: ");
    Serial.print(i);
    Serial.println(digitalRead(glass[i].reedPin));
    // Glass wasn't there (false), but is there now (LOW Input) -> turn on again
    if (digitalRead(glass[i].reedPin) == LOW && glass[i].getGlassStatus() == false)
    {
      glass[i].glassStatus = true;  // put glass onto board
      turnOn(i);
    }
    // Glass was there (true), but got removed (HIGH Input)
    else if (digitalRead(glass[i].reedPin) == HIGH && glass[i].getGlassStatus() == true)
    {
      glass[i].getGlassStatus() = false;   // remove glass from board
      stateVar = REMOVED; // state machine to switch into REMOVED state
      turnOff(i);
    }
  }
}

/* Checks position of glasses (reed switches), sets stateVar accordingly */
void checkPattern(void)
{
  // DEFAULT: no specific pattern
  // RAUTE: 0, 1, 2, 4
  // TURBOPENIS: 0, 4, 7, 8
  // VAJIZZLING: 1, 2, 4
  // TRIANGLE: 0, 1, 2
  // if pattern change -> turn off, change color, turn on
}

/* turns on glass with given index, color is given by glass object */
void turnOn(int glassIndex)
{
  for(int k = 1; k < 101; k++) 
  {
    int R = int(0.01 * k * glass[glassIndex].getColors().red);
    int G = int(0.01 * k * glass[glassIndex].getColors().green);
    int B = int(0.01 * k * glass[glassIndex].getColors().blue);
    strip_left.setPixelColor(glassIndex, strip_left.Color(R, G, B));
    strip_left.show();
    delay(5); // control duration of fading effect
  }
}

/* turns off glass with given index, color itself stays the same */
void turnOff(int glassIndex)
{
  for(int k = 100; k > -1; k--) 
  {
    int R = int(0.01 * k * glass[glassIndex].getColors().red);
    int G = int(0.01 * k * glass[glassIndex].getColors().green);
    int B = int(0.01 * k * glass[glassIndex].getColors().blue);
    strip_left.setPixelColor(glassIndex, strip_left.Color(R, G, B));
    strip_left.show();
    delay(5); // control duration of fading effect
  }
}

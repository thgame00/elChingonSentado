/*
    Class to control the glasses
    - set/get color
    - set/get status
    - turn on/off LED of glass
*/

#include "glasses.h"

Glass::Glass(uint8_t reedPin, uint8_t port, bool glassStatus, colors colors)
{
    _reedPin        = reedPin;
    _port           = port;
    _glassStatus    = glassStatus;
    _colors.red     = colors.red;
    _colors.green   = colors.green;
    _colors.blue    = colors.blue;
}

void Glass::setColors(int red, int green, int blue)
{
    _colors.red     = red;
    _colors.green   = green;
    _colors.blue    = blue;
}
/*
    Class to control the glasses
    - set/get color
    - set/get status
    - turn on/off LED of glass
*/

#include "stdint.h"

#ifndef GLASSES_H
#define GLASSES_H

#define A 0
#define B 1

typedef struct colors
{
    int red;
    int green;
    int blue;
};

class Glass {
    public:
        Glass(uint8_t reedPin, uint8_t port, bool glassStatus, colors colors);
        /* Setters */
        void setReedPin(uint8_t reedPin)        { _reedPin = reedPin; }
        void setPort(uint8_t port)              { _port = port; }
        void setGlassStatus(bool glassStatus)   { _glassStatus = glassStatus; }
        void setColors(int red, int green, int blue);
        /* Getters */
        uint8_t getReedPin(void)        {return _reedPin;}
        uint8_t getPort(void)           {return _port;}
        bool    getGlassStatus(void)    {return _glassStatus;}
        colors  getColors(void)         {return _colors;}
    private:
        uint8_t _reedPin;       // binary output for pin of MCP that's connected to reed switch of this glass
        uint8_t _port;          // which port of the MCP is the reed switch connected to
        bool    _glassStatus;   // status if glass is still on board (true) or removed (false)
        colors  _colors;        // struct that holds the color values
        // add variable for pattern checking?
};

#endif
/*
    Class to control the glasses
    - set/get color
    - set/get status
    - turn on/off LED of glass
*/

#include "stdint.h"
#include "MCP23017.h"

#ifndef GLASSES_H
#define GLASSES_H

typedef struct colors
{
    int red;
    int green;
    int blue;
};

class Glass {
    public:
        Glass();
        Glass(uint8_t reedPin, MCP_PORT port, bool glassStatus, colors colors);
        /* Setters */
        void setReedPin(uint8_t reedPin)        { _reedPin = reedPin; }
        void setPort(MCP_PORT port)              { _port = port; }
        void setGlassStatus(bool glassStatus)   { _glassStatus = glassStatus; }
        void setColors(int red, int green, int blue);
        /* Getters */
        uint8_t     getReedPin(void)        {return _reedPin;}
        MCP_PORT    getPort(void)           {return _port;}
        bool        getGlassStatus(void)    {return _glassStatus;}
        colors      getColors(void)         {return _colors;}
    private:
        uint8_t     _reedPin;       // binary output for pin of MCP that's connected to reed switch of this glass
        MCP_PORT    _port;          // which port of the MCP is the reed switch connected to
        bool        _glassStatus;   // status if glass is still on board (true) or removed (false)
        colors      _colors;        // struct that holds the color values
        // add variable for pattern checking?
};

#endif
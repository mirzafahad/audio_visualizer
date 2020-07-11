/***********************************************************************
 * @file      audio-visualizer.ino
 * @author    Fahad Mirza (fahadmirza8@gmail.com)
 * @version   V02.0
 * @brief     Main Application
 ***********************************************************************/
/*-- Includes ---------------------------------------------------------*/
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "ble.h"

#define CMD_BUFFER_SIZE  (25U)
#define VALID    (1U)
#define INVALID  (0U)

/*-- Constants --------------------------------------------------------*/
static const uint8_t PIN_AUDIO_IN = A0;
static const uint8_t PIN_RESET    = 31;
static const uint8_t PIN_STROBE   = 11;
static const uint8_t PIN_NEOPIXEL = 7;
static const uint8_t LED_COUNT    = 84;

/*-- Private typedef --------------------------------------------------*/
typedef enum eMode
{
    MUSIC = 0,
    POWER_ON,
    POWER_OFF,
    RAINBOW,
    BACKLIGHT
}eMode_t;


/*-- Private variables ------------------------------------------------*/
// Save all 7 bands value
static uint32_t Bands[7];
char CmdBuffer[CMD_BUFFER_SIZE + 1];

Adafruit_NeoPixel Neopixel(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

eMode_t AppMode = POWER_OFF;

/*-- Function definitions ---------------------------------------------*/
void setup() 
{    
    pinMode(PIN_RESET, OUTPUT);
    digitalWrite(PIN_RESET, LOW);
    
    pinMode(PIN_STROBE, OUTPUT);
    digitalWrite(PIN_STROBE, HIGH);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Config Neopixels
    Neopixel.begin();
    Neopixel.show();              // Turn OFF all LEDs
    Neopixel.setBrightness(255);  // Set BRIGHTNESS to max (max = 255)
    
    Serial.begin(9600);

#ifdef DEBUG
    // Blocking wait for connection when debug mode is enabled via IDE
    while(!Serial) yield();
#endif

    BLE_Init();
}

void loop() 
{ 
    if(BLE_ProcessMsg(CmdBuffer) == VALID)
    {
        parse_cmd(CmdBuffer);
    }

    switch(AppMode)
    {
        case MUSIC:
        {
            audio_visualizer();
            break;
        }
        case RAINBOW:
        {
            rainbow(20);
            break;
        }
        default:
        {
            break;
        }
    }
}


static void parse_cmd(char *cmd)
{
    if(strncmp(cmd, "music", strlen("music")) == 0)
    {
        Neopixel.setBrightness(255);
        AppMode = MUSIC;
    }
    else if(strncmp(cmd, "pwroff", strlen("pwroff")) == 0)
    {
        Neopixel.clear();
        Neopixel.show();
        AppMode = POWER_OFF;
    }
    else if(strncmp(cmd, "pwron", strlen("pwron")) == 0)
    {
        backlight(0); // Whatever last was used
        AppMode = POWER_ON;
    }
    else if(strncmp(cmd, "rainbow", strlen("rainbow")) == 0)
    {
        Neopixel.setBrightness(255);
        AppMode = RAINBOW;
    }
    else if(strncmp(cmd, "bl", strlen("bl")) == 0)
    {
        cmd += 3;
        char *token;
        char rgb[5];
        uint8_t i = 0;

        token = strtok(cmd, ",");
        while(token != NULL)
        {
            rgb[i++] = strtol(token, NULL, 10);
            token = strtok(NULL, ",");
        }
        
        Neopixel.setBrightness(rgb[3]);
        backlight(Neopixel.Color(rgb[0], rgb[1], rgb[2]));
        AppMode = BACKLIGHT;
    }
}

void backlight(uint32_t color)
{
    static uint32_t backlightColor = Neopixel.Color(255, 0, 0);

    if(color != 0)
    {
        backlightColor = color;
    }
    Neopixel.fill(backlightColor);
    Neopixel.show();
}

static void audio_visualizer(void)
{
    read_MSGEQ7(); 
    graph_bands();
}


static void read_MSGEQ7(void)
{
    digitalWrite(PIN_RESET, HIGH);           // Pulse the reset signal
    digitalWrite(PIN_RESET, LOW);            // Causes MSGEQ7 to latch spectrum values
    delayMicroseconds(75);                   // Delay to meet min reset-to-strobe time (72us)

    for(uint8_t i = 0; i < 7; i++)           // Cycle through all 7 bands
    {
        digitalWrite(PIN_STROBE, LOW);        
        delayMicroseconds(40);               // Wait for output to settle (min 36us)
        Bands[i] = analogRead(PIN_AUDIO_IN);

        digitalWrite(PIN_STROBE, HIGH);
        delayMicroseconds(40);               // Pad delay to exceed min strobe-to-strobe time of 72us
    }
}


static void graph_bands(void)
{
    uint8_t mapValue[7];
    
    for(uint8_t i = 0; i < 7; i++)           // Cycle through all 7 bands
    {
        mapValue[i] = map(Bands[i], 0, 1023, 0, 255);
        if (mapValue[i] < 22)
        {
            mapValue[i] = 0;
        }
    }

    /* Band Freq
     0  ---->  63Hz
     1  ---->  160Hz
     2  ---->  400Hz
     3  ---->  1KHz
     4  ---->  2.5KHz
     5  ---->  6.25KHz
     6  ---->  16KHz
    */

    /*
     * Cyan = Neopixel.Color(0, mapValue[1], mapValue[1]);
     * Yellow = Neopixel.Color(mapValue[4], mapValue[4], 0);
     */

    // Set Orange
    uint32_t color = Neopixel.Color(mapValue[2], (int)(mapValue[2] / 2), 0);
    Neopixel.fill(color, 0, 20);

    //Set Blue
    color = Neopixel.Color(0, 0, mapValue[4]);
    Neopixel.fill(color, 20, 24);

    // Set Green
    color = Neopixel.Color(mapValue[0], 0, 0);
    Neopixel.fill(color, 44, 23);

    // Set Red
    color = Neopixel.Color(0, mapValue[6], 0);
    Neopixel.fill(color, 67, 17);

    Neopixel.show();
}


// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) 
{
    static long firstPixelHue = 0;

    for(uint8_t i = 0; i < Neopixel.numPixels(); i++) 
    {
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / Neopixel.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      Neopixel.setPixelColor(i, Neopixel.gamma32(Neopixel.ColorHSV(pixelHue)));
    }
    
    Neopixel.show(); // Update strip with new contents
    delay(wait);     // Pause for a moment

    firstPixelHue += 256;

    if(firstPixelHue >= 5*65536)
    {
        firstPixelHue = 0;
    }
}

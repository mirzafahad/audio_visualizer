/***********************************************************************
 * @file      audio-visualizer.ino
 * @author    Fahad Mirza (fahadmirza8@gmail.com)
 * @version   V2.0
 * @brief     Main Application
 ***********************************************************************/
/*-- Includes ---------------------------------------------------------*/ 
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "ble.h"


/*-- Macros -----------------------------------------------------------*/ 
#define BLE_MSG_BUF_SIZE (25U)
#define VALID            (1U)
#define INVALID          (0U)


/*-- Constants --------------------------------------------------------*/
static const uint8_t PIN_AUDIO_IN = A0;
static const uint8_t PIN_RESET    = 31;
static const uint8_t PIN_STROBE   = 11;
static const uint8_t PIN_NEOPIXEL = 7;
static const uint8_t LED_COUNT    = 84;
static const char* DEVICE_NAME    = "Illuminati";


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
static char BleBuf[BLE_MSG_BUF_SIZE + 1];

// Initial Application Mode
eMode_t AppMode = POWER_OFF;

Adafruit_NeoPixel Neopixel(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);


/*-- Function definitions ---------------------------------------------*/
void setup() 
{
    // Config MSGEQ7
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
    // Check if there is any command from BLE
    if(BLE_ProcessMsg(BleBuf) == VALID)
    {
        parse_msg(BleBuf);
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
            // Backlight doesn't need continuous execution
            break;
        }
    }
}


/******************************************************************************
 * @brief  parse the command received from BLE
 * @param  cmd: pointer of the buffer that holds the command
 * @retval None
 ******************************************************************************/
static void parse_msg(char *msg)
{
    if(strncmp(msg, "music", strlen("music")) == 0)
    {
        Neopixel.setBrightness(255);
        AppMode = MUSIC;
    }
    else if(strncmp(msg, "pwroff", strlen("pwroff")) == 0)
    {
        Neopixel.clear();
        Neopixel.show();
        AppMode = POWER_OFF;
    }
    else if(strncmp(msg, "pwron", strlen("pwron")) == 0)
    {
        backlight(0); // Whatever last was used
        AppMode = POWER_ON;
    }
    else if(strncmp(msg, "rainbow", strlen("rainbow")) == 0)
    {
        Neopixel.setBrightness(255);
        AppMode = RAINBOW;
    }
    else if(strncmp(msg, "bl", strlen("bl")) == 0)
    {
        // BackLight mode. The command looks like this
        // "bl,253,124,78,100" (cmd,r,g,b,brightness)

        // Skip "bl,"
        cmd += 3;
        
        char *token;
        char rgb[5];   // Holder for RGB and brightness
        uint8_t i = 0;

        token = strtok(msg, ",");
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


/******************************************************************************
 * @brief  Turn ON all the LED with one color
 * @param  32bit color value
 * @retval None
 ******************************************************************************/
void backlight(uint32_t color)
{
    // This value is static. So if we press "power on" it will use
    // whatever color was last used as backlight.
    static uint32_t backlightColor = Neopixel.Color(255, 0, 0);

    if(color != 0)
    {
        backlightColor = color;
    }
    Neopixel.fill(backlightColor);
    Neopixel.show();
}


/******************************************************************************
 * @brief  Display audio spectrum on LED strip
 * @param  None
 * @retval None
 ******************************************************************************/
static void audio_visualizer(void)
{
    read_MSGEQ7(); 
    graph_bands();
}


/******************************************************************************
 * @brief  Read MSGEQ7 and populate the global array
 * @param  None
 * @retval None
 ******************************************************************************/
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


/******************************************************************************
 * @brief  Map the audio bands on the LED strip
 * @param  None
 * @retval None
 ******************************************************************************/
static void graph_bands(void)
{
    uint8_t mapValue[7];
    
    for(uint8_t i = 0; i < 7; i++)  // Cycle through all 7 bands
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


/******************************************************************************
 * @brief  Execute the rainbow anumation on the LED strip
 * @param  wait: how long to wait between color transition
 * @retval None
 ******************************************************************************/
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

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
#define CMD_BUFFER_SIZE  (25U)
#define VALID            (1U)
#define INVALID          (0U)
//#define DEBUG


/*-- Constants --------------------------------------------------------*/
static const uint8_t PIN_LDR      = A5;
static const uint8_t PIN_AUDIO_IN = A0;
static const uint8_t PIN_RESET    = 31;
static const uint8_t PIN_STROBE   = 11;
static const uint8_t PIN_NEOPIXEL = 7;
static const uint8_t PIN_PIR      = 27;
static const uint8_t LED_COUNT    = 77;
static const uint8_t PIN_TV       = A4;



/*-- Private typedef --------------------------------------------------*/
typedef enum eMode
{
    POWER_ON = 0,
    POWER_OFF,
    TV_ON,
    RAINBOW,
    BACKLIGHT,
    DARK
}eMode_t;


/*-- Private variables ------------------------------------------------*/
char CmdBuffer[CMD_BUFFER_SIZE + 1];
bool LedIsOn = false;
uint32_t DarkModeCounter = 0;

Adafruit_NeoPixel Neopixel(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Initial Application Mode
eMode_t AppMode = POWER_OFF;


/*-- Function definitions ---------------------------------------------*/
void setup() 
{
    // Config MSGEQ7
    pinMode(PIN_RESET, OUTPUT);
    digitalWrite(PIN_RESET, LOW);
    
    pinMode(PIN_STROBE, OUTPUT);
    digitalWrite(PIN_STROBE, HIGH);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Config Neopixels
    Neopixel.begin();
    Neopixel.show();              // Turn OFF all LEDs
    Neopixel.setBrightness(255);  // Set BRIGHTNESS to max (max = 255)

    // Config PIR
    pinMode(PIN_PIR, INPUT);
    
    Serial.begin(115200);

#ifdef DEBUG
    // Blocking wait for connection when debug mode is enabled via IDE
    while(!Serial) yield();
#endif

    BLE_Init();
}

void loop() 
{ 
    // Check if there is any command from BLE
    if(BLE_ProcessMsg(CmdBuffer) == VALID)
    {
        parse_cmd(CmdBuffer);
    }

    switch(AppMode)
    {
        case RAINBOW:
        {
            rainbow(20);
            break;
        }
        case DARK:
        {
            if (digitalRead(PIN_PIR) == false)
            {
                delay(1);
                DarkModeCounter++;
                if (DarkModeCounter > 30000)
                {
                    Neopixel.clear();
                    Neopixel.show();
                    LedIsOn = false;
                    DarkModeCounter = 0;
                    AppMode = POWER_OFF;
                    
                    //digitalWrite(LED_BUILTIN, LOW);
                }
            }
            else
            {
                DarkModeCounter = 0;
            }
            break;
        }
        default:
        {
            // Backlight doesn't need continuous execution
            break;
        }
    }

    //Serial.printf("P:%d\tD:%ld\n", digitalRead(PIN_PIR), DarkModeCounter);

    if ((digitalRead(PIN_PIR) == true) && (LedIsOn == false) && (read_ldr() <= 30))
    {
        Neopixel.setBrightness(255);
        Neopixel.fill(Neopixel.Color(255, 170, 40));
        Neopixel.show();
        LedIsOn = true;
        AppMode = DARK;
        DarkModeCounter = 0;

        //digitalWrite(LED_BUILTIN, HIGH);
    }

    if (is_tv_on() && (LedIsOn == false))
    {
        backlight(0); // Whatever last was used
        LedIsOn = true;
        AppMode = TV_ON;
    }
    else if ((is_tv_on() == false) && (AppMode == TV_ON))
    {
        Neopixel.clear();
        Neopixel.show();
        LedIsOn = false;
        AppMode = POWER_OFF;
    }

}

bool is_tv_on(void)
{
    if (analogRead(PIN_TV) > 400)
    {
        return true;
    }

    return false;
}

uint32_t read_ldr(void)
{
    uint32_t sensorValueSum = 0;

    for(int i = 0; i < 100; i++)
    {    
        sensorValueSum += analogRead(PIN_LDR);
    }
    
    uint32_t sensorValueAvg = sensorValueSum / 100;

    Serial.println(sensorValueAvg);
    
    return sensorValueAvg;
}


/******************************************************************************
 * @brief  parse the command received from BLE
 * @param  cmd: pointer of the buffer that holds the command
 * @retval None
 ******************************************************************************/
static void parse_cmd(char *cmd)
{
    if(strncmp(cmd, "pwroff", strlen("pwroff")) == 0)
    {
        Neopixel.clear();
        Neopixel.show();
        LedIsOn = false;
        AppMode = POWER_OFF;
    }
    else if(strncmp(cmd, "pwron", strlen("pwron")) == 0)
    {
        backlight(0); // Whatever last was used
        LedIsOn = true;
        AppMode = POWER_ON;
    }
    else if(strncmp(cmd, "rainbow", strlen("rainbow")) == 0)
    {
        Neopixel.setBrightness(255);
        LedIsOn = true;
        AppMode = RAINBOW;
    }
    else if(strncmp(cmd, "bl", strlen("bl")) == 0)
    {
        // BackLight mode. The command looks like this
        // "bl,253,124,78,100" (cmd,r,g,b,brightness)

        // Skip "bl,"
        cmd += 3;
        
        char *token;
        char rgb[5];   // Holder for RGB and brightness
        uint8_t i = 0;

        token = strtok(cmd, ",");
        while(token != NULL)
        {
            rgb[i++] = strtol(token, NULL, 10);
            token = strtok(NULL, ",");
        }
        
        Neopixel.setBrightness(rgb[3]);
        backlight(Neopixel.Color(rgb[0], rgb[1], rgb[2]));
        LedIsOn = true;

        if (AppMode != TV_ON)
        {
            AppMode = BACKLIGHT;
        }
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
    static uint32_t backlightColor = Neopixel.Color(255, 255, 255);

    if(color != 0)
    {
        backlightColor = color;
    }
    Neopixel.fill(backlightColor);
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

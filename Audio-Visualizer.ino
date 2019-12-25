#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const uint8_t PIN_RESET    = 31;
const uint8_t PIN_STROBE   = 11;
const uint8_t PIN_AUDIO_IN = A0;
const uint8_t PIN_NEOPIXEL = 7;
const uint8_t LED_COUNT    = 84;

// Save all 7 bands value
uint32_t Bands[7];

Adafruit_NeoPixel Neopixel(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

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
    Neopixel.setBrightness(255);  // Set BRIGHTNESS to about 2/3 (max = 255)
    Serial.begin(9600);
}

void loop() 
{
    readMSGEQ7();  
    graphBands();
}



void readMSGEQ7(void)
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



void graphBands(void)
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

    /* Band colors
     Red    ---->  63Hz
     Yellow ---->  160Hz
     Green  ---->  400Hz
     Cyan   ---->  1KHz
     Blue   ---->  2.5KHz
     Pink   ---->  6.25KHz
     White  ---->  16KHz
    */

    // Set Pink
    uint32_t color = Neopixel.Color(mapValue[1], 0, mapValue[1]);
    Neopixel.fill(color, 0, 12);

    // Set Cyan
    color = Neopixel.Color(0, mapValue[2], mapValue[2]);
    Neopixel.fill(color, 12, 12);

    //Set Orange
    color = Neopixel.Color(mapValue[3], (int)(mapValue[3] / 2), 0);
    Neopixel.fill(color, 24, 12);

    // Set Blue
    color = Neopixel.Color(0, 0, mapValue[4]);
    Neopixel.fill(color, 36, 12);

    // Set Yellow
    color = Neopixel.Color(mapValue[5], mapValue[5], 0);
    Neopixel.fill(color, 48, 12);

    // Set Green
    color = Neopixel.Color(0, mapValue[6], 0);
    Neopixel.fill(color, 60, 12);

    // Set Red
    color = Neopixel.Color(mapValue[0], 0, 0);
    Neopixel.fill(color, 72, 12);

    Neopixel.show();
}

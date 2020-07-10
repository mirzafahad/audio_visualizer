#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

const uint8_t PIN_AUDIO_IN = A0;
const uint8_t PIN_RESET    = 31;
const uint8_t PIN_STROBE   = 11;
const uint8_t PIN_NEOPIXEL = 7;
const uint8_t LED_COUNT    = 84;

typedef enum eAppCmd
{
    MUSIC,
    RAINBOW,
    POWER_ON,
    POWER_OFF,
    BACK_LIGHT
}eAppCmd_t;

// Save all 7 bands value
uint32_t Bands[7];

Adafruit_NeoPixel Neopixel(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// BLE Service
BLEUart bleuart; // uart over ble

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

    // Setup the BLE LED to be disabled on CONNECT
    Bluefruit.autoConnLed(false);
    // Config the peripheral connection with maximum bandwidth 
    // more SRAM required by SoftDevice
    // Note: All config***() function must be called before begin()
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

    Bluefruit.begin();
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
    Bluefruit.setName("Illuminati");
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
    
    // Configure and Start BLE Uart Service
    bleuart.begin();

    // Set up and start advertising
    startAdv();

    Serial.println("Ready");
}

void loop() 
{ 
    // Forward from BLEUART to HW Serial
    while ( bleuart.available() )
    {
        uint8_t ch;
        ch = (uint8_t) bleuart.read();
        Serial.write(ch);
    }   
}



void audio_visualizer(void)
{
    read_MSGEQ7(); 
    graph_bands();
}

void read_MSGEQ7(void)
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



void graph_bands(void)
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
    color = Neopixel.Color(0, mapValue[6], 0);
    Neopixel.fill(color, 44, 23);

    // Set Red
    color = Neopixel.Color(mapValue[0], 0, 0);
    Neopixel.fill(color, 67, 17);

    Neopixel.show();
}

void startAdv(void)
{
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);

    // Include bleuart 128-bit uuid
    Bluefruit.Advertising.addService(bleuart);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();
  
    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     * 
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html   
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(conn_handle);

    char central_name[32] = { 0 };
    connection->getPeerName(central_name, sizeof(central_name));

    Serial.print("Connected to ");
    Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;

    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

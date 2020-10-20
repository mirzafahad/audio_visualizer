/***********************************************************************
 * @file      ble.cpp
 * @author    Fahad Mirza (fahadmirza8@gmail.com)
 * @version   V02.0
 * @brief     Bluetooth communication module
 ***********************************************************************/
/*-- Includes ---------------------------------------------------------*/
#include <Arduino.h>
#include <bluefruit.h>
#include "ble.h"


/*-- Private macros --------------------------------------------------*/
#define BLE_BUFFER_SIZE    (25U)
#define VALID    (1U)
#define INVALID  (0U)


/*-- Private variables ------------------------------------------------*/
// BLE UART Service
BLEUart bleuart; 


/*-- Private functions ------------------------------------------------*/
void startAdv(void);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);


/*-- Exported functions -----------------------------------------------*/
/************************************************************************
 * @brief  Initialize the BLE stack
 * @param  None
 * @retval None
 ***********************************************************************/
void BLE_Init(void)
{
    // Setup the BLE LED to be disabled on CONNECT
    Bluefruit.autoConnLed(false);
    // Config the peripheral connection with maximum bandwidth 
    // more SRAM required by SoftDevice
    // Note: All config***() function must be called before begin()
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

    Bluefruit.begin();
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
    Bluefruit.setName("Illuminati-TV");
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
    
    // Configure and Start BLE Uart Service
    bleuart.begin();

    // Set up and start advertising
    startAdv();

    Serial.println("Ready");
}


/******************************************************************************
 * @brief  Process incoming BLE messages
 * @param  msgBuffer: pointer of the buffer to hold the BLE message
 * @retval VALID: If a complete message is received
 *         INVALID: If not
 ******************************************************************************/
uint8_t BLE_ProcessMsg(char *msgBuffer)
{
    static char command[BLE_BUFFER_SIZE + 1];
    static uint8_t i = 0;

    // Check if any BLE msg is available to read
    while(bleuart.available())
    {
        command[i] = (uint8_t)bleuart.read();
        //Serial.print(command[i]);
        
        if(command[i] == '\n') // End of the message
        {
            if(i != 0)
            {
                // Null terminating the msg
                command[i] = '\0';
                strncpy(msgBuffer, command, i + 1);
                i = 0;
                return VALID;
            }
        }    
        else if(i == BLE_BUFFER_SIZE)
        {
            Serial.print("overflow");
            i = 0;
        }
        else if(command[i] == 0)
        {
          // Sometimes I receive this zero from
          // the android app. Haven't figure out why.
          // For now just ignore, and eveything will be fine.
        }
        else
        {
            i++;
        }
    }

    return INVALID; 
}


/******************************************************************************
 * @brief  Start Advertising
 * @param  None
 * @retval None
 ******************************************************************************/
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


/******************************************************************************
 * @brief  Callback invoked when central connects
 * @param  conn_handle: BLE handler that is connected
 * @retval None
 ******************************************************************************/
void connect_callback(uint16_t conn_handle)
{
    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(conn_handle);

    char central_name[32] = { 0 };
    connection->getPeerName(central_name, sizeof(central_name));

    Serial.print("Connected to ");
    Serial.println(central_name);
}


/******************************************************************************
 * @brief  Callback invoked when connection is dropped
 * @param  conn_handle: BLE handler that is connected
 *         reason: and the why
 * @retval None
 ******************************************************************************/
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;

    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

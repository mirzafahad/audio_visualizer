/***********************************************************************
 * @file     ble.h
 * @author   Fahad Mirza (fahadmirza8@gmail.com)
 * @version  V2.0
 * @brief    Header file for bluetooth communication module
 ***********************************************************************/

#ifndef BLE_H__
#define BLE_H__

/************************************************************************
 * @brief  Initialize the BLE stack
 * @param  None
 * @retval None
 ***********************************************************************/
void BLE_Init(void);

/******************************************************************************
 * @brief  Process incoming BLE messages
 * @param  msgBuffer: pointer of the buffer to hold the BLE message
 * @retval VALID: If a complete message is received
 *         INVALID: If not
 ******************************************************************************/
uint8_t BLE_ProcessMsg(char *msgBuffer);


#endif /* BLE_H__ */

/***********************************************************************
 * @file     ble.h
 * @author   Fahad Mirza (fahadmirza8@gmail.com)
 * @version  V2.0
 * @brief    Header file for bluetooth communication module
 ***********************************************************************/

#ifndef BLE_H__
#define BLE_H__

void BLE_Init(void);
uint8_t BLE_ProcessMsg(char *msgBuffer);


#endif /* BLE_H__ */

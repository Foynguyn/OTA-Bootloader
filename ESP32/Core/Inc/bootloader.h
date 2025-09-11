/**
 * @file bootloader.h
 * @brief STM32F103C8T6 Bootloader Header - ESP32 Compatible
 * @author Your Name
 * @date 2024
 */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "main.h"
#include "usart.h"
#include "gpio.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/* Memory Layout Definitions */
#define FLASH_BASE_ADDR             0x08000000
#define FLASH_SIZE                  (128 * 1024)    // 128KB
#define FLASH_PAGE_SIZE             1024            // 1KB per page

/* Bootloader Memory Layout */
#define BOOTLOADER_START_ADDR       0x08000000      // 0KB
#define BOOTLOADER_SIZE             (16 * 1024)     // 16KB
#define BOOTLOADER_END_ADDR         (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE - 1)

/* App Current Memory Layout */
#define APP_CURRENT_START_ADDR      0x08004000      // 16KB
#define APP_CURRENT_SIZE            (56 * 1024)     // 56KB  
#define APP_CURRENT_END_ADDR        (APP_CURRENT_START_ADDR + APP_CURRENT_SIZE - 1)
#define APP_CURRENT_FLAG_ADDR       (APP_CURRENT_END_ADDR - FLASH_PAGE_SIZE + 1) // Last page

/* App Old Memory Layout */
#define APP_OLD_START_ADDR          0x0800E000      // 72KB
#define APP_OLD_SIZE                (56 * 1024)     // 56KB
#define APP_OLD_END_ADDR            (APP_OLD_START_ADDR + APP_OLD_SIZE - 1)
#define APP_OLD_FLAG_ADDR           (APP_OLD_END_ADDR - FLASH_PAGE_SIZE + 1) // Last page

/* Simple Application Flag Structure - stored at end of each partition */
typedef struct {
    uint32_t magic;             // Magic number: 0xABCDEF00
    uint32_t valid;             // 0xAAAAAAAA = valid, 0x00000000 = invalid
    uint32_t size;              // Application size in bytes
    uint32_t version;           // Application version (optional)
} __attribute__((packed)) app_flag_t;

/* Constants */
#define APP_FLAG_MAGIC              0xABCDEF00
#define APP_FLAG_VALID              0xAAAAAAAA
#define APP_FLAG_INVALID            0x00000000

#define UART_TIMEOUT                5000            // 5 seconds
#define UART_RX_BUFFER_SIZE         256
#define FLASH_WRITE_BUFFER_SIZE     256

/* Simple Protocol - No commands needed, just receive binary data */
#define MAX_FIRMWARE_SIZE           (APP_CURRENT_SIZE - FLASH_PAGE_SIZE)  // 55KB max
#define UART_RECEIVE_TIMEOUT        10000   // 10 seconds timeout

/* Bootloader States */
typedef enum {
    BL_STATE_IDLE,
    BL_STATE_RECEIVING,
    BL_STATE_PROGRAMMING,
    BL_STATE_VERIFYING,
    BL_STATE_COMPLETE,
    BL_STATE_ERROR
} bootloader_state_t;

/* Function Prototypes */

/* Bootloader Core Functions */
void Bootloader_Init(void);
void Bootloader_Main(void);
void Bootloader_JumpToApp(uint32_t app_addr);

/* Flash Management */
HAL_StatusTypeDef Flash_ErasePage(uint32_t page_addr);
HAL_StatusTypeDef Flash_EraseApp(uint32_t app_start_addr, uint32_t app_size);
HAL_StatusTypeDef Flash_WriteData(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef Flash_ReadData(uint32_t addr, uint8_t *data, uint32_t size);

/* Application Management */
HAL_StatusTypeDef App_ValidateFlag(uint32_t flag_addr, app_flag_t *flag);
HAL_StatusTypeDef App_WriteFlag(uint32_t flag_addr, app_flag_t *flag);
HAL_StatusTypeDef App_CopyToOld(void);
HAL_StatusTypeDef App_IsValidApp(uint32_t app_addr);

/* UART Communication - Simple */
HAL_StatusTypeDef UART_ReceiveFirmware(uint32_t app_addr, uint32_t max_size, uint32_t *received_size);

/* Utility Functions */
void Delay_ms(uint32_t ms);
void LED_Toggle(void);
void System_Reset(void);

/* Debug Functions */
void Debug_Print(const char *format, ...);
void Debug_PrintHex(uint8_t *data, uint32_t size);

#endif /* __BOOTLOADER_H */

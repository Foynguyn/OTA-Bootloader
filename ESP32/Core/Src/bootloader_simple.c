/**
 * @file bootloader_simple.c
 * @brief Simple STM32F103C8T6 Bootloader - Compatible with ESP32 FOTA
 * @note Receives pure binary data from ESP32, no headers or CRC32 required
 */

#include "bootloader.h"
#include <stdio.h>
#include <stdarg.h>

/* Global Variables */
static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

/**
 * @brief Initialize bootloader
 */
void Bootloader_Init(void)
{
    /* Initialize UART */
    HAL_UART_Init(&huart1);
    
    /* Print bootloader info */
    Debug_Print("\r\n=== STM32F103C8T6 Simple Bootloader v1.0 ===\r\n");
    Debug_Print("Compatible with ESP32 FOTA System\r\n");
    Debug_Print("Bootloader: 0x%08X - 0x%08X (%d KB)\r\n", 
                BOOTLOADER_START_ADDR, BOOTLOADER_END_ADDR, BOOTLOADER_SIZE/1024);
    Debug_Print("App Current: 0x%08X - 0x%08X (%d KB)\r\n", 
                APP_CURRENT_START_ADDR, APP_CURRENT_END_ADDR, APP_CURRENT_SIZE/1024);
    Debug_Print("App Old: 0x%08X - 0x%08X (%d KB)\r\n", 
                APP_OLD_START_ADDR, APP_OLD_END_ADDR, APP_OLD_SIZE/1024);
}

/**
 * @brief Main bootloader loop
 */
void Bootloader_Main(void)
{
    app_flag_t current_flag, old_flag;
    HAL_StatusTypeDef status;
    
    Debug_Print("\r\n=== Bootloader Starting ===\r\n");
    
    /* Check current app */
    bool current_valid = false;
    status = App_ValidateFlag(APP_CURRENT_FLAG_ADDR, &current_flag);
    if (status == HAL_OK && current_flag.valid == APP_FLAG_VALID) {
        if (App_IsValidApp(APP_CURRENT_START_ADDR) == HAL_OK) {
            current_valid = true;
            Debug_Print("Current App: Valid, Size: %d bytes\r\n", (int)current_flag.size);
        }
    }
    
    /* Check old app */
    bool old_valid = false;
    status = App_ValidateFlag(APP_OLD_FLAG_ADDR, &old_flag);
    if (status == HAL_OK && old_flag.valid == APP_FLAG_VALID) {
        if (App_IsValidApp(APP_OLD_START_ADDR) == HAL_OK) {
            old_valid = true;
            Debug_Print("Old App: Valid, Size: %d bytes\r\n", (int)old_flag.size);
        }
    }
    
    /* Wait for firmware update or timeout to boot app */
    Debug_Print("Waiting for firmware from ESP32 (10 seconds timeout)...\r\n");
    Debug_Print("Send any data to start firmware update\r\n");
    
    uint32_t timeout = HAL_GetTick() + 10000; // 10 second timeout
    uint8_t dummy;
    
    while (HAL_GetTick() < timeout) {
        /* Check for any UART data */
        if (HAL_UART_Receive(&huart1, &dummy, 1, 100) == HAL_OK) {
            Debug_Print("Firmware update detected!\r\n");
            Bootloader_ReceiveFirmware();
            return; // After update, system will reset
        }
        
        /* Toggle LED to show bootloader is running */
        LED_Toggle();
        HAL_Delay(100);
    }
    
    /* Timeout reached, try to boot application */
    Debug_Print("Timeout reached, booting application...\r\n");
    
    /* Try current app first */
    if (current_valid) {
        Debug_Print("Booting Current App...\r\n");
        Bootloader_JumpToApp(APP_CURRENT_START_ADDR);
    }
    /* Fallback to old app */
    else if (old_valid) {
        Debug_Print("Booting Old App...\r\n");
        Bootloader_JumpToApp(APP_OLD_START_ADDR);
    }
    /* No valid app found */
    else {
        Debug_Print("ERROR: No valid application found!\r\n");
        Debug_Print("Staying in bootloader mode...\r\n");
        
        /* Stay in bootloader mode and wait for firmware */
        while (1) {
            if (HAL_UART_Receive(&huart1, &dummy, 1, 1000) == HAL_OK) {
                Debug_Print("Firmware update detected!\r\n");
                Bootloader_ReceiveFirmware();
                return;
            }
            LED_Toggle();
        }
    }
}

/**
 * @brief Receive firmware from ESP32 (pure binary data)
 */
void Bootloader_ReceiveFirmware(void)
{
    uint32_t received_size = 0;
    HAL_StatusTypeDef status;
    
    Debug_Print("=== Receiving Firmware from ESP32 ===\r\n");
    
    /* Check if we need to backup current app to old */
    app_flag_t current_flag;
    if (App_ValidateFlag(APP_CURRENT_FLAG_ADDR, &current_flag) == HAL_OK && 
        current_flag.valid == APP_FLAG_VALID) {
        Debug_Print("Backing up current app to old partition...\r\n");
        App_CopyToOld();
    }
    
    /* Erase current app partition */
    Debug_Print("Erasing current app partition...\r\n");
    status = Flash_EraseApp(APP_CURRENT_START_ADDR, APP_CURRENT_SIZE);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to erase current app partition\r\n");
        return;
    }
    
    /* Receive firmware data directly from ESP32 */
    Debug_Print("Receiving firmware data (pure binary)...\r\n");
    Debug_Print("ESP32 will send binary data directly...\r\n");
    
    status = UART_ReceiveFirmware(APP_CURRENT_START_ADDR, MAX_FIRMWARE_SIZE, &received_size);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to receive firmware\r\n");
        return;
    }
    
    Debug_Print("Firmware received successfully: %d bytes\r\n", (int)received_size);
    
    /* Validate received firmware */
    if (App_IsValidApp(APP_CURRENT_START_ADDR) != HAL_OK) {
        Debug_Print("ERROR: Received firmware is not valid!\r\n");
        return;
    }
    
    /* Write application flag */
    app_flag_t new_flag = {
        .magic = APP_FLAG_MAGIC,
        .valid = APP_FLAG_VALID,
        .size = received_size,
        .version = HAL_GetTick() // Use timestamp as version
    };
    
    status = App_WriteFlag(APP_CURRENT_FLAG_ADDR, &new_flag);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to write application flag\r\n");
        return;
    }
    
    /* Firmware update completed successfully */
    Debug_Print("Firmware update completed successfully!\r\n");
    
    /* Reset system to boot new firmware */
    Debug_Print("Resetting system to boot new firmware...\r\n");
    HAL_Delay(1000);
    System_Reset();
}

/**
 * @brief Jump to application
 * @param app_addr: Application start address
 */
void Bootloader_JumpToApp(uint32_t app_addr)
{
    uint32_t app_stack;
    uint32_t app_entry;
    void (*app_reset_handler)(void);
    
    /* Check if application is valid */
    app_stack = *((uint32_t*)app_addr);
    app_entry = *((uint32_t*)(app_addr + 4));
    
    /* Validate stack pointer */
    if ((app_stack & 0x2FFE0000) != 0x20000000) {
        Debug_Print("ERROR: Invalid stack pointer: 0x%08X\r\n", (unsigned int)app_stack);
        return;
    }
    
    /* Validate entry point */
    if ((app_entry & 0xFF000000) != 0x08000000) {
        Debug_Print("ERROR: Invalid entry point: 0x%08X\r\n", (unsigned int)app_entry);
        return;
    }
    
    Debug_Print("Jumping to application at 0x%08X\r\n", (unsigned int)app_addr);
    Debug_Print("Stack: 0x%08X, Entry: 0x%08X\r\n", (unsigned int)app_stack, (unsigned int)app_entry);
    
    /* Disable all interrupts */
    __disable_irq();
    
    /* Deinitialize peripherals */
    HAL_UART_DeInit(&huart1);
    HAL_DeInit();
    
    /* Set vector table offset */
    SCB->VTOR = app_addr;
    
    /* Set stack pointer */
    __set_MSP(app_stack);
    
    /* Jump to application */
    app_reset_handler = (void (*)(void))(app_entry);
    app_reset_handler();
}

/**
 * @brief Receive firmware via UART (compatible with ESP32)
 * @param app_addr: Application start address
 * @param max_size: Maximum firmware size
 * @param received_size: Pointer to received size variable
 * @return HAL status
 */
HAL_StatusTypeDef UART_ReceiveFirmware(uint32_t app_addr, uint32_t max_size, uint32_t *received_size)
{
    uint32_t total_received = 0;
    uint32_t write_addr = app_addr;
    HAL_StatusTypeDef status;
    uint32_t last_receive_time = HAL_GetTick();
    uint32_t no_data_timeout = 2000; // 2 seconds timeout for no data
    
    Debug_Print("Ready to receive firmware data...\r\n");
    
    while (total_received < max_size) {
        /* Receive data chunk */
        status = HAL_UART_Receive(&huart1, uart_rx_buffer, UART_RX_BUFFER_SIZE, 1000);
        
        if (status == HAL_OK) {
            /* Data received successfully */
            last_receive_time = HAL_GetTick();
            
            /* Write to flash */
            status = Flash_WriteData(write_addr, uart_rx_buffer, UART_RX_BUFFER_SIZE);
            if (status != HAL_OK) {
                Debug_Print("ERROR: Failed to write to flash at 0x%08X\r\n", (unsigned int)write_addr);
                return status;
            }
            
            write_addr += UART_RX_BUFFER_SIZE;
            total_received += UART_RX_BUFFER_SIZE;
            
            /* Print progress every 1KB */
            if (total_received % 1024 == 0) {
                Debug_Print("Received: %d bytes\r\n", (int)total_received);
            }
            
        } else if (status == HAL_TIMEOUT) {
            /* Check if we've been waiting too long without data */
            if (HAL_GetTick() - last_receive_time > no_data_timeout) {
                if (total_received > 0) {
                    /* We received some data, assume transmission is complete */
                    Debug_Print("No more data received, assuming transmission complete\r\n");
                    break;
                } else {
                    Debug_Print("ERROR: No data received within timeout\r\n");
                    return HAL_ERROR;
                }
            }
        } else {
            Debug_Print("ERROR: UART receive error\r\n");
            return status;
        }
    }
    
    *received_size = total_received;
    Debug_Print("Total firmware received: %d bytes\r\n", (int)total_received);
    
    return HAL_OK;
}

/* ============================================================================ */
/* Flash Management Functions */
/* ============================================================================ */

/**
 * @brief Erase flash page
 */
HAL_StatusTypeDef Flash_ErasePage(uint32_t page_addr)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;
    HAL_StatusTypeDef status;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return status;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = page_addr;
    erase_init.NbPages = 1;

    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief Erase application area
 */
HAL_StatusTypeDef Flash_EraseApp(uint32_t app_start_addr, uint32_t app_size)
{
    uint32_t num_pages = (app_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;

    Debug_Print("Erasing %d pages...\r\n", (int)num_pages);

    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t page_addr = app_start_addr + (i * FLASH_PAGE_SIZE);

        if (Flash_ErasePage(page_addr) != HAL_OK) {
            Debug_Print("ERROR: Failed to erase page at 0x%08X\r\n", (unsigned int)page_addr);
            return HAL_ERROR;
        }

        if ((i + 1) % 10 == 0) {
            Debug_Print("Erased %d/%d pages\r\n", (int)(i + 1), (int)num_pages);
        }
    }

    return HAL_OK;
}

/**
 * @brief Write data to flash
 */
HAL_StatusTypeDef Flash_WriteData(uint32_t addr, uint8_t *data, uint32_t size)
{
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return status;

    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t word_data = 0xFFFFFFFF;

        if (i < size) word_data = (word_data & 0xFFFFFF00) | data[i];
        if (i + 1 < size) word_data = (word_data & 0xFFFF00FF) | (data[i + 1] << 8);
        if (i + 2 < size) word_data = (word_data & 0xFF00FFFF) | (data[i + 2] << 16);
        if (i + 3 < size) word_data = (word_data & 0x00FFFFFF) | (data[i + 3] << 24);

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word_data);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

/**
 * @brief Read data from flash
 */
HAL_StatusTypeDef Flash_ReadData(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint8_t *flash_ptr = (uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++) {
        data[i] = flash_ptr[i];
    }
    return HAL_OK;
}

/* ============================================================================ */
/* Application Management Functions */
/* ============================================================================ */

/**
 * @brief Validate application flag
 */
HAL_StatusTypeDef App_ValidateFlag(uint32_t flag_addr, app_flag_t *flag)
{
    Flash_ReadData(flag_addr, (uint8_t*)flag, sizeof(app_flag_t));

    if (flag->magic != APP_FLAG_MAGIC) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Write application flag
 */
HAL_StatusTypeDef App_WriteFlag(uint32_t flag_addr, app_flag_t *flag)
{
    HAL_StatusTypeDef status;

    status = Flash_ErasePage(flag_addr);
    if (status != HAL_OK) return status;

    status = Flash_WriteData(flag_addr, (uint8_t*)flag, sizeof(app_flag_t));
    return status;
}

/**
 * @brief Check if application is valid (simple check)
 */
HAL_StatusTypeDef App_IsValidApp(uint32_t app_addr)
{
    uint32_t app_stack = *((uint32_t*)app_addr);
    uint32_t app_entry = *((uint32_t*)(app_addr + 4));

    /* Validate stack pointer */
    if ((app_stack & 0x2FFE0000) != 0x20000000) {
        return HAL_ERROR;
    }

    /* Validate entry point */
    if ((app_entry & 0xFF000000) != 0x08000000) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Copy current app to old partition
 */
HAL_StatusTypeDef App_CopyToOld(void)
{
    app_flag_t current_flag;
    uint8_t buffer[256];
    uint32_t bytes_to_copy, bytes_copied = 0;

    if (App_ValidateFlag(APP_CURRENT_FLAG_ADDR, &current_flag) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Erase old partition */
    if (Flash_EraseApp(APP_OLD_START_ADDR, APP_OLD_SIZE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Copy application data */
    bytes_to_copy = current_flag.size;

    while (bytes_copied < bytes_to_copy) {
        uint32_t chunk_size = (bytes_to_copy - bytes_copied > sizeof(buffer)) ?
                              sizeof(buffer) : (bytes_to_copy - bytes_copied);

        Flash_ReadData(APP_CURRENT_START_ADDR + bytes_copied, buffer, chunk_size);

        if (Flash_WriteData(APP_OLD_START_ADDR + bytes_copied, buffer, chunk_size) != HAL_OK) {
            return HAL_ERROR;
        }

        bytes_copied += chunk_size;
    }

    /* Copy flag */
    return App_WriteFlag(APP_OLD_FLAG_ADDR, &current_flag);
}

/* ============================================================================ */
/* Utility Functions */
/* ============================================================================ */

void LED_Toggle(void)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

void System_Reset(void)
{
    HAL_NVIC_SystemReset();
}

void Debug_Print(const char *format, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
 * @file bootloader.c
 * @brief STM32F103C8T6 Bootloader Implementation
 */

#include "bootloader.h"
#include "crc32.h"
#include <stdio.h>
#include <stdarg.h>

/* Global Variables */
static bootloader_state_t bl_state = BL_STATE_IDLE;
static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
static uint32_t received_size = 0;
static uint32_t expected_size = 0;
static uint32_t write_address = 0;

/**
 * @brief Initialize bootloader
 */
void Bootloader_Init(void)
{
    /* Initialize CRC32 */
    CRC32_Init();
    
    /* Initialize UART */
    HAL_UART_Init(&huart1);
    
    /* Print bootloader info */
    Debug_Print("\r\n=== STM32F103C8T6 Bootloader v1.0 ===\r\n");
    Debug_Print("Bootloader: 0x%08X - 0x%08X (%d KB)\r\n", 
                BOOTLOADER_START_ADDR, BOOTLOADER_END_ADDR, BOOTLOADER_SIZE/1024);
    Debug_Print("App Current: 0x%08X - 0x%08X (%d KB)\r\n", 
                APP_CURRENT_START_ADDR, APP_CURRENT_END_ADDR, APP_CURRENT_SIZE/1024);
    Debug_Print("App Old: 0x%08X - 0x%08X (%d KB)\r\n", 
                APP_OLD_START_ADDR, APP_OLD_END_ADDR, APP_OLD_SIZE/1024);
    
    bl_state = BL_STATE_IDLE;
}

/**
 * @brief Main bootloader loop
 */
void Bootloader_Main(void)
{
    app_header_t current_header, old_header;
    app_flag_t current_flag, old_flag;
    uint32_t current_version = 0, old_version = 0;
    HAL_StatusTypeDef status;
    
    Debug_Print("\r\n=== Bootloader Starting ===\r\n");
    
    /* Check current app */
    status = App_ValidateHeader(APP_CURRENT_START_ADDR, &current_header);
    if (status == HAL_OK) {
        status = App_ValidateFlag(APP_CURRENT_FLAG_ADDR, &current_flag);
        if (status == HAL_OK && current_flag.valid == APP_FLAG_VALID) {
            current_version = current_header.version;
            Debug_Print("Current App: Version %d.%d, Size: %d bytes\r\n", 
                       (int)(current_version >> 16), (int)(current_version & 0xFFFF), 
                       (int)current_header.size);
        }
    }
    
    /* Check old app */
    status = App_ValidateHeader(APP_OLD_START_ADDR, &old_header);
    if (status == HAL_OK) {
        status = App_ValidateFlag(APP_OLD_FLAG_ADDR, &old_flag);
        if (status == HAL_OK && old_flag.valid == APP_FLAG_VALID) {
            old_version = old_header.version;
            Debug_Print("Old App: Version %d.%d, Size: %d bytes\r\n", 
                       (int)(old_version >> 16), (int)(old_version & 0xFFFF), 
                       (int)old_header.size);
        }
    }
    
    /* Wait for firmware update or timeout to boot app */
    Debug_Print("Waiting for firmware update (5 seconds timeout)...\r\n");
    
    uint32_t timeout = HAL_GetTick() + 5000; // 5 second timeout
    uint8_t cmd;
    
    while (HAL_GetTick() < timeout) {
        /* Check for UART command */
        if (HAL_UART_Receive(&huart1, &cmd, 1, 100) == HAL_OK) {
            if (cmd == CMD_START_DOWNLOAD) {
                Debug_Print("Firmware update requested!\r\n");
                Bootloader_HandleFirmwareUpdate();
                return; // After update, system will reset
            }
        }
        
        /* Toggle LED to show bootloader is running */
        LED_Toggle();
        HAL_Delay(100);
    }
    
    /* Timeout reached, try to boot application */
    Debug_Print("Timeout reached, booting application...\r\n");
    
    /* Try current app first */
    if (current_version > 0) {
        Debug_Print("Booting Current App (Version %d.%d)...\r\n", 
                   (int)(current_version >> 16), (int)(current_version & 0xFFFF));
        Bootloader_JumpToApp(APP_CURRENT_START_ADDR);
    }
    /* Fallback to old app */
    else if (old_version > 0) {
        Debug_Print("Booting Old App (Version %d.%d)...\r\n", 
                   (int)(old_version >> 16), (int)(old_version & 0xFFFF));
        Bootloader_JumpToApp(APP_OLD_START_ADDR);
    }
    /* No valid app found */
    else {
        Debug_Print("ERROR: No valid application found!\r\n");
        Debug_Print("Staying in bootloader mode...\r\n");
        
        /* Stay in bootloader mode and wait for firmware */
        while (1) {
            if (HAL_UART_Receive(&huart1, &cmd, 1, 1000) == HAL_OK) {
                if (cmd == CMD_START_DOWNLOAD) {
                    Debug_Print("Firmware update requested!\r\n");
                    Bootloader_HandleFirmwareUpdate();
                    return;
                }
            }
            LED_Toggle();
        }
    }
}

/**
 * @brief Handle firmware update process
 */
void Bootloader_HandleFirmwareUpdate(void)
{
    app_header_t new_header;
    uint8_t response;
    uint32_t received_bytes = 0;
    uint32_t packet_size;
    HAL_StatusTypeDef status;
    
    Debug_Print("=== Firmware Update Process ===\r\n");
    
    /* Send OK response */
    response = RESP_OK;
    HAL_UART_Transmit(&huart1, &response, 1, 1000);
    
    /* Receive firmware header */
    status = HAL_UART_Receive(&huart1, (uint8_t*)&new_header, sizeof(app_header_t), 10000);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to receive firmware header\r\n");
        response = RESP_ERROR;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        return;
    }
    
    /* Validate header */
    if (new_header.magic != APP_HEADER_MAGIC) {
        Debug_Print("ERROR: Invalid header magic: 0x%08X\r\n", (unsigned int)new_header.magic);
        response = RESP_ERROR;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        return;
    }
    
    if (new_header.size > (APP_CURRENT_SIZE - sizeof(app_header_t) - FLASH_PAGE_SIZE)) {
        Debug_Print("ERROR: Firmware too large: %d bytes\r\n", (int)new_header.size);
        response = RESP_INVALID_SIZE;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        return;
    }
    
    Debug_Print("New Firmware: Version %d.%d, Size: %d bytes\r\n", 
               (int)(new_header.version >> 16), (int)(new_header.version & 0xFFFF), 
               (int)new_header.size);
    
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
        response = RESP_FLASH_ERROR;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        return;
    }
    
    /* Write header to flash */
    status = Flash_WriteData(APP_CURRENT_START_ADDR, (uint8_t*)&new_header, sizeof(app_header_t));
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to write header\r\n");
        response = RESP_FLASH_ERROR;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        return;
    }
    
    /* Send OK response for header */
    response = RESP_OK;
    HAL_UART_Transmit(&huart1, &response, 1, 1000);
    
    /* Receive firmware data */
    write_address = APP_CURRENT_START_ADDR + sizeof(app_header_t);
    received_bytes = 0;
    
    Debug_Print("Receiving firmware data...\r\n");
    
    while (received_bytes < new_header.size) {
        /* Receive packet size */
        status = HAL_UART_Receive(&huart1, (uint8_t*)&packet_size, 4, 5000);
        if (status != HAL_OK) {
            Debug_Print("ERROR: Failed to receive packet size\r\n");
            response = RESP_ERROR;
            HAL_UART_Transmit(&huart1, &response, 1, 1000);
            return;
        }
        
        if (packet_size > FLASH_WRITE_BUFFER_SIZE) {
            Debug_Print("ERROR: Packet too large: %d bytes\r\n", (int)packet_size);
            response = RESP_INVALID_SIZE;
            HAL_UART_Transmit(&huart1, &response, 1, 1000);
            return;
        }
        
        /* Receive packet data */
        status = HAL_UART_Receive(&huart1, uart_rx_buffer, packet_size, 5000);
        if (status != HAL_OK) {
            Debug_Print("ERROR: Failed to receive packet data\r\n");
            response = RESP_ERROR;
            HAL_UART_Transmit(&huart1, &response, 1, 1000);
            return;
        }
        
        /* Write to flash */
        status = Flash_WriteData(write_address, uart_rx_buffer, packet_size);
        if (status != HAL_OK) {
            Debug_Print("ERROR: Failed to write to flash\r\n");
            response = RESP_FLASH_ERROR;
            HAL_UART_Transmit(&huart1, &response, 1, 1000);
            return;
        }
        
        write_address += packet_size;
        received_bytes += packet_size;
        
        /* Send OK response for packet */
        response = RESP_OK;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        
        /* Print progress */
        if (received_bytes % 1024 == 0 || received_bytes == new_header.size) {
            Debug_Print("Progress: %d/%d bytes (%d%%)\r\n", 
                       (int)received_bytes, (int)new_header.size, 
                       (int)((received_bytes * 100) / new_header.size));
        }
    }
    
    /* Verify CRC32 */
    Debug_Print("Verifying firmware CRC32...\r\n");
    uint32_t calculated_crc = CRC32_CalculateFlash(APP_CURRENT_START_ADDR + sizeof(app_header_t), 
                                                   new_header.size);
    
    if (calculated_crc != new_header.crc32) {
        Debug_Print("ERROR: CRC32 mismatch! Expected: 0x%08X, Got: 0x%08X\r\n", 
                   (unsigned int)new_header.crc32, (unsigned int)calculated_crc);
        response = RESP_INVALID_CRC;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        return;
    }
    
    /* Write application flag */
    app_flag_t new_flag = {
        .magic = APP_FLAG_MAGIC,
        .valid = APP_FLAG_VALID,
        .version = new_header.version,
        .crc32 = CRC32_Calculate((uint8_t*)&new_flag, sizeof(app_flag_t) - 4)
    };
    
    status = App_WriteFlag(APP_CURRENT_FLAG_ADDR, &new_flag);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to write application flag\r\n");
        response = RESP_FLASH_ERROR;
        HAL_UART_Transmit(&huart1, &response, 1, 1000);
        return;
    }
    
    /* Firmware update completed successfully */
    Debug_Print("Firmware update completed successfully!\r\n");
    response = RESP_OK;
    HAL_UART_Transmit(&huart1, &response, 1, 1000);
    
    /* Reset system to boot new firmware */
    Debug_Print("Resetting system...\r\n");
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

/* ============================================================================ */
/* Flash Management Functions */
/* ============================================================================ */

/**
 * @brief Erase flash page
 * @param page_addr: Page address to erase
 * @return HAL status
 */
HAL_StatusTypeDef Flash_ErasePage(uint32_t page_addr)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;
    HAL_StatusTypeDef status;

    /* Unlock flash */
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    /* Configure erase */
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = page_addr;
    erase_init.NbPages = 1;

    /* Erase page */
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    /* Lock flash */
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief Erase application area
 * @param app_start_addr: Application start address
 * @param app_size: Application size
 * @return HAL status
 */
HAL_StatusTypeDef Flash_EraseApp(uint32_t app_start_addr, uint32_t app_size)
{
    uint32_t page_addr;
    uint32_t num_pages;
    HAL_StatusTypeDef status;

    /* Calculate number of pages to erase */
    num_pages = (app_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;

    Debug_Print("Erasing %d pages starting from 0x%08X\r\n", (int)num_pages, (unsigned int)app_start_addr);

    /* Erase pages */
    for (uint32_t i = 0; i < num_pages; i++) {
        page_addr = app_start_addr + (i * FLASH_PAGE_SIZE);

        status = Flash_ErasePage(page_addr);
        if (status != HAL_OK) {
            Debug_Print("ERROR: Failed to erase page at 0x%08X\r\n", (unsigned int)page_addr);
            return status;
        }

        /* Print progress */
        if ((i + 1) % 10 == 0 || (i + 1) == num_pages) {
            Debug_Print("Erased %d/%d pages\r\n", (int)(i + 1), (int)num_pages);
        }
    }

    return HAL_OK;
}

/**
 * @brief Write data to flash
 * @param addr: Flash address
 * @param data: Data buffer
 * @param size: Data size
 * @return HAL status
 */
HAL_StatusTypeDef Flash_WriteData(uint32_t addr, uint8_t *data, uint32_t size)
{
    HAL_StatusTypeDef status;
    uint32_t i;

    /* Unlock flash */
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    /* Write data word by word */
    for (i = 0; i < size; i += 4) {
        uint32_t word_data = 0xFFFFFFFF;

        /* Prepare word data */
        if (i < size) word_data = (word_data & 0xFFFFFF00) | data[i];
        if (i + 1 < size) word_data = (word_data & 0xFFFF00FF) | (data[i + 1] << 8);
        if (i + 2 < size) word_data = (word_data & 0xFF00FFFF) | (data[i + 2] << 16);
        if (i + 3 < size) word_data = (word_data & 0x00FFFFFF) | (data[i + 3] << 24);

        /* Write word */
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word_data);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
    }

    /* Lock flash */
    HAL_FLASH_Lock();

    return HAL_OK;
}

/**
 * @brief Read data from flash
 * @param addr: Flash address
 * @param data: Data buffer
 * @param size: Data size
 * @return HAL status
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
 * @brief Validate application header
 * @param app_addr: Application address
 * @param header: Pointer to header structure
 * @return HAL status
 */
HAL_StatusTypeDef App_ValidateHeader(uint32_t app_addr, app_header_t *header)
{
    /* Read header from flash */
    Flash_ReadData(app_addr, (uint8_t*)header, sizeof(app_header_t));

    /* Check magic number */
    if (header->magic != APP_HEADER_MAGIC) {
        return HAL_ERROR;
    }

    /* Check size */
    if (header->size == 0 || header->size > (APP_CURRENT_SIZE - sizeof(app_header_t) - FLASH_PAGE_SIZE)) {
        return HAL_ERROR;
    }

    /* Verify CRC32 */
    uint32_t calculated_crc = CRC32_CalculateFlash(app_addr + sizeof(app_header_t), header->size);
    if (calculated_crc != header->crc32) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Validate application flag
 * @param flag_addr: Flag address
 * @param flag: Pointer to flag structure
 * @return HAL status
 */
HAL_StatusTypeDef App_ValidateFlag(uint32_t flag_addr, app_flag_t *flag)
{
    /* Read flag from flash */
    Flash_ReadData(flag_addr, (uint8_t*)flag, sizeof(app_flag_t));

    /* Check magic number */
    if (flag->magic != APP_FLAG_MAGIC) {
        return HAL_ERROR;
    }

    /* Verify CRC32 */
    uint32_t calculated_crc = CRC32_Calculate((uint8_t*)flag, sizeof(app_flag_t) - 4);
    if (calculated_crc != flag->crc32) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Write application flag
 * @param flag_addr: Flag address
 * @param flag: Pointer to flag structure
 * @return HAL status
 */
HAL_StatusTypeDef App_WriteFlag(uint32_t flag_addr, app_flag_t *flag)
{
    HAL_StatusTypeDef status;

    /* Calculate CRC32 */
    flag->crc32 = CRC32_Calculate((uint8_t*)flag, sizeof(app_flag_t) - 4);

    /* Erase flag page */
    status = Flash_ErasePage(flag_addr);
    if (status != HAL_OK) {
        return status;
    }

    /* Write flag */
    status = Flash_WriteData(flag_addr, (uint8_t*)flag, sizeof(app_flag_t));

    return status;
}

/**
 * @brief Get application version
 * @param app_addr: Application address
 * @param version: Pointer to version variable
 * @return HAL status
 */
HAL_StatusTypeDef App_GetVersion(uint32_t app_addr, uint32_t *version)
{
    app_header_t header;
    HAL_StatusTypeDef status;

    status = App_ValidateHeader(app_addr, &header);
    if (status == HAL_OK) {
        *version = header.version;
    } else {
        *version = 0;
    }

    return status;
}

/**
 * @brief Copy current app to old partition
 * @return HAL status
 */
HAL_StatusTypeDef App_CopyToOld(void)
{
    app_header_t current_header;
    app_flag_t current_flag;
    HAL_StatusTypeDef status;
    uint8_t buffer[256];
    uint32_t bytes_to_copy, bytes_copied = 0;

    Debug_Print("Copying current app to old partition...\r\n");

    /* Validate current app */
    status = App_ValidateHeader(APP_CURRENT_START_ADDR, &current_header);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Current app is invalid\r\n");
        return status;
    }

    status = App_ValidateFlag(APP_CURRENT_FLAG_ADDR, &current_flag);
    if (status != HAL_OK || current_flag.valid != APP_FLAG_VALID) {
        Debug_Print("ERROR: Current app flag is invalid\r\n");
        return HAL_ERROR;
    }

    /* Erase old partition */
    status = Flash_EraseApp(APP_OLD_START_ADDR, APP_OLD_SIZE);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to erase old partition\r\n");
        return status;
    }

    /* Copy header + application data */
    bytes_to_copy = sizeof(app_header_t) + current_header.size;

    while (bytes_copied < bytes_to_copy) {
        uint32_t chunk_size = (bytes_to_copy - bytes_copied > sizeof(buffer)) ?
                              sizeof(buffer) : (bytes_to_copy - bytes_copied);

        /* Read from current */
        Flash_ReadData(APP_CURRENT_START_ADDR + bytes_copied, buffer, chunk_size);

        /* Write to old */
        status = Flash_WriteData(APP_OLD_START_ADDR + bytes_copied, buffer, chunk_size);
        if (status != HAL_OK) {
            Debug_Print("ERROR: Failed to copy data at offset %d\r\n", (int)bytes_copied);
            return status;
        }

        bytes_copied += chunk_size;

        /* Print progress */
        if (bytes_copied % 1024 == 0 || bytes_copied == bytes_to_copy) {
            Debug_Print("Copied %d/%d bytes\r\n", (int)bytes_copied, (int)bytes_to_copy);
        }
    }

    /* Copy flag */
    app_flag_t old_flag = current_flag;
    status = App_WriteFlag(APP_OLD_FLAG_ADDR, &old_flag);
    if (status != HAL_OK) {
        Debug_Print("ERROR: Failed to write old app flag\r\n");
        return status;
    }

    Debug_Print("Successfully copied current app to old partition\r\n");
    return HAL_OK;
}

/**
 * @brief Calculate CRC32 for application
 * @param addr: Application address
 * @param size: Application size
 * @return CRC32 value
 */
uint32_t App_CalculateCRC32(uint32_t addr, uint32_t size)
{
    return CRC32_CalculateFlash(addr, size);
}

/* ============================================================================ */
/* UART Communication Functions */
/* ============================================================================ */

/**
 * @brief Receive data via UART
 * @param buffer: Data buffer
 * @param size: Data size
 * @param timeout: Timeout in ms
 * @return HAL status
 */
HAL_StatusTypeDef UART_ReceiveData(uint8_t *buffer, uint32_t size, uint32_t timeout)
{
    return HAL_UART_Receive(&huart1, buffer, size, timeout);
}

/**
 * @brief Send response via UART
 * @param response: Response code
 * @return HAL status
 */
HAL_StatusTypeDef UART_SendResponse(uint8_t response)
{
    return HAL_UART_Transmit(&huart1, &response, 1, 1000);
}

/**
 * @brief Send data via UART
 * @param data: Data buffer
 * @param size: Data size
 * @return HAL status
 */
HAL_StatusTypeDef UART_SendData(uint8_t *data, uint32_t size)
{
    return HAL_UART_Transmit(&huart1, data, size, 5000);
}

/* ============================================================================ */
/* Utility Functions */
/* ============================================================================ */

/**
 * @brief Delay in milliseconds
 * @param ms: Delay time in ms
 */
void Delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
 * @brief Toggle LED (if available)
 */
void LED_Toggle(void)
{
    /* Toggle built-in LED on PC13 if configured */
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

/**
 * @brief Reset system
 */
void System_Reset(void)
{
    HAL_NVIC_SystemReset();
}

/**
 * @brief Debug print function
 * @param format: Printf-style format string
 * @param ...: Variable arguments
 */
void Debug_Print(const char *format, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
 * @brief Print hex data
 * @param data: Data buffer
 * @param size: Data size
 */
void Debug_PrintHex(uint8_t *data, uint32_t size)
{
    char hex_buffer[8];

    for (uint32_t i = 0; i < size; i++) {
        snprintf(hex_buffer, sizeof(hex_buffer), "%02X ", data[i]);
        HAL_UART_Transmit(&huart1, (uint8_t*)hex_buffer, 3, 1000);

        if ((i + 1) % 16 == 0) {
            HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 1000);
        }
    }

    if (size % 16 != 0) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 1000);
    }
}

/* ============================================================================ */
/* Missing Function Declaration */
/* ============================================================================ */

/**
 * @brief Handle firmware update process (declaration)
 */
void Bootloader_HandleFirmwareUpdate(void);

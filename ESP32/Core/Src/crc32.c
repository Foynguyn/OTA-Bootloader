/**
 * @file crc32.c
 * @brief CRC32 calculation implementation
 */

#include "crc32.h"

/* CRC32 Polynomial: 0x04C11DB7 (IEEE 802.3) */
#define CRC32_POLYNOMIAL    0xEDB88320UL

/* CRC32 lookup table */
static uint32_t crc32_table[256];
static uint8_t crc32_table_initialized = 0;

/**
 * @brief Initialize CRC32 lookup table
 */
void CRC32_Init(void)
{
    uint32_t crc;
    uint32_t i, j;
    
    if (crc32_table_initialized) {
        return;
    }
    
    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    
    crc32_table_initialized = 1;
}

/**
 * @brief Calculate CRC32 for data buffer
 * @param data: Pointer to data buffer
 * @param length: Length of data in bytes
 * @return CRC32 value
 */
uint32_t CRC32_Calculate(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i;
    
    if (!crc32_table_initialized) {
        CRC32_Init();
    }
    
    for (i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFFUL;
}

/**
 * @brief Calculate CRC32 for data in flash memory
 * @param flash_addr: Flash memory address
 * @param length: Length of data in bytes
 * @return CRC32 value
 */
uint32_t CRC32_CalculateFlash(uint32_t flash_addr, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i;
    uint8_t *data = (uint8_t *)flash_addr;
    
    if (!crc32_table_initialized) {
        CRC32_Init();
    }
    
    for (i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFFUL;
}

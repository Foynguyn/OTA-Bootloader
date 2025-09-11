/**
 * @file crc32.h
 * @brief CRC32 calculation for bootloader
 */

#ifndef __CRC32_H
#define __CRC32_H

#include <stdint.h>

/* CRC32 Functions */
uint32_t CRC32_Calculate(const uint8_t *data, uint32_t length);
uint32_t CRC32_CalculateFlash(uint32_t flash_addr, uint32_t length);
void CRC32_Init(void);

#endif /* __CRC32_H */

/*
 * Boot Log Interface
 * 
 * Logging wrapper for bootloader debug output
 */
#ifndef BOOT_LOG_H
#define BOOT_LOG_H

#include <stdio.h>

#define BOOT_PRINTF(...)  printf(__VA_ARGS__)

#endif /* BOOT_LOG_H */

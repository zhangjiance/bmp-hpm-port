/*
 * Boot Log Interface
 * 
 * Logging wrapper for bootloader debug output
 * In Release builds (NDEBUG defined), all logging is disabled
 */
#ifndef BOOT_LOG_H
#define BOOT_LOG_H

#include <stdio.h>

#ifdef NDEBUG
/* Release build - disable all logging */
#define BOOT_PRINTF(...)  ((void)0)
#else
/* Debug build - enable logging via printf */
#define BOOT_PRINTF(...)  printf(__VA_ARGS__)
#endif

#endif /* BOOT_LOG_H */

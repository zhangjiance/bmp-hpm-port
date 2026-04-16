/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BOOTUF2_CONFIG_H
#define BOOTUF2_CONFIG_H

#define CONFIG_BOOTUF2_CACHE_SIZE         2048  /* 2KB cache */
#define CONFIG_BOOTUF2_SECTOR_SIZE        512
#define CONFIG_BOOTUF2_SECTOR_PER_CLUSTER 2
#define CONFIG_BOOTUF2_SECTOR_RESERVED    1
#define CONFIG_BOOTUF2_NUM_OF_FAT         2
#define CONFIG_BOOTUF2_ROOT_ENTRIES       64

#define CONFIG_BOOTUF2_FAMILYID      0x0A4D5048  /* HPM family ID */
#define CONFIG_BOOTUF2_FLASHMAX      0xE0000     /* Application region: 896KB (1MB - 128KB boot) */
#define CONFIG_BOOTUF2_PAGE_COUNTMAX 1792        /* 896KB / 512 bytes per page */

#endif

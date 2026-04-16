#include <stdint.h>

/* First word at app base for bootloader validity check. */
__attribute__((section(".uf2_signature"), used))
const uint32_t uf2_signature = 0x0A4D5048UL;

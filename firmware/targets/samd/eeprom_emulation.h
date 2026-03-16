#pragma once

#include <stdint.h>

static uint8_t eeprom_read_byte (const uint8_t *__p) { return 0xff;}
static uint16_t eeprom_read_word (const uint16_t *__p) { return 0xffff;}
static uint32_t eeprom_read_dword (const uint32_t *__p) { return 0xffffffff;}
static void eeprom_write_byte (uint8_t *__p, uint8_t __value) {}
static void eeprom_write_word (uint16_t *__p, uint16_t __value) {}
static void eeprom_write_dword (uint32_t *__p, uint32_t __value) {}

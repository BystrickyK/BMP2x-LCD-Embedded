#pragma once

#include "InterruptIn.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "mbed.h"
#include "mbed_debug.h"
#include "mstd_utility"
#include "rtos.h"
#include <cstdlib>
#include <stdlib.h>
#include <string>

#define INSTRUCTION 0x00
#define DATA 0x02

#define MODE_8_BIT 0x33
#define MODE_4_BIT 0x22
#define MODE_2_LINES 0x28

#define DISPLAY_ON 0x0C
#define DISPLAY_OFF 0x08

#define CLEAR_DISPLAY 0x01
#define RETURN_HOME 0x02
#define JUMP_SECOND 0xC0
#define JUMP_FIRST 0x80

void send_bit(bool bit);
void send_byte(const char& data);

void lcd_write(const char& data, const char mode);
void lcd_write_8_bit(const char& data);
void lcd_write_data(const char& data);
void lcd_write_instruction(const char data);
void lcd_clear();
void lcd_initialize();
void lcd_first_line();
void lcd_second_line();
void lcd_write_string(const std::string& lcd_string);

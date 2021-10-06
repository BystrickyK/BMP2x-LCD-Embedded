#include "InterruptIn.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "mbed.h"
#include "mstd_utility"
#include "rtos.h"
#include <stdlib.h>
#include <string>

#define INSTRUCTION 0x00
#define DATA 0x02

#define MODE_4_BIT 0x10
#define CLEAR_DISPLAY 0x01
#define RETURN_HOME 0x02
#define SET_ENTRY_MODE_INCREMENT 0x06
#define JUMP_FIRST_LINE 0x20
#define JUMP_SECOND_LINE 0x30 // guessing

DigitalOut MOSI(D11);
DigitalOut SCK(D13);
DigitalOut CS(D10);



Thread thread1;
volatile bool running = true;
char random_number;

void send_bit(bool bit);
void send_byte(const char& data);

void lcd_write(const char& data, const char mode);
void lcd_write_8_bit(const char& data);
void lcd_write_data(const char& data);
void lcd_write_instruction(const char data);
void lcd_clear();
void lcd_initialize();
void lcd_write_string(std::string& lcd_string);

// main() runs in its own thread in the OS
int main()
{
    lcd_initialize();
    char random_number;
    std::string lcd_string;
    while (true) {
        random_number = std::rand() % 0xFF;
        lcd_string = "Random #: " + std::to_string(random_number);
        // lcd_string.insert(0, " ", 2);
        lcd_write_string(lcd_string);
        ThisThread::sleep_for(250ms);
        lcd_clear();
    }
}




void send_bit(bool bit){
    MOSI = bit;
    wait_us(1);
    SCK = true;
    wait_us(1);
    SCK = false;
}

void send_byte(const char& data){
    bool bit = 0;
    char bit_mask = 0x80;
    for(char i=0; i<8; i++){
        bit = data & bit_mask;
        send_bit(bit);
        bit_mask = bit_mask >> 1;
        wait_us(2);
    }
    SCK = true;
    wait_us(5);
    SCK = false;
    wait_us(5);
}



void lcd_write_8_bit(const char& data){
    CS = true;
    wait_us(2);
    send_byte(data);
    wait_us(2);
    CS = false;
    wait_us(5);
}

void lcd_write(const char& data, char mode){
    // Send first 4 most significant bits
    char data_out = (data & 0xF0) | mode;
    lcd_write_8_bit(data_out);
    // Send last 4 bits
    data_out = (data << 4) | mode;
    lcd_write_8_bit(data_out);
}

void lcd_write_data(const char& data){
    // send_byte(DATA);
    lcd_write(data, DATA);
}

void lcd_write_instruction(const char data){
    // send_byte(INSTRUCTION);
    lcd_write(data, INSTRUCTION);
}

void set_4bit_mode(){
    lcd_write_instruction(MODE_4_BIT);
    wait_us(5);
}

void lcd_clear(){
    lcd_write_instruction(CLEAR_DISPLAY);
    // send_byte(CLEAR_DISPLAY);
    wait_us(2000);
}

void lcd_initialize(){
        set_4bit_mode();
    lcd_clear();
    wait_us(2000);
    lcd_write_instruction(SET_ENTRY_MODE_INCREMENT);
    lcd_write_instruction(RETURN_HOME);
    // set_4bit_mode();
    // lcd_clear();
    // // lcd_write_instruction(MODE_4_BIT);
    // // lcd_write_instruction(CLEAR_DISPLAY);
    // lcd_write_instruction(SET_ENTRY_MODE_INCREMENT);
    // lcd_write_instruction(RETURN_HOME);
}

void lcd_write_string(std::string& lcd_string){
    for (char c: lcd_string){
        lcd_write_data(c);
    }
}
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

DigitalOut MOSI(D11);
DigitalOut SCK(D13);
DigitalOut EN(D10);

DigitalOut DEBUG(D8);


void send_bit(bool bit);
void send_byte(const char& data);

void lcd_write(const char& data, const char mode);
void lcd_write_8_bit(const char& data);
void lcd_write_data(const char& data);
void lcd_write_instruction(const char data);
void lcd_clear();
void lcd_initialize();
void lcd_write_string(const std::string& lcd_string);

void debug_beep();
void instruction_soup(); // Sends 100 random instructions to unstuck the LCD display

// main() runs in its own thread in the OS
int main()
{
    ThisThread::sleep_for(200ms);
    lcd_initialize();
    char random_number;
    std::string lcd_string;

    while (true) {

        random_number = std::rand() % 0xFF;
        lcd_string = "Random #:" + std::to_string(random_number);  // Create string
        lcd_string += std::string(16 - lcd_string.size(), ' ');   // Pad with spaces
        lcd_write_string(lcd_string);

        ThisThread::sleep_for(250ms);
        // lcd_clear();
        lcd_write_instruction(CLEAR_DISPLAY);
    }
}


void send_bit(bool bit){
    MOSI = bit;
    wait_us(1);
    SCK = true;
    wait_us(1);
    SCK = false;
    wait_us(1);
}

void send_byte(const char& data){
    bool bit = 0;
    char bit_mask = 0x80;
    for(char i=0; i<8; i++){
        bit = data & bit_mask;
        send_bit(bit);
        bit_mask = bit_mask >> 1;
        wait_us(1);
    }
    SCK = true;
    wait_us(1);
    SCK = false;
    wait_us(1);
}

void lcd_write_8_bit(const char& data){
    send_byte(data);
    EN = true;
    // debug_beep();
    wait_us(1);
    // wait_us(250000/SPEED);
    EN = false;
    // debug_beep();
    wait_us(1);
    // wait_us(250000/SPEED);

}

void lcd_write(const char& data, char mode){
    // Send first 4 most significant bits
    char data_out = (data & 0xF0) | mode;
    lcd_write_8_bit(data_out);
    // Send last 4 bits
    data_out = (data << 4) | mode;
    lcd_write_8_bit(data_out);
        // debug_beep();
        // debug_beep();
        // debug_beep();
}


void lcd_write_data(const char& data){
    lcd_write(data, DATA);
}


void lcd_write_instruction(const char data){
    lcd_write(data, INSTRUCTION);
    wait_us(3000);
}



void set_mode(){
    lcd_write_instruction(MODE_8_BIT);
    lcd_write_instruction(MODE_4_BIT);
    lcd_write_instruction(MODE_2_LINES);
}

void lcd_clear(){
    lcd_write_instruction(CLEAR_DISPLAY);
}

void lcd_initialize(){
    lcd_write_instruction(0x00);
    set_mode();
    lcd_clear();
    lcd_write_instruction(DISPLAY_ON); // + turn off cursor visibility
    // instruction_soup();
}

void lcd_write_string(const std::string& lcd_string){
    for (const char& c: lcd_string){
        lcd_write_data(c);
    }
}

void debug_beep(){
    DEBUG = true;
    ThisThread::sleep_for(2ms);
    DEBUG = false;
    ThisThread::sleep_for(50ms);
}

void instruction_soup(){
    char instr;
    for (int i=0; i<100; i++){
        instr = std::rand() % 0xFF;
        lcd_write_instruction(instr);
        printf("%d\n",instr);
    }
}
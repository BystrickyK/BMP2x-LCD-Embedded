#include "InterruptIn.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "mbed.h"
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
DigitalOut OE_(D10);
DigitalOut CS(D9);



Thread thread1;
volatile bool running = true;
char random_number;

void send_bit(bool bit);
void send_byte(char data);

void lcd_write(char& data, const char mode);
void lcd_write_data(char data);
void lcd_write_instruction(char data);
void lcd_clear();
void lcd_initialize();
void lcd_write_string(std::string& lcd_string);

// void print_rand(){
//     while(true){
//         random_number = std::rand()%0xFF;
//         send_byte(static_cast<const char>(random_number));
//         ThisThread::sleep_for(250ms);
//     }
// }

// main() runs in its own thread in the OS
int main()
{
    OE_ = false;
    lcd_initialize();
    char random_number;
    std::string lcd_string;
    while (true) {
        random_number = std::rand() % 0xFF;
        lcd_string = "Random #: " + std::to_string(random_number);
        lcd_write_string(lcd_string);
        ThisThread::sleep_for(1s);
        lcd_write_instruction(CLEAR_DISPLAY);
    }
}




void send_bit(bool bit){
    MOSI = bit;
    wait_us(1);
    SCK = true;
    wait_us(1);
    SCK = false;
}

void send_byte(char data){
    bool bit = 0;
    char bit_mask = 0x80;
    for(char i=0; i<8; i++){
        bit = data & bit_mask;
        send_bit(bit);
        bit_mask = bit_mask >> 1;
        wait_us(5);
    }
    SCK = true;
    wait_us(5);
    SCK = false;
    // wait_us(25000);
    wait_us(5);
}





void lcd_write(char& data, char mode){
    // Send first 4 most significant bits
    char data_out = (data & 0xF0) | mode;
    CS = true;
    wait_us(100);
    send_byte(static_cast<const char>(data_out));
    wait_us(100);
    CS = false;
    wait_us(100);
    // Send last 4 bits
    data_out = (data << 4) | mode;
    CS = true;
    wait_us(100);
    send_byte(static_cast<const char>(data_out));
    wait_us(100);
    CS = false;
    wait_us(100);
}

void lcd_write_data(char data){
    send_byte(DATA);
    lcd_write(data, DATA);
}

void lcd_write_instruction(char data){
    send_byte(INSTRUCTION);
    lcd_write(data, INSTRUCTION);
}

void set_4bit_mode(){
    send_byte(MODE_4_BIT);
    wait_us(5);
}

void lcd_clear(){
    send_byte(CLEAR_DISPLAY);
    wait_us(5);
}

void lcd_initialize(){
    set_4bit_mode();
    lcd_clear();
    wait_us(2000);
    lcd_write_instruction(SET_ENTRY_MODE_INCREMENT);
    lcd_write_instruction(RETURN_HOME);
}

void lcd_write_string(std::string& lcd_string){
    for (char &c: lcd_string){
        lcd_write_data(c);
    }
}
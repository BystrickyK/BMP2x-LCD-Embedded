#include "comm.h"

DigitalOut MOSI(D11);
DigitalOut SCK(D13);
DigitalOut EN(D10);
DigitalOut DEBUG(D8);

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

void lcd_first_line(){
    lcd_write_instruction(JUMP_FIRST);
}

void lcd_second_line(){
    lcd_write_instruction(JUMP_SECOND);
}
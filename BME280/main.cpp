#include "InterruptIn.h"
#include "LittleFileSystem.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "bme280_defs.h"
#include "mbed.h"
#include "mbed_debug.h"
#include "mstd_utility"
#include "rtos.h"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <vector>
#include <array>
#include "comm.h"
#include "BME280_driver/bme280.h"
/* Change in bme280_defs.h

line
121 -> BME280 chip identifier
122 -> // #define BME280_CHIP_ID                            UINT8_C(0x60)
// Must've been redefined, the real ID written in the chip ID register
// is actually 0x58
123 -> #define BME280_CHIP_ID                            UINT8_C(0x58) 

*/


bme280_dev dev;  // main BME280 API object
int8_t rslt = 1;
uint8_t dev_addr = 0x76<<1;

I2C i2c(D14, D15);
I2C* i2c_ptr = &i2c;

int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
void user_delay_us(uint32_t period, void *intf_ptr);

// main() runs in its own thread in the OS
int main()
{ 

    dev.intf = BME280_I2C_INTF;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_us = user_delay_us;

    ThisThread::sleep_for(500ms);
    lcd_initialize();
    std::string lcd_string;

    bme280_data comp_data;

    bool device_found = false;
    dev_addr -= 20;      

    while (!device_found){  
        dev.intf_ptr = &dev_addr;
        // TODO: Problem with failing chip ID check
        rslt = bme280_init(&dev);
        // rslt = user_i2c_read(0xD0, &data[0], 1, &dev_addr);
        // printf("%d | %x\n", data[0], data[0]);

        lcd_first_line();
        lcd_string = "Result: " + std::to_string(rslt) + "  ";
        lcd_write_string(lcd_string);

        lcd_second_line();
        lcd_string = "I2C Add: " + std::to_string(dev_addr) + "  ";
         lcd_write_string(lcd_string);

        ThisThread::sleep_for(250ms);

        if (rslt == 0 ) {
            device_found = true;
            lcd_first_line();
            lcd_write_string("Address found...");
            ThisThread::sleep_for(2s);
             lcd_clear();
         }
        else dev_addr++;
    }

    /* Recommended mode of operation: Indoor navigation */
	dev.settings.osr_h = BME280_OVERSAMPLING_2X;
	dev.settings.osr_p = BME280_OVERSAMPLING_4X;
	dev.settings.osr_t = BME280_OVERSAMPLING_2X;
	dev.settings.filter = BME280_FILTER_COEFF_2;
	dev.settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

    uint8_t settings_sel;
	settings_sel = BME280_OSR_PRESS_SEL;
	settings_sel |= BME280_OSR_TEMP_SEL;
	settings_sel |= BME280_OSR_HUM_SEL;
	settings_sel |= BME280_STANDBY_SEL;
	settings_sel |= BME280_FILTER_SEL;
    rslt = bme280_set_sensor_settings(settings_sel, &dev);

    if (rslt==0) rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, &dev); // Set normal mode
    if (rslt==0){
        lcd_first_line();
        lcd_write_string("Sensor mode:");
        lcd_second_line();
        lcd_write_string("Normal");
         ThisThread::sleep_for(2s);
         lcd_clear();
    }
    else{
         lcd_first_line();
         lcd_write_string("Sensor mode:");
         lcd_second_line();
         lcd_write_string("Error");
         ThisThread::sleep_for(2s);
          lcd_clear();
    }

    while (true) {
        rslt = bme280_get_sensor_data(7, &comp_data, &dev);
        printf("%d ||| \t%d.%d Pa | %d.%d degC | %d.%d %relHum\n", rslt,
        int(comp_data.pressure), int(comp_data.pressure*100)%100,
        int(comp_data.temperature), int(comp_data.temperature*1000)%1000,
        int(comp_data.humidity), int(comp_data.humidity*1000)%1000);

        ThisThread::sleep_for(500ms);
    } // while(1) end
} // Main end



int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    rslt = int8_t(i2c_ptr->write((*(int*)intf_ptr), (const char*)&reg_addr, 1));
    // if (rslt!=0) return rslt;
    rslt = int8_t(i2c_ptr->read((*(int*)intf_ptr), (char*)reg_data, int(len)));
    return rslt;
}

int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    std::vector<uint8_t> i2c_data_package; // Create data to send through I2C
    i2c_data_package.emplace_back(reg_addr); // First byte == register address
    i2c_data_package.insert(i2c_data_package.end(), &reg_data[0], &reg_data[len]);

    rslt = int8_t(i2c_ptr->write((*(int*)intf_ptr), (const char*)&i2c_data_package[0], len+1));
    return rslt;
}

void user_delay_us(uint32_t period, void *intf_ptr){
    wait_us(period);
}
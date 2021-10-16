#include "InterruptIn.h"
#include "LittleFileSystem.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "bmp2_defs.h"
#include "cmsis_os2.h"
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
#include "lcd_comm.h"
#include "BMP2-Sensor-API/bmp2.h"

#define BMP2_64BIT_COMPENSATION


bmp2_dev dev;  // main BMP2 API object

I2C i2c(D14, D15);
I2C* i2c_ptr = &i2c;

// Strategies for BMP2x API
int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
void user_delay_us(uint32_t period, void *intf_ptr);

int8_t find_I2C_device_address(bmp2_dev* dev, uint8_t dev_addr);
int8_t configure_sensor_module(bmp2_dev* dev);

int8_t measurement_thread_fun(bmp2_dev* dev, bmp2_data* comp_data);
int8_t display_thread_fun(bmp2_data* comp_data);

uint64_t clock_ms() { return us_ticker_read() / 1000; }

// Set up threads
Thread measurement_thread{osPriorityHigh};
Thread display_thread{osPriorityNormal};

int main() // MAIN START
{ 
    // Bosch Sensortec BMP2x API object settings
        dev.intf = BMP2_I2C_INTF;   // Use I2C
        dev.read = user_i2c_read;   // Define read strategy
        dev.write = user_i2c_write; // Define write strategy
        dev.delay_us = user_delay_us;
        uint8_t rslt;

    // LCD display initialization
        lcd_initialize();     

    // Sweep across I2C device address space to find the sensor
        rslt = find_I2C_device_address(&dev, 0x76);

    // Configure oversampling modes, filter coefficient and measurement period 
        rslt = configure_sensor_module(&dev);
    
    // Start measurement thread
        bmp2_data comp_data;  // Measurement data container
        auto measurement_lambda = [&](){
            measurement_thread_fun(&dev, &comp_data);
        };
        measurement_thread.start(measurement_lambda);

    // Start display thread
        auto display_lambda = [&](){
            display_thread_fun(&comp_data);
        };
        display_thread.start(display_lambda);

} // MAIN END


/* Strategy functions */

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

/*  */

int8_t find_I2C_device_address(bmp2_dev* dev, uint8_t dev_addr=0x76){

    std::string lcd_string;
    uint8_t rslt = 0;
    bool device_found = false;   

    while (!device_found){  
        dev->intf_ptr = &dev_addr;
        rslt = bmp2_init(dev);

        lcd_string = "Result: " + std::to_string(rslt) + "  ";
        lcd_write_string_to_line(lcd_string, 1);

        lcd_string = "I2C Add: " + std::to_string(dev_addr) + "  ";
        lcd_write_string_to_line(lcd_string, 2);

        ThisThread::sleep_for(12ms);

        if (rslt == 0 ) {
            bmp2_soft_reset(dev);
            device_found = true;
            lcd_first_line();
            lcd_write_string("Address found...");
            ThisThread::sleep_for(1s);
            lcd_clear();
            }
        else dev_addr++; 
    } // while(!device_found) end
    return 0; // return 0 on success
} // function definition end

int8_t configure_sensor_module(bmp2_dev* dev){

    uint8_t rslt;
    uint8_t tmp;

    bmp2_config conf;
	conf.os_mode = BMP2_OS_MODE_HIGH_RESOLUTION;
	conf.filter = BMP2_FILTER_COEFF_8;
	conf.odr = BMP2_ODR_250_MS;
    rslt = bmp2_set_config(&conf, dev);

    // Configure power mode, normal == periodical measurements
    if (rslt==0) rslt = bmp2_set_power_mode(BMP2_POWERMODE_NORMAL, &conf, dev);
    else return 1;

    if (rslt==0){
        bmp2_get_power_mode(&tmp, dev);
        lcd_write_string_to_line("Power mode: " + std::to_string(tmp), 1);

        bmp2_get_regs(0xF4, &tmp, 1, dev);
        lcd_write_string_to_line("Ctrl: " + std::to_string(tmp), 2);

        ThisThread::sleep_for(2s);
        lcd_clear();
    }
    else{
        lcd_write_string_to_line("Power mode:", 1);
        lcd_write_string_to_line("Error", 2);
        ThisThread::sleep_for(2s);
        lcd_clear();
        return 1;
    }
    return 0;
} // function definition end

int8_t measurement_thread_fun(bmp2_dev* dev, bmp2_data* comp_data){

    uint8_t rslt;
    auto time_start = clock_ms();
    auto time_elapsed = clock_ms();

    while(true){
        rslt = bmp2_get_sensor_data(comp_data, dev);
        ThisThread::sleep_for(250ms);
        time_elapsed = clock_ms() - time_start;
        time_start = clock_ms();
        printf("\tMeas | %llu\n", time_elapsed);
    }
} // function definition end

int8_t display_thread_fun(bmp2_data* comp_data){

    std::string temp_dec, lcd_string;
    auto temperature = comp_data->temperature;
    auto pressure = comp_data->pressure;
    auto time_start = clock_ms();
    auto time_elapsed = clock_ms();

    while(true){    
        temperature = comp_data->temperature;
        pressure = comp_data->pressure;

        // Display temperature
            temp_dec = std::to_string(int(temperature*100)%100);
            if (temp_dec.size()==1) temp_dec = "0" + temp_dec; // add leading zero 

            lcd_string = "T: " + std::to_string(int(temperature)) + 
            "." + temp_dec + " degC";
            lcd_write_string_to_line(lcd_string, 1);

        // Display pressure
            lcd_string = "p: " + std::to_string(int(pressure)) + 
            "." + std::to_string(int(pressure*10)%10) + " Pa";
            lcd_write_string_to_line(lcd_string, 2);

        ThisThread::sleep_for(250ms);
        time_elapsed = clock_ms() - time_start;
        time_start = clock_ms();
        printf("Disp | %llu\n", time_elapsed);
    } // while(true) end
} // function definition end

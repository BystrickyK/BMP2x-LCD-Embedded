#include "InterruptIn.h"
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

int8_t rslt = 1;
uint8_t dev_addr = 0x76<<1;

I2C i2c(D14, D15);
I2C* i2c_ptr = &i2c;

int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);

uint8_t read_calibration_data(bme280_calib_data* calib_data, uint8_t dev_add);

template <typename T>
T from_2_bytes(uint8_t LSB, uint8_t MSB);

// main() runs in its own thread in the OS
int main()
{ 
    ThisThread::sleep_for(500ms);
    lcd_initialize();
    std::string lcd_string;
    std::array<uint8_t, 8> data;
    bme280_uncomp_data uncomp_data;
    bme280_calib_data calib_data;

    bool address_found = false;
    bool calibration_successful = false;
    bool sensor_mode_set = false;

    while (true) {      

        if (!address_found){  
            rslt = user_i2c_read(data[0], &data[0], 1, &dev_addr);

            lcd_first_line();
            lcd_string = "Result: " + std::to_string(rslt);
            lcd_write_string(lcd_string);

            lcd_second_line();
            lcd_string = "I2C Add: " + std::to_string(dev_addr);
            lcd_write_string(lcd_string);

            ThisThread::sleep_for(100ms);
            // lcd_clear();

            if (rslt == 0 ) {
                address_found = true;
                ThisThread::sleep_for(2s);
                lcd_clear();
                printf("Address found...");
            }
            else dev_addr++;
        }

        if (!calibration_successful)
        {
            rslt = read_calibration_data(&calib_data, dev_addr);
            if (rslt==0){
                calibration_successful=true;
                printf("Calibration successful...");
            }
        }

        if (calibration_successful && !sensor_mode_set){
            data[0] = 0x27;
            rslt = user_i2c_write(0xF4, &data[0], 1, &dev_addr);
            if (rslt==0){
                sensor_mode_set=true;
                printf("Sensor activated...");
            }
        }

        if (sensor_mode_set){
            rslt = user_i2c_read(0xF7, &data[0], 8, &dev_addr);
            bme280_parse_sensor_data(&data[0], &uncomp_data);
            printf("%d \t | %d \t | %d\n", 
            uncomp_data.pressure, uncomp_data.temperature, uncomp_data.humidity);
        }

    ThisThread::sleep_for(50ms);

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
    i2c_data_package.insert(i2c_data_package.end(), &reg_data[0], &reg_data[len-1]);

    rslt = int8_t(i2c_ptr->write((*(int*)intf_ptr), (const char*)&i2c_data_package[0], int(len)));
    return rslt;
}

uint8_t read_calibration_data(bme280_calib_data* calib_data, uint8_t dev_add){
    // https://www.mouser.com/datasheet/2/783/BST-BME280-DS002-1509607.pdf

    // read 24 bytes with the temperature/pressure calibration data
    std::array<uint8_t, 24> data;
    uint8_t result = 0;

    uint8_t reg_add = 0x88;
    result = user_i2c_read(reg_add, &data[0], 24, &dev_add);
    if (result != 0) return result;
    calib_data->dig_t1 = from_2_bytes<uint16_t>(data[0], data[1]);
    calib_data->dig_t2 = from_2_bytes<int16_t>(data[2], data[3]);
    calib_data->dig_t3 = from_2_bytes<int16_t>(data[4], data[5]);
    calib_data->dig_p1 = from_2_bytes<uint16_t>(data[6], data[7]);
    calib_data->dig_p2 = from_2_bytes<int16_t>(data[8], data[9]);
    calib_data->dig_p3 = from_2_bytes<int16_t>(data[10], data[11]);
    calib_data->dig_p4 = from_2_bytes<int16_t>(data[12], data[13]);
    calib_data->dig_p5 = from_2_bytes<int16_t>(data[14], data[15]);
    calib_data->dig_p6 = from_2_bytes<int16_t>(data[16], data[17]);
    calib_data->dig_p7 = from_2_bytes<int16_t>(data[18], data[19]);
    calib_data->dig_p8 = from_2_bytes<int16_t>(data[20], data[21]);
    calib_data->dig_p9 = from_2_bytes<int16_t>(data[22], data[23]);

    reg_add = 0xA1;
    result = user_i2c_read(reg_add, &data[0], 9, &dev_add);
    if (result != 0) return result;
    calib_data->dig_h1 = data[0];
    calib_data->dig_h2 = from_2_bytes<int16_t>(data[1], data[2]);
    calib_data->dig_h3 = data[3];
    calib_data->dig_h4 = static_cast<int16_t>(data[4]<<4 | data[5]);
    calib_data->dig_h5 = static_cast<int16_t>(data[6]<<8 | data[7]);
    calib_data->dig_h6 = static_cast<int8_t>(data[8]);
    return result;
    }



template <typename T>
T from_2_bytes(uint8_t LSB, uint8_t MSB)
{
    return static_cast<T>(uint16_t(MSB<<8 | LSB));
}
/**
 * Copyright (C) 2022 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*********************************************************************/
/* system header files */
/*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*********************************************************************/
/* own header files */
/*********************************************************************/
#include "coines.h"
#include "bmi08x.h"
#include "common.h"

/*********************************************************************/
/*                              Macros                               */
/*********************************************************************/

/* Buffer size allocated to store raw FIFO data for accel */
#define BMI08X_ACC_FIFO_RAW_DATA_BUFFER_SIZE             UINT16_C(1024)

/* Length of data to be read from FIFO for accel */
#define BMI08X_ACC_FIFO_RAW_DATA_USER_LENGTH             UINT16_C(1024)

/* Number of Accel frames to be extracted from FIFO */
#define BMI08X_ACC_FIFO_FULL_EXTRACTED_DATA_FRAME_COUNT  UINT8_C(100)

/*********************************************************************/
/*                       Global variables                            */
/*********************************************************************/

/*! @brief This structure containing relevant bmi08x info */
struct bmi08x_dev bmi08xdev;

/*! @brief variable to hold the bmi08x accel data */
struct bmi08x_sensor_data bmi08x_accel[100] = { { 0 } };

/*! bmi08x accel int config */
struct bmi08x_accel_int_channel_cfg accel_int_config;

/*********************************************************************/
/*                      Static function declarations                 */
/*********************************************************************/

/*!
 * @brief    This internal API is used to initialize the bmi08x sensor with default
 */
static int8_t init_bmi08x(void);

/*********************************************************************/
/*                            Functions                              */
/*********************************************************************/

/*!
 *  @brief This internal API is used to initializes the bmi08x sensor
 *  settings like power mode and OSRS settings.
 *
 *  @param[in] void
 *
 *  @return void
 *
 */
static int8_t init_bmi08x(void)
{
    int8_t rslt;

    rslt = bmi08a_init(&bmi08xdev);
    bmi08x_error_codes_print_result("bmi08a_init", rslt);

    if (rslt == BMI08X_OK)
    {
        rslt = bmi08g_init(&bmi08xdev);
        bmi08x_error_codes_print_result("bmi08g_init", rslt);
    }

    if (rslt == BMI08X_OK)
    {
        printf("Uploading config file !\n");
        rslt = bmi08a_load_config_file(&bmi08xdev);
        bmi08x_error_codes_print_result("bmi08a_load_config_file", rslt);
    }

    if (rslt == BMI08X_OK)
    {
        bmi08xdev.accel_cfg.odr = BMI08X_ACCEL_ODR_1600_HZ;

        if (bmi08xdev.variant == BMI085_VARIANT)
        {
            bmi08xdev.accel_cfg.range = BMI085_ACCEL_RANGE_16G;
        }
        else if (bmi08xdev.variant == BMI088_VARIANT)
        {
            bmi08xdev.accel_cfg.range = BMI088_ACCEL_RANGE_24G;
        }

        bmi08xdev.accel_cfg.power = BMI08X_ACCEL_PM_ACTIVE;
        bmi08xdev.accel_cfg.bw = BMI08X_ACCEL_BW_NORMAL;

        rslt = bmi08a_set_power_mode(&bmi08xdev);
        bmi08x_error_codes_print_result("bmi08a_set_power_mode", rslt);

        rslt = bmi08a_set_meas_conf(&bmi08xdev);
        bmi08x_error_codes_print_result("bmi08a_set_meas_conf", rslt);

        bmi08xdev.gyro_cfg.odr = BMI08X_GYRO_BW_230_ODR_2000_HZ;
        bmi08xdev.gyro_cfg.range = BMI08X_GYRO_RANGE_250_DPS;
        bmi08xdev.gyro_cfg.bw = BMI08X_GYRO_BW_230_ODR_2000_HZ;
        bmi08xdev.gyro_cfg.power = BMI08X_GYRO_PM_NORMAL;

        rslt = bmi08g_set_power_mode(&bmi08xdev);
        bmi08x_error_codes_print_result("bmi08g_set_power_mode", rslt);

        rslt = bmi08g_set_meas_conf(&bmi08xdev);
        bmi08x_error_codes_print_result("bmi08g_set_meas_conf", rslt);
    }

    return rslt;

}

/*!
 *  @brief This API is used to enable bmi08x interrupt
 *
 *  @param[in] void
 *
 *  @return void
 *
 */
static int8_t enable_bmi08x_interrupt()
{
    int8_t rslt;

    /* Set accel interrupt pin configuration */
    accel_int_config.int_channel = BMI08X_INT_CHANNEL_1;
    accel_int_config.int_type = BMI08X_ACCEL_INT_FIFO_FULL;
    accel_int_config.int_pin_cfg.output_mode = BMI08X_INT_MODE_PUSH_PULL;
    accel_int_config.int_pin_cfg.lvl = BMI08X_INT_ACTIVE_HIGH;
    accel_int_config.int_pin_cfg.enable_int_pin = BMI08X_ENABLE;

    /* Enable accel data ready interrupt channel */
    rslt = bmi08a_set_int_config((const struct bmi08x_accel_int_channel_cfg*)&accel_int_config, &bmi08xdev);
    bmi08x_error_codes_print_result("bmi08a_set_int_config", rslt);

    return rslt;
}

/*!
 *  @brief This API is used to disable bmi08x interrupt
 *
 *  @param[in] void
 *
 *  @return void
 *
 */
static int8_t disable_bmi08x_interrupt()
{
    int8_t rslt;

    /* Set accel interrupt pin configuration */
    accel_int_config.int_channel = BMI08X_INT_CHANNEL_1;
    accel_int_config.int_type = BMI08X_ACCEL_INT_FIFO_FULL;
    accel_int_config.int_pin_cfg.output_mode = BMI08X_INT_MODE_PUSH_PULL;
    accel_int_config.int_pin_cfg.lvl = BMI08X_INT_ACTIVE_HIGH;
    accel_int_config.int_pin_cfg.enable_int_pin = BMI08X_DISABLE;

    /* Disable accel data ready interrupt channel */
    rslt = bmi08a_set_int_config((const struct bmi08x_accel_int_channel_cfg*)&accel_int_config, &bmi08xdev);
    bmi08x_error_codes_print_result("bmi08a_set_int_config", rslt);

    return rslt;
}

/*!
 *  @brief Main Function where the execution getting started to test the code.
 *
 *  @return status
 *
 */
int main(void)
{
    int8_t rslt;

    uint8_t status = 0;

    /* Initialize FIFO frame structure */
    struct bmi08x_fifo_frame fifo_frame = { 0 };

    /* To configure the FIFO accel configurations */
    struct bmi08x_accel_fifo_config config;

    /* Number of bytes of FIFO data */
    uint8_t fifo_data[BMI08X_ACC_FIFO_RAW_DATA_BUFFER_SIZE] = { 0 };

    /* Number of accelerometer frames */
    uint16_t accel_length = BMI08X_ACC_FIFO_FULL_EXTRACTED_DATA_FRAME_COUNT;

    /* Variable to index bytes */
    uint16_t idx = 0;

    uint8_t try = 1;

    /* Variable to store sensor time value */
    uint32_t sensor_time;

    /* Variable to store available fifo length */
    uint16_t fifo_length;

    /* Interface given as parameter
     *           For I2C : BMI08X_I2C_INTF
     *           For SPI : BMI08X_SPI_INTF
     * Sensor variant given as parameter
     *          For BMI085 : BMI085_VARIANT
     *          For BMI088 : BMI088_VARIANT
     */
    rslt = bmi08x_interface_init(&bmi08xdev, BMI08X_I2C_INTF, BMI085_VARIANT);
    bmi08x_error_codes_print_result("bmi08x_interface_init", rslt);

    if (rslt == BMI08X_OK)
    {
        rslt = init_bmi08x();
        bmi08x_error_codes_print_result("init_bmi08x", rslt);

        /* Enable data ready interrupts */
        rslt = enable_bmi08x_interrupt();
        bmi08x_error_codes_print_result("enable_bmi08x_interrupt", rslt);

        printf("Accel FIFO full interrupt data\n");

        if (rslt == BMI08X_OK)
        {
            config.accel_en = BMI08X_ENABLE;

            /* Set FIFO configuration by enabling accelerometer */
            rslt = bmi08a_set_fifo_config(&config, &bmi08xdev);
            bmi08x_error_codes_print_result("bmi08a_set_fifo_config", rslt);

            while (try <= 3)
            {
                rslt = bmi08a_get_data_int_status(&status, &bmi08xdev);
                bmi08x_error_codes_print_result("bmi08a_get_data_int_status", rslt);

                if (status & BMI08X_ACCEL_FIFO_FULL_INT)
                {
                    printf("\nIteration : %d\n", try);

                    /* Update FIFO structure */
                    fifo_frame.data = fifo_data;
                    fifo_frame.length = BMI08X_ACC_FIFO_RAW_DATA_USER_LENGTH;

                    accel_length = BMI08X_ACC_FIFO_FULL_EXTRACTED_DATA_FRAME_COUNT;

                    rslt = bmi08a_get_fifo_length(&fifo_length, &bmi08xdev);
                    bmi08x_error_codes_print_result("bmi08a_get_fifo_length", rslt);

                    printf("FIFO buffer size : %d\n", fifo_frame.length);
                    printf("FIFO length available : %d\n\n", fifo_length);

                    printf("Requested data frames before parsing: %d\n", accel_length);

                    if (rslt == BMI08X_OK)
                    {
                        /* Read FIFO data */
                        rslt = bmi08a_read_fifo_data(&fifo_frame, &bmi08xdev);
                        bmi08x_error_codes_print_result("bmi08a_read_fifo_data", rslt);

                        /* Parse the FIFO data to extract accelerometer data from the FIFO buffer */
                        rslt = bmi08a_extract_accel(bmi08x_accel, &accel_length, &fifo_frame, &bmi08xdev);
                        bmi08x_error_codes_print_result("bmi08a_extract_accel", rslt);

                        printf("Parsed accelerometer frames: %d\n", accel_length);

                        /* Print the parsed accelerometer data from the FIFO buffer */
                        for (idx = 0; idx < accel_length; idx++)
                        {
                            printf("ACCEL[%d] X : %d\t Y : %d\t Z : %d\n",
                                   idx,
                                   bmi08x_accel[idx].x,
                                   bmi08x_accel[idx].y,
                                   bmi08x_accel[idx].z);
                        }

                        rslt = bmi08a_get_sensor_time(&bmi08xdev, &sensor_time);
                        bmi08x_error_codes_print_result("bmi08a_get_sensor_time", rslt);

                        printf("Sensor time : %.4lf   s\n", (sensor_time * BMI08X_SENSORTIME_RESOLUTION));
                    }

                    try++;
                }
            }
        }

        /* Disable data ready interrupts */
        rslt = disable_bmi08x_interrupt();
        bmi08x_error_codes_print_result("disable_bmi08x_interrupt", rslt);
    }

    bmi08x_coines_deinit();

    return rslt;
}

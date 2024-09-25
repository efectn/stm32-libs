#include "stm32l4xx_hal.h"
#include "bmp180.h"
#include "math.h"

// BMP180_Init initializes BMP180 at the given I2C handle and sets the mode to standard.
HAL_StatusTypeDef BMP180_Init(BMP180 *bmp180, I2C_HandleTypeDef *hi2c)
{
    if (bmp180 == NULL || hi2c == NULL)
    {
        return HAL_ERROR;
    }

    uint8_t chip_id = 0;
    if (HAL_I2C_Mem_Read(hi2c, BMP180_ADDRESS, BMP180_REG_Chip_ID, 1, &chip_id, 1, 1000) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // verify chip id
    if (chip_id != 0x55)
    {
        return HAL_ERROR;
    }

    bmp180->hi2c = hi2c;
    bmp180->mode = BMP180_MODE_STANDARD;
    bmp180->calibration = (BMP180_Calibration){0};
    bmp180->sea_level_pressure = 101325.0;

    return HAL_OK;
}

// BMP180_SetMode sets the mode of BMP180.
HAL_StatusTypeDef BMP180_SetMode(BMP180 *bmp180, BMP180_Mode mode)
{
    bmp180->mode = mode;

    return HAL_OK;
}

// BMP180_GetCalibration reads the calibration data from BMP180.
HAL_StatusTypeDef BMP180_GetCalibration(BMP180 *bmp180)
{
    uint8_t data[22];
    if (HAL_I2C_Mem_Read(bmp180->hi2c, BMP180_ADDRESS, BMP180_REG_Calibration_Start, 1, data, 22, 1000) != HAL_OK)
    {
        return HAL_ERROR;
    }

    bmp180->calibration.AC1 = (data[0] << 8) | data[1];
    bmp180->calibration.AC2 = (data[2] << 8) | data[3];
    bmp180->calibration.AC3 = (data[4] << 8) | data[5];
    bmp180->calibration.AC4 = (data[6] << 8) | data[7];
    bmp180->calibration.AC5 = (data[8] << 8) | data[9];
    bmp180->calibration.AC6 = (data[10] << 8) | data[11];
    bmp180->calibration.B1 = (data[12] << 8) | data[13];
    bmp180->calibration.B2 = (data[14] << 8) | data[15];
    bmp180->calibration.MB = (data[16] << 8) | data[17];
    bmp180->calibration.MC = (data[18] << 8) | data[19];
    bmp180->calibration.MD = (data[20] << 8) | data[21];

    return HAL_OK;
}

// BMP180_GetUncompensatedTemperature reads the raw temperature from BMP180.
HAL_StatusTypeDef BMP180_GetUncompensatedTemperature(BMP180 *bmp180, int32_t *temperature)
{
    uint8_t write = BMP180_REG_CTRL_TEMPERATURE;
    if (HAL_I2C_Mem_Write(bmp180->hi2c, BMP180_ADDRESS, BMP180_REG_CTRL, 1, &write, 1, 1000) != HAL_OK)
    {
        return HAL_ERROR;
    }
    HAL_Delay(5); // wait for conversion which takes 4.5ms according to the datasheet

    uint8_t data[2] = {0};
    if (HAL_I2C_Mem_Read(bmp180->hi2c, BMP180_ADDRESS, BMP180_REG_READ, 1, data, 2, 1000) != HAL_OK)
    {
        return HAL_ERROR;
    }

    *temperature = (data[0] << 8) + data[1];

    return HAL_OK;
}

// BMP180_GetUncompensatedPressure reads the raw pressure from BMP180.
HAL_StatusTypeDef BMP180_GetUncompensatedPressure(BMP180 *bmp180, int32_t *pressure)
{
    uint8_t write = BMP180_REG_CTRL_PRESSURE + (bmp180->mode << 6);
    if (HAL_I2C_Mem_Write(bmp180->hi2c, BMP180_ADDRESS, BMP180_REG_CTRL, 1, &write, 1, 1000) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Waiting is dependent on the mode
    switch (bmp180->mode)
    {
    case BMP180_MODE_ULTRALOWPOWER:
        HAL_Delay(5); // wait for conversion which takes 4.5ms according to the datasheet
        break;
    case BMP180_MODE_STANDARD:
        HAL_Delay(8); // wait for conversion which takes 7.5ms according to the datasheet
        break;
    case BMP180_MODE_HIGHRES:
        HAL_Delay(14); // wait for conversion which takes 13.5ms according to the datasheet
        break;
    case BMP180_MODE_ULTRAHIGHRES:
        HAL_Delay(26); // wait for conversion which takes 25.5ms according to the datasheet
        break;
    default:
        return HAL_ERROR;
    }

    uint8_t data[3] = {0};
    if (HAL_I2C_Mem_Read(bmp180->hi2c, BMP180_ADDRESS, BMP180_REG_READ, 1, data, 3, 1000) != HAL_OK)
    {
        return HAL_ERROR;
    }

    *pressure = ((data[0] << 16) + (data[1] << 8) + data[2]) >> (8 - bmp180->mode);

    return HAL_OK;
}

// BMP180_GetTemperature calculates the temperature from the raw temperature.
HAL_StatusTypeDef BMP180_GetTemperature(BMP180 *bmp180, double *temperature)
{
    int32_t ut = 0;
    int32_t x1 = 0;
    int32_t x2 = 0;
    int32_t b5 = 0;

    if (BMP180_GetUncompensatedTemperature(bmp180, &ut) != HAL_OK)
    {
        return HAL_ERROR;
    }

    x1 = (ut - bmp180->calibration.AC6) * bmp180->calibration.AC5 / pow(2, 15);
    x2 = (bmp180->calibration.MC * pow(2, 11)) / (x1 + bmp180->calibration.MD);
    b5 = x1 + x2;

    *temperature = (int32_t)((b5 + 8) / pow(2, 4)) / 10.0; // It's needed to divide temp by 10 as BMP180 calculates temperature in 0.1C scale

    return HAL_OK;
}

// BMP180_GetPressure calculates the pressure from the raw temperature and raw pressure.
HAL_StatusTypeDef BMP180_GetPressure(BMP180 *bmp180, double *pressure)
{
    int32_t up = 0;
    int32_t x1 = 0;
    int32_t x2 = 0;
    int32_t x3 = 0;
    int32_t b3 = 0;
    uint32_t b4 = 0;
    int32_t b6 = 0;
    uint32_t b7 = 0;

    // Retrieve the temperature data first
    {
        int32_t ut = 0;
        int32_t x1 = 0;
        int32_t x2 = 0;
        int32_t b5 = 0;

        if (BMP180_GetUncompensatedTemperature(bmp180, &ut) != HAL_OK)
        {
            return HAL_ERROR;
        }

        x1 = (ut - bmp180->calibration.AC6) * bmp180->calibration.AC5 / pow(2, 15);
        x2 = (bmp180->calibration.MC * pow(2, 11)) / (x1 + bmp180->calibration.MD);
        b5 = x1 + x2;
        b6 = b5 - 4000;
    }

    if (BMP180_GetUncompensatedPressure(bmp180, &up) != HAL_OK)
    {
        return HAL_ERROR;
    }

    x1 = (bmp180->calibration.B2 * (b6 * b6 / pow(2, 12))) / pow(2, 11);
    x2 = bmp180->calibration.AC2 * b6 / pow(2, 11);
    x3 = x1 + x2;
    b3 = (((bmp180->calibration.AC1 * 4 + x3) << bmp180->mode) + 2) / 4;
    x1 = bmp180->calibration.AC3 * b6 / pow(2, 13);
    x2 = (bmp180->calibration.B1 * (b6 * b6 / pow(2, 12))) / pow(2, 16);
    x3 = ((x1 + x2) + 2) / pow(2, 2);
    b4 = bmp180->calibration.AC4 * (uint32_t)(x3 + 32768) / pow(2, 15);
    b7 = ((uint32_t)up - b3) * (50000 >> bmp180->mode);

    if (b7 < 0x80000000)
    {
        *pressure = (b7 * 2) / b4;
    }
    else
    {
        *pressure = (b7 / b4) * 2;
    }

    x1 = (*pressure / pow(2, 8)) * (*pressure / pow(2, 8));
    x1 = (x1 * 3038) / pow(2, 16);
    x2 = (-7357 * *pressure) / pow(2, 16);

    *pressure = *pressure + (x1 + x2 + 3791) / pow(2, 4);

    return HAL_OK;
}

// BMP180_GetAltitude calculates the altitude from the pressure.
HAL_StatusTypeDef BMP180_GetAltitude(BMP180 *bmp180, double *altitude)
{
    double pressure = 0;
    if (BMP180_GetPressure(bmp180, &pressure) != HAL_OK)
    {
        return HAL_ERROR;
    }

    *altitude = 44330.0 * (1.0 - pow(pressure / bmp180->sea_level_pressure, 1 / 5.255));

    return HAL_OK;
}

// BMP180_SetSeaLevelPressure sets the sea level pressure for altitude calculation.
// It is 101325 Pa by default.
HAL_StatusTypeDef BMP180_SetSeaLevelPressure(BMP180 *bmp180, double sea_level_pressure)
{
    bmp180->sea_level_pressure = sea_level_pressure;

    return HAL_OK;
}

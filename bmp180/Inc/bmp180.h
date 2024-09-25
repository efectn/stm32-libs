#ifndef BMP180_H

#define BMP180_H

#include "stm32l4xx_hal.h"

#define BMP180_ADDRESS 0x77 << 1 // 0xEE

#define BMP180_REG_Chip_ID 0xD0
#define BMP180_REG_Calibration_Start 0xAA
#define BMP180_REG_Calibration_End 0xBF
#define BMP180_REG_CTRL 0xF4
#define BMP180_REG_CTRL_TEMPERATURE 0x2E
#define BMP180_REG_CTRL_PRESSURE 0x34
#define BMP180_REG_READ 0xF6 // 0xF6 - 0xF7 - 0xF8 (MSB - LSB - XLSB)

// BMP180_Mode is the mode of BMP180.
typedef enum
{
    BMP180_MODE_ULTRALOWPOWER = 0, // conversion time 4.5ms
    BMP180_MODE_STANDARD = 1,      // conversion time 7.5ms
    BMP180_MODE_HIGHRES = 2,       // conversion time 13.5ms
    BMP180_MODE_ULTRAHIGHRES = 3   // conversion time 25.5ms
} BMP180_Mode;

// BMP180_Calibration is the calibration data of BMP180.
typedef struct
{
    int16_t AC1;
    int16_t AC2;
    int16_t AC3;
    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;
    int16_t B1;
    int16_t B2;
    int16_t MB;
    int16_t MC;
    int16_t MD;
} BMP180_Calibration;

// BMP180 is the BMP180 sensor.
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    BMP180_Mode mode;
    BMP180_Calibration calibration;
    double sea_level_pressure;
} BMP180;

// BMP180_Init initializes BMP180 at the given I2C handle and sets the mode to standard.
HAL_StatusTypeDef BMP180_Init(BMP180 *bmp180, I2C_HandleTypeDef *hi2c);

// BMP180_SetMode sets the mode of BMP180.
HAL_StatusTypeDef BMP180_SetMode(BMP180 *bmp180, BMP180_Mode mode);

// BMP180_GetCalibration reads the calibration data from BMP180.
HAL_StatusTypeDef BMP180_GetCalibration(BMP180 *bmp180);

// BMP180_GetUncompensatedTemperature reads the raw temperature from BMP180.
HAL_StatusTypeDef BMP180_GetUncompensatedTemperature(BMP180 *bmp180, int32_t *temperature);

// BMP180_GetUncompensatedPressure reads the raw pressure from BMP180.
HAL_StatusTypeDef BMP180_GetUncompensatedPressure(BMP180 *bmp180, int32_t *pressure);

// BMP180_GetTemperature calculates the temperature from the raw temperature.
HAL_StatusTypeDef BMP180_GetTemperature(BMP180 *bmp180, double *temperature);

// BMP180_GetPressure calculates the pressure from the raw temperature and raw pressure.
HAL_StatusTypeDef BMP180_GetPressure(BMP180 *bmp180, double *pressure);

// BMP180_GetAltitude calculates the altitude using the pressure.
HAL_StatusTypeDef BMP180_GetAltitude(BMP180 *bmp180, double *altitude);

// BMP180_SetSeaLevelPressure sets the sea level pressure for altitude calculation.
// It is 101325 Pa by default.
HAL_StatusTypeDef BMP180_SetSeaLevelPressure(BMP180 *bmp180, double sea_level_pressure);

#endif // BMP180_H
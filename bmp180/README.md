# BMP180

BMP180 is a digital pressure sensor which is developed by Bosch. It is a low-cost sensor with a simple I2C interface. This library provides functions to read temperature and pressure from the sensor. 

I have created this library according to the [BMP180 datasheet](https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf). You can check the datasheet for further information.

## How to Use the Library

- Copy the BMP180 folder to the Drivers folder in your project.

- Add library executable path to CmakeLists.txt:

```cmake
# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Drivers/bmp180/Src/bmp180.c
)
```

- Add library include path to CmakeLists.txt:

```cmake
# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Drivers/bmp180/Inc
)
```

- Then, you can import it from your source code:

```c
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  
  char msg[100];
  
  BMP180 bmp180 = {0};
  int status = BMP180_Init(&bmp180, &hi2c1);
  if (status == HAL_ERROR)
  {
    sprintf(msg, "Error initializing BMP180\n");
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
  }
  BMP180_GetCalibration(&bmp180); // You have to call this function after initialization. Otherwise, the temperature and pressure readings will be incorrect.

  while (1)
  {
    double temperature = 0;
    BMP180_GetTemperature(&bmp180, &temperature);

    memset(msg, 0, 100);
    sprintf(msg, "Temperature: %f\n", temperature);
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);

    double pressure = 0;
    BMP180_GetPressure(&bmp180, &pressure);

    memset(msg, 0, 100);
    sprintf(msg, "Pressure: %f\n", pressure);
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);

    double altitude = 0;
    BMP180_GetAltitude(&bmp180, &altitude);

    memset(msg, 0, 100);
    sprintf(msg, "Altitude: %f\n", altitude);
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);

    HAL_Delay(1000);
  }
}
```

**Note:** You need to initialize I2C before using the BMP180 library. This library doesn't initialize the I2C peripheral. You can use STM32CubeMX to generate the initialization code for I2C.

**Note:** You have to call `BMP180_GetCalibration` function after initialization. Otherwise, the temperature and pressure readings will be incorrect.

**Note:** I have included `#include "stm32l4xx_hal.h"` since i have a STM32L4 series MCU. You need change it according to the MCU series you have.

## Functions

### BMP180_Init

BMP180_Init initializes BMP180 at the given I2C handle and sets the mode to standard.

It returns `HAL_ERROR` if there is an error during BMP180 initialization.

```c
HAL_StatusTypeDef BMP180_Init(BMP180 *bmp180, I2C_HandleTypeDef *hi2c)
```

### BMP180_SetMode

BMP180_SetMode sets the mode of BMP180. Available mods are:
- BMP180_MODE_ULTRALOWPOWER
- BMP180_MODE_STANDARD **(default mode)**
- BMP180_MODE_HIGHRES
- BMP180_MODE_ULTRAHIGHRES

```c
HAL_StatusTypeDef BMP180_SetMode(BMP180 *bmp180, BMP180_Mode mode)
```

### BMP180_GetCalibration

BMP180_GetCalibration reads the calibration data from BMP180.

```c
HAL_StatusTypeDef BMP180_GetCalibration(BMP180 *bmp180);
```

### BMP180_GetUncompensatedTemperature

BMP180_GetUncompensatedTemperature reads the raw temperature from BMP180.

```c
HAL_StatusTypeDef BMP180_GetUncompensatedTemperature(BMP180 *bmp180, int32_t *temperature)
```

### BMP180_GetUncompensatedPressure

BMP180_GetUncompensatedPressure reads the raw pressure from BMP180.

```c
HAL_StatusTypeDef BMP180_GetUncompensatedPressure(BMP180 *bmp180, int32_t *pressure)
```

### BMP180_GetTemperature

BMP180_GetTemperature calculates the temperature from the raw temperature.

```c
HAL_StatusTypeDef BMP180_GetTemperature(BMP180 *bmp180, double *temperature)
```

### BMP180_GetPressure

BMP180_GetPressure calculates the pressure from the raw temperature and raw pressure.

```c
HAL_StatusTypeDef BMP180_GetPressure(BMP180 *bmp180, double *pressure)
```

### BMP180_GetAltitude

BMP180_GetAltitude calculates the altitude using the pressure.

```c
HAL_StatusTypeDef BMP180_GetAltitude(BMP180 *bmp180, double *altitude)
```

### BMP180_SetSeaLevelPressure

BMP180_SetSeaLevelPressure sets the sea level pressure for altitude calculation.
It is 101325 Pa by default.

```c
HAL_StatusTypeDef BMP180_SetSeaLevelPressure(BMP180 *bmp180, double sea_level_pressure)
```
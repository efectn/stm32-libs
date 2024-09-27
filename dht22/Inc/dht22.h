#ifndef DHT22_H

#define DHT22_H

#include "stm32l4xx_hal.h"

#define SYSTICK_LOAD (HAL_RCC_GetSysClockFreq() / 1000000U)
#define SYSTICK_DELAY_CALIB (SYSTICK_LOAD >> 1)

typedef enum
{
    DHT22_TEMP_CELSIUS,
    DHT22_TEMP_FAHRENHEIT,
    DHT22_TEMP_KELVIN,
} DHT22_TEMP_UNIT;

typedef enum
{
    DHT22_DWT,
    DHT22_Timer,
    DHT22_Systick,
} DHT22_DelayMethod;

typedef struct
{
    GPIO_TypeDef *port;
    uint32_t pin;
    DHT22_DelayMethod delay_method;
    DHT22_TEMP_UNIT temp_unit;
    TIM_HandleTypeDef *timer;
} DHT22;

// DHT22_Init initializes the DHT22 for communication.
// It is recommended to use DWT or timer mode as a delay method.
// You need to call DHT22_Configure_Timer after this function if you want to use timer mode.
HAL_StatusTypeDef DHT22_Init(DHT22 *dht22, DHT22_DelayMethod delay_method, GPIO_TypeDef *port, uint32_t pin);

// DHT22_Configure_Timer configures the timer for DHT22 communication.
HAL_StatusTypeDef DHT22_Configure_Timer(DHT22 *dht22, TIM_HandleTypeDef *timer, uint32_t apb_freq);

// DHT22_SetTempUnit sets the temperature unit for the DHT22. Available units are:
// DHT22_TEMP_CELSIUS, DHT22_TEMP_FAHRENHEIT, DHT22_TEMP_KELVIN
HAL_StatusTypeDef DHT22_SetTempUnit(DHT22 *dht22, DHT22_TEMP_UNIT temp_unit);

// DHT22 starts DHT22 communication and reads the temperature and humidity.
// If temperature or humidity is NULL, the value is not read.
//
// Returns HAL_OK if the communication was successful, HAL_ERROR otherwise.
HAL_StatusTypeDef DHT22_Read(DHT22 *dht22, double *temperature, double *humidity);

#endif // DHT22_H
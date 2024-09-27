#include "dht22.h"
#include "stm32l4xx_hal.h"

static void _dht22_change_gpio_mode(GPIO_TypeDef *port, uint32_t pin, uint32_t mode)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = mode == GPIO_MODE_INPUT ? GPIO_PULLUP : GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void _dht22_delay_us(const DHT22 *dht22, volatile uint32_t us)
{
    switch (dht22->delay_method)
    {
    case DHT22_DWT:
        uint32_t au32_initial_ticks = DWT->CYCCNT;
        uint32_t au32_ticks = (HAL_RCC_GetHCLKFreq() / 1000000);
        us *= au32_ticks;

        while ((DWT->CYCCNT - au32_initial_ticks) < us - au32_ticks)
            ;

        return;
    case DHT22_Timer:
        __HAL_TIM_SET_COUNTER(dht22->timer, 0);
        while (__HAL_TIM_GET_COUNTER(dht22->timer) < us)
            ;

        return;
    case DHT22_Systick:
        uint32_t start = SysTick->VAL;
        uint32_t ticks = (us * SYSTICK_LOAD) - SYSTICK_DELAY_CALIB;
        while ((start - SysTick->VAL) < ticks)
            ;

        return;
    }
}

// TODO: Add timeout to prevent infinite loop in case watchdog is not enabled
static void _dht22_wait_for_pin_state(const DHT22 *dht22, GPIO_PinState state)
{
    while (HAL_GPIO_ReadPin(dht22->port, dht22->pin) == state)
        ;
}

// DHT22_Init initializes the DHT22 for communication.
// It is recommended to use DWT or timer mode as a delay method.
// You need to call DHT22_Configure_Timer after this function if you want to use timer mode.
HAL_StatusTypeDef DHT22_Init(DHT22 *dht22, DHT22_DelayMethod delay_method, GPIO_TypeDef *port, uint32_t pin)
{
    dht22->port = port;
    dht22->pin = pin;
    dht22->delay_method = delay_method;
    dht22->temp_unit = DHT22_TEMP_CELSIUS;

    // Start DWT timer if delay method is DWT
    if (delay_method == DHT22_DWT)
    {
        // Disable & enable TRC
        CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // 0x01000000;

        // Disable and enable clock cycle counter
        DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;  // 0x00000001;

        // Reset the clock cycle counter value
        DWT->CYCCNT = 0;

        // 3 NO OPERATION instructions
        __ASM volatile("NOP");
        __ASM volatile("NOP");
        __ASM volatile("NOP");
    }

    _dht22_change_gpio_mode(dht22->port, dht22->pin, GPIO_MODE_OUTPUT_PP);

    return HAL_OK;
}

// DHT22_Configure_Timer configures the timer for DHT22 communication.
HAL_StatusTypeDef DHT22_Configure_Timer(DHT22 *dht22, TIM_HandleTypeDef *timer, uint32_t apb_freq)
{
    timer->Init.Prescaler = (apb_freq / 1000000) - 1;
    timer->Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->Init.Period = 0xffff - 1;
    timer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(timer) != HAL_OK)
    {
        return HAL_ERROR;
    }

    dht22->timer = timer;
    HAL_TIM_Base_Start(dht22->timer);

    return HAL_OK;
}

// DHT22_SetTempUnit sets the temperature unit for the DHT22. Available units are:
// DHT22_TEMP_CELSIUS, DHT22_TEMP_FAHRENHEIT, DHT22_TEMP_KELVIN
HAL_StatusTypeDef DHT22_SetTempUnit(DHT22 *dht22, DHT22_TEMP_UNIT temp_unit)
{
    dht22->temp_unit = temp_unit;
    return HAL_OK;
}

// DHT22 starts DHT22 communication and reads the temperature and humidity.
// If temperature or humidity is NULL, the value is not read.
//
// Returns HAL_OK if the communication was successful, HAL_ERROR otherwise.
HAL_StatusTypeDef DHT22_Read(DHT22 *dht22, double *temperature, double *humidity)
{
    // Ensure the DHT22 is in a known state
    _dht22_change_gpio_mode(dht22->port, dht22->pin, GPIO_MODE_OUTPUT_PP);

    // Start the communication
    HAL_GPIO_WritePin(dht22->port, dht22->pin, GPIO_PIN_RESET);
    HAL_Delay(18);

    HAL_GPIO_WritePin(dht22->port, dht22->pin, GPIO_PIN_SET);
    _dht22_delay_us(dht22, 30);

    // Change GPIO to input, DHT22 is now ready to respond
    _dht22_change_gpio_mode(dht22->port, dht22->pin, GPIO_MODE_INPUT);

    // Wait for the DHT22 to pull the line low and then high
    _dht22_wait_for_pin_state(dht22, GPIO_PIN_RESET);
    _dht22_wait_for_pin_state(dht22, GPIO_PIN_SET);

    uint8_t data[5] = {0}; // 0: humidity integral, 1: humidity decimal, 2: temperature integral, 3: temperature decimal, 4: checksum

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            // Wait for the DHT22 to pull the line hight
            _dht22_wait_for_pin_state(dht22, GPIO_PIN_RESET);

            _dht22_delay_us(dht22, 40);

            if (HAL_GPIO_ReadPin(dht22->port, dht22->pin) == GPIO_PIN_SET)
            {
                data[i] |= (1 << (7 - j)); // MSB is first
            }

            // Wait for the DHT22 to pull the line low
            _dht22_wait_for_pin_state(dht22, GPIO_PIN_SET);
        }
    }

    // Checksum check
    if (((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4])
    {
        return HAL_ERROR;
    }

    if (temperature != NULL)
    {
        switch (dht22->temp_unit)
        {
        case DHT22_TEMP_CELSIUS:
            *temperature = ((data[2] << 8) | data[3]) / 10.0;
            break;
        case DHT22_TEMP_FAHRENHEIT:
            *temperature = (((data[2] << 8) | data[3])) / 10.0 * 1.8 + 32;
            break;
        case DHT22_TEMP_KELVIN:
            *temperature = ((data[2] << 8) | data[3]) / 10.0 + 273.15;
            break;
        }
    }

    if (humidity != NULL)
    {
        *humidity = ((data[0] << 8) | data[1]) / 10.0;
    }

    return HAL_OK;
}
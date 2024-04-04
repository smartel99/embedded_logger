# embedded_logger
C++23 logging module for embedded systems

This library currently requires FreeRTOS and one of the HAL libraries for STM32 from STMicro to work.

## Example
```c++
#include "cmsis_os.h"
#include "gpio.h"
#include "main.h"
#include "usart.h"

#include "logger.h"
#include "proxy_sink.h"
#include "uart_sink.h"

int main()
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    Logging::Logger::setGetTime(&HAL_GetTick);
    auto* uartSink = Logging::Logger::addSink<Logging::MtUartSink>(&huart1);
    Logging::Logger::addSink<Logging::ProxySink>(g_usbTag, uartSink);

    ROOT_LOGI("Logger Initialized.");

    /* Init scheduler */
    osKernelInitialize();

    /* Call init function for freertos objects (in freertos.c) */
    MX_FREERTOS_Init();

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    /* Infinite loop */
    volatile bool t = true;
    while (t) {}
}
```

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define BUFF_SIZE 5
#define SAMPLE_RATE 1
#define PERIOD_RATE 1000/SAMPLE_RATE 
#define CONVERSION_FACTOR 3.3f / (1 << 12)

QueueHandle_t g_AdcTempQueue;

/**
 * @brief Convierte el valor del ADC a temperatura segun la formula del fabricante
 * 
 * @param usAdcValue 
 * @return float 
 */
float fGetCoreTemp(uint16_t usAdcValue){
    return  27 - (( CONVERSION_FACTOR * usAdcValue) - 0.706)/0.001721;
}

/**
 * @brief Tarea que se encarga de imprimir en pantalla la temperatura del core
 * 
 * @param params 
 */
void vShowTempTask(void *params){
    uint16_t usAdcValueReceive;
    for(;;){
        if(xQueueReceive(g_AdcTempQueue, &usAdcValueReceive, portMAX_DELAY)){
            printf("Temp: %.2f °C\n", fGetCoreTemp(usAdcValueReceive));
            vTaskDelay(10);
        }
    }
}

/**
 * @brief Tarea que se encarga de leer el valor del ADC
 * 
 * @param params 
 */
void vReadAdcTask(void *params){
    assert(PERIOD_RATE >= 1);
    uint16_t usAdcValue = 0;
    for(;;){
        usAdcValue = adc_read();
        xQueueSend(g_AdcTempQueue, &usAdcValue, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(PERIOD_RATE));
    }
}
/**
 * @brief Configuro el ADC
 * 
 */
void adc_config(){
    adc_init();                                     // Inicio el periferico
    adc_set_temp_sensor_enabled(true);
    adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM);
}

int main()
{
    stdio_init_all();

    adc_config();

    g_AdcTempQueue = xQueueCreate(BUFF_SIZE, sizeof(uint16_t));

    xTaskCreate(
        vReadAdcTask,
        "ReadAdcTask",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    xTaskCreate(
        vShowTempTask,
        "ShowTempTask",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    vTaskStartScheduler();
    while (true) {
    }
}

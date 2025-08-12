#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define BUFF_SIZE 5
#define SAMPLE_RATE 1
#define PERIOD_RATE 1000/SAMPLE_RATE
#define ADC_CLK_BASE 48000000.f
#define CONVERSION_FACTOR 3.3f / (1 << 12)

#define GET_FIFO

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
//  * @param params 
 */
void vShowTempTask(void *params){
    adc_run(true);
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
    assert(PERIOD_RATE>=1);
    uint16_t usAdcValue = 0;
    for(;;){
        adc_irq_set_enabled(true);
        adc_run(true);
        vTaskDelay(pdMS_TO_TICKS(PERIOD_RATE));
    }
}

/**
 * @brief Interrupcion que inicia la conversion del adc
 * 
 */
void IRQ_ReadAdcFifo(){
    adc_irq_set_enabled(false);
    adc_run(false);
    uint16_t usAdcValue = adc_fifo_get();
    adc_fifo_drain();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_AdcTempQueue, &usAdcValue, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                                          // Cambio el contexto del Scheduler para que ejecute la tarea de procesamiento
}

/**
 * @brief Configuro el ADC
 * 
 */
void adc_config(){
    adc_init();                                     // Inicio el periferico
    adc_set_temp_sensor_enabled(true);
    adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM);

    adc_fifo_setup(
        true,      // Habilito el fifo
        true,      // Cada muestra pushea al FIFO
        1,        // Genera solicitud DMA o IRQ al tener al menos 1 muestra
        false,     // Desactivo el bit de error
        false      // El registro va a contener un dato de mas de un byte, sera de 16bit aunque el adc es de 12bit
    );    

    adc_set_clkdiv(ADC_CLK_BASE/(float)SAMPLE_RATE);        // Seteo el sample rate del adc
    
    irq_set_exclusive_handler(ADC_IRQ_FIFO, IRQ_ReadAdcFifo);
    
    adc_irq_set_enabled(true);
    irq_set_enabled(ADC_IRQ_FIFO, true);
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

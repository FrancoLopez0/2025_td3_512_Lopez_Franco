#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "lcd.h"
#include "helper.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#define SDA_PIN 4U
#define SCL_PIN 5U
#define LCD_DIR 0x27 
#define INPUT_PIN 7U
#define AUX_PIN 8U

SemaphoreHandle_t InputSemaphoreHandler;

void vPrintTask(void *params){
    char frec_str[25];
    lcd_set_cursor(0,0);
    lcd_string("Frecuencia:");
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(200));
        lcd_set_cursor(1,0);
        uint32_t periodos = uxSemaphoreGetCount(InputSemaphoreHandler);
        sprintf(frec_str, "%5.0f hz", (float)periodos / 200e-3);
        lcd_string(frec_str);
        xQueueReset(InputSemaphoreHandler);
    }
}

void IRQ_Input(uint gpio, uint32_t events){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(InputSemaphoreHandler, &xHigherPriorityTaskWoken);
}

void gpio_config(void){
    gpio_init(INPUT_PIN);
    gpio_set_input_enabled(INPUT_PIN, true);
    gpio_set_irq_enabled_with_callback(INPUT_PIN, GPIO_IRQ_EDGE_RISE, true, &IRQ_Input);
}

void i2c_config(void){
    i2c_init(i2c0, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    bi_decl(bi_2pins_with_func(SDA_PIN,SCL_PIN, GPIO_FUNC_I2C));
}

int main()
{
    stdio_init_all();

    gpio_config();

    i2c_config();

    pwm_user_init(AUX_PIN, 9881);

    lcd_init(i2c0, LCD_DIR);

    InputSemaphoreHandler = xSemaphoreCreateCounting(5000, 0);

    xTaskCreate(
        vPrintTask,
        "PollingDownTask",
        configMINIMAL_STACK_SIZE*2,
        NULL,
        2,
        NULL
    );

    vTaskStartScheduler();

    while (true) {
    }
}

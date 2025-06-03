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
#define LCD_DIR 0x3F 
#define INPUT_PIN 7U
#define AUX_PIN 8U


SemaphoreHandle_t InputSemaphoreHandler;

void vPollingUpTask(void *params){
    
    uint32_t prev_time = 0;
    uint32_t period = 0;
    float frec = 0;

    for(;;){

        if(gpio_get(INPUT_PIN)){
            xSemaphoreGive(InputSemaphoreHandler);
            while(gpio_get(INPUT_PIN));
        }
    }
}

void vPrintTask(void *params){
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(200));
        uint32_t periodos = uxSemaphoreGetCount(InputSemaphoreHandler) ;
        printf("Frec: %.2f hz\n", (float)periodos / 200e-3);
        xQueueReset(InputSemaphoreHandler);
        // xSemaphoreTake(InputSemaphoreHandler, portMAX_DELAY);
    }
}


void gpio_config(void){
    gpio_init(INPUT_PIN);
    gpio_set_input_enabled(INPUT_PIN, true);
}

void i2c_config(void){
    i2c_init(i2c_default, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    bi_decl(bi_2pins_with_func(SDA_PIN,SCL_PIN, GPIO_FUNC_I2C));

    lcd_init(i2c0, LCD_DIR);
}

int main()
{
    stdio_init_all();

    gpio_config();

    i2c_config();

    pwm_user_init(AUX_PIN, 1000);

    InputSemaphoreHandler = xSemaphoreCreateCounting(1000, 0);

    xTaskCreate(
        vPollingUpTask,
        "PollingUpTask",
        configMINIMAL_STACK_SIZE,
        NULL,
        1,
        NULL
    );

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

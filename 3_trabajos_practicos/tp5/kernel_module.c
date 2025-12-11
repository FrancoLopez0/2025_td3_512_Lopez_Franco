#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "gpio_driver.h"
#define LED_PIN 16

static uint8_t gpio_led = LED_PIN;

// Etiqueta para el autor del modulo
#define AUTHOR	"Franco López"

static struct task_struct *thread1;
static struct task_struct *thread2;


static int thread_led_on(void *params) {

	int led_pin = *(uint8_t *)params;

	while(!kthread_should_stop()) {
		// Mensaje para el Kernel
		printk(KERN_INFO "%s: Led on!\n", AUTHOR);
		gpio_set(led_pin);
		msleep(1000);
	}
	return 0;
}

static int thread_led_off(void *params) {

	int led_pin = *(uint8_t *)params;

	msleep(500);
	while(!kthread_should_stop()) {
		// Mensaje para el Kernel
		printk(KERN_INFO "%s: Led off!\n", AUTHOR);
		gpio_clr(led_pin);
		msleep(1000);
	}
	return 0;
}


/**
 * @brief Se llama cuando el modulo se carga en el kernel
*/
static int __init kernel_module_init(void) {
	printk(KERN_INFO "%s: Modulo cargado\n", AUTHOR);

	void __iomem* map_addr = gpio_map();
	if (!map_addr) {
		printk(KERN_ERR "%s: Error en mapeo de memoria\n", AUTHOR);
		return -1;
	}
	gpio_set_dir_output(gpio_led);

	thread1 = kthread_run(thread_led_on, &gpio_led, "thread1");
	if (IS_ERR(thread1)) {
		printk(KERN_ERR "%s: Error al crear thread hola\n", AUTHOR);
		return -1;
	}

	thread2 = kthread_run(thread_led_off, &gpio_led, "thread2");
	if (IS_ERR(thread2)) {
		printk(KERN_ERR "%s: Error al crear thread chau\n", AUTHOR);
		// Frenar el hilo 1
		kthread_stop(thread1);
		return -1;
	}

	return 0;
}

/**
 * @brief Se llama cuando el modulo se quita del kernel
 */
static void __exit kernel_module_exit(void) {
	printk(KERN_INFO "%s: Limpiando los recursos!\n", AUTHOR);
	gpio_clr(gpio_led);
	gpio_unmap();
	if (thread1) {
		kthread_stop(thread1);
	}

	if (thread2) {
		kthread_stop(thread2);
	}
}

// Registro la funcion de inicializacion y salida
module_init(kernel_module_init);
module_exit(kernel_module_exit);

// Informacion del modulo
MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("UTN FRA Tecnicas Digitales III - TP5: GPOS");

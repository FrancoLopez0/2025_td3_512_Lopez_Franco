#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#define LED_PIN 2

// Etiqueta para el autor del modulo
#define AUTHOR	"Franco López"

static struct task_struct *thread1;
static struct task_struct *thread2;


static int thread_hola(void *params) {

	while(!kthread_should_stop()) {
		// Mensaje para el Kernel
		printk(KERN_INFO "%s: Hola desde el kernel!\n", AUTHOR);
		msleep(1000);
	}
	return 0;
}

static int thread_chau(void *params) {

	msleep(500);
	while(!kthread_should_stop()) {
		// Mensaje para el Kernel
		printk(KERN_INFO "%s: Chau desde el kernel!\n", AUTHOR);
		msleep(1000);
	}
	return 0;
}


/**
 * @brief Se llama cuando el modulo se carga en el kernel
*/
static int __init kernel_module_init(void) {
	printk(KERN_INFO "%s: Modulo cargado\n", AUTHOR);

	thread1 = kthread_run(thread_hola, NULL, "thread1");
	if (IS_ERR(thread1)) {
		printk(KERN_ERR "%s: Error al crear thread hola\n", AUTHOR);
		return -1;
	}

	thread2 = kthread_run(thread_chau, NULL, "thread2");
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

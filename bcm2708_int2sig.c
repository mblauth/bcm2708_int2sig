// GPIO interrupt to signal dispatcher for the Raspberry Pi

#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/list.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <mach/platform.h>
#include <linux/time.h>
#include <linux/delay.h>

#include "RPI.h"

// text below will show up in /proc/interrupt
#define GPIO_ANY_GPIO_DESC "GPIO Interrupt to POSIX signal"

#define FIRST_GPIO 2
#define SECOND_GPIO 3
#define THIRD_GPIO 8

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marvin Blauth");
MODULE_DESCRIPTION("A module translating GPIO Interrupts to POSIX signals");

int first_handling_task_pid = -1;
int second_handling_task_pid = -1;
int third_handling_task_pid = -1;

module_param(first_handling_task_pid, int, S_IRUGO | S_IWUSR);
module_param(second_handling_task_pid, int, S_IRUGO | S_IWUSR);
module_param(third_handling_task_pid, int, S_IRUGO | S_IWUSR);


unsigned int last_interrupt_time = 0;
static uint64_t epochMilli;

short int irq_gpio0 = 0;
short int irq_gpio1 = 0;
short int irq_gpio2 = 0;

unsigned int millis (void) {
    struct timeval tv ;
    uint64_t now ;

    do_gettimeofday(&tv) ;
    now  = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ;

    return (uint32_t)(now - epochMilli) ;
}

__always_inline struct task_struct* task_for_pid(long pid) {
    return pid_task(find_vpid(pid), PIDTYPE_PID);
}

__always_inline irqreturn_t gpio_int_handler(int irq, void *dev_id, struct pt_regs *regs, long recipient_pid) {
    struct task_struct *task;
    int status;

    if(recipient_pid == -1) {
        //printk("interrupt handling task not set\n");
        return IRQ_HANDLED;
    }

    task = task_for_pid(recipient_pid);
    if(!task) {
        return IRQ_HANDLED;
    }

    status = send_sig(22, task, 0); // send SIGPOLL
    if (0 > status) {
        printk("error sending signal\n");
        return IRQ_HANDLED;
    }

    return IRQ_HANDLED;
}

__always_inline irqreturn_t debounced_gpio_int_handler(int irq, void *dev_id, struct pt_regs *regs, long recipient_pid) {
    unsigned int interrupt_time = millis();
  
    if (interrupt_time - last_interrupt_time < 1000)
        return IRQ_HANDLED;
    last_interrupt_time = interrupt_time;

    return gpio_int_handler(irq, dev_id, regs, recipient_pid);
}

static irqreturn_t irq_handler_first_gpio(int irq, void *dev_id, struct pt_regs *regs) {
    return debounced_gpio_int_handler(irq, dev_id, regs, first_handling_task_pid);
}

static irqreturn_t irq_handler_second_gpio(int irq, void *dev_id, struct pt_regs *regs) {
    return debounced_gpio_int_handler(irq, dev_id, regs, second_handling_task_pid);
}

static irqreturn_t irq_handler_third_gpio(int irq, void *dev_id, struct pt_regs *regs) {
    return gpio_int_handler(irq, dev_id, regs, third_handling_task_pid);
}

void config_int(int gpio, short int *irq_gpio, irq_handler_t handler) {
    if (gpio_request(gpio, GPIO_ANY_GPIO_DESC)) {
        printk("GPIO request failure: %s\n", GPIO_ANY_GPIO_DESC);
        return;
    }

    if ((*irq_gpio = gpio_to_irq(gpio)) < 0 ) {
        printk("GPIO to IRQ mapping failure %s\n", GPIO_ANY_GPIO_DESC);
        return;
    }

    printk(KERN_NOTICE "Mapped int %d\n", *irq_gpio);

    if (request_irq(*irq_gpio,
                   handler,
                   IRQF_TRIGGER_RISING,
                   GPIO_ANY_GPIO_DESC,
                   NULL)) {
        printk("Irq Request failure\n");
        return;
    }
}

void r_int_config(void) {

    struct timeval tv ;

    do_gettimeofday(&tv) ;
    epochMilli = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ;

    config_int(FIRST_GPIO, &irq_gpio0, (irq_handler_t)irq_handler_first_gpio);
    config_int(SECOND_GPIO, &irq_gpio1, (irq_handler_t)irq_handler_second_gpio);
    config_int(THIRD_GPIO, &irq_gpio2, (irq_handler_t)irq_handler_third_gpio);

    return;
}

void r_int_release(void) {
    //free_irq(irq_gpio0, NULL);
    //free_irq(irq_gpio1, NULL);
    free_irq(irq_gpio2, NULL);
    //gpio_free(FIRST_GPIO);
    //gpio_free(SECOND_GPIO);
    gpio_free(THIRD_GPIO);
}

int init_module(void) {

    printk("loading bcm2708_int2sig\n");


    gpio.map     = ioremap(GPIO_BASE, 4096);
    gpio.addr    = (volatile unsigned int *)gpio.map;

    //INP_GPIO(FIRST_GPIO);
    //INP_GPIO(SECOND_GPIO);
    INP_GPIO(THIRD_GPIO);
    PULL_UP(THIRD_GPIO);
    PULL_DOWN(17);

    r_int_config();

    return 0;
}

void cleanup_module(void) {
    printk("unloading bcm2708_int2sig\n");

    r_int_release();
    if (gpio.addr) {
        iounmap(gpio.addr);
    }
}














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
#include <mach/gpio.h>
#include <linux/time.h>

#include "RPI.h"

// text below will be seen in 'cat /proc/interrupt' command
#define GPIO_ANY_GPIO_DESC "GPIO Interrupt to POSIX signal"

#define GPIO_ANY_GPIO 3
#define FIRST_GPIO 2
#define SECOND_GPIO 3

volatile unsigned int val;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marvin Blauth");
MODULE_DESCRIPTION("A module translating GPIO Interrupts to POSIX signals");

static char *msg=NULL;

unsigned long flags;

int first_handling_task_pid = -1;
int second_handling_task_pid = -1;
module_param(first_handling_task_pid, int, S_IRUGO | S_IWUSR);
module_param(second_handling_task_pid, int, S_IRUGO | S_IWUSR);


unsigned int last_interrupt_time = 0;
static uint64_t epochMilli;

short int irq_gpio0 = 0;
short int irq_gpio1 = 0;

unsigned int millis (void)
{
    struct timeval tv ;
    uint64_t now ;

    do_gettimeofday(&tv) ;
    now  = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ;

    return (uint32_t)(now - epochMilli) ;
}

/****************************************************************************/
/* IRQ handler - fired on interrupt                                         */
/***************************************************************************/

__always_inline irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs, long recipient_pid) {
    unsigned int interrupt_time = millis();
    struct task_struct *task;
    int status;
  
    if (interrupt_time - last_interrupt_time < 1000)
        return IRQ_HANDLED;
    last_interrupt_time = interrupt_time;

    if(recipient_pid == -1) {
        printk("interrupt handling task not set\n");
        return IRQ_HANDLED;
    }

    task = pid_task(find_vpid(recipient_pid), PIDTYPE_PID);
    if(!task) {
        printk("error finding interrupt handling task\n");
        return IRQ_HANDLED;
    }

    status = send_sig(22, task, 0); // send SIGPOLL
    if (0 > status) {
        printk("error sending signal\n");
        return IRQ_HANDLED;
    }

    return IRQ_HANDLED;
}

static irqreturn_t irq_handler_first_gpio(int irq, void *dev_id, struct pt_regs *regs) {
    return r_irq_handler(irq, dev_id, regs, first_handling_task_pid);
}

static irqreturn_t irq_handler_second_gpio(int irq, void *dev_id, struct pt_regs *regs) {
    return r_irq_handler(irq, dev_id, regs, second_handling_task_pid);
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

/****************************************************************************/
/* This function configures interrupts.                                     */
/****************************************************************************/

void r_int_config(void) {

    struct timeval tv ;

    do_gettimeofday(&tv) ;
    epochMilli = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ;

    config_int(FIRST_GPIO, &irq_gpio0, (irq_handler_t)irq_handler_first_gpio);
    config_int(SECOND_GPIO, &irq_gpio1, (irq_handler_t)irq_handler_second_gpio);

    return;
}


/****************************************************************************/
/* This function releases interrupts.                                       */
/****************************************************************************/

void r_int_release(void) {
    free_irq(irq_gpio0, NULL);
    free_irq(irq_gpio1, NULL);
    gpio_free(FIRST_GPIO);
    gpio_free(SECOND_GPIO);
}

int init_module(void)
{

    printk("loading bcm2708_int2sig\n");


    gpio.map     = ioremap(GPIO_BASE, 4096);//p->map;
    gpio.addr    = (volatile unsigned int *)gpio.map;
    msg          = (char *)kmalloc(8, GFP_KERNEL);
    if (msg !=NULL)
        printk("malloc allocator address: 0x%x\n", msg);

    INP_GPIO(3);

    r_int_config();

    return 0;
}

void cleanup_module(void)
{
    printk("unloading bcm2708_int2sig\n");

    r_int_release();
    /* if the timer was mapped (final step of successful module init) */
    if (gpio.addr){
        /* release the mapping */
        iounmap(gpio.addr);
    }
    if (msg){
        /* release the malloc */
        kfree(msg);
    }
}























































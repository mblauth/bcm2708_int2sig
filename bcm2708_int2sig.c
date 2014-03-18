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

volatile unsigned int val;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marvin Blauth");
MODULE_DESCRIPTION("A module translating GPIO Interrupts to POSIX signals");

static char   *msg=NULL;

unsigned long flags;
int handling_task_pid = -1;
module_param(handling_task_pid, int, 0);

/****************************************************************************/
/* Interrupts variables block                                               */
/****************************************************************************/
short int irq_any_gpio = 0;

unsigned int last_interrupt_time = 0;
static uint64_t epochMilli;

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

static irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs) {
    unsigned int interrupt_time = millis();
    struct task_struct *task;
    int status;
  
    if (interrupt_time - last_interrupt_time < 1000)
        return IRQ_HANDLED;
    last_interrupt_time = interrupt_time;

 // printk(KERN_NOTICE "received interrupt\n");

    if(handling_task_pid == -1) {
        printk("interrupt handling task not set\n");
        return IRQ_HANDLED;
    }

    task = pid_task(find_vpid(handling_task_pid), PIDTYPE_PID);
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

/****************************************************************************/
/* This function configures interrupts.                                     */
/****************************************************************************/

void r_int_config(void) {

    struct timeval tv ;

    do_gettimeofday(&tv) ;
    epochMilli = (uint64_t)tv.tv_sec * (uint64_t)1000    + (uint64_t)(tv.tv_usec / 1000) ;

    if (gpio_request(GPIO_ANY_GPIO, GPIO_ANY_GPIO_DESC)) {
        printk("GPIO request failure: %s\n", GPIO_ANY_GPIO_DESC);
        return;
    }

    if ( (irq_any_gpio = gpio_to_irq(GPIO_ANY_GPIO)) < 0 ) {
        printk("GPIO to IRQ mapping failure %s\n", GPIO_ANY_GPIO_DESC);
        return;
    }

    printk(KERN_NOTICE "Mapped int %d\n", irq_any_gpio);

    if (request_irq(irq_any_gpio,
                   (irq_handler_t ) r_irq_handler,
                   IRQF_TRIGGER_RISING,
                   GPIO_ANY_GPIO_DESC,
                   NULL)) {
        printk("Irq Request failure\n");
        return;
    }

   return;
}


/****************************************************************************/
/* This function releases interrupts.                                       */
/****************************************************************************/

void r_int_release(void) {
    free_irq(irq_any_gpio, NULL);
    gpio_free(GPIO_ANY_GPIO);
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






















































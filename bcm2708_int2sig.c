#include <linux/module.h>   
#include <linux/string.h>    
#include <linux/fs.h>      
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/cdev.h>

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/list.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <mach/platform.h>
#include <mach/gpio.h>
#include <linux/time.h>

#include "RPI.h"

#define MY_MAJOR        89
#define MY_MINOR        0
#define GPIO_ANY_GPIO                17

// text below will be seen in 'cat /proc/interrupt' command
#define GPIO_ANY_GPIO_DESC           "MyInterrupt"

// below is optional, used in more complex code, in our case, this could be
// NULL
#define GPIO_ANY_GPIO_DEVICE_DESC    "mydevice"

int major, ret_val;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RBiswasx");
MODULE_DESCRIPTION("A Simple Character Device Driver module");

static int my_open(struct inode *, struct file *);
static ssize_t my_read(struct file *, char *, size_t, loff_t *);
static ssize_t my_write(struct file *, const char *, size_t, loff_t *);
static int my_close(struct inode *, struct file *);

static char   msg[100] = {0};
volatile unsigned int *addr_GPHEN0=NULL;
volatile unsigned int *addr_GPEDS0=NULL;

struct file_operations my_fops = {
        read    :       my_read,
        write   :       my_write,
        open    :       my_open,
        release :       my_close,
        owner   :       THIS_MODULE
        };

struct cdev my_cdev;

/****************************************************************************/
/* Interrupts variables block                                               */
/****************************************************************************/
short int irq_any_gpio    = 0;

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
   unsigned long flags;
   volatile unsigned int val;
   unsigned int interrupt_time = millis();

   if (interrupt_time - last_interrupt_time < 1000) 
   {
     //printk(KERN_NOTICE "Ignored Interrupt!!!!! [%d]%s \n", irq, (char *) dev_id);
     return IRQ_HANDLED;
   }
   last_interrupt_time = interrupt_time;

   // disable hard interrupts (remember them in flag 'flags')
   local_irq_save(flags);

   if (((val=GPIO_READ(23)) & 0xffffffff))
   printk(KERN_NOTICE "Interrupt [%d] for device %s was triggered !.\n", irq, (char *) dev_id);
   val =0;

   if (((val=GPIO_READ(23)) & 0xffffffff))
          GPIO_SET = 1 << 4;
   else
          GPIO_CLR = 1 << 4;

   //printk(KERN_NOTICE "Interrupt [%d] for device %s was triggered !.\n", irq, (char *) dev_id);

   // restore hard interrupts
   local_irq_restore(flags);

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
                   IRQF_TRIGGER_HIGH,
                   GPIO_ANY_GPIO_DESC,
                   GPIO_ANY_GPIO_DEVICE_DESC)) {
      printk("Irq Request failure\n");
      return;
   }

   return;
}


/****************************************************************************/
/* This function releases interrupts.                                       */
/****************************************************************************/

void r_int_release(void) {

   free_irq(irq_any_gpio, GPIO_ANY_GPIO_DEVICE_DESC);
   gpio_free(GPIO_ANY_GPIO);

   return;
}

int init_module(void)
{

        dev_t devno;
        unsigned int count;
        int err;

        printk("<1>Hello World\n");

        devno = MKDEV(MY_MAJOR, MY_MINOR);
        register_chrdev_region(devno, 1, "mydevice");

        cdev_init(&my_cdev, &my_fops);

        my_cdev.owner = THIS_MODULE;
        count = 1;
        err = cdev_add(&my_cdev, devno, count);
        printk("'mknod /dev/mydevice c %d 0'.\n", MY_MAJOR);

        gpio.map     = ioremap(GPIO_BASE, 4096);//p->map;
        gpio.addr    = (volatile unsigned int *)gpio.map;

        INP_GPIO(4);    // Green LED
        OUT_GPIO(4);
        printk("*****GPIO 4 as GREEN Light ******************\n");

        INP_GPIO(24);   // Green LED
        OUT_GPIO(24);

        // Define pin 4 as INPUT for MP3_DREQ
        INP_GPIO(23);   
        
        if (err < 0)
        {
                printk("Device Add Error\n");
                return -1;
        }
        r_int_config();

        return 0;
}

void cleanup_module(void)
{
        dev_t devno;

        printk("<1> Goodbye\n");

        devno = MKDEV(MY_MAJOR, MY_MINOR);
        r_int_release();
        /* if the timer was mapped (final step of successful module init) */
        if (gpio.addr){
        /* release the mapping */
        iounmap(gpio.addr);
        }

        unregister_chrdev_region(devno, 1);
        cdev_del(&my_cdev);
}

static int my_open(struct inode *inod, struct file *fil)
{
                int major;
                int minor;

                major = imajor(inod);
                minor = iminor(inod);
                printk("\n*****Some body is opening me at major %d  minor %d*****\n",major, minor);
                return 0;
}

static ssize_t my_read(struct file *filp, char *buff, size_t len, loff_t *off)
{
                int major;
                short count;
                major = MAJOR(filp->f_dentry->d_inode->i_rdev);
                count = copy_to_user(buff, msg, len);

                printk("*****Some body is reading me at major %d*****\n",major);
                printk("*****Number of bytes read :: %d *************\n",len);          

                return 0;
}

static ssize_t my_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
        int minor, i, j, k;
                short count;
                memset(msg, 0, 100);
                count =copy_from_user(msg, buff, len);

                for (i=0; i< 10000; i++){
                for (j=0; j< 1000; j++){
                for (k=0; k< 1000; k++){
                GPIO_SET = 1 << 24;
                }
                }
                }
                GPIO_CLR = 1 << 4;

                minor = MAJOR(filp->f_dentry->d_inode->i_rdev);
                printk("*****Some body is writing me at major %d*****\n",minor);
                printk("*****Number of bytes written :: %d **********\n",len);

                return 0;
}

static int my_close(struct inode *inod, struct file *fil)
{
        #if 0
        printk(KERN_ALERT "Device closed\n");
        return 0;
        #endif
                int minor;
                minor = MAJOR(fil->f_dentry->d_inode->i_rdev);
                printk("*****Some body is closing me at major %d*****\n",minor);
                return 0;
}
/*
static int my_control(struct inode *inode, struct file *filp, unsigned int command, unsigned long argument){
                int minor;
                minor = MINOR(filp->f_dentry->d_inode->i_rdev);
                printk("*****Some body is controlling me at minor %d*****\n",minor);
                return 0;
                }
*/
























































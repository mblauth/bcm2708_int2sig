
#define BCM2708_PERI_BASE       0x20000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)  // GPIO controller
#define GPPUD                   (gpio.addr + 37) // Pin Pull-up/down Enable
#define GPPUDCLK0               (gpio.addr + 38) // Pin Pull-up/down Enable Clock
 
//#define BLOCK_SIZE            (4*1024)
 
// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x)
#define INP_GPIO(g)       *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)       *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))
#define SET_PUD(p)        *GPPUD = p & 3; udelay(50)
#define SET_PUDCLK0(g)    *GPPUDCLK0 = 1 << (g & 31); udelay(50)
#define PULL_UP(g)        SET_PUD(PUD_UP); SET_PUDCLK0(g); SET_PUD(0); SET_PUDCLK0(0)
#define PULL_DOWN(g)      SET_PUD(PUD_DOWN); SET_PUDCLK0(g); SET_PUD(0); SET_PUDCLK0(0)

#define GPIO_SET  *(gpio.addr + 7)   // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR  *(gpio.addr + 10)  // clears bits which are 1 ignores bits which are 0

#define PUD_UP    2
#define PUD_DOWN  1
 
#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))

// IO Acces
struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};

struct bcm2835_peripheral gpio = {GPIO_BASE};


 
 


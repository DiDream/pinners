/*
Autores:
* Juan José Barroso Gimenez
* Elena Miniaeva
* Alex Jimmy Montaño Fuentes
*/

#include <asm/io.h>
#include <mach/platform.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/timer.h>
#include <linux/err.h>
#include <linux/jiffies.h>

#include <linux/device.h>

#include <linux/delay.h>

MODULE_LICENSE("Dual BSD/GPL");

struct GPIO_Regs
{
	uint32_t GPFSEL[6];
	uint32_t Reserved1;
	uint32_t GPSET[2];
	uint32_t Reserved2;
	uint32_t GPCLR[2];
	uint32_t Reserver3;
	uint32_t GPLEV[2];
	uint32_t Reserver4;
	uint32_t GPEDS[2];
	uint32_t Reserver5;
	uint32_t GPREN[2];
	uint32_t Reserver6;
	uint32_t GPFEN[2];
	uint32_t Reserver7;
	uint32_t GPHEN[2];
	uint32_t Reserver8;
	uint32_t GPLEN[2];
	uint32_t Reserver9;
	uint32_t GPAREN[2];
	uint32_t Reserver10;
	uint32_t GPAFEN[2];
	uint32_t Reserver11;
	uint32_t GPPUD;
	uint32_t GPPUDCLK[2];

};

struct GPIO_Regs *gpio_regs;

static struct timer_list s_BlinkTimer;
static int s_BlinkPeriod = 1000;
static struct class *myDeviceClass;
static struct device *myDeviceObject;

static void SetGPIOOutputValue(int GPIO, bool outputValue);
static ssize_t my_callback(struct device* dev, struct device_attribute* attr,const char* buf, size_t count);

static void SetGPIOFunction(int GPIO, int functionCode);
static void toogleTurnLed(int pinInput, int pinLed);

static void pinToogleHandler(unsigned long unused);
static void BlinkTimerHandler(unsigned long unused);
static void setPullUpRegister();
static void waitCycles(int cycles);
static void setOneBitGPPUDCLK(bit);



static DEVICE_ATTR(period, S_IWUSR | S_IWGRP | S_IWOTH, NULL,my_callback);


static void SetGPIOOutputValue(int GPIO, bool outputValue)
{
    if (outputValue)
        gpio_regs ->GPSET[GPIO / 32] = (1 << (GPIO % 32));
    else
        gpio_regs ->GPCLR[GPIO / 32] = (1 << (GPIO % 32));
}

static ssize_t my_callback(struct device* dev, struct device_attribute* attr,const char* buf, size_t count) {
	long period;
	if (kstrtol(buf, 10, &period) < 0)
		return -EINVAL;
	if (period == 1)
		SetGPIOOutputValue(27, true);
	else
		SetGPIOOutputValue(27, false);
	return count;
}



static void SetGPIOFunction(int GPIO, int functionCode)
{
    int registerIndex = GPIO / 10;
    int bit = (GPIO % 10) * 3;

    unsigned oldValue = gpio_regs ->GPFSEL[registerIndex];
    unsigned mask = 0b111 << bit;
    printk("Changing function of GPIO%d from %x to %x\n", GPIO, (oldValue >> bit) & 0b111, functionCode);
    gpio_regs ->GPFSEL[registerIndex] = (oldValue & ~mask) | ((functionCode << bit) & mask);
}



// Funcion para hacer que el led cambie estado on->off, off->on, Se llamara
// de forma automatica tras las interrupciones del temporizador indicadas
/*
static void BlinkTimerHandler(unsigned long unused) {
	static bool on = false;
	on = !on;

	//SetGPIOOutputValue(27, on);
	//SetGPIOOutputValue(22, !on);
	mod_timer(&s_BlinkTimer, jiffies + msecs_to_jiffies(s_BlinkPeriod));
	
}
*/

static void toogleTurnLed(int pinInput, int pinLed){
	uint32_t result = gpio_regs -> GPLEV[0] & (1 << (pinInput % 32));

	if (result == 0) {
		SetGPIOOutputValue(pinLed, true);
	}
	else {
		SetGPIOOutputValue(pinLed, false);
	}
}
static void pinToogleHandler(unsigned long unused) {
	toogleTurnLed(18, 27);
	toogleTurnLed(23, 22);

	mod_timer(&s_BlinkTimer, jiffies + msecs_to_jiffies(s_BlinkPeriod));
}


static void setPullUpRegister(){
	gpio_regs -> GPPUD = gpio_regs -> GPPUD | 0x02;
}

static void waitCycles(int cycles){

	while(cycles < 0){
		cycles--;	
	}
}

static void setOneBitGPPUDCLK(bit){
	int index = bit / 32;
	bit = bit % 32;
	gpio_regs -> GPPUDCLK[index] = gpio_regs -> GPPUDCLK[index] | (1 << bit);
}



static int init_Module(void) {
	printk(KERN_ALERT "Accediendo al GPIO\n");
	gpio_regs = (struct GPIO_Regs *) __io_address(GPIO_BASE);


	SetGPIOFunction(22, 1);
	SetGPIOFunction(27, 1);
	SetGPIOFunction(23, 0);
	SetGPIOFunction(18, 0);

	/*
	setup_timer(&s_BlinkTimer, BlinkTimerHandler, 0);
	mod_timer(&s_BlinkTimer, jiffies + msecs_to_jiffies(s_BlinkPeriod));
	
	
	int result;
	
	myDeviceClass = class_create(THIS_MODULE, "LedBlink");
	myDeviceObject = device_create(myDeviceClass, NULL, 0, NULL, "LedBlink");
	result = device_create_file(myDeviceObject, &dev_attr_period);
	*/


	setPullUpRegister();
	waitCycles(150);

	setOneBitGPPUDCLK(23);
	waitCycles(150);
	
	setOneBitGPPUDCLK(18);
	waitCycles(150);

	setup_timer(&s_BlinkTimer, pinToogleHandler, 0);
	mod_timer(&s_BlinkTimer, jiffies + msecs_to_jiffies(s_BlinkPeriod));
		
	
	return 0;
	
}






	//Hacer una función que active o desactive el pin
static void close_Module(void) {
	// Cerrar el módulo y dejar el pin como estaba
	SetGPIOOutputValue(22, false);
	SetGPIOOutputValue(27, false);

	del_timer(&s_BlinkTimer);
	
}



module_init(init_Module);
module_exit(close_Module);

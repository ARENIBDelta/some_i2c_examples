#define PART_LM4F120B2QR
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_i2c.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/i2c.h"
#include <stdint.h>

#define I2C_SLAVE_ADDRESS       0x60

unsigned int i2c_flag = 0;
unsigned char data = 0x00;
unsigned long err = 0x00;
unsigned long data_recv = 0x00;

void i2c3_master_interrupt(void) {
	unsigned long status = I2CMasterIntStatusEx(I2C3_MASTER_BASE, false);
	if (status & I2C_MASTER_INT_TIMEOUT) {
		i2c_flag = 2;
		I2CMasterIntClearEx(I2C3_MASTER_BASE, I2C_MASTER_INT_TIMEOUT);
	}
	if (status & I2C_MASTER_INT_DATA) {
		err = I2CMasterErr(I2C3_MASTER_BASE);
		if (err != I2C_MASTER_ERR_NONE)
			i2c_flag = 2;
		else
			i2c_flag = 1;
		I2CMasterIntClearEx(I2C3_MASTER_BASE, I2C_MASTER_INT_DATA);
	}
}

int main(void) {
     SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
     SysCtlDelay(10000);

     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

     GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
     GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

     GPIOPinConfigure(GPIO_PD0_I2C3SCL);
     GPIOPinConfigure(GPIO_PD1_I2C3SDA);

     SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);

     I2CMasterInitExpClk( I2C3_MASTER_BASE, SysCtlClockGet(), false);
     SysCtlDelay(10000);

	 I2CMasterTimeoutSet(I2C3_MASTER_BASE, 0x7d);
	 I2CMasterSlaveAddrSet(I2C3_MASTER_BASE, I2C_SLAVE_ADDRESS, false);
	 I2CMasterIntEnableEx(I2C3_MASTER_BASE, I2C_MASTER_INT_TIMEOUT|I2C_MASTER_INT_DATA);

	 IntEnable(INT_I2C3);

	 IntMasterEnable();

     while(1) {
    	 data++;
    	 i2c_flag=0;

    	 //Send one byte
		 I2CMasterSlaveAddrSet(I2C3_MASTER_BASE, I2C_SLAVE_ADDRESS, false);
		 I2CMasterDataPut(I2C3_MASTER_BASE, data);
		 I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);
    	 while(!i2c_flag);
    	 if (i2c_flag == 2) {
    		 //I2C Failure
    		 continue;
    	 }
    	 else if (i2c_flag == 1) {
    		 //I2C Success
    	 }

    	 //Get one byte
		 I2CMasterSlaveAddrSet( I2C3_MASTER_BASE, I2C_SLAVE_ADDRESS, true);
		 I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
		 while(!i2c_flag);
    	 if (i2c_flag == 2) {
    		 //I2C Failure
    		 continue;
    	 }
    	 else if (i2c_flag == 1) {
    		 //I2C Success
    		 data_recv = I2CMasterDataGet(I2C3_MASTER_BASE);
    	 }

		 SysCtlDelay(10000);

     }
 }

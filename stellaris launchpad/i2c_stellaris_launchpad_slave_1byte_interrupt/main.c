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

unsigned char data_recv;
unsigned char data_send;
unsigned long status;

void i2c3_slave_interrupt(void) {
    unsigned long status = I2CSlaveIntStatusEx(I2C3_SLAVE_BASE, false);
    if (status & I2C_SLAVE_INT_START) {
    	//Start condition
    }
    else if (status & I2C_SLAVE_INT_STOP) {
    	//Stop condition
    }
    else if (status & I2C_SLAVE_INT_DATA) {
    	//Data in or out
		status = I2CSlaveStatus(I2C3_SLAVE_BASE);
		if (status & I2C_SCSR_RREQ) {
			data_recv = I2CSlaveDataGet(I2C3_SLAVE_BASE);
			data_send = data_recv * 2;
		}
		else if (status & I2C_SCSR_TREQ) {
			I2CSlaveDataPut(I2C3_SLAVE_BASE, data_send);
		}
    }
    I2CSlaveIntClearEx(I2C3_SLAVE_BASE, status);
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

     I2CSlaveInit(I2C3_SLAVE_BASE, I2C_SLAVE_ADDRESS);
     I2CSlaveAddressSet(I2C3_SLAVE_BASE, I2C_SLAVE_ADDRESS, 0);
     I2CSlaveIntEnableEx(I2C3_SLAVE_BASE, I2C_SLAVE_INT_START|I2C_SLAVE_INT_STOP|I2C_SLAVE_INT_DATA);
     I2CSlaveEnable(I2C3_SLAVE_BASE);
     IntEnable(INT_I2C3);
	 IntMasterEnable();

     while(1) {
    	 SysCtlDelay(100000);
     }
 }

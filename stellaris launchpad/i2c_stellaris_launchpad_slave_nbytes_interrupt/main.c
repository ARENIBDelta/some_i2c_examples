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

typedef enum {op_add, op_mult, op_div, op_subst} operation;
struct packet_from_master {
	int op1;
	int op2;
	operation op;
};

struct packet_to_master {
	int res;
};

enum {sending, receiving} what_we_re_doing;
struct packet_from_master data_recv = {0, 0, op_add};
unsigned char sent_bytes = 0;

struct packet_to_master data = {0x00};
unsigned char received_bytes = 0;

unsigned long status;

void i2c3_slave_interrupt(void) {
    unsigned long int_status = I2CSlaveIntStatusEx(I2C3_SLAVE_BASE, false);
    if (int_status & I2C_SLAVE_INT_START) {
    	//Start condition
    	sent_bytes = 0;
    	received_bytes = 0;
    }
    else if (int_status & I2C_SLAVE_INT_STOP) {
    	//Stop condition
		if (received_bytes == sizeof(data_recv)) {
			//Reception success
			switch(data_recv.op) {
			case op_add:
				data.res = data_recv.op1 + data_recv.op2;
				break;
			case op_mult:
				data.res = data_recv.op1 * data_recv.op2;
				break;
			case op_div:
				data.res = data_recv.op1 / data_recv.op2;
				break;
			case op_subst:
				data.res = data_recv.op1 - data_recv.op2;
				break;
			default:
				//Error
				data.res = 0;
				break;
			}
		}
    }
    else if (int_status & I2C_SLAVE_INT_DATA) {
    	//Data in or out
		status = I2CSlaveStatus(I2C3_SLAVE_BASE);
		if (status & I2C_SCSR_RREQ) {
			if (received_bytes > sizeof(data_recv))
				received_bytes = 0;
			unsigned char *p = (unsigned char *)&data_recv + received_bytes;
			*p = I2CSlaveDataGet(I2C3_SLAVE_BASE);
			received_bytes++;
		}
		else if (status & I2C_SCSR_TREQ) {
			I2CSlaveDataPut(I2C3_SLAVE_BASE, 0x42);
			if (sent_bytes > sizeof(data))
				sent_bytes = 0;
			unsigned char *p = (unsigned char *)&data + sent_bytes;
			I2CSlaveDataPut(I2C3_SLAVE_BASE, *p);
			sent_bytes++;
		}
    }
    I2CSlaveIntClearEx(I2C3_SLAVE_BASE, int_status);
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

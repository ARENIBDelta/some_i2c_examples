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

typedef enum {op_add, op_mult, op_div, op_subst} operation;
struct packet_to_slave {
	int op1;
	int op2;
	operation op;
};

struct packet_from_slave {
	int op1;
};

#define I2C_SLAVE_ADDRESS       0x60

enum {sending, receiving} what_we_re_doing;
struct packet_to_slave data = {0, 0, op_add};
unsigned char sent_bytes = 0;

struct packet_from_slave data_recv = {0x00};
unsigned char received_bytes = 0;

unsigned int i2c_flag = 0;
unsigned long err = 0x00;

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

		if (what_we_re_doing == sending) {
			sent_bytes++;
			if (sent_bytes < sizeof(data)-1) {
				//Continuing
				 I2CMasterDataPut(I2C3_MASTER_BASE, ((unsigned char *)&data)[sent_bytes]);
				 I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
			}
			else if (sent_bytes == sizeof(data)-1) {
				//Last byte remaining
				 I2CMasterDataPut(I2C3_MASTER_BASE, ((unsigned char *)&data)[sent_bytes]);
				 I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
			}
			else {
				//Transaction is done
				i2c_flag = 1;
			}
		}
		else if (what_we_re_doing == receiving) {
			unsigned char *p = (unsigned char *)&data_recv + received_bytes;
			*p = I2CMasterDataGet(I2C3_MASTER_BASE);
			received_bytes++;
			if (received_bytes < sizeof(data_recv) - 1) {
				I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
			}
			else if (received_bytes == sizeof(data_recv) - 1) {
				I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
			}
			else {
				//Transaction is done
				i2c_flag = 1;
			}
		}


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
    	 i2c_flag=0;
    	 //Let's send various challenges to our slave
    	 data.op1++;
    	 data.op2+=2;
    	 switch(data.op1 % 4) {
    	 case 0:
    		 data.op = op_add;
    		 break;
    	 case 1:
    		 data.op = op_mult;
    		 break;
    	 case 2:
    		 data.op = op_div;
    		 break;
    	 case 3:
    		 data.op = op_subst;
    		 break;
    	 }

    	 what_we_re_doing = sending;
    	 sent_bytes = 0;
    	 //Start to send the full structure
		 I2CMasterSlaveAddrSet(I2C3_MASTER_BASE, I2C_SLAVE_ADDRESS, false);
		 I2CMasterDataPut(I2C3_MASTER_BASE, *((unsigned char *)&data));
		 I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    	 while(!i2c_flag);
    	 if (i2c_flag == 2) {
    		 //I2C Failure
    		 continue;
    	 }
    	 else if (i2c_flag == 1) {
    		 //I2C Success
    	 }

    	 what_we_re_doing = receiving;
    	 received_bytes = 0;
    	 //Start to receive the full structure
		 I2CMasterSlaveAddrSet(I2C3_MASTER_BASE, I2C_SLAVE_ADDRESS, true);
		 I2CMasterControl(I2C3_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);

    	 while(!i2c_flag);
    	 if (i2c_flag == 2) {
    		 //I2C Failure
    		 continue;
    	 }
    	 else if (i2c_flag == 1) {
    		 //I2C Success
    	 }

		 SysCtlDelay(SysCtlClockGet()/100);

     }
 }

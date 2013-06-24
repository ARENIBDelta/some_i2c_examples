#define PART_LM4F120B2QR
//*****************************************************************************
//
//i2cEcho.c - echo message between I2C0 and I2C2 ports
//
//*****************************************************************************


#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup i2c_examples_list
//! <h1>I2C Master Loopback (i2c_master_slave_loopback)</h1>
//!
//! This example shows how to configure the I2C0 module as master mode
//! and I2C2 as slave mode.
//! This includes setting up the master and slave module.  You will have
//! to add two pull up resistors for this to work.
//! one resistor between Vcc and SCL
//! one resistor between Vcc and SDA
//!
//! This example uses both the Slave and Master IRQ to handle the data
//! transfer.  There is a throttle bit in use just to help control
//! data flow in this example.  In practice this would not be needed as
//! the application code should take care of this.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - I2C0 peripheral
//! - GPIO Port B peripheral (for I2C0 pins)
//! - I2C0SCL - PB2
//! - I2C0SDA - PB3
//! - I2C2SCL - PE4
//! - I2C2SDA - PE5
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of I2C.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - irq_I2C0MasterIntHandler()
//! - irq_I2C2SlaveIntHandler()
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Set the address for slave module. This is a 7-bit address sent in the
// following format:
//                      [A6:A5:A4:A3:A2:A1:A0:RS]
//
// A zero in the R/S position of the first byte means that the master
// transmits (sends) data to the selected slave, and a one in this position
// means that the master receives data from the slave.
//
//*****************************************************************************
#define SLAVE_ADDRESS 0x3C

#define WRITE_BYTE (false) //*< instruct slave I2C device that the master is doing a write operation
#define READ_BYTE (false) //*< instruct slave I2C device that the master is doing a read operation

//*****************************************************************************
//
// Global variable to hold the I2C data that has been received.
//
//*****************************************************************************
static volatile unsigned long g_ulMasterData;

//*****************************************************************************
//
// This is a flag that gets set in the interrupt handler to indicate that an
// interrupt occurred.  This is only used in this example to help throttle
// communication.  In practice this would not be needed as your application
// code would rely on the IRQ's for best performance.
//
//*****************************************************************************
static volatile unsigned long g_ulIntFlag = 0;
#define MASTER_IRQ      (1)
#define SLAVE_IRQ       (2)

//*****************************************************************************
//
// This function sets up UART0 to be used for a console to display information
// as the example is running.
//
//*****************************************************************************
void InitConsole(void)
{
  //
  // Configure the relevant pins such that UART0 owns them.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  //
  // Open UART0 for debug output.
  //
  UARTStdioInit(0);

}


//*****************************************************************************
//
// The interrupt handler for the for I2C0 data master interrupt.
//
//*****************************************************************************
void irq_I2C0MasterIntHandler(void)
{
  //
  // Clear the I2C0 interrupt flag.
  //
  I2CMasterIntClear(I2C0_MASTER_BASE);

  //
  // Read the data from the slave.
  //
  g_ulMasterData = I2CMasterDataGet(I2C0_MASTER_BASE);

  //
  // Set a flag to indicate that the interrupt occurred.
  //
  g_ulIntFlag &= ~MASTER_IRQ;
}


//*****************************************************************************
//
// The interrupt handler for the for I2C2 data slave interrupt.
//
//*****************************************************************************
void irq_I2C2SlaveIntHandler(void)
{
  static volatile unsigned int uiData = 0;
  unsigned int uiStatus;
  uiStatus = I2CSlaveStatus(I2C2_SLAVE_BASE);

  //
  // Clear the I2C0 interrupt flag.
  //
  I2CSlaveIntClear(I2C2_SLAVE_BASE);

  if(uiStatus & I2C_SLAVE_ACT_TREQ)
    {
      // based on status, master is requesting data
      I2CSlaveDataPut(I2C2_SLAVE_BASE,uiData);
    }
  if(uiStatus &I2C_SLAVE_ACT_RREQ)
    {
      // based on status, master is pushing data
      uiData = I2CSlaveDataGet(I2C2_SLAVE_BASE);
      if(++uiData > 'Z')
        uiData='A';
    }

  //
  // Set a flag to indicate that the interrupt occurred.
  //
  g_ulIntFlag &= ~SLAVE_IRQ;

}

void InitI2C(void)
{
  //
  // For this example I2C0 is used with PortB[3:2].  The actual port and
  // pins used may be different on your part, consult the data sheet for
  // more information.  GPIO port B needs to be enabled so these pins can
  // be used.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

  //
  // The I2C0 peripheral must be enabled before use.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

  //
  // Configure the pin muxing for I2C0 functions on port B2 and B3.
  // This step is not necessary if your part does not support pin muxing.
  //
  GPIOPinConfigure(GPIO_PB2_I2C0SCL);
  GPIOPinConfigure(GPIO_PB3_I2C0SDA);

  //
  // Select the I2C function for these pins.  This function will also
  // configure the GPIO pins pins for I2C operation, setting them to
  // open-drain operation with weak pull-ups.  Consult the data sheet
  // to see which functions are allocated per pin.
  //
  GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_2 | GPIO_PIN_3);


  // Set GPIO Pins for Open-Drain operation (I have two Rpulls=10K Ohm to 5V on the SCL and SDA lines)
  GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_OD);
  GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_OD);

  // Give control to the I2C0 Module
  GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_DIR_MODE_HW);
  GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_DIR_MODE_HW);

  //
  // The I2C2 peripheral must be enabled before use.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

  //
  // Configure the pin muxing for I2C2 functions on port B2 and B3.
  // This step is not necessary if your part does not support pin muxing.
  //
  GPIOPinConfigure(GPIO_PE4_I2C2SCL);
  GPIOPinConfigure(GPIO_PE5_I2C2SDA);

  //
  // Select the I2C function for these pins.  This function will also
  // configure the GPIO pins pins for I2C operation, setting them to
  // open-drain operation with weak pull-ups.  Consult the data sheet
  // to see which functions are allocated per pin.
  //
  GPIOPinTypeI2C(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);


  // Set GPIO Pins for Open-Drain operation (I have two Rpulls=10K Ohm to 5V on the SCL and SDA lines)
  GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_OD);
  GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_OD);

  // Give control to the I2C2 Module
  GPIODirModeSet(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW);
  GPIODirModeSet(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_DIR_MODE_HW);


}

//*****************************************************************************
//
// Configure an I2C0 master to echo data to the I2C2 slave
//
//
//*****************************************************************************
int
main(void)
{
  unsigned long ulDataTx = 'A';
  int loopTest = 1;
  unsigned long i2cStatus;
  //
  // Set the clocking to run directly from the external crystal/oscillator.
  // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
  // crystal on your board.
  //
  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
      SYSCTL_XTAL_16MHZ);

  //
  // Set up the serial console to use for displaying messages.  This is just
  // for this example program and is not needed for proper I2C operation.
  //
  InitConsole();

  InitI2C();

  //
  // Enable the I2C2 interrupt on the processor (NVIC).
  //
  IntEnable(INT_I2C0);
  IntEnable(INT_I2C2);

  //
  // Enable interrupts to the processor.
  //
  IntMasterEnable();

  //
  // Configure and turn on the I2C2 slave interrupt.  The I2CSlaveIntEnableEx()
  // gives you the ability to only enable specific interrupts.  For this case
  // we are only interrupting when the slave device receives data.
  //
  I2CSlaveIntEnableEx(I2C2_SLAVE_BASE, I2C_SLAVE_INT_DATA);

  // configure and turn on the I2C0 master interrupt.  The I2CMasterIntEnableEx()
  // gives you the ability to only enable specific interrupts.
  I2CMasterIntEnableEx(I2C0_MASTER_BASE,I2C_MASTER_INT_DATA|I2C_MASTER_INT_TIMEOUT);

  //
  // Enable and initialize the I2C0 master module.  Use the system clock for
  // the I2C0 module.  The last parameter sets the I2C data transfer rate.
  // If false the data rate is set to 100kbps and if true the data rate will
  // be set to 400kbps.  For this example we will use a data rate of 100kbps.
  //
  I2CMasterInitExpClk(I2C0_MASTER_BASE, SysCtlClockGet(), false);

  //
  // Enable the I2C0 slave module.
  //
  I2CSlaveEnable(I2C2_SLAVE_BASE);

  //
  // Set the slave address to SLAVE_ADDRESS.  In loopback mode, it's an
  // arbitrary 7-bit number (set in a macro above) that is sent to the
  // I2CMasterSlaveAddrSet function.
  //
  I2CSlaveInit(I2C2_SLAVE_BASE, SLAVE_ADDRESS);


  //
  // Display the example setup on the console.
  //
  UARTprintf("I2C Slave Interrupt Example ->");
  UARTprintf("\n   Module = I2C0");
  UARTprintf("\n   Mode = Receive interrupt on the Slave module");
  UARTprintf("\n   Rate = 100kbps\n\n");

  while(loopTest)
    {
      //
      // Tell the master module what address it will place on the bus when
      // communicating with the slave.  Set the address to SLAVE_ADDRESS
      // (as set in the slave module).  The receive parameter is set to false
      // which indicates the I2C Master is initiating a writes to the slave.  If
      // true, that would indicate that the I2C Master is initiating reads from
      // the slave.
      //
      I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, SLAVE_ADDRESS, false);

           //
      // Place the data to be sent in the data register.
      //
      I2CMasterDataPut(I2C0_MASTER_BASE, ulDataTx);

      //
      // Initiate send of single byte of data from the master.
      //
      g_ulIntFlag |= SLAVE_IRQ;
      I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);

      //
      // Indicate the direction of the data.
      //
      UARTprintf("Transferring from: Master -> Slave\n");

      //
      // Display the data that I2C0 is transferring.
      //
      UARTprintf("  Sending: '%c : %x'", ulDataTx,ulDataTx);

      // setup throttle
      while( g_ulIntFlag & SLAVE_IRQ )
        ;

      // now wait for a reply from the slave.
      I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, SLAVE_ADDRESS, true);
      g_ulIntFlag |= MASTER_IRQ;
      I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

      i2cStatus = I2CMasterErr(I2C0_MASTER_BASE);
      if(i2cStatus != I2C_MASTER_ERR_NONE)
        {
          UARTprintf("\n\n****  I2CMasterErr: '%x'\n\n", i2cStatus);
          break;
        }
      //
      // Wait for master interrupt to occur.
      //
      while(g_ulIntFlag & MASTER_IRQ)
        ;


      //
      // Display the data that the slave has echoed back to us.
      //
      UARTprintf("  Received: '%c : %x'\n\n", g_ulMasterData,g_ulMasterData);
      ulDataTx = g_ulMasterData;
      loopTest = 1;
      g_ulIntFlag = 0;
      ulDataTx++;
      if(ulDataTx=='Z')
        ulDataTx = 'A';
    }
}

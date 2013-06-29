/* Includes ------------------------------------------------------------------*/
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_i2c.h"

/* Private defines -----------------------------------------------------------*/
#define SLAVE_ADDRESS 0x60
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

//! Byte swap unsigned int
uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

//! Byte swap int
int32_t swap_int32( int32_t val )
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

typedef enum {op_add, op_mult, op_div, op_subst} operation;
struct packet_to_slave {
	int32_t op1;
	int32_t op2;
	int32_t op;
};

struct packet_from_slave {
	int32_t op1;
};

struct packet_to_slave data = {0, 0, op_add};
struct packet_from_slave data_recv = {0x00};

uint8_t i2c_write(struct packet_to_slave *data) {
    //
    // Write n bytes
    //
    uint8_t res = 1;
    volatile uint32_t timeout;
    uint16_t i;
    
    I2C_AcknowledgeConfig(I2C_ACK_CURR);
  
    //Wait for the bus to be ready
    timeout = 0x0FFF;
    while(timeout-- && I2C_GetFlagStatus(I2C_FLAG_BUSBUSY));
    if (!timeout) {
      //Error
      res = 0;
      return res;
    }
    
    //Send the start condition
    I2C_GenerateSTART(ENABLE);
    timeout = 0x0FFF;
    while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
    if (!timeout) {
      //Error
      res = 0;
      goto stop;
    }
    
    //Send the slave address
    I2C_Send7bitAddress((SLAVE_ADDRESS << 1) | 0x01, I2C_DIRECTION_TX);
    timeout = 0x0FFF;
    while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    if (!timeout) {
      //Error
      res = 0;
      goto stop;
    }
    else if (SET == I2C_GetFlagStatus(I2C_FLAG_ACKNOWLEDGEFAILURE)) {
        I2C_ClearFlag(I2C_FLAG_ACKNOWLEDGEFAILURE);
        res = 0;
        goto stop;
    }
    
    //Send the data
    for(i=0; i<sizeof(struct packet_to_slave); i++) {
      uint8_t ldata = *((uint8_t *)data + i);
      I2C_SendData(ldata);
      timeout = 0x0FFF;
      while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
      if (!timeout) {
        //Error
        res = 0;
        goto stop;
      }
    }
    
  stop:
    //Send the stop condition
    I2C_GenerateSTOP(ENABLE);
    
    return res;
}

uint8_t i2c_read(struct packet_from_slave *data) {
    //
    // Read one byte
    //
    uint8_t res = 1;
    volatile uint32_t timeout;
    uint16_t i;
  
    //Wait for the bus to be ready
    timeout = 0x0FFF;
    while(timeout-- && I2C_GetFlagStatus(I2C_FLAG_BUSBUSY));
    if (!timeout) {
      //Error
      res = 0;
      return res;
    }
    
    //Send the start condition
    I2C_GenerateSTART(ENABLE);
    timeout = 0x0FFF;
    while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
    if (!timeout) {
      //Error
      res = 0;
      goto stop;
    }
    
    //Send the slave address
    I2C_Send7bitAddress(SLAVE_ADDRESS << 1, I2C_DIRECTION_RX);
    timeout = 0x0FFF;
    while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    if (!timeout) {
      //Error
      res = 0;
      goto stop;
    }
    else if (SET == I2C_GetFlagStatus(I2C_FLAG_ACKNOWLEDGEFAILURE)) {
        I2C_ClearFlag(I2C_FLAG_ACKNOWLEDGEFAILURE);
        I2C_GenerateSTOP(ENABLE);
        res = 0;
        goto stop;
    }
    
    for (i=0; i<sizeof(struct packet_from_slave); i++) {
      if (i == sizeof(struct packet_from_slave) - 1) {
        I2C_AcknowledgeConfig(I2C_ACK_NONE);
        //Send the stop condition
        I2C_GenerateSTOP(ENABLE);
      }

      //Wait for the data to be received
      timeout = 0x0FFF;
      while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED));
      if (!timeout) {
        //Error
        res = 0;
      }
      else {
        //Get the data
        uint8_t *p = (uint8_t *)data + i;
        *p = I2C_ReceiveData();
      }
    }
    
   
  stop:
    //If something bad happened, send the stop condition
    if (!res)
      I2C_GenerateSTOP(ENABLE);
    
    return res;
}

void i2c_reinit(void) {
  //I2C Init
  I2C_SoftwareResetCmd(ENABLE);
  I2C_SoftwareResetCmd(DISABLE);
  I2C_Cmd(DISABLE);
  I2C_DeInit();
  I2C_Cmd(ENABLE);
  I2C_Init(I2C_MAX_STANDARD_FREQ,
     0x00,
     I2C_DUTYCYCLE_2, 
     I2C_ACK_CURR,
     I2C_ADDMODE_7BIT, 
     I2C_MAX_INPUT_FREQ
  );
  I2C_AcknowledgeConfig(I2C_ACK_CURR);
}

void main(void)
{
  //Clock init
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); //16 MHz is funnier than 2

  //LED Init
  GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_OUT_PP_LOW_SLOW);

  while (1) {
    //Let's send various challenges to our slave
    data.op1 += 1;
    data.op2 += 2;
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
    
    //Endianness conversion so that our device can communicate with other platform
    struct packet_to_slave data_other_endianness;
    data_other_endianness.op = swap_int32(data.op);
    data_other_endianness.op1 = swap_int32(data.op1);
    data_other_endianness.op2 = swap_int32(data.op2);
    
    //Init the peripheral
    i2c_reinit();

    //Write then read one byte
    if (i2c_write(&data_other_endianness)) {
      //Wait a bit and read the result
      volatile uint32_t lcount = 0x80;
      while(lcount--);
      if (i2c_read(&data_recv)) {
        //Success
        struct packet_from_slave data_recv_other_endianness;
        data_recv_other_endianness.op1 = swap_int32(data_recv.op1);
        data_recv_other_endianness.op1 = data_recv_other_endianness.op1; //To keep the compiler happy
      }
      else {
        //Error
      }
      GPIO_WriteLow(GPIOD, GPIO_PIN_0);
    }
    else {
      //Error
      GPIO_WriteHigh(GPIOD, GPIO_PIN_0);
    }
    
    //Wait a bit, toggle the LED and do it again
    volatile uint32_t count = 100000;
    while(count--);
  }
  
  
  
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


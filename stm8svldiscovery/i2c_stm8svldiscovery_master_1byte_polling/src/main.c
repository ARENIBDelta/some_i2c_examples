/* Includes ------------------------------------------------------------------*/
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_i2c.h"

/* Private defines -----------------------------------------------------------*/
#define SLAVE_ADDRESS 0x60
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

uint8_t i2c_write(uint8_t data) {
    //
    // Write one byte
    //
    uint8_t res = 1;
    volatile uint32_t timeout;
    
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
    I2C_SendData(data);
    timeout = 0x0FFF;
    while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    if (!timeout) {
      //Error
      res = 0;
      goto stop;
    }
    
  stop:
    //Send the stop condition
    I2C_GenerateSTOP(ENABLE);
    
    return res;
}

uint8_t i2c_read(uint8_t *data) {
    //
    // Read one byte
    //
    uint8_t res = 1;
    volatile uint32_t timeout;
  
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
    
    I2C_AcknowledgeConfig(I2C_ACK_NONE);
    
  stop:
    //Send the stop condition
    I2C_GenerateSTOP(ENABLE);
    
    if (res) {
      //Wait for the data to be received
      timeout = 0x0FFF;
      while(timeout-- && !I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED));
      if (!timeout) {
        //Error
        res = 0;
      }
      else {
        //Get the data
        *data = I2C_ReceiveData();
      }
    }
    
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

  i2c_reinit();
 
  uint8_t data = 0;
  uint8_t data_recv;
  
  while (1) {
    data++;
    i2c_reinit();

    //Write then read one byte
    if (i2c_write(data)) {
      volatile uint32_t lcount = 1000;
      while(lcount--);
      if (i2c_read(&data_recv)) {
        //Success
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


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

//Stuff we'll exchange over the bus
typedef enum {op_add, op_mult, op_div, op_subst} operation;
struct packet_to_slave {
	int32_t op1;
	int32_t op2;
	int32_t op;
};

struct packet_from_slave {
	int32_t res;
};

struct packet_to_slave data_recv = {0, 0, op_add};
uint16_t bytes_received = 0;
struct packet_from_slave data = {0x12345678};
struct packet_from_slave data_other_endianness = {0xAABBCCDD};
uint16_t bytes_sent = 0;

//Called in stm8s_it.c
void i2c_interrupt(void) {
  uint8_t *p;

  if ((I2C->SR2) != 0){
    I2C->SR2 = 0;
  }

  unsigned int event = I2C_GetLastEvent();

  if (event == I2C_EVENT_SLAVE_BYTE_TRANSMITTING) {
    p = (uint8_t *)&data + bytes_sent;
    I2C_SendData(*p);
    bytes_sent++;
    if (bytes_sent > sizeof(data)) {
      bytes_sent = 0;
    }
  }
  else if (event == I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED) {
    bytes_sent = 0;
  }

  if (event == I2C_EVENT_SLAVE_BYTE_RECEIVED) {
    p = (uint8_t *)&data_recv + bytes_received;
    *p = I2C_ReceiveData();
    bytes_received++;
    if (bytes_received >= sizeof(data_recv)) {
      //We received a full packet, let's solve the "challenge" and prepare
      //a response packet
      struct packet_to_slave data_recv_other_endianness;
      data_recv_other_endianness.op = swap_int32(data_recv.op);
      data_recv_other_endianness.op1 = swap_int32(data_recv.op1);
      data_recv_other_endianness.op2 = swap_int32(data_recv.op2);

      switch (data_recv_other_endianness.op) {
      case op_add:
        data_other_endianness.res = data_recv_other_endianness.op1 + data_recv_other_endianness.op2;
        break;
      case op_mult:
        data_other_endianness.res = data_recv_other_endianness.op1 * data_recv_other_endianness.op2;
        break;
      case op_div:
        data_other_endianness.res = data_recv_other_endianness.op1 / data_recv_other_endianness.op2;
        break;
      case op_subst:
        data_other_endianness.res = data_recv_other_endianness.op1 - data_recv_other_endianness.op2;
        break;
      }
      data.res = swap_int32(data_other_endianness.res);

      bytes_received = 0;
    }
  }
  else if (event == I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED) {
      bytes_received = 0;
  }
  
  if (event & I2C_EVENT_SLAVE_STOP_DETECTED) {
    I2C->CR2 |= I2C_CR2_ACK;
    bytes_sent = 0;
  }
  
}

void i2c_reinit(void) {
  //I2C Init
  I2C_SoftwareResetCmd(ENABLE);
  I2C_SoftwareResetCmd(DISABLE);
  I2C_Cmd(DISABLE);
  I2C_DeInit();
  I2C_Cmd(ENABLE);
  I2C_Init(I2C_MAX_STANDARD_FREQ,
     SLAVE_ADDRESS << 1,
     I2C_DUTYCYCLE_2, 
     I2C_ACK_CURR,
     I2C_ADDMODE_7BIT, 
     I2C_MAX_INPUT_FREQ
  );
  I2C_AcknowledgeConfig(I2C_ACK_CURR);
  //I2C_StretchClockCmd(DISABLE);
  I2C_ITConfig((I2C_IT_TypeDef)(I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF), ENABLE);
}

void main(void)
{
  //Clock init
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); //16 MHz is funnier than 2

  //LED Init
  GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_OUT_PP_LOW_SLOW);

  //Init I2C
  i2c_reinit();
  
  enableInterrupts();
  
  while (1);
  
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


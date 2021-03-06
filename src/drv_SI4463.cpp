/**
  ******************************************************************************
  * @author  泽耀科技 ASHINING
  * @version V3.0
  * @date    2016-10-08
  * @brief   SI4463配置C文件
  ******************************************************************************
  * @attention
  *
  * 官网	:	http://www.ashining.com
  * 淘宝	:	https://shop105912646.taobao.com
  * 阿里巴巴:	https://cdzeyao.1688.com
  ******************************************************************************
  */

#include "drv_SI446x.h"
#include <SPI.h>

const static uint8_t config_table[] = RADIO_CONFIGURATION_DATA_ARRAY;

void SI_SET_CSN_LOW()
{
  digitalWrite(SI_CSN_PIN, LOW);
}

void SI_SET_CSN_HIGH()
{
  digitalWrite(SI_CSN_PIN, HIGH);
}

void SI_SET_SDN_HIGH()
{
  digitalWrite(SI_SDN_PIN, HIGH);
}

void SI_SET_SDN_LOW()
{
  digitalWrite(SI_SDN_PIN, LOW);
}

uint8_t drv_spi_read_write_byte(uint8_t data)
{
  return SPI.transfer(data);
}

void drv_delay_us(uint32_t us)
{
  delayMicroseconds(us);
}

/**
  * @brief :SI446x引脚初始化
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Gpio_Init(void)
{
  pinMode(SI_CSN_PIN, OUTPUT);
  pinMode(SI_SDN_PIN, OUTPUT);
  pinMode(SI_IRQ_PIN, INPUT_PULLUP);
  digitalWrite(SI_CSN_PIN, HIGH);
  digitalWrite(SI_SDN_PIN, LOW);
}

/**
  * @brief :SI446x等待CTS状态
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Wait_Cts(void)
{
  uint8_t l_Cts;
  uint16_t l_ReadCtsTimes = 0;

  do
  {
    SI_SET_CSN_LOW();

    //读CTS状态
    drv_spi_read_write_byte(READ_CMD_BUFF);
    l_Cts = drv_spi_read_write_byte(0xFF);

    SI_SET_CSN_HIGH();

    if (1000 == l_ReadCtsTimes++)
    {
      SI446x_Init();
      break;
    }

  } while (l_Cts != 0xFF); //直到读CTS的返回值等于0xFF
}

/**
  * @brief :SI446x写命令
  * @param :
  *			@pCmd:命令首地址
  *			@CmdNumber：命令个数
  * @note  :无
  * @retval:无
  */
void SI446x_Write_Cmds(uint8_t *pCmd, uint8_t CmdNumber)
{
  SI446x_Wait_Cts();

  SI_SET_CSN_LOW();

  while (CmdNumber--)
  {
    drv_spi_read_write_byte(*pCmd);
    pCmd++;
  }

  SI_SET_CSN_HIGH();
}

/**
  * @brief :SI446x POWER_UP
  * @param :
  *			@Xo_Freq:晶振频率
  * @note  :SI446x复位之后需要调用
  * @retval:无
  */
void SI446x_Power_Up(uint32_t Xo_Freq)
{
  uint8_t l_Cmd[7] = {0};

  l_Cmd[0] = POWER_UP;
  l_Cmd[1] = 0x01;
  l_Cmd[2] = 0x00;
  l_Cmd[3] = Xo_Freq >> 24;
  l_Cmd[4] = Xo_Freq >> 16;
  l_Cmd[5] = Xo_Freq >> 8;
  l_Cmd[6] = Xo_Freq;
  SI446x_Write_Cmds(l_Cmd, 7);
}

/**
  * @brief :SI446x读CTS和命令应答
  * @param :
  *			@pRead:返回数据首地址
  *			@Length:长度
  * @note  :无
  * @retval:无
  */
void SI446x_Read_Response(uint8_t *pRead, uint8_t Length)
{
  SI446x_Wait_Cts();
  SI_SET_CSN_LOW();

  drv_spi_read_write_byte(READ_CMD_BUFF);
  while (Length--)
  {
    *pRead = drv_spi_read_write_byte(0xFF);
    pRead++;
  }

  SI_SET_CSN_HIGH();
}

/**
  * @brief :SI446x空操作
  * @param :无
  * @note  :无
  * @retval:无
  */
uint8_t SI446x_Nop(void)
{
  uint8_t l_Cts;

  SI_SET_CSN_LOW();

  l_Cts = drv_spi_read_write_byte(NOP);

  SI_SET_CSN_HIGH();

  return l_Cts;
}

/**
  * @brief :SI446x读设备基本信息
  * @param :
  *			@pRead:返回数据首地址
  * @note  :无
  * @retval:无
  */
void SI446x_Get_Part_Informatoin(uint8_t *pRead)
{
  uint8_t l_Cmd = PART_INFO;

  SI446x_Write_Cmds(&l_Cmd, 1);
  SI446x_Read_Response(pRead, 9);
}

/**
  * @brief :SI446x读设备功能版本信息
  * @param :
  *			@pRead:返回数据首地址
  * @note  :无
  * @retval:无
  */
void SI446x_Get_Fun_Informatoin(uint8_t *pRead)
{
  uint8_t l_Cmd = FUNC_INFO;

  SI446x_Write_Cmds(&l_Cmd, 1);
  SI446x_Read_Response(pRead, 7);
}

/**
  * @brief :SI446x读中断状态
  * @param :
  *			@pRead:返回数据首地址
  * @note  :无
  * @retval:无
  */
void SI446x_Interrupt_Status(uint8_t *pRead)
{
  uint8_t l_Cmd[4] = {0};

  l_Cmd[0] = GET_INT_STATUS;
  l_Cmd[1] = 0;
  l_Cmd[2] = 0;
  l_Cmd[3] = 0;

  SI446x_Write_Cmds(l_Cmd, 4);
  SI446x_Read_Response(pRead, 9);
}

/**
  * @brief :SI446x读取属性值
  * @param :
  *			@Group_Num:属性组(参考SI446X_PROPERTY)
  *			@Num_Props:读取的属性个数
  *			@pRead:返回数据首地址
  * @note  :无
  * @retval:无
  */
void SI446x_Get_Property(SI446X_PROPERTY Group_Num, uint8_t Num_Props, uint8_t *pRead)
{
  uint8_t l_Cmd[4] = {0};

  l_Cmd[0] = GET_PROPERTY;
  l_Cmd[1] = Group_Num >> 8;
  l_Cmd[2] = Num_Props;
  l_Cmd[3] = Group_Num;

  SI446x_Write_Cmds(l_Cmd, 4);
  SI446x_Read_Response(pRead, Num_Props + 1);
}

/**
  * @brief :SI446x设置属性
  * @param :
  *			@Group_Num:属性组(参考SI446X_PROPERTY)
  *			@Num_Props:设置的属性个数
  *			@pWrite:写地址设备
  * @note  :无
  * @retval:无
  */
void SI446x_Set_Property(SI446X_PROPERTY Group_Num, uint8_t Num_Props, uint8_t *pWrite)
{
  uint8_t l_Cmd[20] = {0}, i = 0;

  if (Num_Props >= 16)
  {
    return; //数量不大于16
  }

  l_Cmd[i++] = SET_PROPERTY;
  l_Cmd[i++] = Group_Num >> 8;
  l_Cmd[i++] = Num_Props;
  l_Cmd[i++] = Group_Num;

  while (Num_Props--)
  {
    l_Cmd[i++] = *pWrite;
    pWrite++;
  }
  SI446x_Write_Cmds(l_Cmd, i);
}

/**
  * @brief :SI446x设置属性组1属性
  * @param :
  *			@Group_Num:属性组
  *			@Start_Prop:开始设置的属性号(参考SI446X_PROPERTY)
  * @note  :无
  * @retval:无
  */
void SI446x_Set_Property_1(SI446X_PROPERTY Group_Num, uint8_t Start_Prop)
{
  uint8_t l_Cmd[5] = {0};

  l_Cmd[0] = SET_PROPERTY;
  l_Cmd[1] = Group_Num >> 8;
  l_Cmd[2] = 1;
  l_Cmd[3] = Group_Num;
  l_Cmd[4] = Start_Prop;

  SI446x_Write_Cmds(l_Cmd, 5);
  SI446x_Read_Response(l_Cmd, 1);
}

/**
  * @brief :SI446x读取属性组1属性
  * @param :
  *			@Group_Num:开始的属性号(参考SI446X_PROPERTY)
  * @note  :无
  * @retval:无
  */
uint8_t SI446x_Get_Property_1(SI446X_PROPERTY Group_Num)
{
  uint8_t l_Cmd[4] = {0};

  l_Cmd[0] = GET_PROPERTY;
  l_Cmd[1] = Group_Num >> 8;
  l_Cmd[2] = 1;
  l_Cmd[3] = Group_Num;
  SI446x_Write_Cmds(l_Cmd, 4);

  SI446x_Read_Response(l_Cmd, 2);

  return l_Cmd[1];
}

/**
  * @brief :SI446x复位
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Reset(void)
{
  SI_SET_SDN_HIGH(); //关设备
  drv_delay_us(20);  //延时 等待设备完全断电
  SI_SET_SDN_LOW();  //开设备
  SI_SET_CSN_HIGH(); //取消SPI片选
  //	drv_delay_us( 35 );
}

/**
  * @brief :SI446x配置GPIO
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Config_Gpio(uint8_t Gpio_0, uint8_t Gpio_1, uint8_t Gpio_2, uint8_t Gpio_3, uint8_t Irq, uint8_t Sdo, uint8_t Gen_Config)
{
  uint8_t l_Cmd[10] = {0};

  l_Cmd[0] = GPIO_PIN_CFG;
  l_Cmd[1] = Gpio_0;
  l_Cmd[2] = Gpio_1;
  l_Cmd[3] = Gpio_2;
  l_Cmd[4] = Gpio_3;
  l_Cmd[5] = Irq;
  l_Cmd[6] = Sdo;
  l_Cmd[7] = Gen_Config;

  SI446x_Write_Cmds(l_Cmd, 8);
  SI446x_Read_Response(l_Cmd, 8);
}

/**
  * @brief :SI446x模块配置
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Config_Init(void)
{
  uint8_t i;
  uint16_t j = 0;

  while ((i = config_table[j]) != 0)
  {
    j += 1;
    SI446x_Write_Cmds((uint8_t *)config_table + j, i);
    j += i;
  }
#if PACKET_LENGTH > 0 //固定数据长度

  SI446x_Set_Property_1(PKT_FIELD_1_LENGTH_7_0, PACKET_LENGTH);

#else //动态数据长度

  SI446x_Set_Property_1(PKT_CONFIG1, 0x00);
  SI446x_Set_Property_1(PKT_CRC_CONFIG, 0x00);
  SI446x_Set_Property_1(PKT_LEN_FIELD_SOURCE, 0x01);
  SI446x_Set_Property_1(PKT_LEN, 0x2A);
  SI446x_Set_Property_1(PKT_LEN_ADJUST, 0x00);
  SI446x_Set_Property_1(PKT_FIELD_1_LENGTH_12_8, 0x00);
  SI446x_Set_Property_1(PKT_FIELD_1_LENGTH_7_0, 0x01);
  SI446x_Set_Property_1(PKT_FIELD_1_CONFIG, 0x00);
  SI446x_Set_Property_1(PKT_FIELD_1_CRC_CONFIG, 0x00);
  SI446x_Set_Property_1(PKT_FIELD_2_LENGTH_12_8, 0x00);
  SI446x_Set_Property_1(PKT_FIELD_2_LENGTH_7_0, MAX_PACKET_LENGTH);
  SI446x_Set_Property_1(PKT_FIELD_2_CONFIG, 0x00);
  SI446x_Set_Property_1(PKT_FIELD_2_CRC_CONFIG, 0x00);

#endif

  //4463 的GDO2 GDO3控制射频开关 33 32
  // 发射：GDO2 = 0, GDO3 = 1
  // 接收：GDO2 = 1, GDO3 = 0
  SI446x_Config_Gpio(0, 0, 33 | 0x40, 32 | 0x40, 0, 0, 0); //4463才需要配置
}

void SI446x_Set_Packet_Variable_Length(uint8_t length)
{
#if PACKET_LENGTH == 0 //固定数据长度
  SI446x_Set_Property_1(PKT_FIELD_2_LENGTH_7_0, length);
#endif
}

/**
  * @brief :SI446x写TX FIFO
  * @param :
  *			@pWriteData：写数据首地址
  *			@Length：数据长度
  * @note  :无
  * @retval:无
  */
void SI446x_Write_TxFifo(uint8_t *pWriteData, uint8_t Length)
{
  SI_SET_CSN_LOW();
  drv_spi_read_write_byte(WRITE_TX_FIFO);
  while (Length--)
  {
    drv_spi_read_write_byte(*pWriteData++);
  }
  SI_SET_CSN_HIGH();
}

/**
  * @brief :SI446x 复位RX FIFO
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Reset_RxFifo(void)
{
  uint8_t l_Cmd[2] = {0};

  l_Cmd[0] = FIFO_INFO;
  l_Cmd[1] = 0x02;
  SI446x_Write_Cmds(l_Cmd, 2);
}

/**
  * @brief :SI446x 复位TX FIFO
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Reset_TxFifo(void)
{
  uint8_t l_Cmd[2] = {0};

  l_Cmd[0] = FIFO_INFO;
  l_Cmd[1] = 0x02;
  SI446x_Write_Cmds(l_Cmd, 2);
}

/**
  * @brief :SI446x发送数据包
  * @param :
  *			@pTxData：发送数据首地址
  *			@Length：数据长度
  *			@Channel：通道
  *			@Condition：发送状况选择
  * @note  :无
  * @retval:无
  */
void SI446x_Send_Packet(uint8_t *pTxData, uint8_t Length, uint8_t Channel, uint8_t Condition)
{
  uint8_t l_Cmd[5] = {0};
  // uint8_t tx_len = Length;

  // SI446x_Reset_TxFifo(); //清空TX FIFO

  SI446x_Set_Packet_Variable_Length(Length);

  SI_SET_CSN_LOW();

  drv_spi_read_write_byte(WRITE_TX_FIFO);

#if PACKET_LENGTH == 0 //动态数据长度

  // tx_len++;
  drv_spi_read_write_byte(Length);

#endif

  while (Length--)
  {
    drv_spi_read_write_byte(*pTxData++); //写数据到TX FIFO
  }

  SI_SET_CSN_HIGH();

  l_Cmd[0] = START_TX;
  l_Cmd[1] = Channel;
  l_Cmd[2] = Condition;
  l_Cmd[3] = 0;
  l_Cmd[4] = PACKET_LENGTH;

  SI446x_Write_Cmds(l_Cmd, 5); //发送数据包
  SI446x_Read_Response(l_Cmd, 1);
  SI446x_Set_Packet_Variable_Length(MAX_PACKET_LENGTH);
}

/**
  * @brief :SI446x启动发送
  * @param :
  *			@Length：数据长度
  *			@Channel：通道
  *			@Condition：发送状况选择
  * @note  :无
  * @retval:无
  */
void SI446x_Start_Tx(uint8_t Channel, uint8_t Condition, uint16_t Length)
{
  uint8_t l_Cmd[5] = {0};

  l_Cmd[0] = START_TX;
  l_Cmd[1] = Channel;
  l_Cmd[2] = Condition;
  l_Cmd[3] = Length >> 8;
  l_Cmd[4] = Length;

  SI446x_Write_Cmds(l_Cmd, 5);
}

/**
  * @brief :SI446x读RX FIFO数据
  * @param :
  *			@pRxData：数据首地址
  * @note  :无
  * @retval:数据个数
  */
uint8_t SI446x_Read_Packet(uint8_t *pRxData)
{
  uint8_t length = 0, i = 0;

  SI446x_Wait_Cts();
  SI_SET_CSN_LOW();

  drv_spi_read_write_byte(READ_RX_FIFO);

#if PACKET_LENGTH == 0

  length = drv_spi_read_write_byte(0xFF);

#else

  length = PACKET_LENGTH;

#endif
  i = length;
  if (length > MAX_PACKET_LENGTH)
  {
    SI_SET_CSN_HIGH();
    SI446x_Reset_RxFifo();
    sprintf((char *)pRxData, "Invalid packet: %d, %c\n", length, length);
    return 0;
    // return strlen((char *)pRxData);
  }

  while (length--)
  {
    *pRxData++ = drv_spi_read_write_byte(0xFF);
  }

  SI_SET_CSN_HIGH();

  return i;
}

/**
  * @brief :SI446x启动接收
  * @param :
  *			@Channel：通道
  *			@Condition：开始接收状态
  *			@Length：接收长度
  *			@Next_State1：下一个状态1
  *			@Next_State2：下一个状态2
  *			@Next_State3：下一个状态3
  * @note  :无
  * @retval:无
  */
void SI446x_Start_Rx(uint8_t Channel, uint8_t Condition, uint16_t Length, uint8_t Next_State1, uint8_t Next_State2, uint8_t Next_State3)
{
  uint8_t l_Cmd[8] = {0};

  SI446x_Reset_RxFifo();
  // SI446x_Reset_TxFifo();

  l_Cmd[0] = START_RX;
  l_Cmd[1] = Channel;
  l_Cmd[2] = Condition;
  l_Cmd[3] = Length >> 8;
  l_Cmd[4] = Length;
  l_Cmd[5] = Next_State1;
  l_Cmd[6] = Next_State2;
  l_Cmd[7] = Next_State3;

  SI446x_Write_Cmds(l_Cmd, 8);
}

/**
  * @brief :SI446x读取当前数据包信息
  * @param :
  *			@pReadData：数据存放地址
  *			@FieldNumMask：掩码域
  *			@Length：长度
  *			@DiffLen：不同长度
  * @note  :无
  * @retval:无
  */
void SI446x_Get_Packet_Information(uint8_t *pReadData, uint8_t FieldNumMask, uint16_t Length, uint16_t DiffLen)
{
  uint8_t l_Cmd[6] = {0};

  l_Cmd[0] = PACKET_INFO;
  l_Cmd[1] = FieldNumMask;
  l_Cmd[2] = Length >> 8;
  l_Cmd[3] = Length;
  l_Cmd[4] = DiffLen >> 8;
  l_Cmd[5] = DiffLen;

  SI446x_Write_Cmds(l_Cmd, 6);
  SI446x_Read_Response(pReadData, 3);
}

/**
  * @brief :SI446x读取FIFO状态
  * @param :
  *			@pReadData：数据存放地址
  * @note  :无
  * @retval:无
  */
void SI446x_Get_Fifo_Information(uint8_t *pReadData)
{
  uint8_t l_Cmd[2] = {0};

  l_Cmd[0] = FIFO_INFO;
  l_Cmd[1] = 0x03;

  SI446x_Write_Cmds(l_Cmd, 2);
  SI446x_Read_Response(pReadData, 3);
}

void SI446x_Get_Modem_Status(uint8_t *pReadData)
{
  uint8_t l_Cmd[2] = {0};

  l_Cmd[0] = GET_MODEM_STATUS;

  SI446x_Write_Cmds(l_Cmd, 1);
  SI446x_Read_Response(pReadData, 9);
}

/**
  * @brief :SI446x状态切换
  * @param :
  *			@NextStatus：下一个状态
  * @note  :无
  * @retval:无
  */
void SI446x_Change_Status(uint8_t NextStatus)
{
  uint8_t l_Cmd[2] = {0};

  l_Cmd[0] = CHANGE_STATE;
  l_Cmd[1] = NextStatus;

  SI446x_Write_Cmds(l_Cmd, 2);
}

/**
  * @brief :SI446x获取设备当前状态
  * @param :
  * @note  :无
  * @retval:设备当前状态
  */
uint8_t SI446x_Get_Device_Status(void)
{
  uint8_t l_Cmd[3] = {0};

  l_Cmd[0] = REQUEST_DEVICE_STATE;

  SI446x_Write_Cmds(l_Cmd, 1);
  SI446x_Read_Response(l_Cmd, 3);

  return l_Cmd[1] & 0x0F;
}

void SI446x_Get_Chip_Status(uint8_t *pReadData)
{
  uint8_t l_Cmd[3] = {0};

  l_Cmd[0] = GET_CHIP_STATUS;

  SI446x_Write_Cmds(l_Cmd, 1);
  SI446x_Read_Response(pReadData, 4);
}

/**
  * @brief :SI446x功率设置
  * @param :
  *			@PowerLevel：数据存放地址
  * @note  :无
  * @retval:设备当前状态
  */
void SI446x_Set_Power(uint8_t PowerLevel)
{
  SI446x_Set_Property_1(PA_PWR_LVL, PowerLevel);
}

/**
  * @brief :SI446x初始化
  * @param :无
  * @note  :无
  * @retval:无
  */
void SI446x_Init(void)
{
  SI446x_Gpio_Init();        //SI4463引脚初始化
  SI446x_Reset();            //SI4463复位
  SI446x_Power_Up(30000000); //reset 后需要Power up设备 晶振30MHz
  SI446x_Config_Init();      //SI4463模块初始化
  SI446x_Set_Power(0x7F);    //功率设置
  SI446x_Change_Status(6);   //切换到RX状态
  while (6 != SI446x_Get_Device_Status())
    ;
  SI446x_Start_Rx(0, 0, PACKET_LENGTH, 0, 0, 0);
}

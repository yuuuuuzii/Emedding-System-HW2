/**
  ******************************************************************************
  * @file    Wifi/WiFi_Client_Server/src/main.c
  * @author  MCD Application Team
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lsm6dsl.h"

/* Private defines -----------------------------------------------------------*/

#define TERMINAL_USE
#define LSM6DSL_ACC_GYRO_SM_THS             0x13

/* Update SSID and PASSWORD with own Access point settings */
#define SSID     "YOUR_SSID"
#define PASSWORD "YOUR_PASSWORD"


uint8_t RemoteIP[] = {172, 20, 10, 3}; // cell phone ipv4 , separate

#define RemotePORT 8002 // TCP server

#define WIFI_WRITE_TIMEOUT 10000
#define WIFI_READ_TIMEOUT  10000

#define CONNECTION_TRIAL_MAX          10

#if defined (TERMINAL_USE)
#define TERMOUT(...)  printf(__VA_ARGS__)
#else
#define TERMOUT(...)
#endif



/* Private variables ---------------------------------------------------------*/
#if defined (TERMINAL_USE)
extern UART_HandleTypeDef hDiscoUart;
#endif /* TERMINAL_USE */
static uint8_t RxData [500];
static volatile int Flag = 0;

#define ERR_FORMAT  (-1)
#define ERR_SEND    (-2)

/* Private function prototypes -----------------------------------------------*/
#if defined (TERMINAL_USE)
#ifdef __GNUC__
/* With GCC, small TERMOUT (option LD Linker->Libraries->Small TERMOUT
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
#endif /* TERMINAL_USE */

static void SystemClock_Config(void);



extern  SPI_HandleTypeDef hspi;

/* Private functions ---------------------------------------------------------*/
static int Send_WIFI_Data(int socket);
static void Custom_SIG_MOTION_Init(void);
static void SIG_MOTION_EXTI_Init(void);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  uint8_t  MAC_Addr[6] = {0};
  uint8_t  IP_Addr[4] = {0};
//  uint8_t TxData[] = "STM32 : Hello!\n";
  int32_t Socket = -1;

  uint16_t Datalen;
  int32_t ret;
  int16_t Trials = CONNECTION_TRIAL_MAX;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();
  /* Configure LED2 */
  BSP_LED_Init(LED2);
  BSP_ACCELERO_Init();
  Custom_SIG_MOTION_Init();
  SIG_MOTION_EXTI_Init();


#if defined (TERMINAL_USE)
  /* Initialize all configured peripherals */
  hDiscoUart.Instance = DISCOVERY_COM1; //board name UART1 (terminal out)
  hDiscoUart.Init.BaudRate = 115200;
  hDiscoUart.Init.WordLength = UART_WORDLENGTH_8B;
  hDiscoUart.Init.StopBits = UART_STOPBITS_1;
  hDiscoUart.Init.Parity = UART_PARITY_NONE;
  hDiscoUart.Init.Mode = UART_MODE_TX_RX;
  hDiscoUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hDiscoUart.Init.OverSampling = UART_OVERSAMPLING_16;
  hDiscoUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hDiscoUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  BSP_COM_Init(COM1, &hDiscoUart);
#endif /* TERMINAL_USE */

  TERMOUT("****** WIFI Module in TCP Client mode demonstration ****** \n\n");
  TERMOUT("TCP Client Instructions :\n");
  TERMOUT("1- Make sure your Phone is connected to the same network that\n");
  TERMOUT("   you configured using the Configuration Access Point.\n");
  TERMOUT("2- Create a server by using the android application TCP Server\n");
  TERMOUT("   with port(8002).\n");
  TERMOUT("3- Get the Network Name or IP Address of your Android from the step 2.\n\n");


//  uint8_t read_value;
//
//  while(1){
//	  read_value = SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_FUNC_SRC);
//	  if((read_value & 0x40)){
//		  TERMOUT("Move");
//	  }
//	  else{
//		  TERMOUT("Static");
//	  }
//	  HAL_Delay(100);
//  }


  /*Initialize  WIFI module */
  if(WIFI_Init() ==  WIFI_STATUS_OK)
  {
    TERMOUT("> WIFI Module Initialized.\n");
    if(WIFI_GetMAC_Address(MAC_Addr, sizeof(MAC_Addr)) == WIFI_STATUS_OK)
    {
      TERMOUT("> es-wifi module MAC Address : %X:%X:%X:%X:%X:%X\n",
              MAC_Addr[0],
              MAC_Addr[1],
              MAC_Addr[2],
              MAC_Addr[3],
              MAC_Addr[4],
              MAC_Addr[5]);
    }
    else
    {
      TERMOUT("> ERROR : CANNOT get MAC address\n");
      BSP_LED_On(LED2);
    }

    if( WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) == WIFI_STATUS_OK)
    {
      TERMOUT("> es-wifi module connected \n");
      if(WIFI_GetIP_Address(IP_Addr, sizeof(IP_Addr)) == WIFI_STATUS_OK)
      {
        TERMOUT("> es-wifi module got IP Address : %d.%d.%d.%d\n",
                IP_Addr[0],
                IP_Addr[1],
                IP_Addr[2],
                IP_Addr[3]);

        TERMOUT("> Trying to connect to Server: %d.%d.%d.%d:%d ...\n",
                RemoteIP[0],
                RemoteIP[1],
                RemoteIP[2],
                RemoteIP[3],
                RemotePORT);

        while (Trials--)//try connection Trials times.
        {//TCP
          if( WIFI_OpenClientConnection(0, WIFI_TCP_PROTOCOL, "TCP_CLIENT", RemoteIP, RemotePORT, 0) == WIFI_STATUS_OK)
          {
            TERMOUT("> TCP Connection opened successfully.\n");
            Socket = 0;
            break;
          }
        }


        if(Socket == -1)
        {
          TERMOUT("> ERROR : Cannot open Connection\n");
          BSP_LED_On(LED2);
        }
      }
      else
      {
        TERMOUT("> ERROR : es-wifi module CANNOT get IP address\n");
        BSP_LED_On(LED2);
      }
    }
    else
    {
      TERMOUT("> ERROR : es-wifi module NOT connected\n");
      BSP_LED_On(LED2);
    }
  }
  else
  {
    TERMOUT("> ERROR : WIFI Module cannot be initialized.\n");
    BSP_LED_On(LED2);
  }

  while(1)
  {
    if(Socket != -1)
    {
      ret = WIFI_ReceiveData(Socket, RxData, sizeof(RxData)-1, &Datalen, WIFI_READ_TIMEOUT);

      if(ret == WIFI_STATUS_OK)
      {
        if(Datalen > 0)
        {
          RxData[Datalen] = 0;

          if(RxData[0] == 's')
          {
        	  int err = Send_WIFI_Data(Socket);

			  if(err < 0){
				  TERMOUT("> ERROR : Failed to Send Data, connection closed, reason : %d\n", err);
				  break;
			  }
          }

        }
      }
      else
      {
        TERMOUT("> ERROR : Failed to Receive Data, connection closed\n");
        break;
      }
    }
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */

static int Send_WIFI_Data(int Socket){

	int16_t pDataXYZ[3];
	uint16_t Datalen;
	uint8_t transmit_data[128]; //8 bit;
	uint8_t Sig_detect_message[] = "EVENT\n";
	uint32_t cur_time;

    while(1)
    {

		BSP_ACCELERO_AccGetXYZ(pDataXYZ);

		TERMOUT("Accelero X: %d Y:%d Z:%d\n\n", pDataXYZ[0], pDataXYZ[1], pDataXYZ[2]);
		cur_time = HAL_GetTick();

		int len = snprintf((char *)transmit_data, sizeof(transmit_data), "%d, %d, %d, %lu\n",
						 pDataXYZ[0], pDataXYZ[1], pDataXYZ[2], (unsigned long)cur_time); // return a size not contain \0

		if(len < 0 || (size_t)len >= sizeof(transmit_data)){
		  TERMOUT("Snprintf Buffer overflow\n");
		  return ERR_FORMAT;
		}

		int32_t err = WIFI_SendData(Socket, transmit_data, len, &Datalen, WIFI_WRITE_TIMEOUT);

		if (err != WIFI_STATUS_OK)
		{
		TERMOUT("WIFI_SendData error[ACCELERO]\n");
		return ERR_SEND; // -2 means sending error;
		}

    	if(Flag){
    		SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_FUNC_SRC);
    		Flag = 0;
    		err = WIFI_SendData(Socket, Sig_detect_message, strlen((char *)Sig_detect_message), &Datalen, WIFI_WRITE_TIMEOUT);

    		if (err != WIFI_STATUS_OK)
    		{
				TERMOUT("WIFI_SendData error[MOTION]\n");
				return ERR_SEND; // -2 means sending error;
    		}

    	}

		HAL_Delay(50);
    }

    return 0;
}


static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* MSI is enabled after System reset, activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 7;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
}

#if defined (TERMINAL_USE)
/**
  * @brief  Retargets the C library TERMOUT function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  if(ch == '\n')
  {
    uint8_t cr = '\r';
    HAL_UART_Transmit(&hDiscoUart, (uint8_t *)&cr, 1, 0xFFFF);
  }

  HAL_UART_Transmit(&hDiscoUart, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}
#endif /* TERMINAL_USE */

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: TERMOUT("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case (GPIO_PIN_1):
    {
      SPI_WIFI_ISR();
      break;
    }
    case (GPIO_PIN_11):
	{
    	Flag = 1;
	}
    default:
    {
      break;
    }
  }
}

void SPI3_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi);
}

void EXTI15_10_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
}

static void Custom_SIG_MOTION_Init(void){

	/* Write FUNC_CFG_ACCESS */
	uint8_t tmp;

	tmp = 0x80;
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_FUNC_CFG_ACCESS, tmp);

	/* Write SM_THS  */
	tmp = 0x08;
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_SM_THS, tmp);

	/* Clear FUNC_CFG_ACCESS */
	tmp = 0x00;
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_FUNC_CFG_ACCESS, tmp);

	/* Write CTRL10_C */
	tmp = 0x05; // FUNC_EN and SIGN_MOTION_EN
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_CTRL10_C, tmp);

	/* Write TAP_CFG to set latched mode */
	tmp = SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_TAP_CFG1);
	tmp |= 0x01;
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_TAP_CFG1, tmp);

	/* Clear FUNC_SRC */
	SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_FUNC_SRC);

	/* Write INT1_CTRL */
	tmp = 0x40; // INT1_SIGN_MOT
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_INT1_CTRL, tmp);

}


static void SIG_MOTION_EXTI_Init(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	/* Enable clock */
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/* Configure Button pin as input with External interrupt */
	gpio_init_structure.Pin = GPIO_PIN_11;
	gpio_init_structure.Pull = GPIO_PULLDOWN;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	gpio_init_structure.Mode = GPIO_MODE_IT_RISING;

	HAL_GPIO_Init(GPIOD, &gpio_init_structure);

	/* Enable and set Button EXTI Interrupt to the lowest priority */
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0x0F, 0x00);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}


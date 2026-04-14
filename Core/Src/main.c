/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

// Change Heap size in FLASH.ld and RAM.ld to 0x600
// To transmit through USB, call CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
// To receive, modify  CDC_Receive_FS in usbd_cdc_if.c,
//
// static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
// {
//   /* USER CODE BEGIN 6 */

  // Example: Echo the data back to the PC
//   CDC_Transmit_FS(Buf, *Len);

  // Or: Set a flag to process this buffer in your main loop
  // My_Custom_Receive_Handler(Buf, *Len);

//   USBD_CDC_SetRxBuffer(&hUsbDeviceFS, Buf);
//   USBD_CDC_ReceivePacket(&hUsbDeviceFS);
//   return (USBD_OK);
//   /* USER CODE END 6 */
// }


/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "dma_printf.h"
#include "dma_scanf.h"
#include "log.h"
#include "w25q_mem.h"
#include "project_info.h"

#include "lfs.h"
#include "littlefs_qspi.h"
#include "usbd_cdc_if.h" // Include the CDC interface header
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RTC_CODE 0xAA55
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

FDCAN_HandleTypeDef hfdcan1;

QSPI_HandleTypeDef hqspi;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_tx;
DMA_HandleTypeDef hdma_usart3_rx;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uartTask */
osThreadId_t uartTaskHandle;
const osThreadAttr_t uartTask_attributes = {
  .name = "uartTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for timeTask */
osThreadId_t timeTaskHandle;
const osThreadAttr_t timeTask_attributes = {
  .name = "timeTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */
FDCAN_TxHeaderTypeDef   TxHeader;
uint8_t                 TxData[8];

FDCAN_RxHeaderTypeDef   RxHeader;
uint8_t                 RxData[8];

uint32_t                TxMailbox;

uint8_t flag = 0;

int number;

extern struct lfs_config qspi_lfs_cfg;

// Declare LFS as global variable so it can used in different functions.
lfs_t lfs;


  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_RTC_Init(void);
static void MX_QUADSPI_Init(void);
void StartDefaultTask(void *argument);
void StartUartTask(void *argument);
void StartTimeTask(void *argument);

/* USER CODE BEGIN PFP */
void Set_RTC_Time(void);
void Set_RTC_Date(void);
void run_menu1(void);
void run_menu2(void);

uint8_t bin_to_packed_bcd(uint8_t binary_value);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



int __io_putchar(int ch)
{
  dma_printf_putc(ch&0xFF);
  return ch;
}

int __io_getchar(void)
{
  return dma_scanf_getc_blocking();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
  dma_printf_send_it(huart);
}

/**
  * @brief  Rx FIFO 0 callback.
  * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  RxFifo0ITs: indicates which Rx FIFO 0 interrupts are signalled.
  *                     This parameter can be any combination of @arg FDCAN_Rx_Fifo0_Interrupts.
  * @retval None
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
    {
    /* Reception Error */
    Error_Handler();
    }

#if 0
    /* Display LEDx */
    if ((RxHeader.Identifier == 0x321) && (RxHeader.IdType == FDCAN_STANDARD_ID) && (RxHeader.DataLength == FDCAN_DLC_BYTES_2))
    {
      LED_Display(RxData[0]);
      ubKeyNumber = RxData[0];
    }
#endif

    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
      /* Notification Error */
      Error_Handler();
    }
  }
}



void Set_RTC_Time(void)
{
	RTC_TimeTypeDef sTime = {0};
	uint8_t buffer[3];

	printf("Enter Hour (00-23): ");
	scanf("%s", buffer);
	sTime.Hours = (buffer[0] - '0')*10 + (buffer[1] - '0');

	printf("Enter Minute (00-59): ");
	scanf("%s", buffer);
	sTime.Minutes = (buffer[0] - '0')*10 + (buffer[1] - '0');

	printf("Enter Second (00-59): ");
	scanf("%s", buffer);
	sTime.Seconds = (buffer[0] - '0')*10 + (buffer[1] - '0');

	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
		printf("RTC SetTime Failed\n\r");
	else
		printf("RTC Time is Set to: %02d:%02d:%02d\n\r", sTime.Hours, sTime.Minutes, sTime.Seconds);

}



void Set_RTC_Date(void)
{
	RTC_DateTypeDef sDate = {0};
	uint8_t buffer[3];

	printf("Enter Year (00-99): ");
	scanf("%s", buffer);
	sDate.Year = (buffer[0] - '0')*10 + (buffer[1] - '0');

	printf("Enter Month (1-12): ");
	scanf("%s", buffer);
	sDate.Month = (buffer[0] - '0')*10 + (buffer[1] - '0');

	printf("Enter Day (1-31): ");
	scanf("%s", buffer);
	sDate.Date = (buffer[0] - '0')*10 + (buffer[1] - '0');

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
		printf("RTC SetDate Failed\n\r");
	else
		printf("RTC Date is Set to: %02d/%02d/%02d\n\r", sDate.Year, sDate.Month, sDate.Date);

}


void printHex(unsigned char *buffer, int buffer_size)
{
//    int buffer_size = sizeof(buffer);

    for (int i = 0; i < buffer_size; i++) {
        printf("%02X ", buffer[i]); // Print byte in hex, padded with 0 to 2 digits
        if ((i + 1) % 16 == 0) {    // Check if 16 bytes have been printed
            printf("\n\r");           // Print a newline
        }
    }
    // Print a final newline if the last line was not a full 16 bytes
    if (buffer_size % 16 != 0) {
        printf("\n\r");
    }
}

/**
 * @brief Converts a single 8-bit unsigned binary value to packed BCD.
 * @param binary_value The 8-bit unsigned binary value (0-255).
 * @return The packed BCD representation.
 *         The upper nibble contains the tens digit, the lower nibble contains the units digit.
 *         Returns 0xFF if the input is out of range (0-99).
 */
uint8_t bin_to_packed_bcd(uint8_t binary_value) {
    if (binary_value > 99) {
        // A single byte in packed BCD can only represent values from 0-99.
        return 0xFF; // Indicate an error or out-of-range value
    }

    uint8_t tens_digit = binary_value / 10;
    uint8_t units_digit = binary_value % 10;

    // Pack the BCD digits into a single byte
    return (tens_digit << 4) | units_digit;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  uint32_t CPU_Sn0, CPU_Sn1, CPU_Sn2;
  W25Q_STATE state;		// temp status variable

  //lfs_t lfs;
  int rc;

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  MX_FDCAN1_Init();
  MX_RTC_Init();
  MX_QUADSPI_Init();
  /* USER CODE BEGIN 2 */

  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_FDCAN_Start(&hfdcan1);

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  dma_printf_init(&huart3);
  dma_scanf_init(&huart3);

  printf("Project %s\n\r", PROJECT_NAME);
  printf("Build at %s %s\r\n", __DATE__, __TIME__);

  CPU_Sn0 = *(__IO uint32_t*)(0x1FF1E800);
  CPU_Sn1 = *(__IO uint32_t*)(0x1FF1E800 + 4);
  CPU_Sn2 = *(__IO uint32_t*)(0x1FF1E800 + 8);

  printf("\r\nCPU : STM32H743VITx, LQFP100, 主频: %ldMHz\r\n", SystemCoreClock / 1000000);
  printf("UID = %08lX %08lX %08lX\n\r", CPU_Sn2, CPU_Sn1, CPU_Sn0);


  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  state = W25Q_Init();
  if (state != W25Q_OK) {
	printf("W25Q initialization failed\n\r");
  }


  run_menu1();
  run_menu2();

//  lfs_format(&lfs, &qspi_lfs_cfg);

  rc = lfs_mount(&lfs, &qspi_lfs_cfg);
  if (rc != 0) {
	  printf("lfs_mount() failed, RC=%d\n\r", rc);
      rc = lfs_format(&lfs, &qspi_lfs_cfg);
	  printf("lfs_format() failed, RC=%d\n\r", rc);
      rc = lfs_mount(&lfs, &qspi_lfs_cfg);
	  printf("lfs_mount() failed, RC=%d\n\r", rc);

  } else {
	  printf("LFS_mount() success\n\r");
  }




  printf("Check MEM_CLR pin \n\r");
  // IF Button Is Pressed
  if(HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_15) == 0)
  {
    printf("Memory Clearing\n\r");
    HAL_PWR_EnableBkUpAccess();
	// Clear RTC Backup data Register 1
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0);
    HAL_PWR_DisableBkUpAccess();
  }






  printf("Before Config RTC\n\r");
  memset(&sTime, 0, sizeof(sTime));
  memset(&sDate, 0, sizeof(sDate));

  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  printf("RTC Time: %02d:%02d:%02d\n\r", sTime.Hours, sTime.Minutes, sTime.Seconds);
  printf("RTC Date: %02d/%02d/%02d\n\r", sDate.Date, sDate.Month, sDate.Year);

  // Check if Data stored in BackUp register1: No Need to write it and then turn LED1
  // Read the Back Up Register 1 Data
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != RTC_CODE)
  {


     printf("BKP_DR1 != %04X\n\r", RTC_CODE);
     // Write Back Up Register 1 Data
     HAL_PWR_EnableBkUpAccess();
     // Writes a data in a RTC Backup data Register 1
     HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_CODE);
     Set_RTC_Time();
     Set_RTC_Date();
     memset(&sTime, 0, sizeof(sTime));
     memset(&sDate, 0, sizeof(sDate));
     HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
     HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

     printf("RTC Time: %02d:%02d:%02d\n\r", sTime.Hours, sTime.Minutes, sTime.Seconds);
     printf("RTC Date: %02d/%02d/%02d\n\r", sDate.Month, sDate.Date, sDate.Year);
     HAL_PWR_DisableBkUpAccess();
  } else {
	  printf("BKP_DR1 == %04X\n\r", RTC_CODE);
  }

  memset(&sTime, 0, sizeof(sTime));
  memset(&sDate, 0, sizeof(sDate));
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  printf("RTC Time: %02d:%02d:%02d\n\r", sTime.Hours, sTime.Minutes, sTime.Seconds);
  printf("RTC Date: %02d/%02d/%02d\n\r", sDate.Month, sDate.Date, sDate.Year);




  lfs_file_t file;
  char filename[64];
  char lfs_buf[128];

  sprintf(filename, "hello_%02d%02d%02d_%02d%02d%02d.txt", sDate.Month, sDate.Date, sDate.Year, sTime.Hours, sTime.Minutes, sTime.Seconds);
  printf("File (%s) created\n\r", filename);
  lfs_file_open(&lfs, &file, filename, LFS_O_RDWR | LFS_O_CREAT);
  lfs_file_write(&lfs, &file, "Hello World", 11);
  lfs_file_close(&lfs, &file);

  lfs_file_open(&lfs, &file, "hello.txt", LFS_O_RDWR);
  lfs_file_read(&lfs, &file, lfs_buf, 11);
  lfs_file_close(&lfs, &file);


  for(int ii = 0; ii<11; ii++) {
	  printf("%c ", lfs_buf[ii]);
  }
  printf("\n\r");

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of uartTask */
  uartTaskHandle = osThreadNew(StartUartTask, NULL, &uartTask_attributes);

  /* creation of timeTask */
  timeTaskHandle = osThreadNew(StartTimeTask, NULL, &timeTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 120;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FDCAN|RCC_PERIPHCLK_USART3;
  PeriphClkInitStruct.PLL2.PLL2M = 1;
  PeriphClkInitStruct.PLL2.PLL2N = 30;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL2;
  PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */
////	  hfdcan1.Init.Mode = FDCAN_MODE_EXTERNAL_LOOPBACK;

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 48;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1 = 16;
  hfdcan1.Init.NominalTimeSeg2 = 3;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 16;
  hfdcan1.Init.DataTimeSeg2 = 3;
  hfdcan1.Init.MessageRAMOffset = 0;
  hfdcan1.Init.StdFiltersNbr = 1;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.RxFifo0ElmtsNbr = 1;
  hfdcan1.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.RxFifo1ElmtsNbr = 0;
  hfdcan1.Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.RxBuffersNbr = 0;
  hfdcan1.Init.RxBufferSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.TxEventsNbr = 0;
  hfdcan1.Init.TxBuffersNbr = 16;
  hfdcan1.Init.TxFifoQueueElmtsNbr = 16;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  hfdcan1.Init.TxElmtSize = FDCAN_DATA_BYTES_8;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 3;
  hqspi.Init.FifoThreshold = 4;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hqspi.Init.FlashSize = 24;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_3;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 240-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LD1_Pin|HB_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin HB_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|HB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
  uint32_t num =  number;
//  RTC_TimeTypeDef sTime = {0};
//  RTC_DateTypeDef sDate = {0};
  /* Infinite loop */
  for(;;)
  {
	  printf("Result: %lu\t", num);

//	  memset(&sTime, 0, sizeof(sTime));
//	  memset(&sDate, 0, sizeof(sDate));
//	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
//	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	  printf("%02d:%02d:%02d\t", sTime.Hours, sTime.Minutes, sTime.Seconds);
	  printf("%02d/%02d/%02d\n\r", sDate.Month, sDate.Date, sDate.Year);

#if 1
		  TxHeader.IdType = FDCAN_STANDARD_ID;
		  TxHeader.Identifier = 0x321;
		  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
		  TxHeader.DataLength = 3;
		  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
		  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
		  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
		  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
		  TxHeader.MessageMarker = 0;

		  TxData[0] = bin_to_packed_bcd(sTime.Seconds);
		  TxData[1] = bin_to_packed_bcd(sTime.Minutes);
		  TxData[2] = bin_to_packed_bcd(sTime.Hours);



		  HAL_StatusTypeDef status;
		  status = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
		  if (status != HAL_OK)
		  {
////			 HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
//			 printf("HAL_FDCAN_AddMessageToTxFifoQ: %d\n\r", status);
			 HAL_Delay(1000);
		     Error_Handler ();
		  }
		  num++;
#endif

	HAL_GPIO_TogglePin(HB_GPIO_Port, HB_Pin);
    osDelay(100);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartUartTask */
/**
* @brief Function implementing the uartTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUartTask */
void StartUartTask(void *argument)
{
  /* USER CODE BEGIN StartUartTask */
  char msg[80];
  uint32_t i;

  i = 0;
  /* Infinite loop */
  for(;;)
  {


	  sprintf(msg, "%04ld\t%02d:%02d:%02d\t%02d/%02d/%02d\n\r", i++, sTime.Hours, sTime.Minutes, sTime.Seconds, sDate.Month, sDate.Date, sDate.Year);


	// Transmit via USB
	// Parameters: (Buffer pointer, length)
	CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    osDelay(10);
  }
  /* USER CODE END StartUartTask */
}

/* USER CODE BEGIN Header_StartTimeTask */
/**
* @brief Function implementing the timeTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTimeTask */
void StartTimeTask(void *argument)
{
  /* USER CODE BEGIN StartTimeTask */
  /* Infinite loop */
  for(;;)
  {
    memset(&sTime, 0, sizeof(sTime));
	memset(&sDate, 0, sizeof(sDate));
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    osDelay(1000);
  }
  /* USER CODE END StartTimeTask */
}

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
	static uint32_t c;
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM3) {
    c++;
    if ( c > 99 ) {
//	  HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_0);
      HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin );

	  flag = 1;
	  c = 0;
    }
  }
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  // Example: Echo the data back to the PC
//   CDC_Transmit_FS(Buf, *Len);

  // Or: Set a flag to process this buffer in your main loop
  // My_Custom_Receive_Handler(Buf, *Len);

//   USBD_CDC_SetRxBuffer(&hUsbDeviceFS, Buf);
//   USBD_CDC_ReceivePacket(&hUsbDeviceFS);
//   return (USBD_OK);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

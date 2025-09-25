/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include ".\BSP\LCD\lcd.h"
#include ".\BSP\W25Q64\w25q64.h"
#include ".\BSP\delay\delay.h"
#include ".\BSP\Touch\touch.h"
#include ".\BSP\LED\led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void W25Q16_Test(void)
{
  uint32_t id = 0;
  uint8_t buf[512], recv[512];
  W25Q64_Init();
  W25Q64_ReadID(&id);
  printf("W25Q64 ID:%#x\r\n", id);
	
  HAL_Delay(10);
 for(int i = 0; i < 512; i++)
 {
   buf[i] = i;
 }
 HAL_Delay(100);  

 W25Q64_EraseSector(0x000000); // 擦除第0扇区
 W25Q64_PageWrite(0x000000, buf, 256); // 写入300字节
 W25Q64_ReadData(0x000000, recv, 256); // 读取300字节
 
 HAL_Delay(10);//加上必要延迟,避免卡死
 for(int i = 0; i < 300; i++)
 {
   printf("recv[%d]:%d\r\n", i, recv[i]);
 }
}

void LCD_Test(void)
{
  lcd_init(1);		/*初始化LCD,0竖屏, 1横屏*/
  if(lcddev.id != 0x9341) return; //初始化的时候经过ID验证，如果不对就终止

	lcd_clear(RED);
  if(lcddev.dir == 0)
  {
    lcd_draw_line(0, 0, 240, 320, BLACK);//竖屏显示:width:240,x; height:320,y
    lcd_draw_line(0, 320, 240, 0, BLACK);
  
    lcd_show_char(0, 0, '$', 16, 0, BLACK);    //高：16 宽：08
    lcd_show_char(8, 0, '1', 16, 0, BLACK);
    lcd_show_char(16, 0, 'F', 16, 0, BLACK);

    lcd_show_char(0, 16, 'a', 12, 0, BLACK);    //高：12 宽：06
    lcd_show_char(6, 16, '@', 12, 0, BLACK);

    lcd_show_char(0, 28, 'a', 24, 0, BLACK);    //高：24 宽：12
    lcd_show_char(12, 28, '!', 24, 0, BLACK);

    lcd_show_string(0, 52, 240, 16, 16, "Hello World!", BLUE);

    lcd_draw_circle(200, 200, 40, BLUE);
  }
  else
  {
    lcd_draw_line(0, 0, 320, 240, BLACK);//横屏显示:width:320,x; height:240,y
    lcd_draw_line(0, 240, 320, 0, BLACK);

    lcd_show_char(0, 0, '$', 16, 0, BLACK);     //高：16 宽：08
    lcd_show_char(8, 0, '2', 16, 0, BLACK);
    lcd_show_char(16, 0, 'F', 16, 0, BLACK);

    lcd_show_string(0, 16, 240, 16, 16, "Hello World!", BLUE);

    lcd_draw_circle(200, 200, 40, BLUE);
  }
}

void lcd_draw_dialog(void)//清屏复位函数
{
  lcd_clear(WHITE);
  lcd_show_string(lcddev.width - 24, 0, 24, 16, 16, "RST", BLUE);
}

void rtp_test(void)
{
  uint8_t i = 0;

  while(1)
  {
    tp_dev.scan(0);

    if(tp_dev.sta & TP_PRES_DOWN)
    {
      if(tp_dev.x[0] < lcddev.width && tp_dev.y[0] < lcddev.height)
      {
        if(tp_dev.x[0] > (lcddev.width -24) && tp_dev.y[0] < 16)
        {
          lcd_draw_dialog();
        }
        else
        {
            tp_draw_big_point(tp_dev.x[0], tp_dev.y[0], RED);
        }
      }
    }
    else
    {
      HAL_Delay(10);//没有按键按下时延时一定时间防止卡死
    }

    if(key == 0xe3)
    {
      W25Q64_EraseBlock(0x000000);
      lcd_clear(WHITE);
      tp_dev.adjust();
      TP_SaveAdjustData();
      lcd_draw_dialog();
      key = 0;
    }

    i++;
    if(i % 20 == 0)  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
//  MX_FSMC_Init();//初始化放在lcd_Init函数中
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
//  LCD_Test();
//  W25Q16_Test();  /*F407开发板上的Flash是W25Q16,只有2MB,32个块*/\

	led_init();
	lcd_init(0);//0，竖屏；1，横屏。每次切换显示方式要重新校准，按下K1校准
	key_init();
	tp_dev.init();


lcd_clear(WHITE);
lcd_show_string(40, 160, lcddev.width, lcddev.height, 16, "Screen Exit Adjust!", RED);
HAL_Delay(1000);
lcd_clear(WHITE);
lcd_draw_dialog();

if(tp_dev.touchtype & 0x80)
{
  //电容屏测试
}
else
{
  rtp_test();//电阻屏测试
}
  /* USER CODE END 2 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

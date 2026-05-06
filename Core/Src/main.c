/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Valdymo pultelio pagrindine programa
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* LoRa SPI linijos */
#define LORA_NSS_PORT      GPIOA
#define LORA_NSS_PIN       GPIO_PIN_4

#define LORA_SCK_PORT      GPIOA
#define LORA_SCK_PIN       GPIO_PIN_5

#define LORA_MOSI_PORT     GPIOA
#define LORA_MOSI_PIN      GPIO_PIN_6

#define LORA_MISO_PORT     GPIOA
#define LORA_MISO_PIN      GPIO_PIN_7

#define LORA_RST_PORT      GPIOB
#define LORA_RST_PIN       GPIO_PIN_0

/* Vartotojo mygtukas */
#define BUTTON_PORT        GPIOA
#define BUTTON_PIN         GPIO_PIN_8

/* LED indikatoriai */
#define LED_GREEN_PORT     GPIOB
#define LED_GREEN_PIN      GPIO_PIN_12

#define LED_RED_PORT       GPIOB
#define LED_RED_PIN        GPIO_PIN_13

#define LED_GREEN2_PORT    GPIOB
#define LED_GREEN2_PIN     GPIO_PIN_14

#define LED_RED2_PORT      GPIOB
#define LED_RED2_PIN       GPIO_PIN_15

/* LoRa registrai */
#define REG_FIFO           0x00
#define REG_OP_MODE        0x01
#define REG_FRF_MSB        0x06
#define REG_FRF_MID        0x07
#define REG_FRF_LSB        0x08
#define REG_FIFO_ADDR_PTR  0x0D
#define REG_FIFO_TX_BASE   0x0E
#define REG_FIFO_RX_BASE   0x0F
#define REG_IRQ_FLAGS      0x12
#define REG_PAYLOAD_LENGTH 0x22
#define REG_MODEM_CONFIG1  0x1D
#define REG_MODEM_CONFIG2  0x1E
#define REG_SYNC_WORD      0x39
#define REG_VERSION        0x42

/* LoRa režimai */
#define MODE_SLEEP         0x80
#define MODE_STDBY         0x81
#define MODE_TX            0x83

/* Komanda krovinio paleidimo sistemai */
#define RELEASE_COMMAND    "BAKIS_OK"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

uint8_t lora_ok = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

uint8_t SW_SPI_Transfer(uint8_t data);
uint8_t LoRa_ReadReg(uint8_t addr);
void LoRa_WriteReg(uint8_t addr, uint8_t value);
void LoRa_Reset(void);
uint8_t LoRa_Init(void);
uint8_t LoRa_Send(uint8_t *data, uint8_t size);

void LEDs_All_Off(void);
void LEDs_Idle_State(void);
void LEDs_LoRa_Error(void);
void LEDs_Command_Sent(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief Programiškai realizuota SPI duomenu perdavimo funkcija.
  * @param data Siunciamas baitas.
  * @retval Gautas baitas.
  */
uint8_t SW_SPI_Transfer(uint8_t data)
{
    uint8_t received = 0;

    for (int8_t i = 7; i >= 0; i--)
    {
        if (data & (1 << i))
        {
            HAL_GPIO_WritePin(LORA_MOSI_PORT, LORA_MOSI_PIN, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(LORA_MOSI_PORT, LORA_MOSI_PIN, GPIO_PIN_RESET);
        }

        HAL_GPIO_WritePin(LORA_SCK_PORT, LORA_SCK_PIN, GPIO_PIN_SET);

        if (HAL_GPIO_ReadPin(LORA_MISO_PORT, LORA_MISO_PIN) == GPIO_PIN_SET)
        {
            received |= (1 << i);
        }

        HAL_GPIO_WritePin(LORA_SCK_PORT, LORA_SCK_PIN, GPIO_PIN_RESET);
    }

    return received;
}

/**
  * @brief Nuskaito LoRa modulio registra.
  * @param addr Registro adresas.
  * @retval Registro reikšme.
  */
uint8_t LoRa_ReadReg(uint8_t addr)
{
    uint8_t val = 0;

    HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_RESET);

    SW_SPI_Transfer(addr & 0x7F);
    val = SW_SPI_Transfer(0x00);

    HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_SET);

    return val;
}

/**
  * @brief Irašo reikšme i LoRa modulio registra.
  * @param addr Registro adresas.
  * @param value Irašoma reikšme.
  * @retval None
  */
void LoRa_WriteReg(uint8_t addr, uint8_t value)
{
    HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_RESET);

    SW_SPI_Transfer(addr | 0x80);
    SW_SPI_Transfer(value);

    HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_SET);
}

/**
  * @brief Atlieka LoRa modulio aparatini perkrovima.
  * @retval None
  */
void LoRa_Reset(void)
{
    HAL_GPIO_WritePin(LORA_RST_PORT, LORA_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);

    HAL_GPIO_WritePin(LORA_RST_PORT, LORA_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
}

/**
  * @brief Inicializuoja RA-02 LoRa moduli.
  * @retval 1 - inicializacija sekminga, 0 - klaida.
  */
uint8_t LoRa_Init(void)
{
    uint8_t version = 0;

    HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LORA_SCK_PORT, LORA_SCK_PIN, GPIO_PIN_RESET);

    LoRa_Reset();

    version = LoRa_ReadReg(REG_VERSION);

    if (version != 0x12)
    {
        return 0;
    }

    /*
      LoRa modulio konfiguracija:
      - LoRa režimas
      - 433 MHz dažnis
      - nustatyti TX/RX FIFO adresai
      - nustatyti moduliacijos parametrai
      - SyncWord = 0x12
    */

    LoRa_WriteReg(REG_OP_MODE, 0x00);
    HAL_Delay(10);

    LoRa_WriteReg(REG_OP_MODE, MODE_SLEEP);
    HAL_Delay(10);

    LoRa_WriteReg(REG_OP_MODE, MODE_STDBY);
    HAL_Delay(10);

    /* 433 MHz dažnis */
    LoRa_WriteReg(REG_FRF_MSB, 0x6C);
    LoRa_WriteReg(REG_FRF_MID, 0x40);
    LoRa_WriteReg(REG_FRF_LSB, 0x00);

    /*
      RegModemConfig1 = 0x72
      BW = 125 kHz, CR = 4/5, explicit header mode

      RegModemConfig2 = 0x70
      SF = 7, CRC išjungtas
    */
    LoRa_WriteReg(REG_MODEM_CONFIG1, 0x72);
    LoRa_WriteReg(REG_MODEM_CONFIG2, 0x70);

    /* SyncWord turi sutapti su krovinio paleidimo sistemos LoRa nustatymu */
    LoRa_WriteReg(REG_SYNC_WORD, 0x12);

    /* FIFO pradžios adresai */
    LoRa_WriteReg(REG_FIFO_TX_BASE, 0x00);
    LoRa_WriteReg(REG_FIFO_RX_BASE, 0x00);

    /* Išvalomi pertraukimo požymiai */
    LoRa_WriteReg(REG_IRQ_FLAGS, 0xFF);

    LoRa_WriteReg(REG_OP_MODE, MODE_STDBY);
    HAL_Delay(10);

    return 1;
}

/**
  * @brief Išsiuncia duomenu paketa per LoRa moduli.
  * @param data Duomenu masyvas.
  * @param size Duomenu kiekis baitais.
  * @retval 1 - išsiusta sekmingai, 0 - siuntimo klaida.
  */
uint8_t LoRa_Send(uint8_t *data, uint8_t size)
{
    uint32_t timeout = 0;

    LoRa_WriteReg(REG_OP_MODE, MODE_STDBY);
    LoRa_WriteReg(REG_FIFO_ADDR_PTR, 0x00);

    for (uint8_t i = 0; i < size; i++)
    {
        LoRa_WriteReg(REG_FIFO, data[i]);
    }

    LoRa_WriteReg(REG_PAYLOAD_LENGTH, size);

    /* Išvalomi IRQ požymiai */
    LoRa_WriteReg(REG_IRQ_FLAGS, 0xFF);

    /* Ijungiamas siuntimo režimas */
    LoRa_WriteReg(REG_OP_MODE, MODE_TX);

    /*
      Laukiama TxDone požymio.
      Naudojamas timeout, kad programa neužstrigtu, jei modulis neatsakytu.
    */
    timeout = HAL_GetTick();

    while ((LoRa_ReadReg(REG_IRQ_FLAGS) & 0x08) == 0)
    {
        if ((HAL_GetTick() - timeout) > 3000)
        {
            LoRa_WriteReg(REG_OP_MODE, MODE_STDBY);
            return 0;
        }
    }

    /* Išvalomas TxDone požymis */
    LoRa_WriteReg(REG_IRQ_FLAGS, 0x08);

    /* Grižtama i budejimo režima */
    LoRa_WriteReg(REG_OP_MODE, MODE_STDBY);

    return 1;
}

/**
  * @brief Išjungia visus LED indikatorius.
  * @retval None
  */
void LEDs_All_Off(void)
{
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_GREEN2_PORT, LED_GREEN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RED2_PORT, LED_RED2_PIN, GPIO_PIN_RESET);
}

/**
  * @brief Pradine pultelio busena, kai LoRa modulis veikia.
  * @retval None
  */
void LEDs_Idle_State(void)
{
    /*
      PB12 - žalias LED: LoRa modulis veikia
      PB14 - žalias LED: sistema paruošta komandai
    */
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(LED_GREEN2_PORT, LED_GREEN2_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED2_PORT, LED_RED2_PIN, GPIO_PIN_RESET);
}

/**
  * @brief LED busena, kai LoRa modulis neinicializuotas.
  * @retval None
  */
void LEDs_LoRa_Error(void)
{
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LED_GREEN2_PORT, LED_GREEN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RED2_PORT, LED_RED2_PIN, GPIO_PIN_SET);
}

/**
  * @brief LED busena po sekmingo komandos išsiuntimo.
  * @retval None
  */
void LEDs_Command_Sent(void)
{
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_GREEN2_PORT, LED_GREEN2_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED2_PORT, LED_RED2_PIN, GPIO_PIN_SET);
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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */

  LEDs_All_Off();

  lora_ok = LoRa_Init();

  if (lora_ok)
  {
      LEDs_Idle_State();
  }
  else
  {
      LEDs_LoRa_Error();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (lora_ok)
      {
          LEDs_Idle_State();

          /*
            Mygtukas PA8 prijungtas su išoriniu pull-up rezistoriumi.
            Todel:
            - nepaspaustas = loginis 1
            - paspaustas = loginis 0
          */
          if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
          {
              HAL_Delay(50);

              if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
              {
                  uint8_t msg[] = RELEASE_COMMAND;

                  LEDs_All_Off();
                  HAL_Delay(300);

                  if (LoRa_Send(msg, sizeof(msg) - 1))
                  {
                      LEDs_Command_Sent();
                      HAL_Delay(2000);
                  }
                  else
                  {
                      LEDs_LoRa_Error();
                      HAL_Delay(1000);
                  }

                  /*
                    Laukiama, kol vartotojas atleis mygtuka.
                    Taip išvengiama pakartotinio komandos siuntimo.
                  */
                  while (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
                  {
                      HAL_Delay(10);
                  }

                  HAL_Delay(100);
              }
          }
      }
      else
      {
          /*
            Jei LoRa modulis neinicializuotas, periodiškai bandoma
            inicializuoti ji iš naujo.
          */
          LEDs_LoRa_Error();
          HAL_Delay(1000);

          lora_ok = LoRa_Init();

          if (lora_ok)
          {
              LEDs_Idle_State();
          }
      }

      HAL_Delay(50);

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  __disable_irq();

  while (1)
  {
      HAL_GPIO_TogglePin(LED_RED_PORT, LED_RED_PIN);
      HAL_Delay(300);
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
  (void)file;
  (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
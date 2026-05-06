/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Krovinio paleidimo sistemos pagrindine programa
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
//#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
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

/* Elektromagneto valdymas */
#define GATE_PORT          GPIOA
#define GATE_PIN           GPIO_PIN_2

/* LoRa registrai */
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE         0x0E
#define REG_FIFO_RX_BASE         0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_MODEM_CONFIG1        0x1D
#define REG_MODEM_CONFIG2        0x1E
#define REG_SYNC_WORD            0x39
#define REG_VERSION              0x42

/* LoRa režimai */
#define MODE_SLEEP               0x80
#define MODE_STDBY               0x81
#define MODE_RX_CONTINUOUS       0x85

/* IRQ veliavos */
#define IRQ_RX_DONE              0x40
#define IRQ_PAYLOAD_CRC_ERROR    0x20

/* Priimama komanda */
#define RELEASE_COMMAND          "BAKIS_OK"
#define RELEASE_COMMAND_LENGTH   8

/* Elektromagneto aktyvavimo trukme */
#define MAGNET_ACTIVE_TIME_MS    1000

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

uint8_t lora_ok = 0;
uint8_t rx_buffer[32];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

uint8_t SW_SPI_Transfer(uint8_t data);
uint8_t LoRa_ReadReg(uint8_t addr);
void LoRa_WriteReg(uint8_t addr, uint8_t value);

void LoRa_Reset(void);
uint8_t LoRa_Init(void);
void LoRa_SetRxMode(void);
uint8_t LoRa_Receive(uint8_t *buffer, uint8_t max_size);

uint8_t Sensors_Are_Allowed(void);
void Electromagnet_Activate(void);

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
      LoRa modulio konfiguracija turi sutapti su valdymo pultelio nustatymais:
      - 433 MHz dažnis
      - BW = 125 kHz
      - SF = 7
      - CR = 4/5
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

    /* SyncWord turi sutapti su pultelio LoRa nustatymu */
    LoRa_WriteReg(REG_SYNC_WORD, 0x12);

    /* FIFO pradžios adresai */
    LoRa_WriteReg(REG_FIFO_TX_BASE, 0x00);
    LoRa_WriteReg(REG_FIFO_RX_BASE, 0x00);

    /* Išvalomi pertraukimo požymiai */
    LoRa_WriteReg(REG_IRQ_FLAGS, 0xFF);

    LoRa_SetRxMode();

    return 1;
}

/**
  * @brief Perjungia LoRa moduli i nuolatini priemimo režima.
  * @retval None
  */
void LoRa_SetRxMode(void)
{
    LoRa_WriteReg(REG_OP_MODE, MODE_STDBY);
    HAL_Delay(5);

    LoRa_WriteReg(REG_FIFO_ADDR_PTR, 0x00);
    LoRa_WriteReg(REG_IRQ_FLAGS, 0xFF);

    LoRa_WriteReg(REG_OP_MODE, MODE_RX_CONTINUOUS);
    HAL_Delay(5);
}

/**
  * @brief Tikrina, ar gautas LoRa paketas, ir ji nuskaito.
  * @param buffer Buferis priimtiems duomenims.
  * @param max_size Maksimalus buferio dydis.
  * @retval Priimtu baitu skaicius. Jei duomenu nera - 0.
  */
uint8_t LoRa_Receive(uint8_t *buffer, uint8_t max_size)
{
    uint8_t irq_flags = 0;
    uint8_t packet_size = 0;
    uint8_t current_addr = 0;

    irq_flags = LoRa_ReadReg(REG_IRQ_FLAGS);

    if ((irq_flags & IRQ_RX_DONE) == 0)
    {
        return 0;
    }

    /* Jeigu ivyko CRC klaida, paketas ignoruojamas */
    if (irq_flags & IRQ_PAYLOAD_CRC_ERROR)
    {
        LoRa_WriteReg(REG_IRQ_FLAGS, 0xFF);
        LoRa_SetRxMode();
        return 0;
    }

    packet_size = LoRa_ReadReg(REG_RX_NB_BYTES);
    current_addr = LoRa_ReadReg(REG_FIFO_RX_CURRENT_ADDR);

    if (packet_size > max_size)
    {
        packet_size = max_size;
    }

    LoRa_WriteReg(REG_FIFO_ADDR_PTR, current_addr);

    for (uint8_t i = 0; i < packet_size; i++)
    {
        buffer[i] = LoRa_ReadReg(REG_FIFO);
    }

    /* Išvalomi IRQ požymiai */
    LoRa_WriteReg(REG_IRQ_FLAGS, 0xFF);

    /* Gražinamas RX režimas */
    LoRa_SetRxMode();

    return packet_size;
}

/**
  * @brief Patikrina, ar jutikliu duomenys leidžia paleisti krovini.
  * @retval 1 - paleidimas leidžiamas, 0 - paleidimas blokuojamas.
  */
uint8_t Sensors_Are_Allowed(void)
{
    /*
      Šioje vietoje veliau gali buti irašomas realus jutikliu tikrinimas.

      Pavyzdžiui:
      - aukšcio jutiklio reikšmes patikra;
      - padeties arba pasvirimo kampo patikra;
      - kitu saugos salygu tikrinimas.

      Šiame galutiniame baziniame variante laikoma, kad salygos tenkinamos.
    */

    return 1;
}

/**
  * @brief Aktyvuoja elektromagneta nustatytam laikui.
  * @retval None
  */
void Electromagnet_Activate(void)
{
    HAL_GPIO_WritePin(GATE_PORT, GATE_PIN, GPIO_PIN_SET);
    HAL_Delay(MAGNET_ACTIVE_TIME_MS);
    HAL_GPIO_WritePin(GATE_PORT, GATE_PIN, GPIO_PIN_RESET);
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

 // MX_GPIO_Init();

  /* USER CODE BEGIN 2 */

  /* Užtikrinama, kad elektromagnetas paleidimo metu butu išjungtas */
  HAL_GPIO_WritePin(GATE_PORT, GATE_PIN, GPIO_PIN_RESET);

  lora_ok = LoRa_Init();

  /*
    Jeigu LoRa modulis inicializuotas sekmingai, sistema pradeda laukti
    komandos iš valdymo pultelio. Jeigu modulis neatsako, programa cikle
    bandys inicializuoti ji iš naujo.
  */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (lora_ok)
      {
          uint8_t received_size = 0;

          received_size = LoRa_Receive(rx_buffer, sizeof(rx_buffer));

          if (received_size > 0)
          {
              /*
                Tikrinama, ar gautas paketas atitinka krovinio paleidimo komanda.
                Komanda turi sutapti su pultelyje siunciama reikšme: "BAKIS_OK".
              */
              if (received_size == RELEASE_COMMAND_LENGTH)
              {
                  if (memcmp(rx_buffer, RELEASE_COMMAND, RELEASE_COMMAND_LENGTH) == 0)
                  {
                      /*
                        Krovinys paleidžiamas tik tuo atveju, jeigu jutikliu
                        duomenys patenka i leistinas ribas.
                      */
                      if (Sensors_Are_Allowed())
                      {
                          Electromagnet_Activate();
                      }
                  }
              }
          }
      }
      else
      {
          /*
            Jei LoRa modulis neinicializuotas, periodiškai bandoma
            inicializuoti ji iš naujo.
          */
          HAL_GPIO_WritePin(GATE_PORT, GATE_PIN, GPIO_PIN_RESET);
          HAL_Delay(1000);

          lora_ok = LoRa_Init();
      }

      HAL_Delay(10);

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
      HAL_GPIO_WritePin(GATE_PORT, GATE_PIN, GPIO_PIN_RESET);
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
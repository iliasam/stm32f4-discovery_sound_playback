
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "pdm_filter.h"
#include "waverecorder.h" 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint16_t buf_idx = 0, buf_idx1 = 0;
uint16_t *writebuffer;
uint16_t counter = 0;

/* Current state of the audio recorder interface intialization */
static uint32_t AudioRecInited = 0;
PDMFilter_InitStruct Filter;
uint16_t RecBuf[PCM_OUT_SIZE], RecBuf1[PCM_OUT_SIZE];

__IO uint32_t Data_Status = 0; //1 - filter is ended with filling buffer

uint16_t* pAudioRecBuf; /* Main buffer pointer for the recorded data storing */

/* Temporary data sample */
static uint16_t InternalBuffer[INTERNAL_BUFF_SIZE];
static uint32_t InternalBufferSize = 0;

/* Private function prototypes -----------------------------------------------*/
static void WaveRecorder_GPIO_Init(void);
static void WaveRecorder_SPI_Init(uint32_t Freq);
static void WaveRecorder_NVIC_Init(void);

/* Private functions ---------------------------------------------------------*/

void simple_rec_start(void)
{
  WaveRecorderInit(32000, 16, 1);
  WaveRecorderStart(RecBuf, PCM_OUT_SIZE);
}

/**
* @brief  Initialize wave recording
* @param  AudioFreq: Sampling frequency
*         BitRes: Audio recording Samples format (from 8 to 16 bits)
*         ChnlNbr: Number of input microphone channel
* @retval None
*/
uint32_t WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr)
{ 
  /* Check if the interface is already initialized */
  if (AudioRecInited)
  {
    /* No need for initialization */
    return 0;
  }
  else
  {
    /* Enable CRC module */
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    
    /* Filter LP & HP Init */
    //Filter.LP_HZ = 8000;
    //Filter.HP_HZ = 10;
    Filter.LP_HZ = 0;
    Filter.HP_HZ = 0;
    
    Filter.Fs = 16000;
    Filter.Out_MicChannels = 1;
    Filter.In_MicChannels = 1;
    
    PDM_Filter_Init((PDMFilter_InitStruct *)&Filter);
    
    WaveRecorder_GPIO_Init();
    WaveRecorder_NVIC_Init();
    WaveRecorder_SPI_Init(AudioFreq);
    
    /* Set state of the audio recorder to initialized */
    AudioRecInited = 1;
    
    return 0;
  }  
}

/**
  * @brief  Start audio recording
  * @param  pbuf: pointer to a buffer
  *         size: Buffer size
  * @retval None
  */
uint8_t WaveRecorderStart(uint16_t* pbuf, uint32_t size)
{
  /* Check if the interface has already been initialized */
  if (AudioRecInited)
  {
    /* Store the location and size of the audio buffer */
    pAudioRecBuf = pbuf;
    //AudioRecCurrSize = size; //not used
    
    /* Enable the Rx buffer not empty interrupt */
    SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
    /* The Data transfer is performed in the SPI interrupt routine */
    /* Enable the SPI peripheral */
    I2S_Cmd(SPI2, ENABLE); 
    
    return 0;
  }
  else
  {
    return 1; /* Cannot perform operation */
  }
}



/**
* @brief  This function handles AUDIO_REC_SPI global interrupt request.
* @param  None
* @retval None
*/
void AUDIO_REC_SPI_IRQHANDLER(void)
{  
  u16 volume;
  u16 app;
  
  /* Check if data are available in SPI Data register */
  if (SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE) != RESET)
  {
    app = SPI_I2S_ReceiveData(SPI2);
    InternalBuffer[InternalBufferSize++] = HTONS(app);
    
    /* Check to prevent overflow condition */
    if (InternalBufferSize >= INTERNAL_BUFF_SIZE)
    {
      InternalBufferSize = 0;//from start
      
      volume = 80;
      PDM_Filter_64_LSB((uint8_t *)InternalBuffer, (uint16_t *)pAudioRecBuf, volume , (PDMFilter_InitStruct *)&Filter);
      Data_Status = 1;       
    }
  }
}


/**
  * @brief  Initialize GPIO for wave recorder.
  * @param  None
  * @retval None
  */
static void WaveRecorder_GPIO_Init(void)
{  
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK | SPI_MOSI_GPIO_CLK, ENABLE);
  
  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK | SPI_MOSI_GPIO_CLK, ENABLE);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  
  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
  GPIO_Init(SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
  
  /* Connect SPI pins to AF5 */  
  GPIO_PinAFConfig(SPI_SCK_GPIO_PORT, SPI_SCK_SOURCE, SPI_SCK_AF);
  
  /* SPI MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  SPI_MOSI_PIN;
  GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(SPI_MOSI_GPIO_PORT, SPI_MOSI_SOURCE, SPI_MOSI_AF);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param  Freq :Audio frequency
  * @retval None
  */
static void WaveRecorder_SPI_Init(uint32_t Freq)
{
  I2S_InitTypeDef I2S_InitStructure;

  /* Enable the SPI clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
  
  /* SPI configuration */
  SPI_I2S_DeInit(SPI2);
  I2S_InitStructure.I2S_AudioFreq = Freq / 2;//impossible to diable second channel
  I2S_InitStructure.I2S_Standard = I2S_Standard_LSB;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(SPI2, &I2S_InitStructure);

  /* Enable the Rx buffer not empty interrupt */
  SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
}


/**
  * @brief  Initialize the NVIC.
  * @param  None
  * @retval None
  */
static void WaveRecorder_NVIC_Init(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3); 
  /* Configure the SPI interrupt priority */
  NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


#ifdef  USE_FULL_ASSERT

/**
* @brief  Reports the name of the source file and the source line number
*   where the assert_param error has occurred.
* @param  file: pointer to the source file name
* @param  line: assert_param error line source number
* @retval None
*/
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while (1)
  {
  }
}
#endif


/**
  * @brief  Stop audio recording
  * @param  None
  * @retval None
  */
uint32_t WaveRecorderStop(void)
{
  /* Check if the interface has already been initialized */
  if (AudioRecInited)
  {
    I2S_Cmd(SPI2, DISABLE); 
    return 0;
  }
  else
  {
    return 1;
  }
}

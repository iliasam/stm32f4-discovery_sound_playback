/**
  ******************************************************************************
  * Microphone playback using external I2S DAC chip
  * author: ILIASAM
  * Be carefull about files "libPDMFilter_IAR.a" / "libPDMFilter_Keil" !!!!!
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4_discovery_audio_codec.h"
#include "waverecorder.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//for testing
const int16_t sinebuf1[96] = {0,0, 4276,4276, 8480,8480, 12539,12539, 16383,16383, 19947,19947, 23169,23169, 25995,25995,
                             28377,28377, 30272,30272, 31650,31650, 32486,32486, 32767,32767, 32486,32486, 31650,31650, 30272,30272,
                             28377,28377, 25995,25995, 23169,23169, 19947,19947, 16383,16383, 12539,12539, 8480,8480, 4276,4276,
                             0,0, -4276,-4276, -8480,-8480, -12539,-12539, -16383,-16383, -19947,-19947, -23169,-23169, -25995,-25995,
                             -28377,-28377, -30272,-30272, -31650,-31650, -32486,-32486, -32767,-32767, -32486,-32486, -31650,-31650, -30272,-30272,
                             -28377,-28377, -25995,-25995, -23169,-23169, -19947,-19947, -16383,-16383, -12539,-12539, -8480,-8480, -4276,-4276
                             };


int16_t audiodata[64];//sound data to be played by I2S DAC chip

__IO uint8_t UserButtonPressed = 0;

extern uint16_t RecBuf[PCM_OUT_SIZE];//microphone buffer - filled in "waverecorder.c"

/* Private function prototypes -----------------------------------------------*/
void Delay_ms(uint32_t ms);

/* Private functions ---------------------------------------------------------*/


int main(void)
{
  // codec types #define AUDIO_MAL_MODE_NORMAL
  // AUDIO_MAL_DMA_IT_TC_EN - no DAC
  
  RCC_ClocksTypeDef RCC_Clocks;
  
  /* Initialize LEDs and User_Button on STM32F4-Discovery --------------------*/
  STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI); 
  
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);
  
  RCC_GetClocksFreq(&RCC_Clocks);
  
  uint8_t volume = 80;
  EVAL_AUDIO_Init(OUTPUT_DEVICE_HEADPHONE, volume, I2S_AudioFreq_8k);
  
  STM_EVAL_LEDOff(LED6);
  Delay_ms(10);
  simple_rec_start();//initialize sound recording
  
  EVAL_AUDIO_Play((uint16_t*)(&audiodata[0]),16 * 2 * 2);//start playback
  
  //all code is working using interrupts
  
  while(1)
  {
    asm("nop");
  }
  
}


/**
* @brief  Calculates the remaining file size and new position of the pointer.
* @param  None
* @retval None
*/
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t* pBuffer, uint32_t Size)
{
  uint8_t i;
  /* Check if the end of file has been reached */
  //Called when data are transferred
#ifdef AUDIO_MAL_MODE_NORMAL  
  for (i=0; i < 32; i++)
  {
    audiodata[i] = RecBuf[i >> 1];//make pseudo stereo from nono signal
  }  
  
  EVAL_AUDIO_Play((uint16_t*)(&audiodata[0]), 16 * 2 * 2);
  STM_EVAL_LEDToggle(LED6);
  
#endif
}


void Delay_ms(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq (&RCC_Clocks);
  nCount=(RCC_Clocks.HCLK_Frequency/10000)*ms;
  for (; nCount!=0; nCount--);
}

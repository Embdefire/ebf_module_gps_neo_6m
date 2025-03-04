/**
  ******************************************************************************
  * @file    bsp_usart_dma.c
  * @author  fire
  * @version V1.0
  * @date    2019-xx-xx
  * @brief   GPS测试
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火  STM32 H750 开发板  
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 
  
#include "./usart/bsp_usart_dma.h"
#include "./led/bsp_led.h"

DMA_HandleTypeDef DMA_Handle;
UART_HandleTypeDef GPS_UartHandle;



/* DMA传输结束标志 */
__IO uint8_t GPS_TransferEnd = 0, GPS_HalfTransferEnd = 0;
/* DMA接收缓冲  */
uint8_t gps_rbuff[GPS_RBUFF_SIZE];

 /**
  * @brief  GPS_USART GPIO 配置,工作模式配置。115200 8-N-1
  * @param  无
  * @retval 无
  */  
void GPS_USART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
        
    GPS_USART_RX_GPIO_CLK_ENABLE();
    GPS_USART_TX_GPIO_CLK_ENABLE();
    
    /* 配置串口1时钟源*/
		RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART6;
		RCC_PeriphClkInit.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
		HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);
    /* 使能串口6时钟 */
    GPS_USART_CLK_ENABLE();

    /* 配置Tx引脚为复用功能  */
    GPIO_InitStruct.Pin = GPS_USART_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPS_USART_TX_AF;
    HAL_GPIO_Init(GPS_USART_TX_GPIO_PORT, &GPIO_InitStruct);
    
    /* 配置Rx引脚为复用功能 */
    GPIO_InitStruct.Pin = GPS_USART_RX_PIN;
    GPIO_InitStruct.Alternate = GPS_USART_RX_AF;
    HAL_GPIO_Init(GPS_USART_RX_GPIO_PORT, &GPIO_InitStruct); 
    
    /* 配置串GPS_USART 模式 */
    GPS_UartHandle.Instance = GPS_USART;
    GPS_UartHandle.Init.BaudRate = GPS_USART_BAUDRATE;
    GPS_UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    GPS_UartHandle.Init.StopBits = UART_STOPBITS_1;
    GPS_UartHandle.Init.Parity = UART_PARITY_NONE;
    GPS_UartHandle.Init.Mode = UART_MODE_TX_RX;
    GPS_UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    GPS_UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
    GPS_UartHandle.Init.OneBitSampling = UART_ONEBIT_SAMPLING_DISABLED;
    GPS_UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&GPS_UartHandle);
}

/**
  * @brief  USART_DMA_Config gps dma接收配置
  * @param  无
  * @retval 无
  */
void USART_DMA_Config(void)
{
  /*开启DMA时钟*/
  GPS_USART_DMA_CLK_ENABLE();
  
  DMA_Handle.Instance = GPS_USART_DMA_STREAM;
  /* Deinitialize the stream for new transfer */
  HAL_DMA_DeInit(&DMA_Handle);
  /*usart6对应数据流*/	
  DMA_Handle.Init.Request = GPS_USART_DMA_REQUEST; 
  /*方向：从内存到外设*/		
  DMA_Handle.Init.Direction= DMA_PERIPH_TO_MEMORY;	
  /*外设地址不增*/	    
  DMA_Handle.Init.PeriphInc = DMA_PINC_DISABLE; 
  /*内存地址自增*/
  DMA_Handle.Init.MemInc = DMA_MINC_ENABLE;	
  /*外设数据单位*/	
  DMA_Handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  /*内存数据单位 8bit*/
  DMA_Handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;	
  /*DMA模式：不断循环*/
  DMA_Handle.Init.Mode = DMA_CIRCULAR;	 
  /*优先级：中*/	
  DMA_Handle.Init.Priority = DMA_PRIORITY_MEDIUM;      
  /*禁用FIFO*/
  DMA_Handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;        
  DMA_Handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;    
  /*存储器突发传输 1个节拍*/
  DMA_Handle.Init.MemBurst = DMA_MBURST_SINGLE;    
  /*外设突发传输 1个节拍*/
  DMA_Handle.Init.PeriphBurst = DMA_PBURST_SINGLE;    
  /*配置DMA2的数据流7*/		   
  /* Configure the DMA stream */
  HAL_DMA_Init(&DMA_Handle); 
   /* Associate the DMA handle */
  __HAL_LINKDMA(&GPS_UartHandle, hdmarx, DMA_Handle);
		/*配置GPS使用的DMA中断优先级*/
	HAL_NVIC_SetPriority(GPS_DMA_IRQ, 0, 0);
	 /*使能DMA中断*/
	HAL_NVIC_EnableIRQ(GPS_DMA_IRQ);
	 /*配置中断条件*/
	__HAL_DMA_ENABLE_IT(&DMA_Handle,DMA_IT_HT|DMA_IT_TC);

}


/**
  * @brief  GPS_Config gps 初始化
  * @param  无
  * @retval 无
  */
void GPS_Config(void)
{
  GPS_USART_Config();
  USART_DMA_Config();  
}


/**
  * @brief  GPS_ProcessDMAIRQ GPS DMA中断服务函数
  * @param  None.
  * @retval None.
  */
void GPS_ProcessDMAIRQ(void)
{
	
	if(__HAL_DMA_GET_FLAG(&DMA_Handle,DMA_FLAG_HTIF3_7) != RESET)        /* DMA 半传输完成 */
  { 
    GPS_HalfTransferEnd = 1;                //设置半传输完成标志位
		LED1_ON;
  }
  else if(__HAL_DMA_GET_FLAG(&DMA_Handle,DMA_FLAG_TCIF3_7)!= RESET)     /* DMA 传输完成 */
  {
     LED1_OFF;
    GPS_TransferEnd = 1;                    //设置传输完成标志位
  }
	HAL_DMA_IRQHandler(&DMA_Handle);
}


/**
  * @brief  trace 在解码时输出捕获的GPS语句
  * @param  str: 要输出的字符串，str_size:数据长度
  * @retval 无
  */
void trace(const char *str, int str_size)
{
  #ifdef __GPS_DEBUG    //在gps_config.h文件配置这个宏，是否输出调试信息
    uint16_t i;
    printf("\r\nTrace: ");
    for(i=0;i<str_size;i++)
      printf("%c",*(str+i));
  
    printf("\n");
  #endif
}

/**
  * @brief  error 在解码出错时输出提示消息
  * @param  str: 要输出的字符串，str_size:数据长度
  * @retval 无
  */
void error(const char *str, int str_size)
{
    #ifdef __GPS_DEBUG   //在gps_config.h文件配置这个宏，是否输出调试信息

    uint16_t i;
    printf("\r\nError: ");
    for(i=0;i<str_size;i++)
      printf("%c",*(str+i));
    printf("\n");
    #endif
}

/**
  * @brief  error 在解码出错时输出提示消息
  * @param  str: 要输出的字符串，str_size:数据长度
  * @retval 无
  */
void gps_info(const char *str, int str_size)
{

    uint16_t i;
    printf("\r\nInfo: ");
    for(i=0;i<str_size;i++)
      printf("%c",*(str+i));
    printf("\n");
}



/******************************************************************************************************** 
**     函数名称:            bit        IsLeapYear(uint8_t    iYear) 
**    功能描述:            判断闰年(仅针对于2000以后的年份) 
**    入口参数：            iYear    两位年数 
**    出口参数:            uint8_t        1:为闰年    0:为平年 
********************************************************************************************************/ 
static uint8_t IsLeapYear(uint8_t iYear) 
{ 
    uint16_t    Year; 
    Year    =    2000+iYear; 
    if((Year&3)==0) 
    { 
        return ((Year%400==0) || (Year%100!=0)); 
    } 
     return 0; 
} 

/******************************************************************************************************** 
**     函数名称:            void    GMTconvert(uint8_t *DT,uint8_t GMT,uint8_t AREA) 
**    功能描述:            格林尼治时间换算世界各时区时间 
**    入口参数：            *DT:    表示日期时间的数组 格式 YY,MM,DD,HH,MM,SS 
**                        GMT:    时区数 
**                        AREA:    1(+)东区 W0(-)西区 
********************************************************************************************************/ 
void    GMTconvert(nmeaTIME *SourceTime, nmeaTIME *ConvertTime, uint8_t GMT,uint8_t AREA) 
{ 
    uint32_t    YY,MM,DD,hh,mm,ss;        //年月日时分秒暂存变量 
     
    if(GMT==0)    return;                //如果处于0时区直接返回 
    if(GMT>12)    return;                //时区最大为12 超过则返回         

    YY    =    SourceTime->year;                //获取年 
    MM    =    SourceTime->mon;                 //获取月 
    DD    =    SourceTime->day;                 //获取日 
    hh    =    SourceTime->hour;                //获取时 
    mm    =    SourceTime->min;                 //获取分 
    ss    =    SourceTime->sec;                 //获取秒 

    if(AREA)                        //东(+)时区处理 
    { 
        if(hh+GMT<24)    hh    +=    GMT;//如果与格林尼治时间处于同一天则仅加小时即可 
        else                        //如果已经晚于格林尼治时间1天则进行日期处理 
        { 
            hh    =    hh+GMT-24;        //先得出时间 
            if(MM==1 || MM==3 || MM==5 || MM==7 || MM==8 || MM==10)    //大月份(12月单独处理) 
            { 
                if(DD<31)    DD++; 
                else 
                { 
                    DD    =    1; 
                    MM    ++; 
                } 
            } 
            else if(MM==4 || MM==6 || MM==9 || MM==11)                //小月份2月单独处理) 
            { 
                if(DD<30)    DD++; 
                else 
                { 
                    DD    =    1; 
                    MM    ++; 
                } 
            } 
            else if(MM==2)    //处理2月份 
            { 
                if((DD==29) || (DD==28 && IsLeapYear(YY)==0))        //本来是闰年且是2月29日 或者不是闰年且是2月28日 
                { 
                    DD    =    1; 
                    MM    ++; 
                } 
                else    DD++; 
            } 
            else if(MM==12)    //处理12月份 
            { 
                if(DD<31)    DD++; 
                else        //跨年最后一天 
                {               
                    DD    =    1; 
                    MM    =    1; 
                    YY    ++; 
                } 
            } 
        } 
    } 
    else 
    {     
        if(hh>=GMT)    hh    -=    GMT;    //如果与格林尼治时间处于同一天则仅减小时即可 
        else                        //如果已经早于格林尼治时间1天则进行日期处理 
        { 
            hh    =    hh+24-GMT;        //先得出时间 
            if(MM==2 || MM==4 || MM==6 || MM==8 || MM==9 || MM==11)    //上月是大月份(1月单独处理) 
            { 
                if(DD>1)    DD--; 
                else 
                { 
                    DD    =    31; 
                    MM    --; 
                } 
            } 
            else if(MM==5 || MM==7 || MM==10 || MM==12)                //上月是小月份2月单独处理) 
            { 
                if(DD>1)    DD--; 
                else 
                { 
                    DD    =    30; 
                    MM    --; 
                } 
            } 
            else if(MM==3)    //处理上个月是2月份 
            { 
                if((DD==1) && IsLeapYear(YY)==0)                    //不是闰年 
                { 
                    DD    =    28; 
                    MM    --; 
                } 
                else    DD--; 
            } 
            else if(MM==1)    //处理1月份 
            { 
                if(DD>1)    DD--; 
                else        //新年第一天 
                {               
                    DD    =    31; 
                    MM    =    12; 
                    YY    --; 
                } 
            } 
        } 
    }         

    ConvertTime->year   =    YY;                //更新年 
    ConvertTime->mon    =    MM;                //更新月 
    ConvertTime->day    =    DD;                //更新日 
    ConvertTime->hour   =    hh;                //更新时 
    ConvertTime->min    =    mm;                //更新分 
    ConvertTime->sec    =    ss;                //更新秒 
}  



/*********************************************END OF FILE**********************/

/**********************************************************************************************************************

 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *         File:  UART_program.c
 *        Author: Mohamed Osama
 *		   Date:  May 9, 2024
 *  Description:  <Write File DESCRIPTION here>     
 *  
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include "UART_private.h"
#include "UART_interface.h"

/**********************************************************************************************************************
*  LOCAL MACROS CONSTANT\FUNCTION
*********************************************************************************************************************/

/**********************************************************************************************************************
 *  LOCAL DATA 
 *********************************************************************************************************************/
static pCallBackNotification USART1_TC_INT_Callback = NULL;
static pCallBackNotification USART2_TC_INT_Callback = NULL;
static pCallBackNotification USART6_TC_INT_Callback = NULL;

static pCallBackNotification USART1_RXNE_INT_Callback = NULL;
static pCallBackNotification USART2_RXNE_INT_Callback = NULL;
static pCallBackNotification USART6_RXNE_INT_Callback = NULL;
/**********************************************************************************************************************
 *  GLOBAL DATA
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *  LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
static void UART_ClockEnable(const UART_InitTypeDef* UARTConfig);
static void UART_SetBaudRate(const UART_InitTypeDef* UARTConfig);
static u16 UART_u16CalculateBRR(u32 Copy_u32PeripheralClock, u32 Copy_u32BaudRate);
static void UART_voidUSART2DirectInit(const UART_InitTypeDef* UARTConfig);
static void UART_voidGPIOConfig(const UART_InitTypeDef* UARTConfig);
static void UART_TC_INT_Init(const UART_InitTypeDef* UARTConfig);
static void UART_RXNE_INT_Init(const UART_InitTypeDef* UARTConfig);
/**********************************************************************************************************************
 *  LOCAL FUNCTIONS
 *********************************************************************************************************************/
static void UART_ClockEnable(const UART_InitTypeDef* UARTConfig)
{
	if(UARTConfig->UART_Instance == USART1)
		{
			RCC_voidEnablePeripheralClock(RCC_APB2, RCC_APB2_USART1EN);
		}
	else if(UARTConfig->UART_Instance == USART2)
		{
			RCC_voidEnablePeripheralClock(RCC_APB1, RCC_APB1_USART2EN);
		}
	else if(UARTConfig->UART_Instance == USART6)
		{
			RCC_voidEnablePeripheralClock(RCC_APB2, RCC_APB2_USART6EN);
		}
}
static void UART_voidGPIOConfig(const UART_InitTypeDef* UARTConfig)
{
	RCC_voidEnablePeripheralClock(RCC_AHB, RCC_AHB_GPIOAEN);
	if(UARTConfig->UART_Instance == USART1)
		{
		GPIO_voidSetPinMode(GPIO_PORTA,GPIO_PIN9, GPIO_ALTERNATE_FUNCTION_PIN_MODE);
		GPIO_voidSetPinOutputMode(GPIO_PORTA,GPIO_PIN9, GPIO_OUTPUT_PIN_PUSH_PULL);
		GPIO_voidSetPinOutputSpeed(GPIO_PORTA,GPIO_PIN9, GPIO_OUTPUT_PIN_HIGH_SPEED);
		GPIO_voidSetPinAlternateFunction(GPIO_PORTA,GPIO_PIN9, USART1_TX_AF);

		GPIO_voidSetPinMode(GPIO_PORTA,GPIO_PIN10, GPIO_ALTERNATE_FUNCTION_PIN_MODE);
		GPIO_voidSetPinPullUpDownResistor(GPIO_PORTA,GPIO_PIN10, GPIO_INPUT_PIN_PULL_UP);
		GPIO_voidSetPinAlternateFunction(GPIO_PORTA,GPIO_PIN10, USART1_RX_AF);
		}
	else if(UARTConfig->UART_Instance == USART2)
	{
		GPIO_voidSetPinMode(GPIO_PORTA,GPIO_PIN2, GPIO_ALTERNATE_FUNCTION_PIN_MODE);
		GPIO_voidSetPinOutputMode(GPIO_PORTA,GPIO_PIN2, GPIO_OUTPUT_PIN_PUSH_PULL);
		GPIO_voidSetPinOutputSpeed(GPIO_PORTA,GPIO_PIN2, GPIO_OUTPUT_PIN_HIGH_SPEED);
		GPIO_voidSetPinAlternateFunction(GPIO_PORTA,GPIO_PIN2, USART2_TX_AF);

		GPIO_voidSetPinMode(GPIO_PORTA,GPIO_PIN3, GPIO_ALTERNATE_FUNCTION_PIN_MODE);
		GPIO_voidSetPinOutputMode(GPIO_PORTA,GPIO_PIN3, GPIO_OUTPUT_PIN_PUSH_PULL);
		GPIO_voidSetPinOutputSpeed(GPIO_PORTA,GPIO_PIN3, GPIO_OUTPUT_PIN_HIGH_SPEED);
		GPIO_voidSetPinPullUpDownResistor(GPIO_PORTA,GPIO_PIN3, GPIO_INPUT_PIN_PULL_UP);
		GPIO_voidSetPinAlternateFunction(GPIO_PORTA,GPIO_PIN3, USART2_RX_AF);
	}
	else if(UARTConfig->UART_Instance == USART6)
	{
		GPIO_voidSetPinMode(GPIO_PORTA,GPIO_PIN11, GPIO_ALTERNATE_FUNCTION_PIN_MODE);
		GPIO_voidSetPinAlternateFunction(GPIO_PORTA,GPIO_PIN11, USART6_TX_AF);

		GPIO_voidSetPinMode(GPIO_PORTA,GPIO_PIN12, GPIO_ALTERNATE_FUNCTION_PIN_MODE);
		GPIO_voidSetPinAlternateFunction(GPIO_PORTA,GPIO_PIN12, USART6_RX_AF);
	}
}
static void UART_SetBaudRate(const UART_InitTypeDef* UARTConfig)
{
	if(UARTConfig->UART_Instance == USART1)
	{
		USART1->USART_BRR = UART_u16CalculateBRR(APB2_CLK, UARTConfig->UART_BaudRate);
	}
	else if (UARTConfig->UART_Instance == USART2)
	{
		USART2->USART_BRR = UART_u16CalculateBRR(APB1_CLK, UARTConfig->UART_BaudRate);
	}
	else if (UARTConfig->UART_Instance == USART6)
	{
		USART6->USART_BRR = UART_u16CalculateBRR(APB2_CLK, UARTConfig->UART_BaudRate);
	}
}
static u16 UART_u16CalculateBRR(u32 Copy_u32PeripheralClock, u32 Copy_u32BaudRate)
{
	u32 Local_u32Divider = 16U * Copy_u32BaudRate;
	u32 Local_u32Mantissa = Copy_u32PeripheralClock / Local_u32Divider;
	u32 Local_u32Remainder = Copy_u32PeripheralClock % Local_u32Divider;
	u32 Local_u32Fraction = (Local_u32Remainder + (Copy_u32BaudRate / 2U)) / Copy_u32BaudRate;

	if(Local_u32Fraction >= 16U)
	{
		Local_u32Mantissa++;
		Local_u32Fraction = 0U;
	}

	return (u16)((Local_u32Mantissa << 4) | (Local_u32Fraction & 0xFU));
}
static void UART_voidUSART2DirectInit(const UART_InitTypeDef* UARTConfig)
{
	volatile u32* RCC_AHB1ENR = (volatile u32*)0x40023830;
	volatile u32* RCC_APB1ENR = (volatile u32*)0x40023840;
	volatile u32* GPIOA_MODER = (volatile u32*)0x40020000;
	volatile u32* GPIOA_OTYPER = (volatile u32*)0x40020004;
	volatile u32* GPIOA_OSPEEDR = (volatile u32*)0x40020008;
	volatile u32* GPIOA_PUPDR = (volatile u32*)0x4002000C;
	volatile u32* GPIOA_AFRL = (volatile u32*)0x40020020;

	*RCC_AHB1ENR |= (1U << 0);
	*RCC_APB1ENR |= (1U << 17);

	*GPIOA_MODER &= ~((3U << (2U * 2U)) | (3U << (3U * 2U)));
	*GPIOA_MODER |=  ((2U << (2U * 2U)) | (2U << (3U * 2U)));
	*GPIOA_OTYPER &= ~((1U << 2U) | (1U << 3U));
	*GPIOA_OSPEEDR &= ~((3U << (2U * 2U)) | (3U << (3U * 2U)));
	*GPIOA_OSPEEDR |=  ((2U << (2U * 2U)) | (2U << (3U * 2U)));
	*GPIOA_PUPDR &= ~((3U << (2U * 2U)) | (3U << (3U * 2U)));
	*GPIOA_PUPDR |=  (1U << (3U * 2U));   /* PA3 pull-up: prevent floating RX line */
	*GPIOA_AFRL &= ~((0xFU << (4U * 2U)) | (0xFU << (4U * 3U)));
	*GPIOA_AFRL |=  ((7U << (4U * 2U)) | (7U << (4U * 3U)));

	USART2->USART_CR1 = 0x00000000U;
	USART2->USART_CR2 = 0x00000000U;
	USART2->USART_CR3 = 0x00000000U;
	USART2->USART_BRR = UART_u16CalculateBRR(APB1_CLK, UARTConfig->UART_BaudRate);
	USART2->USART_CR1 |= ((1U << UART_TE_BIT_POS) | (1U << UART_RE_BIT_POS) | (1U << UART_UE_BIT_POS));
}
static void UART_TC_INT_Init(const UART_InitTypeDef* UARTConfig)
{
	if(UARTConfig->UART_Instance == USART1)
	{
		USART1_TC_INT_Callback = UARTConfig->UART_TC_INT_Callback;
		SET_BIT(USART1->USART_CR1, USART_TCIE_BIT_POS);
		MNVIC_voidEnableIRQ(USART1_IRQn);
	}
	else if (UARTConfig->UART_Instance == USART2)
	{
		USART2_TC_INT_Callback = UARTConfig->UART_TC_INT_Callback;
		SET_BIT(USART2->USART_CR1, USART_TCIE_BIT_POS);
		MNVIC_voidEnableIRQ(USART2_IRQn);
	}
	else if (UARTConfig->UART_Instance == USART6)
	{
		USART6_TC_INT_Callback = UARTConfig->UART_TC_INT_Callback;
		SET_BIT(USART6->USART_CR1, USART_TCIE_BIT_POS);
		MNVIC_voidEnableIRQ(USART6_IRQn);
	}
}
static void UART_RXNE_INT_Init(const UART_InitTypeDef* UARTConfig)
{
	if(UARTConfig->UART_Instance == USART1)
	{
		USART1_RXNE_INT_Callback = UARTConfig->UART_RXNE_INT_Callback;
		SET_BIT(USART1->USART_CR1, USART_RXNEIE_BIT_POS);
		MNVIC_voidEnableIRQ(USART1_IRQn);
	}
	else if (UARTConfig->UART_Instance == USART2)
	{
		USART2_RXNE_INT_Callback = UARTConfig->UART_RXNE_INT_Callback;
		SET_BIT(USART2->USART_CR1, USART_RXNEIE_BIT_POS);
		MNVIC_voidEnableIRQ(USART2_IRQn);
	}
	else if (UARTConfig->UART_Instance == USART6)
	{
		USART6_RXNE_INT_Callback = UARTConfig->UART_RXNE_INT_Callback;
		SET_BIT(USART6->USART_CR1, USART_RXNEIE_BIT_POS);
		MNVIC_voidEnableIRQ(USART6_IRQn);
	}
}

/**********************************************************************************************************************
 *  GLOBAL FUNCTIONS
 *********************************************************************************************************************/
void UART_voidInit(const UART_InitTypeDef* UARTConfig)
{
 /* Enable UART Clock */
	UART_ClockEnable(UARTConfig);

  /* UART GPIO Config */
   UART_voidGPIOConfig(UARTConfig);

 /* Deinit UART */
	UARTConfig->UART_Instance->USART_CR1 =0x00000000U;
	UARTConfig->UART_Instance->USART_CR2 =0x00000000U;
	UARTConfig->UART_Instance->USART_CR3 =0x00000000U;

/* Set USART Datawidth and Parity */
	UARTConfig->UART_Instance->USART_CR1 = UARTConfig->UART_DataWidth | UARTConfig->UART_Parity;

/* Set USART Stop Bits */
	UARTConfig->UART_Instance->USART_CR2 = UARTConfig->UART_StopBits;

/* Set USART Baudrate */
	UART_SetBaudRate(UARTConfig);
/* Config USART TC interrupt */
	if(UARTConfig->UART_TCIE == INTERRUPT_ENABLED)
	{
		UART_TC_INT_Init(UARTConfig);
	}
/* Config USART RXNE interrupt */
	if(UARTConfig->UART_RXNEIE == INTERRUPT_ENABLED)
	{
		UART_RXNE_INT_Init(UARTConfig);
	}
 /* Enable USART TX and RX */
	UARTConfig->UART_Instance->USART_CR1 |= ((1<< UART_TE_BIT_POS) |(1<< UART_RE_BIT_POS) | (1<< UART_UE_BIT_POS));
}
Std_ReturnType UART_TransmitByte(const UART_InitTypeDef* UARTConfig , u8 Copy_u8Data , u32 Copy_u32Timeout)
{
	Std_ReturnType Local_u8ErrorState = STD_OK;
	u32 Local_u32TimeoutCounter = Copy_u32Timeout;

	while(GET_BIT(UARTConfig->UART_Instance->USART_SR, 7U) == USART_TRANSFER_NOT_COMPLETED)
	{
		if(Copy_u32Timeout != UART_MAX_DELAY)
		{
			if(Local_u32TimeoutCounter == 0U)
			{
				return STD_TIMEOUT;
			}
			Local_u32TimeoutCounter--;
		}
	}

	UARTConfig->UART_Instance->USART_DR = Copy_u8Data;

	Local_u32TimeoutCounter = Copy_u32Timeout;
	while(GET_BIT(UARTConfig->UART_Instance->USART_SR, USART_TC_BIT_POS) == USART_TRANSFER_NOT_COMPLETED)
	{
		if(Copy_u32Timeout != UART_MAX_DELAY)
		{
			if(Local_u32TimeoutCounter == 0U)
			{
				Local_u8ErrorState = STD_TIMEOUT;
				break;
			}
			Local_u32TimeoutCounter--;
		}
	}

	return Local_u8ErrorState;
}
Std_ReturnType UART_voidTransmit(const UART_InitTypeDef* UARTConfig , u8* Ptr_u8Data , u16 Copy_u16DataSize , u32 Copy_u32Timeout)
{
	Std_ReturnType Local_u8ErrorState = STD_OK;
	u16 Local_u16Counter = 0;
	if((NULL == UARTConfig) || (NULL == Ptr_u8Data))
	{
		Local_u8ErrorState = STD_NOK;
	}
	else
	{
		for(Local_u16Counter = 0 ; Local_u16Counter < Copy_u16DataSize ; Local_u16Counter++)
		{
			Local_u8ErrorState = UART_TransmitByte(UARTConfig, Ptr_u8Data[Local_u16Counter], Copy_u32Timeout);
			if(Local_u8ErrorState != STD_OK)
			{
				break;
			}
		}
	}
	return Local_u8ErrorState;
}
Std_ReturnType UART_ReceiveByte(const UART_InitTypeDef* UARTConfig , u8* ptr_u8Data , u32 Copy_u32Timeout)
{
	Std_ReturnType Local_u8ErrorState = STD_OK;
	u32 Local_u32TimeoutCounter = Copy_u32Timeout;

	while(GET_BIT(UARTConfig->UART_Instance->USART_SR, USART_RXNE_BIT_POS) == USART_RECIEVE_BUFFER_EMPTY)
	{
		if((UARTConfig->UART_Instance->USART_SR & 0x0FU) != 0U)
		{
			(void)UARTConfig->UART_Instance->USART_DR;
		}

		if(Copy_u32Timeout != UART_MAX_DELAY)
		{
			if(Local_u32TimeoutCounter == 0U)
			{
				return STD_TIMEOUT;
			}
			Local_u32TimeoutCounter--;
		}
	}

	*ptr_u8Data = (u8)(UARTConfig->UART_Instance->USART_DR & 0xFFU);

	return Local_u8ErrorState;
}
Std_ReturnType UART_voidReceive(const UART_InitTypeDef* UARTConfig , u8* Ptr_u8Data , u16 Copy_u16DataSize , u32 Copy_u32Timeout)
{
	Std_ReturnType Local_u8ErrorState = STD_OK;
	u16 Local_u16Counter = 0;
	if((NULL == UARTConfig) || (NULL == Ptr_u8Data))
	{
		Local_u8ErrorState = STD_NOK;
	}
	else
	{
		for(Local_u16Counter = 0 ; Local_u16Counter < Copy_u16DataSize ; Local_u16Counter++)
		{
			Local_u8ErrorState = UART_ReceiveByte(UARTConfig, &Ptr_u8Data[Local_u16Counter], Copy_u32Timeout);
			if(Local_u8ErrorState != STD_OK)
			{
				break;
			}
		}
	}
	return Local_u8ErrorState;
}

/******************** USART ISRs *******************************************/
void USART1_IRQHandler(void)
{
	if(GET_UART1_TC_FLAG() == INTERRUPT_ACTIVE)
	{
		CLEAR_UART1_TC_FLAG();
		if(USART1_TC_INT_Callback != NULL)
		{
			USART1_TC_INT_Callback();
		}
	}

	if(GET_UART1_RXNE_FLAG() == INTERRUPT_ACTIVE)
	{
		CLEAR_UART1_RXNE_FLAG();
		if(USART1_RXNE_INT_Callback != NULL)
		{
			USART1_RXNE_INT_Callback();
		}
	}
}

void USART2_IRQHandler(void)
{
	if(GET_UART2_TC_FLAG() == INTERRUPT_ACTIVE)
	{
		CLEAR_UART2_TC_FLAG();
		if(USART2_TC_INT_Callback != NULL)
		{
			USART2_TC_INT_Callback();
		}
	}
	if(GET_UART2_RXNE_FLAG() == INTERRUPT_ACTIVE)
	{
		CLEAR_UART2_RXNE_FLAG();
		if(USART2_RXNE_INT_Callback != NULL)
		{
			USART2_RXNE_INT_Callback();
		}
	}
}
void USART6_IRQHandler(void)
{
	if(GET_UART6_TC_FLAG() == INTERRUPT_ACTIVE)
	{
		CLEAR_UART6_TC_FLAG();
		if(USART6_TC_INT_Callback != NULL)
		{
			USART6_TC_INT_Callback();
		}
	}
	if(GET_UART6_RXNE_FLAG() == INTERRUPT_ACTIVE)
	{
		CLEAR_UART6_RXNE_FLAG();
		if(USART6_RXNE_INT_Callback != NULL)
		{
			USART6_RXNE_INT_Callback();
		}
	}
}
/**********************************************************************************************************************
 *  END OF FILE: UART_program.c
 *********************************************************************************************************************/

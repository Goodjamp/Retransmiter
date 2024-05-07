#ifndef __BSP_H__
#define __BSP_H__

/**************************USART TARGET**********/
#define USART_CRSF                         USART1

/**************************I2C TARGET************/
#define LSM303DLHC_I2C                     I2C1

/**************************DMA TARGET************/

// CRSF USART
#define CRSF_USART_DMA                     DMA2
#define CRSF_USART_RX_DMA_STREAM           LL_DMA_STREAM_5
#define CRSF_USART_RX_DMA_CHANNEL          LL_DMA_CHANNEL_4
#define CRSF_USART_TX_DMA_STREAM           LL_DMA_STREAM_7
#define CRSF_USART_TX_DMA_CHANNEL          LL_DMA_CHANNEL_4

// CRSF USART M2M (only DMA 2 support M2M mode)
#define CRSF_USART_M2M_DMA                 DMA2
#define CRSF_USART_M2M_DMA_STREAM          LL_DMA_STREAM_1
#define CRSF_USART_M2M_DMA_CHANNEL         LL_DMA_CHANNEL_4

// LSM303DLHC I2C
#define LSM303DLHC_I2C_DMA                         DMA1

#define LSM303DLHC_I2C_TX_DMA_STREAM               LL_DMA_STREAM_1
#define LSM303DLHC_I2C_TX_DMA_STREAM_IRQ           DMA1_Stream1_IRQn
#define LSM303DLHC_I2C_TX_DMA_STREAM_IRQHandler    DMA1_Stream1_IRQHandler
#define LSM303DLHC_I2C_TX_DMA_CHANNEL              LL_DMA_CHANNEL_0

#define LSM303DLHC_I2C_RX_DMA_STREAM               LL_DMA_STREAM_0
#define LSM303DLHC_I2C_RX_DMA_STREAM_IRQ           DMA1_Stream0_IRQn
#define LSM303DLHC_I2C_RX_DMA_STREAM_IRQHandler    DMA1_Stream0_IRQHandler
#define LSM303DLHC_I2C_RX_DMA_CHANNEL              LL_DMA_CHANNEL_1

/**************************TIMERS TARGET***********/

#define SERVO_CONTROL_TIMER                 TIM3

/**************************EXTERNAL INTERRUPT TARGET***********/

// LSM303DLHC DRDY
#define LSM303DLHC_DRDY_IRQ               EXTI9_5_IRQn
#define LSM303DLHC_DRDY_IRQHandler        EXTI9_5_IRQHandler
#define LSM303DLHC_DRDY_EXT_LINE          LL_EXTI_LINE_7
#define LSM303DLHC_DRDY_SYS_PORT          LL_SYSCFG_EXTI_PORTA
#define LSM303DLHC_DRDY_SYS_LINE          LL_SYSCFG_EXTI_LINE7

/**************************GPIO TARGET***********/

//SERVO control horizontal
#define SERVO_H_GPIO_PORT                  GPIOA
#define SERVO_H_GPIO_PIN                   LL_GPIO_PIN_6

//SERVO control vertical
#define SERVO_V_GPIO_PORT                  GPIOB
#define SERVO_V_GPIO_PIN                   LL_GPIO_PIN_5

// LSM303DLHC I2C
#define LSM303DLHC_I2C_GPIO_PORT           GPIOB
#define LSM303DLHC_I2C_GPIO_SCL_PIN        LL_GPIO_PIN_8
#define LSM303DLHC_I2C_GPIO_SDA_PIN        LL_GPIO_PIN_9

// LSM303DLHC DRDY
#define LSM303DLHC_DRDY_GPIO_PORT          GPIOA
#define LSM303DLHC_DRDY_GPIO_PIN           LL_GPIO_PIN_7

// CRSF USART
#define CRSF_USART_GPIO_TX_PORT            GPIOA
#define CRSF_USART_GPIO_TX_PIN             LL_GPIO_PIN_9
#define CRSF_USART_GPIO_RX_PORT            GPIOA
#define CRSF_USART_GPIO_RX_PIN             LL_GPIO_PIN_10

// KEY_CONTROL
#define KEY_CONTROL_VIDEO_CH_58_RX_PORT    GPIOA
#define KEY_CONTROL_VIDEO_CH_58_RX_PIN     LL_GPIO_PIN_0
#define KEY_CONTROL_VIDEO_CH_13_RX_PORT    GPIOA
#define KEY_CONTROL_VIDEO_CH_13_RX_PIN     LL_GPIO_PIN_1
#define KEY_CONTROL_VIDEO_CH_58_TX_PORT    GPIOB
#define KEY_CONTROL_VIDEO_CH_58_TX_PIN     LL_GPIO_PIN_0
#define KEY_CONTROL_VIDEO_SOURCE_PORT      GPIOB
#define KEY_CONTROL_VIDEO_SOURCE_PIN       LL_GPIO_PIN_1

// LED INDICATION
#define LED_PORT            GPIOC
#define LED_PIN             LL_GPIO_PIN_13

// BUTTONS
#define BUTTONS_MENU_PORT                  GPIOA
#define BUTTONS_MENU_PIN                   LL_GPIO_PIN_12
#define BUTTONS_UP_PORT                    GPIOA
#define BUTTONS_UP_PIN                     LL_GPIO_PIN_11
#define BUTTONS_DOWN_PORT                  GPIOB
#define BUTTONS_DOWN_PIN                   LL_GPIO_PIN_15
#define BUTTONS_ENTER_PORT                 GPIOB
#define BUTTONS_ENTER_PIN                  LL_GPIO_PIN_14

// DEBUG
#define DEBUG_1_GPIO_PORT                  GPIOA
#define DEBUG_1_GPIO_PIN                   LL_GPIO_PIN_3
//#define DEBUG_2_GPIO_PORT                GPIOB
//#define DEBUG_2_GPIO_PIN                 LL_GPIO_PIN_0
//#define DEBUG_3_GPIO_PORT                GPIOB
//#define DEBUG_3_GPIO_PIN                 LL_GPIO_PIN_1
#define DEBUG_2_GPIO_PORT                  GPIOB
#define DEBUG_2_GPIO_PIN                   LL_GPIO_PIN_4
#define DEBUG_3_GPIO_PORT                  GPIOB
#define DEBUG_3_GPIO_PIN                   LL_GPIO_PIN_5
#define DEBUG_4_GPIO_PORT                  GPIOB
#define DEBUG_4_GPIO_PIN                   LL_GPIO_PIN_3
#define DEBUG_5_GPIO_PORT                  GPIOA
#define DEBUG_5_GPIO_PIN                   LL_GPIO_PIN_15

#endif // __BSP_H__

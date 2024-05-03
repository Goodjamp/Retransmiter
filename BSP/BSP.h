#ifndef __BSP_H__
#define __BSP_H__

/**************************USART TARGET**********/
#define USART_CRSF                   USART1

/**************************SPI TARGET************/
#define ETH_SPI_SPI                  SPI1

/**************************I2C TARGET************/
#define SSD1306_I2C                  I2C1

/**************************DMA TARGET************/

// CRSF USART
#define CRSF_USART_DMA               DMA2
#define CRSF_USART_RX_DMA_STREAM     LL_DMA_STREAM_5
#define CRSF_USART_RX_DMA_CHANNEL    LL_DMA_CHANNEL_4
#define CRSF_USART_TX_DMA_STREAM     LL_DMA_STREAM_7
#define CRSF_USART_TX_DMA_CHANNEL    LL_DMA_CHANNEL_4

// CRSF USART M2M (only DMA 2 support M2M mode)
#define CRSF_USART_M2M_DMA            DMA2
#define CRSF_USART_M2M_DMA_STREAM     LL_DMA_STREAM_1
#define CRSF_USART_M2M_DMA_CHANNEL    LL_DMA_CHANNEL_4

// SSD1306 I2C
#define SSD1306_I2C_DMA               DMA1
#define SSD1306_I2C_DMA_STREAM        LL_DMA_STREAM_6
#define SSD1306_I2C_DMA_CHANNEL       LL_DMA_CHANNEL_1

/**************************GPIO TARGET***********/

// CRSF USART
#define CRSF_USART_GPIO_TX_PORT      GPIOA
#define CRSF_USART_GPIO_TX_PIN       LL_GPIO_PIN_9
#define CRSF_USART_GPIO_RX_PORT      GPIOA
#define CRSF_USART_GPIO_RX_PIN       LL_GPIO_PIN_10

// KEY_CONTROL

#define KEY_CONTROL_VIDEO_CH_58_PORT       GPIOB
#define KEY_CONTROL_VIDEO_CH_58_PIN        LL_GPIO_PIN_0
#define KEY_CONTROL_VIDEO_CH_12_PORT       GPIOA
#define KEY_CONTROL_VIDEO_CH_12_PIN        LL_GPIO_PIN_0
#define KEY_CONTROL_BINDING_58_PORT        GPIOA
#define KEY_CONTROL_BINDING_58_PIN         LL_GPIO_PIN_1
#define KEY_CONTROL_VIDEO_SOURCE_PORT      GPIOB
#define KEY_CONTROL_VIDEO_SOURCE_PIN       LL_GPIO_PIN_1

// BUTTONS
#define BUTTONS_MENU_PORT            GPIOA
#define BUTTONS_MENU_PIN             LL_GPIO_PIN_12
#define BUTTONS_UP_PORT              GPIOA
#define BUTTONS_UP_PIN               LL_GPIO_PIN_11
#define BUTTONS_DOWN_PORT            GPIOB
#define BUTTONS_DOWN_PIN             LL_GPIO_PIN_15
#define BUTTONS_ENTER_PORT           GPIOB
#define BUTTONS_ENTER_PIN            LL_GPIO_PIN_14

// DEBUG
#define DEBUG_1_GPIO_PORT            GPIOA
#define DEBUG_1_GPIO_PIN             LL_GPIO_PIN_3
#define DEBUG_2_GPIO_PORT            GPIOB
#define DEBUG_2_GPIO_PIN             LL_GPIO_PIN_0
#define DEBUG_3_GPIO_PORT            GPIOB
#define DEBUG_3_GPIO_PIN             LL_GPIO_PIN_1
#define DEBUG_4_GPIO_PORT            GPIOB
#define DEBUG_4_GPIO_PIN             LL_GPIO_PIN_4
#define DEBUG_5_GPIO_PORT            GPIOB
#define DEBUG_5_GPIO_PIN             LL_GPIO_PIN_5
#define DEBUG_6_GPIO_PORT            GPIOB
#define DEBUG_6_GPIO_PIN             LL_GPIO_PIN_3
#define DEBUG_7_GPIO_PORT            GPIOA
#define DEBUG_7_GPIO_PIN             LL_GPIO_PIN_15

#endif // __BSP_H__

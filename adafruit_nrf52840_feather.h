
// Adafruit nRF52840 Feather Board Definitions
// https://learn.adafruit.com/assets/68545

#ifndef ADAFRUIT_NRF52840_FEATHER_H
#define ADAFRUIT_NRF52840_FEATHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

#define LEDS_NUMBER	   2

#define LED1_R		   NRF_GPIO_PIN_MAP(1,15)
#define LED2_B		   NRF_GPIO_PIN_MAP(1,10)

#define LED_1		   LED1_R
#define LED_2		   LED2_B

#define LEDS_ACTIVE_STATE 1

#define LEDS_LIST { LED_1, LED_2 }

#define LEDS_INV_MASK  LEDS_MASK

#define BSP_LED_0	   LED_1
#define BSP_LED_1	   LED_2

#define BUTTONS_NUMBER 1

#define BUTTON_1	   NRF_GPIO_PIN_MAP(1,02)
#define BUTTON_PULL	   NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1 }

#define BSP_BUTTON_0   BUTTON_1

// I2C exposed on board
#define TWI_INSTANCE_ID			0				// TWI0

// TWI
#define	SCL_PIN_NUMBER	NRF_GPIO_PIN_MAP(0,11)	// Pin 27 P0.11
#define	SDA_PIN_NUMBER	NRF_GPIO_PIN_MAP(0,12)	// Pin 29 P0.12

// UART instance exposed on board
#define	UART_INSTANCE_ID		0				// UART0

// The nRF host board may or may not have hardware pullups, and because the nrfx_uart driver requires pull-up
// for reliable inbound communications from the Notecard, the option is provided here to provide a
// software-based pullup.
#define SERIAL_SOFTWARE_PULLUP

// UART
#define RX_PIN_NUMBER	NRF_GPIO_PIN_MAP(0,24)	// Pin 48 P0.24
#define TX_PIN_NUMBER	NRF_GPIO_PIN_MAP(0,25)	// Pin 49 P0.25
#define RTS_PIN_NUMBER	NRF_GPIO_PIN_MAP(0,0)	// No HWFC
#define CTS_PIN_NUMBER	NRF_GPIO_PIN_MAP(0,0)	// No HWFC

#ifdef __cplusplus
}
#endif

#endif // ADAFRUIT_NRF52840_FEATHER_H

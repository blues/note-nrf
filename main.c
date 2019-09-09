
#include "main.h"
#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrfx_twi.h"
#include "nrf_drv_twi.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_drv_power.h"
#include "nrf_serial.h"
#include "app_timer.h"
#include "app_error.h"
#include "app_util.h"
#include "boards.h"
#include "note.h"

#ifdef USING_SES
#include <cross_studio_io.h>
#endif

// Choose whether to use I2C or SERIAL for the Notecard
#define	NOTECARD_USE_I2C	true

// The Notecard serial port operates at a fixed 9600 N/8/1 with no hardware flow control.
NRF_SERIAL_DRV_UART_CONFIG_DEF(m_uart0_drv_config,
							   RX_PIN_NUMBER, TX_PIN_NUMBER,
							   RTS_PIN_NUMBER, CTS_PIN_NUMBER,
							   NRF_UART_HWFC_DISABLED, NRF_UART_PARITY_EXCLUDED,
							   NRF_UART_BAUDRATE_9600,
							   UART_DEFAULT_CONFIG_IRQ_PRIORITY);

// Serial port definitions
#define SERIAL_FIFO_TX_SIZE 32
#define SERIAL_FIFO_RX_SIZE 32
NRF_SERIAL_QUEUES_DEF(serial_queues, SERIAL_FIFO_TX_SIZE, SERIAL_FIFO_RX_SIZE);
#define SERIAL_BUFF_TX_SIZE 1
#define SERIAL_BUFF_RX_SIZE 1
NRF_SERIAL_BUFFERS_DEF(serial_buffs, SERIAL_BUFF_TX_SIZE, SERIAL_BUFF_RX_SIZE);
NRF_SERIAL_UART_DEF(serial_uart, UART_INSTANCE_ID);
void sleep_handler(void);
NRF_SERIAL_CONFIG_DEF(serial_config, NRF_SERIAL_MODE_IRQ, &serial_queues, &serial_buffs, NULL, sleep_handler);

// TWI config
const nrf_drv_twi_config_t twi_config = {
	.scl				= SCL_PIN_NUMBER,
	.sda				= SDA_PIN_NUMBER,
	.frequency			= NRF_DRV_TWI_FREQ_100K,
	.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
	.clear_bus_init		= false
};

// TWI instance
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

// Coarse-grained timer used for detecting Notecard I/O timeouts
uint64_t appClock = 0;
#define	APPTICK_MILLISECONDS 100
APP_TIMER_DEF(timerAppTick);
void timerAppTickHandler(void *context);

// Data used for Notecard I/O functions
static bool serialReceivedCharAvailable = false;
static char serialReceivedChar;
static volatile bool m_xfer_done = false;

// Forwards
void noteSerialReset(void);
void noteSerialTransmit(uint8_t *text, size_t len, bool flush);
bool noteSerialAvailable(void);
char noteSerialReceive(void);
void noteI2CReset(void);
size_t noteDebugSerialOutput(const char *message);
const char *noteI2CTransmit(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size);
const char *noteI2CReceive(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size, uint32_t *avail);
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);

// Main entry point
int main(void) {

	// Configure board support package used in this example
//	bsp_board_init(BSP_INIT_LEDS|BSP_INIT_BUTTONS);

	// Initialize peripherals including the millisecond clock used for detecting I/O timeouts
	nrf_drv_clock_init();
	nrf_drv_power_init(NULL);
	nrf_drv_clock_lfclk_request(NULL);
	app_timer_init();
	app_timer_create(&timerAppTick, APP_TIMER_MODE_REPEATED, timerAppTickHandler);
	app_timer_start(timerAppTick, APP_TIMER_TICKS(APPTICK_MILLISECONDS), NULL);

	// Register callbacks with note-c subsystem that it needs for I/O, memory, timer
	NoteSetFn(malloc, free, delay, millis);

	// Register callbacks for Notecard I/O
#if NOTECARD_USE_I2C
	NoteSetFnI2C(NOTE_I2C_ADDR_DEFAULT, NOTE_I2C_MAX_DEFAULT, noteI2CReset, noteI2CTransmit, noteI2CReceive);
#else
	NoteSetFnSerial(noteSerialReset, noteSerialTransmit, noteSerialAvailable, noteSerialReceive);
#endif

	// If running under SES, register a debug hook so that we can watch notecard I/O in the Output window
#ifdef USING_SES
	NoteSetFnDebugOutput(noteDebugSerialOutput);
#endif

	// Use this method of invoking main app code so that we can re-use familiar Arduino examples
	setup();
	while (true) loop();

}

// Serial port reset procedure, called before any I/O and called again upon I/O error
void noteSerialReset() {
	static bool first = true;
	if (first)
		first = false;
	else {
		nrf_serial_tx_abort(&serial_uart);
		nrf_serial_rx_drain(&serial_uart);
		nrf_serial_uninit(&serial_uart);
	}
	nrf_serial_init(&serial_uart, &m_uart0_drv_config, &serial_config);
#ifdef SERIAL_SOFTWARE_PULLUP
	nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_PULLUP);
#endif
}

// Serial write data function
void noteSerialTransmit(uint8_t *text, size_t len, bool flush) {
	nrf_serial_write(&serial_uart, text, len, NULL, NRF_SERIAL_MAX_TIMEOUT);
	if (flush)
		nrf_serial_flush(&serial_uart, 0);
}

// Serial "is anything available" function
bool noteSerialAvailable() {
	if (!serialReceivedCharAvailable)
		if (nrf_serial_read(&serial_uart, &serialReceivedChar, 1, NULL, 1) == NRF_SUCCESS)
			serialReceivedCharAvailable = true;
	return serialReceivedCharAvailable;
}

// Blocking serial read a byte function (generally only called if known to be available)
char noteSerialReceive() {
	if (serialReceivedCharAvailable) {
		serialReceivedCharAvailable = false;
	} else {
		while (nrf_serial_read(&serial_uart, &serialReceivedChar, 1, NULL, 1000) != NRF_SUCCESS) ;
	}
	return serialReceivedChar;
}

// I2C reset procedure, called before any I/O and called again upon I/O error
void noteI2CReset() {
	static bool first = true;
	if (first)
		first = false;
	else
		nrfx_twi_uninit(&m_twi.u.twi);
	nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
	nrf_drv_twi_enable(&m_twi);
}

// Transmits in master mode an amount of data, in blocking mode.	 The address
// is the actual address; the caller should have shifted it right so that the
// low bit is NOT the read/write bit. An error message is returned, else NULL if success.
const char *noteI2CTransmit(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size) {
	char *errstr = NULL;
	int writelen = sizeof(uint8_t) + Size;
	uint8_t *writebuf = malloc(writelen);
	if (writebuf == NULL) {
		errstr = "i2c: insufficient memory (write)";
	} else {
		writebuf[0] = Size;
		memcpy(&writebuf[1], pBuffer, Size);
		m_xfer_done = false;
	    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, DevAddress, writebuf, writelen, false);
		free(writebuf);
		if (err_code != NRF_SUCCESS) {
			errstr = "i2c: write error";
		} else {
		    while (m_xfer_done == false);
		}
	}
	return errstr;
}

// Receives in master mode an amount of data in blocking mode. An error mesage returned, else NULL if success.
const char *noteI2CReceive(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size, uint32_t *available) {
	const char *errstr = NULL;
	ret_code_t err_code;
	
	// Retry transmit errors several times, because it's harmless to do so
	for (int i=0; i<3; i++) {
		uint8_t hdr[2];
		hdr[0] = (uint8_t) 0;
		hdr[1] = (uint8_t) Size;
		m_xfer_done = false;
		err_code = nrf_drv_twi_tx(&m_twi, DevAddress, hdr, sizeof(hdr), false);
		if (err_code == NRF_SUCCESS) {
		    while (m_xfer_done == false);
			errstr = NULL;
			break;
		}
		errstr = "i2c: write error";
	}

	// Only receive if we successfully began transmission
	if (errstr == NULL) {

		int readlen = Size + (sizeof(uint8_t)*2);
		uint8_t *readbuf = malloc(readlen);
		if (readbuf == NULL) {
			errstr = "i2c: insufficient memory (read)";
		} else {
		    m_xfer_done = false;
			err_code = nrf_drv_twi_rx(&m_twi, DevAddress, readbuf, readlen);
			if (err_code != NRF_SUCCESS) {
				errstr = "i2c: read error";
			} else {
			    while (m_xfer_done == false);
				uint8_t availbyte = readbuf[0];
				uint8_t goodbyte = readbuf[1];
				if (goodbyte != Size) {
					errstr = "i2c: incorrect amount of data";
				} else {
					*available = availbyte;
					memcpy(pBuffer, &readbuf[2], Size);
				}
			}
			free(readbuf);
		}
	}

	// Done
	return errstr;

}

// Handle sleep while waiting for serial I/O
void sleep_handler(void) {
	__WFE();
	__SEV();
	__WFE();
}

// One second timer handler
void timerAppTickHandler(void *context) {
	appClock += APPTICK_MILLISECONDS;
}

// Delay the specified number of milliseconds
void delay(uint32_t ms) {
	nrf_delay_ms(ms);
}

// Get the number of app milliseconds since boot (this will wrap)
long unsigned int millis() {
	return (long unsigned int) appClock;
}

// If there's a place to do it, display debug output
#ifdef USING_SES
size_t noteDebugSerialOutput(const char *message) {
	debug_printf(message);
}
#endif

// I2C event handler
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context) {
    switch (p_event->type) {
        case NRF_DRV_TWI_EVT_DONE:
            m_xfer_done = true;
            break;
        default:
            break;
    }
}

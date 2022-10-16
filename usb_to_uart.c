/**
 * Pico as usb to uart converter.
 * Must be configured as stdio via usb in CMakeLists.txt
 * Atmel_USB_CDC_Virtual_COM_Driver_v6.0.0.0 (or equivalent) must be installed on computer
 * Putty or similar software can be used to interface to Pico via USB
 *
 * Version 1.0      16.10.2022      Uart config now accepts both upper and lower case. Changed getchar_timeout_us to 0 timeout.
 *                              
 **/

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
//#include "hardware/irq.h"

#define UART_ID uart0
#define RX_PIN 17
#define TX_PIN 16
#define JUMPER_PIN 21

int main() {

    int x = 0;              //Serial input temp variable
    int i = 0;              //Serial input buffer index
    char input[100] = "x";  //Serial input buffer
    
    stdio_init_all();
    stdio_usb_init();

    //Initialize onboard LED:
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    //Initialize input pin used for selecting ASCII vs raw output:
    gpio_init(JUMPER_PIN);
    gpio_set_dir(JUMPER_PIN, GPIO_IN);
    gpio_pull_up(JUMPER_PIN);
    
start:

    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    
    //Wait for USB connection:
    while(!stdio_usb_connected());
    {
        ;
    }

    printf("*** Pico is connected to PC! ***\n");
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    
    //Request UART config from computer:
    printf("Please enter UART config (data bits, parity and stop bits, e.g: 8N1):\n");

    bool uart_config_ok = false;
    uint data_bits;
    uint stop_bits;
    uart_parity_t parity;
    i = 0;
    while(!uart_config_ok)
    {
        //Check for received data from usb:
        x = getchar_timeout_us(10);
        if(x != PICO_ERROR_TIMEOUT)
        {
            input[i] = x;
            i++;
            if(x == '\r')
            {
                if(i != 4)
                    printf("Error: config too short or too long!\n");
                else if((input[0] < '5') || (input[0] > '8'))
                    printf("Error: invalid nr of data bits. Enter value between 5-8!\n");
                else if(!((input[1] == 'N') || (input[1] == 'n') || (input[1] == 'E') || (input[1] == 'e') || (input[1] == 'O') || (input[1] == 'o')))
                    printf("Error: invalid parity value. Select O, E or N!\n");
                else if((input[2] < '1') || (input[2] > '2'))
                    printf("Error: invalid nr of stop bits. Enter value between 1-2!\n");
                else
                {
                    data_bits = input[0]-48;
                    stop_bits = input[2]-48;
                    if((input[1] == 'N') || (input[1] == 'n'))
                        parity = UART_PARITY_NONE;
                    else if((input[1] == 'E') || (input[1] == 'e'))
                        parity = UART_PARITY_EVEN;
                    else if((input[1] == 'O') || (input[1] == 'o'))
                        parity = UART_PARITY_ODD;

                    printf("Config accepted!\n");
                    uart_config_ok = true;
                }
                i = 0;
            }
        }

        //If USB connection is lost, go to start of program and wait for reconnect
        if(!stdio_usb_connected())
            goto start;
    }

    //Request UART baud rate from computer:
    printf("Enter UART baud rate (e.g: 9600):\n");
    bool uart_baud_ok = false;
    int baud_rate = 0;
    i = 0;
    while(!uart_baud_ok)
    {
        //Check for received data from usb:
        x = getchar_timeout_us(10);
        if(x != PICO_ERROR_TIMEOUT)
        {
            input[i] = x;
            i++;
            if(x == '\r')
            {
                baud_rate = atoi(input);
                if((baud_rate < 1) || (baud_rate > 1000000))
                    printf("Error: Invalid baud config!\n");
                else
                {   
                    printf("Baud rate accepted!\n");
                    uart_baud_ok = true;
                }
                i = 0;
            }
        }

        //If USB connection is lost, go to start of program and wait for reconnect
        if(!stdio_usb_connected())
            goto start;
    }

    //Set up UART -------------------------------------------------------------------------------------------------------------------
    uart_init(UART_ID, baud_rate);						    // Set up our UART with the configured baud rate.
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);				// Set GPIO pin 1 to UART function
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);				// Set GPIO pin 0 to UART function
    uart_set_hw_flow(UART_ID, false, false);			    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_format(UART_ID, data_bits, stop_bits, parity);	// Set our configured data format
    //uart_set_fifo_enabled(UART_ID, false);				    // Turn off FIFO's - we want to do this character by character
    
    printf("UART connection is enabled!\n\n");
    
    
    //MAIN LOOP ----------------------------------------------------------------------------------------------------------------------
    char rx_data = 0;
    char tx_data = 0;
    bool ascii_output;
    int j = 0;
    
    while(true)
    {
        //Check ASCII vs raw output selector jumper:
        ascii_output = gpio_get(JUMPER_PIN);    //Pullup active. No jumper = ascii output
        
        //Poll for received UART data and write to computer via USB:
        while(uart_is_readable(UART_ID))
        {
            rx_data = uart_getc(UART_ID);
            if(ascii_output)    //Print ASCII output to PC
                printf("%c", rx_data);
            else                //Else, print raw (decimal) output to PC. 8 bytes per line.
            {
                printf("%3d ", rx_data);
                j++;
                if(j==8)
                {
                    printf("\n");
                    j = 0;
                }
            }
        }

        //Poll for received USB data and write to UART interface:
        x = getchar_timeout_us(0);
        if(x != PICO_ERROR_TIMEOUT)
        {
            tx_data = x;
            uart_tx_wait_blocking(UART_ID);     //Wait for clear TX FIFO
            uart_putc_raw(UART_ID, tx_data);
        }

        //If USB connection is lost, go to start of program and wait for reconnect:
        if(!stdio_usb_connected())
            goto start;

    }

    return 0;

}

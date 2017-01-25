//*****************************************************************************
//
// enet_io.c - I/O control via a web server.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.


//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "drivers/pinout.h"
#include "io.h"
#include "cgifuncs.h"



//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet-based I/O Control (enet_io)</h1>
//!
//! This example application demonstrates web-based I/O control using the
//! Tiva Ethernet controller and the lwIP TCP/IP Stack.  DHCP is used to
//! obtain an Ethernet address.  If DHCP times out without obtaining an
//! address, a static IP address will be chosen using AutoIP.  The address that
//! is selected will be shown on the UART allowing you to access the
//! internal web pages served by the application via your normal web browser.
//!
//! Two different methods of controlling board peripherals via web pages are
//! illustrated via pages labeled ``IO Control Demo 1'' and ``IO Control
//! Demo 2'' in the navigation menu on the left of the application's home page.
//! In both cases, the example allows you to toggle the state of the user LED
//! on the board and set the speed of a blinking LED.
//!
//! ``IO Control Demo 1'' uses JavaScript running in the web browser to send
//! HTTP requests for particular special URLs.  These special URLs are
//! intercepted in the file system support layer (io_fs.c) and used to
//! control the LED and animation LED.  Responses generated by the board are
//! returned to the browser and inserted into the page HTML dynamically by
//! more JavaScript code.
//!
//! ``IO Control Demo 2'' uses standard HTML forms to pass parameters to CGI
//! (Common Gateway Interface) handlers running on the board.  These handlers
//! process the form data and control the animation and LED as requested before
//! sending a response page (in this case, the original form) back to the
//! browser.  The application registers the names and handlers for each of its
//! CGIs with the HTTPD server during initialization and the server calls
//! these handlers after parsing URL parameters each time one of the CGI URLs
//! is requested.
//!
//! Information on the state of the various controls in the second demo is
//! inserted into the served HTML using SSI (Server Side Include) tags which
//! are parsed by the HTTPD server in the application.  As with the CGI
//! handlers, the application registers its list of SSI tags and a handler
//! function with the web server during initialization and this handler is
//! called whenever any registered tag is found in a .shtml, .ssi, .shtm or
//! .xml file being served to the browser.
//!
//! In addition to LED and animation speed control, the second example also
//! allows a line of text to be sent to the board for output to the UART.
//! This is included to illustrate the decoding of HTTP text strings.
//!
//! Note that the web server used by this example has been modified from the
//! example shipped with the basic lwIP package.  Additions include SSI and
//! CGI support along with the ability to have the server automatically insert
//! the HTTP headers rather than having these built in to the files in the
//! file system image.
//!
//! Source files for the internal file system image can be found in the ``fs''
//! directory.  If any of these files are changed, the file system image
//! (io_fsdata.h) should be rebuilt by running the following command from the
//! enet_io directory:
//!
//! ../../../../tools/bin/makefsfile -i fs -o io_fsdata.h -r -h -q
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY    0x80
#define ETHERNET_INT_PRIORITY   0xC0
#define UART4_INT_PRIORITY      0x00

//*****************************************************************************
//
// A set of flags.  The flag bits are defined as follows:
//
//     0 -> An indicator that the animation timer interrupt has occurred.
//
//*****************************************************************************
#define FLAG_TICK            0
static volatile unsigned long g_ulFlags;

//*****************************************************************************
//
// External Application references.
//
//*****************************************************************************
extern void httpd_init(void);
extern void initFifo(void);

//*****************************************************************************
//
// The current IP address.
//
//*****************************************************************************
uint32_t g_ui32IPAddress;

//unsigned char GetValueRespone[20] = {0};

//*****************************************************************************
//
// The system clock frequency.  Used by the SD card driver.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the lwIP timer handler.
    //
    //UARTprintf("Dentro de ST ");
    lwIPTimer(SYSTICKMS);
}

//*****************************************************************************
//
// The interrupt handler for the timer used to pace the animation.
//
//*****************************************************************************
void
AnimTimerIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    MAP_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    //UARTCharPut(UART0_BASE,MAP_SysTickValueGet());
    //UARTprintf("Dentro de AT ");
    //
    // Indicate that a timer interrupt has occurred.
    //
    HWREGBITW(&g_ulFlags, FLAG_TICK) = 1;
}

//*****************************************************************************
//
// Display an lwIP type IP Address.
//
//*****************************************************************************
void
DisplayIPAddress(uint32_t ui32Addr)
{
    char pcBuf[16];

    //
    // Convert the IP Address into a string.
    //
    usprintf(pcBuf, "%d.%d.%d.%d", ui32Addr & 0xff, (ui32Addr >> 8) & 0xff,
            (ui32Addr >> 16) & 0xff, (ui32Addr >> 24) & 0xff);

    //
    // Display the string.
    //
    //UARTprintf(pcBuf);
}

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************



//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void
lwIPHostTimerHandler(void)
{
    uint32_t ui32Idx, ui32NewIPAddress;

    //
    // Get the current IP address.
    //
    ui32NewIPAddress = lwIPLocalIPAddrGet();

    //
    // See if the IP address has changed.
    //
    if(ui32NewIPAddress != g_ui32IPAddress)
    {
        //
        // See if there is an IP address assigned.
        //
        if(ui32NewIPAddress == 0xffffffff)
        {
            //
            // Indicate that there is no link.
            //
           // UARTprintf("Waiting for link.\n");
        }
        else if(ui32NewIPAddress == 0)
        {
            //
            // There is no IP address, so indicate that the DHCP process is
            // running.
            //
           // UARTprintf("Waiting for IP address.\n");
        }
        else
        {
            //
            // Display the new IP address.
            //
           // UARTprintf("IP Address: ");
            DisplayIPAddress(ui32NewIPAddress);
           // UARTprintf("\n");
           // UARTprintf("Open a browser and enter the IP address.\n");
        }

        //
        // Save the new IP address.
        //
        g_ui32IPAddress = ui32NewIPAddress;
    }

    //
    // If there is not an IP address.
    //
    if((ui32NewIPAddress == 0) || (ui32NewIPAddress == 0xffffffff))
    {
        //
        // Loop through the LED animation.
        //

        for(ui32Idx = 1; ui32Idx < 17; ui32Idx++)
        {

            //
            // Toggle the GPIO
            //
            MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                    (MAP_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_1) ^
                     GPIO_PIN_1));

            SysCtlDelay(g_ui32SysClock/(ui32Idx << 1));

        }
    }
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller and lwIP
// TCP/IP stack to control various peripherals on the board via a web
// browser.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MACArray[8];         //Arreglo para guardar la direcci�n MAC del Micro

    //El controlador Ethernet y la PHY integrada reciven dos relojes:
    //Avilitar el OSC principal que es requerido para la capa f�sica
    //SYSCTL_MOSC_HIGHFREQ se usa cuando el MOSC es mayor a los 10 MHz (Ver manual API Functions)
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

    // Correr el PLL (oscilador principal) a 120 MHz
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);


    //API para inicializar/configurar puerto Ethernet
    PinoutSet(true, false);
    UARTStdioConfig(0, 115200, g_ui32SysClock);


    //Configurar UART1 --> Puerto 1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);  //Habilitar UART1
    GPIOPinConfigure(GPIO_PB1_U1TX);              //Configurar pin PB1 como funci�n alterna (TX)
    GPIOPinConfigure(GPIO_PB0_U1RX);              //Configurar pin PB0 como funci�n alterna (RX)
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_1 | GPIO_PIN_0); //Configurar pin PA7 para usarlo como UART

    //Configurar UART2 --> Puerto 2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);  //Habilitar UART2
    GPIOPinConfigure(GPIO_PA7_U2TX);              //Configurar pin PA7 como funci�n alterna (TX)
    GPIOPinConfigure(GPIO_PA6_U2RX);              //Configurar pin PA6 como funci�n alterna (RX)
    GPIOPinConfigure(GPIO_PD4_U2RX);              //Configurar pin PD4 como funci�n alterna (RX)
    GPIOPinConfigure(GPIO_PD5_U2TX);              //Configurar pin PD5 como funci�n alterna (TX)
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_7 | GPIO_PIN_6); //Configurar pin PA7 para usarlo como UART
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5); //Configurar pin PD5 para usarlo como UART

    //Configurar UART3 --> Puerto 3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);  //Habilitar UART3
    GPIOPinConfigure(GPIO_PA5_U3TX);              //Configurar pin PA5 como funci�n alterna (TX)
    GPIOPinConfigure(GPIO_PA4_U3RX);              //Configurar pin PA4 como funci�n alterna (RX)
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_4 | GPIO_PIN_5); //Configurar pin 4,5 para usarlos como UART

    //Configurar UART4 --> Puerto 4
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    GPIOPinConfigure(GPIO_PK1_U4TX);
    GPIOPinConfigure(GPIO_PK0_U4RX);
    GPIOPinTypeUART(GPIO_PORTK_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //Configurar UART5 --> Puerto 5
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
    GPIOPinConfigure(GPIO_PC7_U5TX);
    GPIOPinConfigure(GPIO_PC6_U5RX);
    GPIOPinTypeUART(GPIO_PORTC_BASE,GPIO_PIN_6 | GPIO_PIN_7);

    //Configurar UART6 --> Puerto 6
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART6);
    GPIOPinConfigure(GPIO_PP1_U6TX);
    GPIOPinConfigure(GPIO_PP0_U6RX);
    GPIOPinTypeUART(GPIO_PORTP_BASE,GPIO_PIN_0 | GPIO_PIN_1);

    //Configurar UART7 --> Puerto 7
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);
    GPIOPinConfigure(GPIO_PC5_U7TX);
    GPIOPinConfigure(GPIO_PC4_U7RX);
    GPIOPinTypeUART(GPIO_PORTC_BASE,GPIO_PIN_4 | GPIO_PIN_5);


    // SysTick para generar interrupci�n periodica
    MAP_SysTickPeriodSet(g_ui32SysClock / SYSTICKHZ);
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();

    //
    // Configure the hardware MAC address for Ethernet Controller filtering of
    // incoming packets.  The MAC address will be stored in the non-volatile
    // USER0 and USER1 registers.
    //
    MAP_FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        //
        // Let the user know there is no MAC address
        //
       // UARTprintf("No MAC programmed!\n");

        while(1)
        {
        }
    }


    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split
    // MAC address needed to program the hardware registers, then program
    // the MAC address into the Ethernet Controller registers.
    //
    pui8MACArray[0] = ((ui32User0 >>  0) & 0xff);
    pui8MACArray[1] = ((ui32User0 >>  8) & 0xff);
    pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
    pui8MACArray[3] = ((ui32User1 >>  0) & 0xff);
    pui8MACArray[4] = ((ui32User1 >>  8) & 0xff);
    pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);

    //
    // Initialze the lwIP library, using DHCP.
    //
    //lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);
    lwIPInit(g_ui32SysClock, pui8MACArray, (192u << 24) | (168u << 16) | (11 << 8) | 166,
            (255u << 24) | (255u << 16) | (255 << 8) | 0,
            (255u << 24) | (255u << 16) | (255 << 8) | 0, IPADDR_USE_STATIC);



    //Inicializar Servidor Web
    httpd_init();
//    initFifo();

    //
    // Set the interrupt priorities.  We set the SysTick interrupt to a higher
    // priority than the Ethernet interrupt to ensure that the file system
    // tick is processed if SysTick occurs while the Ethernet handler is being
    // processed.  This is very likely since all the TCP/IP and HTTP work is
    // done in the context of the Ethernet interrupt.
    //
    MAP_IntPrioritySet(INT_UART1, 0);
    MAP_IntPrioritySet(INT_UART2, 0);
    MAP_IntPrioritySet(INT_UART3, 0);
    MAP_IntPrioritySet(INT_UART4, 0);
    MAP_IntPrioritySet(INT_UART5, 0);
    MAP_IntPrioritySet(INT_UART6, 0);
    MAP_IntPrioritySet(INT_UART7, 0);
    MAP_IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
    MAP_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

    //
    // Initialize IO controls
    //
    io_init();
 

    IntEnable(INT_UART1);
//    UARTIntEnable(UART1_BASE, UART_INT_RX); //| UART_INT_RT
    UARTFIFOLevelSet(UART1_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);

    IntEnable(INT_UART2);
//    UARTIntEnable(UART2_BASE, UART_INT_RX); //| UART_INT_RT
    UARTFIFOLevelSet(UART2_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);

    IntEnable(INT_UART3);
//    UARTIntEnable(UART3_BASE, UART_INT_RX); //| UART_INT_RT
    UARTFIFOLevelSet(UART3_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);

    IntEnable(INT_UART4);
//    UARTIntEnable(UART4_BASE, UART_INT_RX); //| UART_INT_RT
    UARTFIFOLevelSet(UART4_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);

    IntEnable(INT_UART5);
//    UARTIntEnable(UART5_BASE, UART_INT_RX); //| UART_INT_RT
    UARTFIFOLevelSet(UART5_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);

    IntEnable(INT_UART6);
//    UARTIntEnable(UART6_BASE, UART_INT_RX); //| UART_INT_RT
    UARTFIFOLevelSet(UART6_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);

    IntEnable(INT_UART7);
//    UARTIntEnable(UART7_BASE, UART_INT_RX); //| UART_INT_RT
    UARTFIFOLevelSet(UART7_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    if(WatchdogLockState(WATCHDOG0_BASE) == true){
        WatchdogUnlock(WATCHDOG0_BASE);
    }
    WatchdogReloadSet(WATCHDOG0_BASE, g_ui32SysClock*10);
    WatchdogResetEnable(WATCHDOG0_BASE);
    WatchdogEnable(WATCHDOG0_BASE);

    //
    // Loop forever, processing the on-screen animation.  All other work is
    // done in the interrupt handlers.
    //
    while(1)
    {
        //if(WatchdogRunning(WATCHDOG0_BASE) == true){
        //    UARTCharPut(UART0_BASE,0X16);
        //}

        //
        // Wait for a new tick to occur.
        //
        while(!g_ulFlags)
        {
            //
            // Do nothing.
            //
        }

        //
        // Clear the flag now that we have seen it.
        //

        HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;

        //
        // Toggle the GPIO
        //
        MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                (MAP_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_1) ^
                 GPIO_PIN_1));
    }
}

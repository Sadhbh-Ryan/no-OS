/***************************************************************************//**
 *   @file   tcp_echo_server_example.c
 *   @brief  Implementation of the TCP echo server example.
 *   @author Ciprian Regus (ciprian.regus@analog.com)
********************************************************************************
 * Copyright 2023(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "common_data.h"

#include "lwip_socket.h"
#include "tcp_socket.h"
#include "no_os_error.h"
#include "adin1110.h"
#include "lwip_adin1110.h"
#include "network_interface.h"
#include "no_os_delay.h"
#include "no_os_init.h"

void HardFault_Handler(void) {
	// Optionally collect fault information
     volatile uint32_t *hardfault_address = (uint32_t *)0xE000ED2C; // HFSR address
     volatile uint32_t hfsr = *hardfault_address;

     // Similarly, read the CFSR which is at 0xE000ED28
     volatile uint32_t cfsr = *(volatile uint32_t *)0xE000ED28;
     // Read MMFAR and BFAR if needed
     volatile uint32_t mmfar = *(volatile uint32_t *)0xE000ED34;
     volatile uint32_t bfar  = *(volatile uint32_t *)0xE000ED38;

    // Now invoke a breakpoint for the debugger to catch
    __asm("BKPT #01");

     while (1); // Stay here so you can inspect via debugger
}

/***************************************************************************//**
 * @brief TCP echo example main execution.
 *
 * @return ret - Result of the example execution.
*******************************************************************************/
int ping_example_main(){
	int ret;

	struct no_os_uart_desc *uart_desc;
	struct lwip_network_desc *lwip_desc;
	struct no_os_gpio_desc *adin1110_reset_gpio;

	uint8_t adin1110_mac_address[6] = {0x00, 0xe0, 0x22, 0x03, 0x25, 0x60};

	ret = no_os_uart_init(&uart_desc, &uart_ip);
	if (ret)
		return ret;
	no_os_uart_stdio(uart_desc);

	printf("Starting Ping Example\n");

	uint32_t connect_timeout = 5000;

	memcpy(adin1110_ip.mac_address, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);
	memcpy(lwip_ip.hwaddr, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);

	printf("Make sure your ethernet port has an address within same range\n");
	
	ret = no_os_lwip_init(&lwip_desc, &lwip_ip);
	if (ret){
		printf("LWIP init error: %d (%s)\n", ret, strerror(-ret));
		return ret;
	}

	printf("Ping the IP of the board with your computer\n");
	
	while (1) {
		no_os_lwip_step(lwip_desc, NULL);
	}

	return 0;
}

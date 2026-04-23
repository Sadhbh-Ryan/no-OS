/***************************************************************************//**
 *   @file   common_data.h
 *   @brief  Defines common data to be used by all examples.
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
#ifndef __COMMON_DATA_H__
#define __COMMON_DATA_H__

#include "no_os_uart.h"
#include "no_os_util.h"

#if defined(APARD32690_ECHO_SERVER_EXAMPLE) || defined(APARD32690_MQTT_EXAMPLE)
#include "lwip_socket.h"
#include "lwip_adin1110.h"
#endif

#include "maxim_uart.h"
#include "maxim_uart_stdio.h"
#include "maxim_gpio.h"
#include "maxim_spi.h"

extern struct no_os_uart_init_param uart_ip;

#if defined(APARD32690_ADIN1110_STANDALONE_EXAMPLE)
extern struct adin1110_init_param adin1110_ip;
#endif

#if defined(APARD32690_ECHO_SERVER_EXAMPLE) || defined(APARD32690_MQTT_EXAMPLE)
#ifndef DISABLE_SECURE_SOCKET
#define CA_CERT                                                             \
    "-----BEGIN CERTIFICATE-----\r\n"                                       \
    "MIICfDCCAiGgAwIBAgIUL0wv7pFOXhrgRA2Ojcwo5KXFs6UwCgYIKoZIzj0EAwIw\r\n"  \
    "gZIxCzAJBgNVBAYTAklFMRAwDgYDVQQIDAdNdW5zdGVyMREwDwYDVQQHDAhMaW1l\r\n"  \
    "cmljazEMMAoGA1UECgwDQURJMREwDwYDVQQLDAhTZWN1cml0eTEWMBQGA1UEAwwN\r\n"  \
    "MTkyLjE2OC4wLjEwMzElMCMGCSqGSIb3DQEJARYWc2FkaGJoLnJ5YW5AYW5hbG9n\r\n"  \
    "LmNvbTAeFw0yNTA5MTkxMzQ0NDBaFw0zNTA5MTcxMzQ0NDBaMIGSMQswCQYDVQQG\r\n"  \
    "EwJJRTEQMA4GA1UECAwHTXVuc3RlcjERMA8GA1UEBwwITGltZXJpY2sxDDAKBgNV\r\n"  \
    "BAoMA0FESTERMA8GA1UECwwIU2VjdXJpdHkxFjAUBgNVBAMMDTE5Mi4xNjguMC4x\r\n"  \
    "MDMxJTAjBgkqhkiG9w0BCQEWFnNhZGhiaC5yeWFuQGFuYWxvZy5jb20wWTATBgcq\r\n"  \
    "hkjOPQIBBggqhkjOPQMBBwNCAATrk4LNopc/v2LwrvOj8dU4HQ41phLLqtIoivt7\r\n"  \
    "ykZvsh7AEujFtrS1X1QS7pnyb2TIK17win2wgNw6wE55EA4Ao1MwUTAdBgNVHQ4E\r\n"  \
    "FgQUapFI+iiRJK7aULq01yFKFPu/cxQwHwYDVR0jBBgwFoAUapFI+iiRJK7aULq0\r\n"  \
    "1yFKFPu/cxQwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNJADBGAiEAr89F\r\n"  \
    "RYOHErV59iYkXtvVLDw5d3cDrjCCtT9EkcFiunkCIQCLttRU95BGtY2j9b3+BBJa\r\n"  \
    "4MQjRSZY7s4dNUBdbgsoVw==\r\n"  \
    "-----END CERTIFICATE-----\r\n"

#define DEVICE_CERT                                                         \
    "-----BEGIN CERTIFICATE-----\r\n"                                       \
    "MIICazCCAhCgAwIBAgIUa08G6M8bGjJJrS5a5HscA8h7jtAwCgYIKoZIzj0EAwIw\r\n"  \
    "gZIxCzAJBgNVBAYTAklFMRAwDgYDVQQIDAdNdW5zdGVyMREwDwYDVQQHDAhMaW1l\r\n"  \
    "cmljazEMMAoGA1UECgwDQURJMREwDwYDVQQLDAhTZWN1cml0eTEWMBQGA1UEAwwN\r\n"  \
    "MTkyLjE2OC4wLjEwMzElMCMGCSqGSIb3DQEJARYWc2FkaGJoLnJ5YW5AYW5hbG9n\r\n"  \
    "LmNvbTAeFw0yNTA5MTkxMzQ2MzhaFw0yNjA5MTkxMzQ2MzhaMIGSMQswCQYDVQQG\r\n"  \
    "EwJJRTEQMA4GA1UECAwHTXVuc3RlcjERMA8GA1UEBwwITGltZXJpY2sxDDAKBgNV\r\n"  \
    "BAoMA0FESTERMA8GA1UECwwIU2VjdXJpdHkxFjAUBgNVBAMMDTE5Mi4xNjguMC4x\r\n"  \
    "MDMxJTAjBgkqhkiG9w0BCQEWFnNhZGhiaC5yeWFuQGFuYWxvZy5jb20wWTATBgcq\r\n"  \
    "hkjOPQIBBggqhkjOPQMBBwNCAAQ9JLMz+wCrAuMCpe/4SjRB8Kvxg+04+UC/bTK7\r\n"  \
    "VmlU1X2mBuBi2MJv2AMlFPI17A95D2+HgPW0IFdYO0u90UIro0IwQDAdBgNVHQ4E\r\n"  \
    "FgQUXYjMOEzBcKTMhovUMxy3Sm7fQPIwHwYDVR0jBBgwFoAUapFI+iiRJK7aULq0\r\n"  \
    "1yFKFPu/cxQwCgYIKoZIzj0EAwIDSQAwRgIhAJEvs+bULFEn6z65uknWAN+8xct1\r\n"  \
    "FTdpF8cwQAYREhmiAiEAgrqLihlJfgI3XCm8OtE7VUE0j1a68flAXMPEhzzlBaw=\r\n"  \
    "-----END CERTIFICATE-----\r\n"

#define DEVICE_PRIVATE_KEY                                                  \
    "-----BEGIN EC PRIVATE KEY-----\r\n"                                    \
    "MHcCAQEEILKXXlAMD2yqPcpAAwC+mIZQQ6O9rdaVnB165Qmj9AI2oAoGCCqGSM49\r\n"  \
    "AwEHoUQDQgAEPSSzM/sAqwLjAqXv+Eo0QfCr8YPtOPlAv20yu1ZpVNV9pgbgYtjC\r\n"  \
    "b9gDJRTyNewPeQ9vh4D1tCBXWDtLvdFCKw==\r\n"  \
    "-----END EC PRIVATE KEY-----\r\n"
#endif

extern struct lwip_network_param lwip_ip;
extern struct adin1110_init_param adin1110_ip;
#endif

#endif /* __COMMON_DATA_H__ */

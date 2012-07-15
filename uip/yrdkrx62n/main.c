/*
 * Copyright (c) 2001, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: main.c,v 1.16 2006/06/11 21:55:03 adam Exp $
 *
 */

#include "uip.h"
#include "uip_arp.h"
// Renesas -- #include "network-device.h"
#include <stdio.h>
#include <string.h>
#include "r_ether.h"
#include "httpd.h"
#include "timer.h"
#include "user-app.h"
#include "hwsetup.h"
#include "lcd.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

extern const struct uip_eth_addr my_mac;

void InitialiseLCD(void);
void ClearLCD(void);
void DisplayuIPDemo(void);
void DisplayIPAddress(const unsigned short ipaddr[] );

/*----------------------------------------------------------------------------*/ 
int
main(void)
{
  int i;
  // Renesas -- uip_ipaddr_t ipaddr;
  struct timer periodic_timer, arp_timer;
  uint32_t ch = 0;

  HardwareSetup();
  InitialiseLCD();
  DisplayuIPDemo();

  __asm volatile ("setpsw I \n");

  timer_init();
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  // Renesas -- network_device_init();
  /* Wait until Ether device initailize succesfully. 
     Make sure Ethernet cable is plugged in. */ 
  while (R_ETHER_ERROR == R_Ether_Open(ch, (uint8_t*)&my_mac.addr[0]));

  // Renesas ++ set Ethernet address
  uip_setethaddr(my_mac);
  uip_init();

  // Renesas -- 
  //uip_ipaddr(ipaddr, 192,168,0,2);
  //uip_sethostaddr(ipaddr);
  dhcpc_init(&my_mac.addr[0], 6);
  httpd_init();

  while (1)
  {
    // Renesas -- uip_len = network_device_read();
    uip_len = R_Ether_Read(ch, (void *)uip_buf);
    if (uip_len > 0)
    {
      if (BUF->type == htons(UIP_ETHTYPE_IP))
      {
        uip_arp_ipin();
        uip_input();
        /* If the above function invocation resulted in data that
           should be sent out on the network, the global variable
           uip_len is set to a value > 0. */
        if (uip_len > 0)
        {
          uip_arp_out();
          // Renesas -- network_device_send();
          R_Ether_Write(ch, (void *)uip_buf, (uint32_t)uip_len);
        }
      }
      else if (BUF->type == htons(UIP_ETHTYPE_ARP))
      {
        uip_arp_arpin();
        /* If the above function invocation resulted in data that
           should be sent out on the network, the global variable
           uip_len is set to a value > 0. */
        if (uip_len > 0)
        {
          // Renesas -- network_device_send();
          R_Ether_Write(ch, (void *)uip_buf, (uint32_t)uip_len);
        }
      }

    }
    else if (timer_expired(&periodic_timer))
    {
      timer_reset(&periodic_timer);
      for (i = 0; i < UIP_CONNS; i++)
      {
        uip_periodic(i);
        /* If the above function invocation resulted in data that
           should be sent out on the network, the global variable
           uip_len is set to a value > 0. */
        if (uip_len > 0)
        {
          uip_arp_out();
          // Renesas -- network_device_send();
          R_Ether_Write(ch, (void *)uip_buf, (uint32_t)uip_len);
        }
      }

#if UIP_UDP
      for (i = 0; i < UIP_UDP_CONNS; i++)
      {
        uip_udp_periodic(i);
        /* If the above function invocation resulted in data that
           should be sent out on the network, the global variable
           uip_len is set to a value > 0. */
        if (uip_len > 0)
        {
          uip_arp_out();
          // Renesas -- network_device_send();
          R_Ether_Write(ch, (void *)uip_buf, (uint32_t)uip_len);
        }
      }
#endif /* UIP_UDP */

      /* Call the ARP timer function every 10 seconds. */
      if (timer_expired(&arp_timer))
      {
        timer_reset(&arp_timer);
        uip_arp_timer();
      }
    }

    // Insert user aplications here.
    // Call WEB application that controls LEDs on the target board.
    user_app();

  }
  return 0;
}
/*---------------------------------------------------------------------------*/

/**********************************************************************************
Name:			InitialiseLCD
Description:	Prepare LCD for operation
Parameters:		None
Returns:		None
**********************************************************************************/
void InitialiseLCD(void)
{
    lcd_open();
    lcd_set_address(0, 0);
    ClearLCD();
}

/**********************************************************************************
Name:			ClearLCD
Description:	Clears the LCD
Parameters:		None
Returns:		None
**********************************************************************************/
void ClearLCD(void)
{
    lcd_string(0,0, "                   ");
    lcd_string(1,0, "                   ");
    lcd_string(2,0, "                   ");
    lcd_string(3,0, "                   ");
    lcd_string(4,0, "                   ");
}
 /**********************************************************************************
Name:			DisplayuIPDemo
Description:	Clears the LCD and displays "uIP Demo" on line 1.    
Parameters:		None
Returns:		None
**********************************************************************************/
void DisplayuIPDemo(void)
{
    ClearLCD();
    lcd_string(1,0, " *** uIP Demo ***  ");
}

/**********************************************************************************
Name:			DisplayIPAddress
Description:	Displays IP address in dotted decimal format.
Parameters:		(unsigned short []) ipaddr - IP address to display
Returns:		None
**********************************************************************************/
void DisplayIPAddress(const unsigned short ipaddr[] )
{
    ClearLCD();
    lcd_string(1,0, " *** uIP Demo ***  ");
    lcd_string(3,0, "                   ");
    lcd_string(3,0, "ip=");
    lcd_display_number(((const uint8_t*)ipaddr)[0]);
    lcd_display_char('.');
    lcd_display_number(((const uint8_t*)ipaddr)[1]);
    lcd_display_char('.');
    lcd_display_number(((const uint8_t*)ipaddr)[2]);
    lcd_display_char('.');
    lcd_display_number(((const uint8_t*)ipaddr)[3]);
}

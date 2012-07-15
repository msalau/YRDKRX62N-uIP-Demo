/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack
 *
 * $Id: clock-arch.c,v 1.2 2006/06/12 08:00:31 adam Exp $
 */

/**
 * \file
 *         Implementation of architecture-specific clock functionality
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "clock-arch.h"
#include <iodefine.h>
#include <yrdkrx62ndef.h>
#include <isr_vectors.h>

static void int_cmt0_isr(void);

static clock_time_t timer_clock_ticks = 0;


/**
 * System timer tick initialization
 *
 * This function initializes a timer interrupt with 100ms tick. 
 *
 */
void  timer_init(void)
{
    /* CMT0 is configured for a 100ms interval */

    MSTP(CMT0) = 0;                         /* Enable CMT0 */
    CMT.CMSTR0.BIT.STR0 = 0;                /* Stop timer */
    CMT0.CMCR.BIT.CKS = 3;                  /* PCLK/512 */
    CMT0.CMCOR = PCLK_FREQUENCY / 512 / 10 - 1;     /* Set 100ms period */
    _isr_vectors[VECT(CMT0,CMI0)] = int_cmt0_isr;
    CMT0.CMCR.BIT.CMIE = 1;                 /* Enable interrupt request */
    IR(CMT0,CMI0) = 0;
    IPR(CMT0,CMI0) = 15;                    /* Set highest interrupt priority */
    IEN(CMT0,CMI0) = 1;                     /* Enable interrupt */
    CMT.CMSTR0.BIT.STR0 = 1;                /* Start timer */
}

/**
 * System timer tick 
 *
 * This function is called from timer interrupt to increment a 
 * system timer tick.  Also an LED on the target board is 
 * toggled to show activity. 
 *
 */
__attribute__((interrupt))
void int_cmt0_isr(void)
{
    LED4 ^= 1;
    timer_clock_ticks++;
}


/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return timer_clock_ticks;
}
/*---------------------------------------------------------------------------*/

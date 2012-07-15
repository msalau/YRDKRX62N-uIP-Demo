/******************************************************************************
* DISCLAIMER

* This software is supplied by Renesas Technology Corp. and is only 
* intended for use with Renesas products. No other uses are authorized.

* This software is owned by Renesas Technology Corp. and is protected under 
* all applicable laws, including copyright laws.

* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, 
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
* PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY 
* DISCLAIMED.

* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS 
* TECHNOLOGY CORP. NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE 
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES 
* FOR ANY REASON RELATED TO THE THIS SOFTWARE, EVEN IF RENESAS OR ITS 
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

* Renesas reserves the right, without notice, to make changes to this 
* software and to discontinue the availability of this software.  
* By using this software, you agree to the additional terms and 
* conditions found by accessing the following link:
* http://www.renesas.com/disclaimer
******************************************************************************
* Copyright (C) 2008. Renesas Technology Corp., All Rights Reserved.
*******************************************************************************	
* File Name    : r_ether.c
* Version      : 1.03
* Description  : Ethernet module device driver
******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 15.02.2010 1.00    First Release
*         : 03.03.2010 1.01    Buffer size is aligned on the 32-byte boundary.
*         : 08.03.2010 1.02    Modification of receiving method
*         : 06.04.2010 1.03    RX62N chnages
******************************************************************************/


/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "iodefine.h"
#include "r_ether.h"
#include "phy.h"

/******************************************************************************
Typedef definitions
******************************************************************************/

/******************************************************************************
Macro definitions
******************************************************************************/

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/
void    _eth_fifoInit(ethfifo p[], uint32_t status);
int32_t _eth_fifoWrite(ethfifo *p, int8_t buf[], int32_t size);
int32_t _eth_fifoRead(ethfifo *p, int8_t buf[]);


/**
 * Pragma definitions for receive and transmit packet 
 * descriptors 
 */
__attribute__((aligned(64)))
static ethfifo rxdesc[ENTRY];    	/* receive packet descriptor */
__attribute__((aligned(64)))
static ethfifo txdesc[ENTRY];    	/* transmit packet descriptor */

/**
 * Pragma definitions for receive and transmitbuffers managed by 
 * the descriptors 
 */
__attribute__((aligned(4)))
static int8_t	rxbuf[ENTRY][BUFSIZE];     /* receive data buffer */
__attribute__((aligned(4)))
static int8_t	txbuf[ENTRY][BUFSIZE];     /* transmit data buffer */

/**
 * Ethernet device driver control structure initialization
 */
struct ei_device  le0 =
{
  "eth0",       /* device name */
  0,            /* open */
  0,            /* Tx_act */
  0,            /* Rx_act */
  0,            /* txing */
  0,            /* irq lock */
  0,            /* dmaing */
  0,            /* current receive discripter */
  0,            /* current transmit discripter */
  0,            /* save irq */
  {
    0,          /* rx packets */
    0,          /* tx packets */
    0,          /* rx errors */
    0,          /* tx errors */
    0,          /* rx dropped */
    0,          /* tx dropped */
    0,          /* multicast */
    0,          /* collisions */
                
    0,          /* rx length errors */
    0,          /* rx over errors */
    0,          /* rx CRC errors */
    0,          /* rx frame errors */
    0,          /* rx fifo errors */
    0,          /* rx missed errors */
                
    0,          /* tx aborted errors */
    0,          /* tx carrier errors */
    0,          /* tx fifo errors */
    0,          /* tx heartbeat errors */
    0           /* tx window errors */
  },
  {
    0,          /* MAC 0 */
    0,          /* MAC 1 */
    0,          /* MAC 2 */
    0,          /* MAC 3 */
    0,          /* MAC 4 */
    0           /* MAC 5 */
  }
};

/**
 * Renesas Ethernet API functions
 */

/******************************************************************************
* Function Name: R_Ether_Open
* Description  : Initializes and enables Ethernet peripheral
* Arguments    : ch - Ethernet channel number
*              : mac_addr - MAC address for the channel
* Return Value : R_ETHER_OK
*              : R_ETHER_ERROR
******************************************************************************/
int32_t R_Ether_Open(uint32_t ch, uint8_t mac_addr[])
{
  int32_t     i;
  uint32_t    mac_h,mac_l;
  int16_t  phydata;

  ch = ch;                                /* Keep compiler happy */

  /* Initialize driver */
  le0.open = 1;
  _eth_fifoInit(rxdesc, (uint32_t)ACT);        
  _eth_fifoInit(txdesc, (uint32_t)0);          
  le0.rxcurrent = &rxdesc[0];
  le0.txcurrent = &txdesc[0];
  le0.mac_addr[0] = mac_addr[0];
  le0.mac_addr[1] = mac_addr[1];
  le0.mac_addr[2] = mac_addr[2];
  le0.mac_addr[3] = mac_addr[3];
  le0.mac_addr[4] = mac_addr[4];
  le0.mac_addr[5] = mac_addr[5];

  /* Initialize EDMAC and ETHERC  */
  EDMAC.EDMR.BIT.SWR = 1;
  for( i = 0 ; i < 0x00000100 ; i++ );    /* Reset EDMAC */

  /* ETHERC */
  /* TODO:  Check bit 5 */
  ETHERC.ECSR.LONG   = 0x00000037;         /* clear all ETHERC status BFR, PSRTO, LCHNG, MPD, ICD */
  /* TODO:  Check bit 5 */
  ETHERC.ECSIPR.LONG = 0x00000020;         /* disable ETHERC status change interrupt */
  ETHERC.RFLR.LONG   = 1518;               /* ether payload is 1500+ CRC              */
  ETHERC.IPGR.LONG = 0x00000014;           /* Intergap is 96-bit time */

  mac_h = ((uint32_t)mac_addr[0] << 24) | \
	    ((uint32_t)mac_addr[1] << 16) | \
	    ((uint32_t)mac_addr[2] << 8 ) | \
	     (uint32_t)mac_addr[3];

  mac_l = ((uint32_t)mac_addr[4] << 8 ) | \
		 (uint32_t)mac_addr[5];

	if(mac_h == 0 && mac_l == 0)
	{
		/*
		 * If 2nd parameter is 0, the MAC address should be acquired from the system, 
		 * depending on user implementation. (e.g.: EEPROM)
		 */
	}
	else
	{
		ETHERC.MAHR = mac_h;
		ETHERC.MALR.LONG = mac_l;
	}

  /* EDMAC */
  EDMAC.EESR.LONG   = 0x47FF0F9F;         /* clear all ETHERC and EDMAC status bits */
  EDMAC.RDLAR  = le0.rxcurrent;           /* initialize Rx Descriptor List Address */
  EDMAC.TDLAR  = le0.txcurrent;           /* initialize Tx Descriptor List Address */
  EDMAC.TRSCER.LONG = 0x00000000;         /* copy-back status is RFE & TFE only */
  EDMAC.TFTR.LONG   = 0x00000000;         /* threshold of Tx_FIFO */
  EDMAC.FDR.LONG    = 0x00000707;         /* transmit fifo & receive fifo is 2048 bytes */
  EDMAC.RMCR.LONG   = 0x00000001;         /* RR in EDRRR is under driver control */
#if __LIT
  EDMAC.EDMR.BIT.DE = 1;
#endif

  /* Initialize PHY */
  phydata = phy_init();

  if (phydata == R_PHY_ERROR)
  {
      return R_ETHER_ERROR;
  }

  /* Start PHY auto negotiation */ 
  phydata = phy_set_autonegotiate();

  if (phydata == R_PHY_ERROR)
  {
      return R_ETHER_ERROR;
  }

  if (phydata & PHY_AN_LINK_PARTNER_FULL)
  {
      /* Full duplex */
      ETHERC.ECMR.BIT.DM = 1;
  }
  else
  {
	/*	Half duplex	*/
	ETHERC.ECMR.BIT.DM = 0;
  }

  if (phydata & PHY_AN_LINK_PARTNER_100BASE)
  {
   	/*	100Mbps	*/
   	ETHERC.ECMR.BIT.RTM = 1;
  }
  else
  {
  	/*	10Mbps	*/
   	ETHERC.ECMR.BIT.RTM = 0;
  }


	/* Enable interrupt */
	/* Sets up interrupt when you use interrupt */
	//EDMAC.EESIPR.LONG = 0x00040000;
	//ICU.IER04.BYTE |= 0x01;
	//ICU.IPR08.BYTE = 4;	// Set priority level

  /* Enable receive and transmit */
  ETHERC.ECMR.BIT.RE = 1;			  
  ETHERC.ECMR.BIT.TE = 1;			  

  /* Enable EDMAC receive */
  EDMAC.EDRRR.LONG  = 0x00000001;                
  for( i = 0 ; i < 0x0000100 ; i++ );

  return R_ETHER_OK;
}

/******************************************************************************
* Function Name: R_Ether_Close
* Description  : Disables Ethernet peripheral
* Arguments    : ch - Ethernet channel number
* Return Value : R_ETHER_OK
*              : R_ETHER_ERROR
******************************************************************************/
int32_t R_Ether_Close(uint32_t ch)
{

  ch = ch;                                /* Keep compiler happy */

  le0.open = 0;
  ETHERC.ECMR.LONG = 0x00000000;          /* disable TE and RE  */
  le0.irqlock = 1;

  return R_ETHER_OK;
}

/******************************************************************************
* Function Name: R_Ether_Write
* Description  : Transmits an Ethernet frame
* Arguments    : ch - Ethernet channel number
*              : buf - pointer to transmit buffer
*              : len - frame length
* Return Value : R_ETHER_OK
*              : R_ETHER_ERROR
******************************************************************************/
int32_t R_Ether_Write(uint32_t ch, void *buf, uint32_t len)
{
  int32_t xmit;
  int32_t flag = FP1;
  int8_t  *data = (int8_t *)buf;

  ch = ch;                                /* Keep compiler happy */

  for( xmit = 0 ; len > 0 ; len -= xmit )
  {
    while( (xmit = _eth_fifoWrite(le0.txcurrent, data, (int32_t)len)) < 0 );

    if( xmit == len )
    {
      flag |= FP0;
    }

    /* Clear previous settings */
    le0.txcurrent->status &= ~(FP1 | FP0);  
    le0.txcurrent->status |= (flag | ACT);
    
    flag = 0;
    le0.txcurrent = le0.txcurrent->next;
    data += xmit;
  }

  le0.stat.tx_packets++;

  if( EDMAC.EDTRR.LONG == 0x00000000 )
  {
      EDMAC.EDTRR.LONG = 0x00000001;
  }

  return R_ETHER_OK;
}

/******************************************************************************
* Function Name: R_Ether_Read
* Description  : Receives an Ethernet frame
* Arguments    : ch - Ethernet channel number
*              : buf - pointer to receive buffer
* Return Value : R_ETHER_OK (0 - No bytes received) 
*              : Number of bytes received (Value greater than zero)
******************************************************************************/
int32_t R_Ether_Read(uint32_t ch, void *buf)
{
  int32_t  receivesize = 0;
  int32_t  recvd;
  int32_t  flag = 1;
  uint8_t readcount = 0;
  int8_t  *data = (int8_t *)buf;

  ch = ch;                                /* Keep compiler happy */

  while (flag)
  {
    recvd = _eth_fifoRead(le0.rxcurrent, data);
	readcount++;

	if (readcount >= 2 && receivesize == 0)
	{
		/* Allow outer loop to run */
		return R_ETHER_OK;
	}

    if (recvd == -1)
    {
      /* No descriptor to process */
    }
    else if (recvd == -2)
    {
      /* Frame error.  Point to next frame.  Clear this descriptor. */
      le0.stat.rx_errors++;

      receivesize = 0;
      le0.rxcurrent->status &= ~(FP1 | FP0 | FE);
      le0.rxcurrent->status &= ~(RFOVER | RAD | RMAF | RRF | RTLF | RTSF | PRE | CERF);
      le0.rxcurrent->status |= ACT;
      le0.rxcurrent = le0.rxcurrent->next;

      if (EDMAC.EDRRR.LONG == 0x00000000L)
      {
        /* Restart if stopped */
        EDMAC.EDRRR.LONG = 0x00000001L;
      }
    }
    else
    {
      /* We have a good buffer. */
      if ((le0.rxcurrent->status & FP1)  == FP1)
      {
        /* Beginning of a frame */
        receivesize = 0;
      }

      if ((le0.rxcurrent->status & FP0) == FP0)
      {
        /* Frame is complete */
        le0.stat.rx_packets++;
        flag = 0;
      }

      receivesize += recvd;
      le0.rxcurrent->status &= ~(FP1 | FP0);
      le0.rxcurrent->status |= ACT;
      le0.rxcurrent = le0.rxcurrent->next;

      data += recvd;

      if (EDMAC.EDRRR.LONG == 0x00000000L)
      {
        /* Restart if stopped */
        EDMAC.EDRRR.LONG = 0x00000001L;
      }
    }
  }

  return (int32_t)receivesize;

}

/**
 * Internal functions
 */

/******************************************************************************
* Function Name: _eth_fifoInit
* Description  : Initialize E-DMAC descriptors
* Arguments    : p - pointer to descriptor
*              : status - initial status of descriptor
* Return Value : none
******************************************************************************/
void  _eth_fifoInit( ethfifo p[], uint32_t status )
{
  ethfifo  *current = 0;
  int32_t i, j;

  for( i = 0 ; i < ENTRY ; i++ )
  {
    current = &p[i];

    if( status == 0 )
    {
      current->buf_p = &txbuf[i][0];
    }
    else
    {
      current->buf_p = &rxbuf[i][0];
    }

    /* Clear buffer */
    for( j = 0 ; j < BUFSIZE ; j++ )
    {
      current->buf_p[j] = 0;
    }
    current->bufsize = BUFSIZE;
    current->size = 0;
    current->status = status;
    current->next = &p[i+1];
  }

  /* Set the last fifo entry to complete the loop */
  current->status |= DL;
  current->next = &p[0];
}

/******************************************************************************
* Function Name: _eth_fifoWrite
* Description  : Transmits data and updates E-DMAC descriptor
* Arguments    : p - pointer to descriptor
*              : buf - pointer to data to transmit 
*              : size - number of data bytes
* Return Value : Number of bytes written to transmit buffer for this descriptor
*              : -1 Descriptor busy
******************************************************************************/
int32_t _eth_fifoWrite( ethfifo *p, int8_t buf[], int32_t size )
{
  int32_t i;
  ethfifo  *current = p;

  if( (current->status & ACT) != 0 )
  {    
    /**
     * Current descriptor is active and ready to transmit or transmitting. 
     **/
    return( -1 );              
  }

  for( i = 0 ; i < size; i++ )
  {
    if( i >= BUFSIZE )
    {
      break;
    }
    else
    {
      /* transfer packet data */
      current->buf_p[i] = buf[i];      
    }
  }

  current->bufsize = (uint16_t)i;

  return( i );

}

/******************************************************************************
* Function Name: _eth_fifoRead
* Description  : Receives data and updates E-DMAC descriptor
* Arguments    : p - pointer to descriptor
*              : buf - pointer to data to receive 
* Return Value : Number of bytes received for this descriptor
*              : -1 Descriptor busy
*              : -2 Frame error
******************************************************************************/
int32_t _eth_fifoRead( ethfifo *p, int8_t buf[])
{
  int32_t i, temp_size;        /* byte counter */
  ethfifo  *current = p;

  if( (current->status & ACT) != 0)
  {
    /**
     * Current descriptor is active and ready to receive or receiving.  
     * This is not an error.
     **/
    return (-1);
  }

  /** 
   * Current descriptor is not active.
   * Process the descriptor and data received.
   **/
  else if( (current->status & FE) != 0)
  {
       /** 
        * Frame error.  
        * Must move to new descriptor as E-DMAC now points to next one.
        **/
       return (-2);
  }
  else
  /* We have a new data w/o errors to process. */
  {
    if ((current->status & FP0) == FP0)
    {
      /* This is the last descriptor.  Complete frame is received.   */
      temp_size = current->size;

      while (temp_size > BUFSIZE)
      {
        temp_size -= BUFSIZE;
      }
    }
    else
    {
      /** 
       * This is either a start or continuos descriptor. 
       * Complete frame is NOT received.
       **/
      temp_size = BUFSIZE;
    }

    /* Copy received data from receive buffer to user buffer */
    for (i = 0; i < temp_size; i++)
    {
      buf[i] = current->buf_p[i];
    }

    /* Return data size received */
    return (temp_size);
  }
}


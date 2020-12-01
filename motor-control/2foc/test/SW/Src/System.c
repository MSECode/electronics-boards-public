//
// system related functions
// system clock, IO ports, system LEDs 
//

#include <p33FJ128MC802.h>

#include <dsp.h>
#include <libpic30.h>  //__delay32
#include <timer.h>

#include "UserParms.h"
#include "System.h"
#include "UserTypes.h"


void ActuateMuxLed()
// Actuate Multiplexed led ports
{

  static short Slot=0;

  if (0 == Slot)
  {
    // slot for green led
    if (1 == LED_status.Green )
      TurnOnLedGreen()
    else 
      TurnOffLed();
    Slot=1;
  }   
  else
  {
    // slot for red led
    if (1 == LED_status.Red)
      TurnOnLedRed()
    else 
      TurnOffLed();
    Slot=0;
  } 

}

void BlinkLed()
// blink leds according to the desired blinking rate
{
  static long RedCounter=0, GreenCounter=0;

  RedCounter++;
  GreenCounter++;
  

  switch(LED_status.GreenBlinkRate){

   case BLINKRATE_STILL:
		LED_status.Green=1;	
   break;

   case BLINKRATE_OFF:
        LED_status.Green=0;
   break;
   
   default:
    // green led blinker
    if(GreenCounter >= LED_status.GreenBlinkRate)
    {  
      GreenCounter=0;

      // toggle green LED
      if(1==LED_status.Green)
      {
        LED_status.Green=0;
      }  
      else
      {
        LED_status.Green=1;
      }
    }
    break;
  } 
 
 

  switch(LED_status.RedBlinkRate){

   case BLINKRATE_STILL:
		LED_status.Red=1;	
   break;

   case BLINKRATE_OFF:
        LED_status.Red=0;
   break;
   
   default: 
    // red blinker
    if(RedCounter >= LED_status.RedBlinkRate)
    {  
      RedCounter = 0;

      // toggle red LED
      if(1==LED_status.Red)
      {
        LED_status.Red=0;
      }  
      else
      {
        LED_status.Red=1;
      }
    }
    break;
  }
}
 


void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
//
// TIMER 1 IRQ Service Routine
// used for  low frequency operation (LED toggling reference....)
//
{  
  // blink leds according to the desired blinking rate
  BlinkLed();  
  // light the leds
  ActuateMuxLed();


 
  IFS0bits.T1IF = 0; // clear flag  
}

void Timer1Config()
// Setup timer 1 registers for low frequency operation (LED, .... )
{
  // reset timer
  WriteTimer1(0);
  // 4,82 msec (207.4Hz)
  OpenTimer1(T1_ON & T1_GATE_OFF & T1_IDLE_STOP & T1_PS_1_64 &
    T1_SYNC_EXT_OFF & T1_SOURCE_INT, 3000);
}


void Timer4Config()
// Setup timer 4 registers for CAN send
{
  unsigned int timertick;

  // reset timer
  WriteTimer4(0);
  // one timertick is 1.6 us
  timertick = ((CAN_OUTPUT_DATARATE * 100.0) / (1.6));
  OpenTimer4(T4_ON & T4_GATE_OFF & T4_IDLE_STOP & T4_PS_1_64 &
    T4_SOURCE_INT, timertick );
}

void oscConfig(void)
{
  /*  Configure Oscillator to operate the device at 40MIPS
  Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
  Fosc= 8M*40/(2*2)=80Mhz for 8M input clock */

  // set up the oscillator and PLL for 40 MIPS
  //            Crystal Frequency  * (DIVISOR+2)
  // Fcy =     ---------------------------------
  //              PLLPOST * (PRESCLR+2) * 4	
  // Crystal  = Defined in UserParms.h
  // Fosc		= Crystal * dPLL defined in UserParms.h
  // Fcy		= DesiredMIPs 

  if(OSCCONbits.COSC == 0b011){
	// already running on PLL (set by bootloader)
    // TODO decide wheter to trust the bootloader or
    // to switch back to EC or FRC and reconfigure
	return;
  }

  PLLFBD = (int)(DPLL * 4 - 2);     // dPLL derived in UserParms.h
  CLKDIVbits.PLLPOST = 0;		    // N1=2
  CLKDIVbits.PLLPRE = 0;		    // N2=2

  __builtin_write_OSCCONH(0x03);    // Initiate Clock Switch to Primary Oscillator (EC) with PLL (NOSC=0b011)
  __builtin_write_OSCCONL(0x01);    // Start clock switching

  while(OSCCONbits.COSC != 0b011);  // Wait for PLL to lock
  while(OSCCONbits.LOCK != 1);
}

void ExternalFaultEnable()
// Enables the external fault (pushbutton) interrupt
{
#ifndef PIN_RA3_IS_DEBUG
  // clear irq flag
  IFS1bits.CNIF = 0;
  //enable external fault
  IEC1bits.CNIE = 1;
#endif
}

void OverCurrentFaultEnable()
// over current fault interrupt enable
{
  // clear irq flag
  IFS3bits.FLTA1IF = 0;
  //enable external fault
  IEC3bits.FLTA1IE = 1;
}

void SetupHWParameters(void)
// Setup variables related to HW implementation
// Current conversion factor and offset
// 
{
  //
  // configure DSP core 
  //

  // multiplication are NOT unsigned
  CORCONbits.US  = 0;
  // enable saturation on DSP data write
  CORCONbits.SATDW = 1;
  // DISABLE saturation on accumulator A. Required by MeasCurr algo
  CORCONbits.SATA  = 0;

  // do not do super saturation (probably don't care because SATA = 0
  CORCONbits.ACCSAT  = 0;
  // program space not visible in data space
  CORCONbits.PSV  = 0;
  // conventional rounding mode
  CORCONbits.RND  = 1;

  // DSP multiplication are in fractional mode.
  CORCONbits.IF  = 0;

  //
  // ADC - Current Measure
  //



  //
  // ADC - VDC-Link Measure
  //


  //
  // SV Generator 
  //

  // Set PWM period to Loop Time
//  SVGenParm.iPWMPeriod = LOOPINTCY;
}

void InterruptPriorityConfig( void )
// Initialize interrupt priority registers
// WARNING: Unused interrupt are currently set to priority 0 (disabled !).
// WARNING: Faults are handled at max priority, even when int are diabled. 
// Pay attention to race conditions (classic scenario:
//  lower priority int preempted by fault isr reenables all things 
//  after fault isr terminates). 
//
// The higher the number the highest the priority
//  7: FAULT(Overload+External)
//  6: ADCDMA
//  5: SPI(DMA)
//  4: CANTX-TIMER4
//  4: CAN RX
//  3: VELOCITY calc TIMER2
//  1: TIMER1... low freq. 
{
  // all IPCx irqs disabled
  IPC0 = 0;
	
  IPC0bits.T1IP    = 1; // Timer1 (IDLE time operations) prioriy 1 (lowest)

  // all IPCx irqs disabled
  IPC1 = 0;
  IPC1bits.DMA0IP  = 6; // DMA0 (adc triggered) priority 6
  IPC1bits.T2IP    = 3; // TIMER2 Velocity calculation timer

  // all IPCx irqs disabled
  IPC2 = 0;
  IPC2bits.SPI1IP  = 5; // SPI int priority 5
  IPC2bits.SPI1EIP = 5; // SPI error INT prioriy 5;
  IPC2bits.T3IP    = 5; // I2T when the 2foc loop is not running (fault,shutdown)
  
  // TODO . handle SPI error int.

  IPC3 = 0;
  IPC3bits.DMA1IP  = 4; // Can TX (DMA1) priority 4;
  IPC3bits.AD1IP   = 6; // ADC interrupt (if not in dma mode)

  IPC4 = 0;
  IPC4bits.CNIP    = 7; // external fault interrupt priority 7( highest, 
  // not masked by DISI instruction. for fault)
  IPC5 = 0;

  IPC6 = 0;
  IPC6bits.DMA2IP   = 4; // Can RX (DMA2) priority 4; 
  IPC6bits.T4IP     = 4; // CAN tx timer

  IPC7 = 0;
  IPC8 = 0;
  IPC8bits.C1RXIP = 0; // can RX not in DMA mode
  IPC8bits.C1IP = 4;
  //IPC8bits.C1IP1
  //IPC8bits.C1IP2
 
  IPC9 = 0;
  IPC9bits.DMA3IP = 5; //DMA3 (spi) priority 5;
  IPC11 = 0;
  IPC14 = 0;
  
  IPC15 = 0;
  IPC15bits.FLTA1IP = 7; // Fault max priority
  IPC16 = 0;
  IPC17 = 0;
  IPC18 = 0;

  // Ensure interrupt nesting is enabled
  INTCON1bits.NSTDIS = 0;
}

void SetupPorts( void )
//
//  init dsPic ports and peripheral mapping
//
{
  // set everything as input
  LATA  = 0x0000;
  TRISA = 0xFFFF;
  
  LATB  = 0x0000;
  TRISB = 0xFFFF;

#ifdef HARDWARE_REVISION_4
  TRISAbits.TRISA4 = 0; // LED Green and Red
#else
  // previous revision has no led available (used for CAN communication)
#endif
  
  // define remappable peripheral allocation
  // see chapter 11 dsPic datasheet
  
  // issue an UNLOCK sequence to change OSCCON.IOLOCK (bit 6) 
  __builtin_write_OSCCONL( OSCCON & ~0x40 );

  // default all pins to DIGITAL
  AD1PCFGL = 0xffff; 
  // TODO: non penso che sia necessario eventualmente modificare solo quanto serve per SPI!!

  //
  // Outputs
  //

  // CAN bus pin configuration
  // set port as output
  // TRISBbits.TRISB4 = 0;

  // Set RP4 as output CANTX (valid for both HW revisions)
  RPOR2bits.RP4R = 0b10000; 

#ifdef PIN_RA3_IS_DEBUG
  // Use RA3 (EXTF/PROG) as Debug output 
  TRISAbits.TRISA3 = 0;
  PORTAbits.RA3 = 0;
#else
  // Enable change notification for external fault
  CNEN1 = 0;
  CNEN2 = 1<<13;
#endif

  //
  // SPI pin configuratio  
  //

#ifdef HARDWARE_REVISION_4
  //  RP8 as SPI-SS (HV)

  // set port P8 as output on HV 
  TRISBbits.TRISB8 = 0;
  // Slave select is driven in Bit Banging
  // P8 connected to B8 port
  RPOR4bits.RP8R = 0b0;
  // SPI1 Slave Select Output su rp1 HV molex
  // RPOR0bits.RP1R = 0b01001; 

  // TODO: mancano i settaggi per TLE 

#else
  // HW version 2  has:
  //  B1 as SPI-SS (HV)

  // set port B1 as output on HV 
  TRISBbits.TRISB1 = 0;
  // Slave select driven in Bit Banging
  RPOR0bits.RP1R = 0b0;
  // SPI1 Slave Select Output su rp1 HV molex
  // RPOR0bits.RP1R = 0b01001; 



  // for other encoders mosi is not defined (yet)

  // SPI pin configuratio
  // set port as output
  // TRISBbits.TRISB5 = 0;
//
// ATTENZIONE mappato sui pin per il DEBUG
// N O N   F U N Z I O N A   I N   D E B U G 
//
  // RPOR2bits.RP5R = 0b01000; // SPI1 Clock Output
  // set port as output
  // TRISBbits.TRISB6 = 0;
//
// ATTENZIONE mappato sui pin per il DEBUG
// N O N   F U N Z I O N A   I N   D E B U G 
//
  // RPOR3bits.RP6R = 0b00111; // SPI1 Data Output 
                   // 0b01001; // SPI1 Slave Select Output
#endif

  //  B3 as SPI-CLK (HU)
  // set port RB3 as output
  TRISBbits.TRISB3 = 0;
  // SPI1 Clock Output su RP3 HU
 // RPOR1bits.RP3R = 0b01000; 


  //
  // Inputs
  //
#ifdef HARDWARE_REVISION_4
  // CAN Rx

  // CANRx connesso a RP2
  RPINR26bits.C1RXR = 2;
  
  // Connect SPI1 MISO on RP9. HW su molex
 // RPINR20bits.SDI1R = 9; 

#else
  // HW version 2  has:
  // RP8 as CANRX
  // RP2 as SPI-MISO (HW)
 
  // CAN Rx
  // Connect ECAN1 Receive to RPx
  // GRRRRRRRRRRRrrrrrrr.......  (!/^��@##�@!!)
  //
  // CANRx connesso a RP8 (LEDRG)
  RPINR26bits.C1RXR = 8;

  // Conect SPI1 Data Input SDI1 to RP7
  // RPINR20bits.SDI1R = 7;  
  // Connect SPI1 MISO on RP2. HW su molex
  RPINR20bits.SDI1R = 2; 
#endif

  // QEA su HEV
  // Connect QEI1 Phase A to RP5
//  RPINR14bits.QEA1R = 5;  
  // QEB su HEW
  // Connect QEI1 Phase B to RP6
 // RPINR14bits.QEB1R = 6;
  // QEZ su HEU
  // Connect QEI1 Index to RP7
 // RPINR15bits.INDX1R = 7;

  // RP5, RP6, RP7 are used for Quadrature encoders and for Hall Effect Sesnsors
  // Configure QEI or HES pins as digital inputs

  // TODO: verificare l'inizializzazione di ADPCFG
  ADPCFG |= (1<<5) | (1<<6) | (1<<7); // QEA/HESU=pin5, QEB/HESV=pin6, QEZ/HESW=pin7

#ifdef HARDWARE_REVISION_4
  // Overcurrent 
  // Connect PWM overcurrent Fault to RP1
  RPINR12bits.FLTA1R = 1;
#else
  // HW version 2  has:
  // RP9 as OVL
  // Overcurrent 
  //Connect PWM1 Fault FLTA1 ro RP9
  //RPINR12bits.FLTA1R = 9;
#endif

  // Analog inputs 
  AD1PCFGLbits.PCFG0 = 0; // AN0 analog - IA
  AD1PCFGLbits.PCFG1 = 0; // AN1 analog - IB
  AD1PCFGLbits.PCFG2 = 0; // AN2 analog - VBUS

  // issue an LOCK sequence 
  __builtin_write_OSCCONL( OSCCON | 0x40 );
}



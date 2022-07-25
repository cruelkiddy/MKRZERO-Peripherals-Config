#include <configHardware.h>

static uint32_t chnl = 0;                   // DMA channel

static __inline__ void ADCsync() __attribute__((always_inline, unused));
static void ADCsync() {
  while (ADC->STATUS.bit.SYNCBUSY == 1);    //Just wait till the ADC is free
}

void setupMyADC(void){
  analogRead(ADCPIN);                       // do some pin init  pinPeripheral()
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  ADCsync();
  ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;      // default, gain = 1/2
  ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC1_Val;   // Vref = 1/2 VDDANA (only for VDDANA > 2.0V)
  ADCsync();
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[ADCPIN].ulADCChannelNumber; // Input Pin selection
  ADCsync();
  ADC->AVGCTRL.reg = 0x00 ;       //no averaging
  ADC->SAMPCTRL.reg = 0x00;  ;    //sample length in 1/2 CLK_ADC cycles
  ADCsync();
  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV16 | ADC_CTRLB_RESSEL_10BIT; // Free run/single conversion is set here
  ADCsync();
  ADC->EVCTRL.bit.STARTEI = 1;  //< A new conversion will be triggered on any incoming event, 
                                //< in my config, I would like it to be the overflow event of TC4
  ADCsync();
  ADC->CTRLA.bit.ENABLE = 0x01;
  ADCsync();
}

void DMAC_Handler(){
    // interrupts DMAC_CHINTENCLR_TERR DMAC_CHINTENCLR_TCMPL DMAC_CHINTENCLR_SUSP
    uint8_t active_channel;

    // disable irqs ?
    __disable_irq();
    active_channel =  DMAC->INTPEND.reg & DMAC_INTPEND_ID_Msk; // get channel number
    DMAC->CHID.reg = DMAC_CHID_ID(active_channel);
    dmadone = DMAC->CHINTFLAG.reg;
    DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TCMPL; // clear
    DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TERR;
    DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_SUSP;
    __enable_irq();
}

void setupMyDMA(){
    // probably on by default
    PM->AHBMASK.reg |= PM_AHBMASK_DMAC ;
    PM->APBBMASK.reg |= PM_APBBMASK_DMAC ;
    NVIC_EnableIRQ( DMAC_IRQn ) ;

    DMAC->BASEADDR.reg = (uint32_t)descriptor_section;
    DMAC->WRBADDR.reg = (uint32_t)wrb;
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);    
}

void ADCDMA(void *rxdata,  size_t hwords){
    uint32_t temp_CHCTRLB_reg;

    DMAC->CHID.reg = DMAC_CHID_ID(chnl);
    DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << chnl));
    temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(0) |
    DMAC_CHCTRLB_TRIGSRC(ADC_DMAC_ID_RESRDY) | DMAC_CHCTRLB_TRIGACT_BEAT;
    DMAC->CHCTRLB.reg = temp_CHCTRLB_reg;
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_MASK ; // enable all 3 interrupts
    dmadone = 0;
    descriptor.descaddr = 0;
    descriptor.srcaddr = (uint32_t) &ADC->RESULT.reg;
    descriptor.btcnt =  hwords;
    descriptor.dstaddr = (uint32_t)rxdata + hwords*2;   // end address
    descriptor.btctrl =  DMAC_BTCTRL_BEATSIZE_HWORD | DMAC_BTCTRL_DSTINC | DMAC_BTCTRL_VALID;
    memcpy(&descriptor_section[chnl],&descriptor, sizeof(dmacdescriptor));

    // start channel
    DMAC->CHID.reg = DMAC_CHID_ID(chnl);
    DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;
}

///< Configuration of TC4 to trigger ADC, samplingRate in ksps
void setupMytimer(int samplingRate){
  // Set up the generic clock (GCLK4) used to clock timers
  REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) |          // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
                    GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |         // Enable GCLK4
                     GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
                     GCLK_GENCTRL_ID(4);          // Select GCLK4
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  // Feed GCLK4 to TC4 and TC5
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TC4 and TC5
                     GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
                     GCLK_CLKCTRL_ID_TC4_TC5;     // Feed the GCLK4 to TC4 and TC5
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization
 
  REG_TC4_COUNT16_CC0 = 0xB71A;                   // Set the TC4 CC0 register as the TOP value in match frequency mode
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);       // Wait for synchronization
  REG_TC4_EVCTRL |= TC_EVCTRL_OVFEO;              // OVF event ouput enable

  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);       // Wait for synchronization
  REG_TC4_CTRLA |= TC_CTRLA_PRESCALER_DIV1024 |   // Set prescaler to 1024, 48MHz/1024 = 46.875kHz
                   TC_CTRLA_WAVEGEN_MFRQ |        // Put the timer TC4 into match frequency (MFRQ) mode 
                   TC_CTRLA_ENABLE;               // Enable TC4
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);       // Wait for synchronization
}

void eventConfig(){
  PM->APBCMASK.reg |= PM_APBCMASK_EVSYS;          // Swith on the event system peripheral
	EVSYS->USER.reg = EVSYS_USER_CHANNEL(2) |				// Attach the event user (receiver) to channel 1 (n + 1)
	                  EVSYS_USER_USER(EVSYS_ID_USER_ADC_START);   // User is the ADC-Start conversion
  EVSYS->CHANNEL.reg =  EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |		// No event edge detection
	                      EVSYS_CHANNEL_PATH_ASYNCHRONOUS   |								// Set event path as asynchronous
	                      EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TC4_OVF) |				// Set event generator (sender) as TC4 overflow
	                      EVSYS_CHANNEL_CHANNEL(1);										      // Attach the generator (sender) to channel 1
}
#pragma once

#include <Arduino.h>

#define ADCPIN A1

void setupMyADC(void);
void setupMytimer(int);

void DMAC_Handler();
void setupMyDMA(void);

void ADCDMA(void *rxdata,  size_t hwords);

void eventConfig(void);

typedef struct {
    uint16_t btctrl;
    uint16_t btcnt;
    uint32_t srcaddr;
    uint32_t dstaddr;
    uint32_t descaddr;
} dmacdescriptor ;
inline volatile dmacdescriptor wrb[12] __attribute__ ((aligned (16)));
inline dmacdescriptor descriptor_section[12] __attribute__ ((aligned (16)));
inline dmacdescriptor descriptor __attribute__ ((aligned (16)));

inline volatile uint32_t dmadone;
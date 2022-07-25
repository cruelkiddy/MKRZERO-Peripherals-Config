This is a template/demo for configuring Timer+ADC+DMA on a SAMD21 microcontroller. It is developed upon [manitou48's code.](https://github.com/manitou48/ZERO/blob/master/adcdma.ino)

Manitou48's code configures ADC as continuous mode and the ADC triggers the DMA once the conversion is done. In this case, ADC sampling rate can only be changed by changing clock frequency and prescaler.

In my case, I wanted the ADC to sample at 1ksps, so I added a timer to generate an overflow event to trigger the ADC to convert. The timer4 is configured as match frequency (MFRQ) mode, in which the timer updates to zero and generates an overflow event when matches CC0 value. For the ADC, the conversion is set to start when it is triggered by any incoming event (in this case is the overflow event of the timer). The ADC sampling rate can then be easily modified by changing the value of CC0:

$$
f_{samping} = \frac{48M}{Prescaler \times CC0}
$$

#include <configHardware.h>

#define HWORDS 1024
uint16_t adcbuf[HWORDS];

void setup(){
	Serial.begin(9600);
	analogWriteResolution(10);
	analogWrite(A1, 64);   // test with DAC 
	
	///< Initialization of all the hardware resources
	setupMytimer(1);
	eventConfig();
    setupMyADC();
    setupMyDMA();
}

void loop(){
	uint32_t t;
    u_int16_t i;
	t = micros();
	ADCDMA(adcbuf, HWORDS);
	while(!dmadone);  // await DMA done isr
	t = micros() - t;
	// Serial.print(t);  Serial.print(" ms   \n");
	for(i = 0; i < 1023; i++)
		Serial.println(adcbuf[i]);
}
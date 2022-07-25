#include <configHardware.h>

#define HWORDS 8
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
    int i;
	t = micros();
	ADCDMA(adcbuf, HWORDS);
	while(!dmadone);  // await DMA done isr
	t = micros() - t;
	Serial.print(t/1000);  Serial.print(" ms   \n");
	for(i = 0; i < 8; i++)
		Serial.println(adcbuf[i]);
	delay(200);
}
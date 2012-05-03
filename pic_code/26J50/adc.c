
#include "my_adc.h"

void initADC()
{
	OpenADC(ADC_FOSC_8 & ADC_RIGHT_JUST & ADC_0_TAD,
		ADC_CH0 & ADC_CH1 &
		ADC_INT_OFF & ADC_VREFPLUS_VDD & 
		ADC_VREFMINUS_VSS, 0b1011);
	SetChanADC(ADC_CH1);
	//SetChanADC(ADC_CH0);
	Delay10TCYx( 50 );
}

void readADC(int *value)
{
	ConvertADC(); // Start conversion
	while( BusyADC() ); // Wait for ADC conversion
	(*value) = ReadADC(); // Read result and put in temp
	Delay1KTCYx(1);  // wait a bit...
}

void stopADC()
{
	CloseADC(); // Disable A/D converter
}
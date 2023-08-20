#include "../../include/main.h"

extern uint16_t vin;

#define VIN_ADC_ATTEN      ADC_ATTEN_DB_11
#define VIN_ADC_WIDTH      ADC_WIDTH_BIT_12

#ifdef ISR_BOX
	#define VIN_ADC_GPIO     4
	#define VIN_ADC_UNIT     ADC_UNIT_2
	#define VIN_ADC_CHANNEL  ADC1_CHANNEL_0
#endif
#ifdef ISR_CU
	#define VIN_ADC_GPIO     34
	#define VIN_ADC_UNIT     ADC_UNIT_1
	#define VIN_ADC_CHANNEL  ADC1_CHANNEL_6
#endif

extern void Adc_Initialize(void);
extern void Adc_Loop(void);

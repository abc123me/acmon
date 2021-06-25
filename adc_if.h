#ifndef _ADC_IF_H
#define _ADC_IF_H

#include "stdint.h"

#define ADC_FNAME_PFX		"/sys/devices/platform/soc/c1100000.bus/c1108680.adc/iio:device0/in_voltage"
#define ADC_FNAME_SFX		"_mean_raw"
#define ADC_CHANNEL_COUNT	8

uint8_t adc_init(); // Returns 1 on success
void adc_free();
uint16_t adc_read(uint8_t ch);

#endif

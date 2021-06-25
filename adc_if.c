#include "adc_if.h"

#include "stdio.h"
#include "stdlib.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"

static int adc_ch_fds[ADC_CHANNEL_COUNT];
static uint8_t opened = 0;

uint8_t adc_init() {
	if(opened)
		return 0;
	opened = 1;
	char fname_buf[128];
	for(uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
		snprintf(fname_buf, 128, "%s%d%s", ADC_FNAME_PFX, i, ADC_FNAME_SFX);
		printf("Opening: %s\n", fname_buf);
		adc_ch_fds[i] = open(fname_buf, O_RDONLY);
		if(adc_ch_fds[i] <= 0)
			return 0;
	}
	return 1;
}
void adc_free() {
	if(!opened)
		return;
	for(uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++)
		if(adc_ch_fds[i] > 0)
			close(adc_ch_fds[i]);
	opened = 0;
}
uint16_t adc_read(uint8_t ch) {
	char buf[4];
	lseek(adc_ch_fds[ch], SEEK_SET, 0);
	read(adc_ch_fds[ch], buf, 4);
	return atoi(buf);
}

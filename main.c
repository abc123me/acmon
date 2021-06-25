#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "signal.h"
#include "unistd.h"
#include "pthread.h"
#include "string.h"
#include "math.h"
#include "time.h"

#include "adc_if.h"
#include "util.h"

#define MAX_CLIENTS 8

struct acmon_t {
	uint32_t avg_period_us;
	uint16_t avg_v;
	uint8_t adc_ch;
};

void on_int(int);
void* dthread(void*);
void* cthread(void*);

uint8_t running = 1;
adc_scale_t* ac_scale_ptr = NULL;
uint8_t ac_scale_len = 0;
adc_scale_t* dc_scale_ptr = NULL;
uint8_t dc_scale_len = 0;

void* cthread(void*) {

}
int main(int argc, char** argv) {
	signal(SIGINT, on_int);
	if(!parse_scale_file("dc_scale.txt", &dc_scale_ptr, &dc_scale_len)) {
		puts("Failed to read DC scale file!");
		return 1;
	}
	if(!parse_scale_file("ac_scale.txt", &ac_scale_ptr, &ac_scale_len)) {
		puts("Failed to read AC scale file!");
		return 1;
	}
	if(!adc_init()) {
		puts("ADC Failed to initialize!");
		return 1;
	}

	pthread_t dthr;
	acmon_t ac_dat;
	pthread_create(&dthr, NULL, dthread, (void*) &ac_dat);

	uint16_t ravg_cnt = 16;
	uint16_t* ravg = (uint16_t*) malloc(ravg_cnt * sizeof(uint16_t));
	uint16_t ravg_pos = 0;
	while(running) {
		usleep(100000);
		float freq = 1e6 / ac_dat.avg_period_us;
		ravg[ravg_pos++] = adc_read(2);
		if(ravg_pos >= ravg_cnt) ravg_pos = 0;
		uint16_t adc_ac = ac_dat.avg_v;
		float vac = read_scale(ac_scale_ptr, ac_scale_len, adc_ac);
		if(freq < 59.8 || freq > 60.2)
			printf("\x1b[1;31m");
		uint32_t adc_dc = 0;
		for(uint16_t i = 0; i < ravg_cnt; i++)
			adc_dc += ravg[i];
		adc_dc /= ravg_cnt;
		float vdc = read_scale(dc_scale_ptr, dc_scale_len, adc_dc);
		printf("Frequency: %.3f, Unscaled ADC: %u = %.1f, %u = %.1f\x1b[0m\n", freq, adc_ac, vac, adc_dc, vdc);
	}

	pthread_join(dthr, NULL);
	adc_free();
	free(ravg);
	return 0;
}
void* dthread(void* dptr) {
	acmon_t* dat = (acmon_t*) dptr;
	uint8_t adc_ch = dat->adc_ch;

	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &params) != 0)
		puts("Failed to set realtime priority on data collection thread!");

	uint16_t ravg_cnt = 1024;
	uint16_t* ravg = (uint16_t*) malloc(ravg_cnt * sizeof(uint16_t));
	uint16_t ravg_pos = 0;
	memset(ravg, ravg_cnt, sizeof(uint16_t));
	uint8_t davg_cnt = 32;
	int16_t* davg = (int16_t*) malloc(davg_cnt * sizeof(int16_t));
	uint8_t davg_pos = 0;
	memset(davg, davg_cnt, sizeof(int16_t));
	uint8_t tavg_cnt = 128;
	uint32_t* tavg = (uint32_t*) malloc(tavg_cnt * sizeof(uint32_t));
	uint8_t tavg_pos = 0;
	memset(tavg, tavg_cnt, sizeof(uint32_t));

	struct timespec last_pk_ctime;
	struct timespec pk_ctime;
	uint16_t last_aval = 0;
	int16_t last_dval = 0;
	while(running) {
		uint16_t aval = adc_read(adc_ch);
		davg[davg_pos++] = last_aval - aval;
		ravg[ravg_pos++] = aval;
		last_aval = aval;
		if(ravg_pos >= ravg_cnt) ravg_pos = 0;
		if(davg_pos >= davg_cnt) davg_pos = 0;

		int32_t dval = 0;
		for(uint8_t i = 0; i < davg_cnt; i++)
			dval += davg[i];
		dval /= davg_cnt;
		if(last_dval < 0 && dval >= 0) { // Peak detect
			last_pk_ctime = pk_ctime;
			clock_gettime(CLOCK_REALTIME, &pk_ctime);
			uint32_t us = (pk_ctime.tv_nsec - last_pk_ctime.tv_nsec) / 1000;
			uint32_t ss = pk_ctime.tv_sec - last_pk_ctime.tv_sec;
			tavg[tavg_pos++] = us + ss * 1000000;
			if(tavg_pos >= tavg_cnt) tavg_pos = 0;

			uint64_t tval = 0;
			for(uint8_t i = 0; i < tavg_cnt; i++)
				tval += tavg[i];
			tval /= tavg_cnt;
			dat->avg_period_us = tval;
		}
		last_dval = dval;

		uint32_t rval = 0;
		for(uint16_t i = 0; i < ravg_cnt; i++)
			rval += ravg[i];
		rval /= ravg_cnt;
		dat->avg_v = rval;
		usleep(100);
	}

	printf("Stopping collection thread!\n");
	free(ravg);
	free(tavg);
	free(davg);
}
void on_int(int i) {
	running = 0;
}

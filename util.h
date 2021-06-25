#ifndef _UTIL_H
#define _UTIL_H

#include "stdint.h"

#define is_ws(c) (c <= ' ' || c > '~')
#define is_num(c) (c >= '0' && c <= '9')
#define is_numf(c) (is_num(c) || c == '.' || c == '+' || c == '-' || c == 'e' || c == 'E')

struct adc_scale_t {
	float voltage;
	uint16_t adc;
};
int32_t indexof(char* str, char of);
int32_t indexofany(char* str, char* of);
char* strip(char* str);
uint8_t parse_scale_file(char* name, adc_scale_t**, uint8_t*);
float read_scale(adc_scale_t*, uint8_t, uint16_t);

#endif

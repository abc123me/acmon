#include "util.h"

#include "string.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"
#include "math.h"
#include "sys/socket.h"


float read_scale(adc_scale_t* scl, uint8_t len, uint16_t adc) {
	uint8_t tpos = 0, bpos = 0;
	uint16_t max_adc = scl[0].adc, min_adc = scl[0].adc;
	for(uint8_t i = 0; i < len; i++) {
		uint16_t adc = scl[i].adc;
		if(adc > max_adc) {
			max_adc = adc;
			tpos = i;
		}
		if(adc < min_adc) {
			min_adc = adc;
			bpos = i;
		}
	}
	uint8_t lbpos = bpos, ltpos = tpos;
	for(uint8_t i = 0; i < len; i++) {
		adc_scale_t cur = scl[i];
		if(cur.adc < adc && adc - scl[bpos].adc > adc - cur.adc) {
			lbpos = bpos; bpos = i;
		} else if(cur.adc > adc && scl[tpos].adc - adc > cur.adc - adc) {
			ltpos = tpos; tpos = i;
		}
	}
	if(tpos == bpos) {
		if(scl[tpos].adc < adc) bpos = lbpos;
		else if(scl[tpos].adc > adc) tpos = ltpos;
	}
	uint16_t min = scl[bpos].adc,      max = scl[tpos].adc;
	float   nmin = scl[bpos].voltage, nmax = scl[tpos].voltage;
//	printf("%u < %u < %u\n", min, adc, max);
	return ((adc - min) / (float)(max - min)) * (nmax - nmin) + nmin;
}
uint8_t parse_scale_file(char* name, adc_scale_t** sbufptr, uint8_t* lenptr) {
	FILE* fp = fopen(name, "r");
	char buf[32];
	uint8_t pnts = 0, pnt = 0;
	adc_scale_t* sbuf;
	for(uint8_t i = 0; i < 2; i++) {
		if(i) {
			if(pnts < 2) {
				fclose(fp);
				printf("Scale must have at least 2 points!");
				return 0;
			} else sbuf = (adc_scale_t*) malloc(sizeof(adc_scale_t) * pnts);
		}
		if(i) {
			printf("Scale file counted to contain %u points!\n", pnts);
			fseek(fp, 0L, SEEK_SET);
		}
		while(1) {
			if(fgets(buf, 32, fp) == NULL) break;
			char* tbuf = strip(buf);
			if(strlen(tbuf) < 3) continue; // Dafuq?
			int32_t mid = indexofany(tbuf, "\t ,");
			if(mid > 0) {
				tbuf[mid] = 0;
				if(i) {
					adc_scale_t scl;
					scl.adc = atoi(tbuf);
					scl.voltage = atof(strip(&tbuf[mid + 1]));
					sbuf[pnt] = scl;
					if(pnt++ >= pnts) {
						fclose(fp);
						printf("Failed to read file, pnt (%u) >= pnts (%u)!\n", pnt, pnts);
						return 0;
					}
					printf("Found scale line: %u, %.1f\n", scl.adc, scl.voltage);
				} else pnts++;
			}
		}
	}
	*sbufptr = sbuf;
	*lenptr = pnts;
	fclose(fp);
	return 1;
}
int32_t indexof(char* str, char of) {
	for(uint16_t i = 0; i < strlen(str); i++)
		if(str[i] == of)
			return i;
	return -1;
}
int32_t indexofany(char* str, char* of) {
	uint16_t of_len = strlen(of), str_len = strlen(str);
	for(uint16_t i = 0; i < str_len; i++)
		for(uint16_t j = 0; j < of_len; j++)
			if(str[i] == of[j])
				return i;
	return -1;
}
char* strip(char* str) {
	char c = *str;
	while(c && is_ws(c))
		c = *(++str);
	uint16_t len = strlen(str);
	if(!len)
		return str;
	char* end = str + len - 1;
	while(is_ws(*end))
		end--;
	*++end = 0;
	return str;
}

#ifndef _PBM_UTILS_
# define _PBM_UTILS_H_

#include <stdint.h>

#define IMAGE_WIDTH	156
#define IMAGE_HEIGHT	11
#define LEDS_NUMBER	11
#define FAN_DISPLAY_8BYTES_PACKET_NUMBER	39
#define FAN_DISPLAY_MAX_NUMBER	8

static const uint16_t pbm_mask[LEDS_NUMBER] = {
	0xFCFF, 0xFBFF, 0xF7FF,
	0xFEFF, 0xCFFF, 0xBFFF,
	0x7FFF, 0xFFFE, 0xFFFD,
	0xFFFB, 0xFFF7
} ;

void pbm_to_usbdata(char id,
		    unsigned char *raster,
		    uint16_t display[156]);

unsigned char *pbm_get_specific_raster(FILE *img);

void free_pbm_rasters(unsigned char **rasters, int n);

#endif /* !_PBM_UTILS_H_ */

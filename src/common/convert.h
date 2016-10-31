#ifndef CONVERT_H_
#define CONVERT_H_

#define PFAN_IMG_W    156

unsigned long long pfan_convert_effect(const char id, const char effect[3]);

void pfan_convert_raster(char id, unsigned char *raster,
		unsigned short display[PFAN_IMG_W]);

#endif /* !CONVERT_H_ */

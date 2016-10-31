#include <linux/limits.h>
#include <pbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "data.h"
#include "raster.h"

#define DEVICE	"/dev/pfan0"

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Missing argument.\n");
		return EXIT_FAILURE;
	}

	char *config_file = argv[1];
	char imgs[MAX_FAN_DISPLAYS][FILEPATH_MAX];
	char effects[MAX_FAN_DISPLAYS][3];
	int img_n;

	if ((img_n = pfan_read_config(config_file, imgs, effects)) < 0) {
		printf("%s: invalid config file.\n", config_file);
		return EXIT_FAILURE;
	}

	FILE *img = NULL;
	unsigned char **rasters = malloc(sizeof(void *) * img_n);

	pm_init(argv[0], 0);

	for (int i = 0; i < img_n; i++) {
		img = pm_openr(imgs[i]);
		if (!img) {
			pfan_free_rasters(rasters, i);
			printf("pfan: can not open '%s'.\n", imgs[i]);
			return EXIT_FAILURE;
		}
		rasters[i] = pfan_create_raster(img);
		pm_close(img);
	}

	int fan_fd = open(DEVICE, O_RDWR);

	if (fan_fd <= 0) {
		pfan_free_rasters(rasters, img_n);
		printf("Cannot open the USB device.\n");
		return EXIT_FAILURE;
	}

	struct pfan_data data;
	int ret = EXIT_SUCCESS;

	data.n = img_n;
	memcpy(data.effects, effects, sizeof(effects));
	data.images = rasters;

	if (write(fan_fd, &data, sizeof(struct pfan_data)) <= 0) {
		printf("Error occured during the data sending. %s\n", strerror(errno));
		ret = EXIT_FAILURE;
	}

	close(fan_fd);
	pfan_free_rasters(rasters, img_n);
	return ret;
}

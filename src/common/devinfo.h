#ifndef DATA_H_
#define DATA_H_

#define PFAN_CLASSNAME    "usb/pfan0"
#define PFAN_DEVNAME      "/dev/pfan0"
#define PFAN_VID           0x0c45
#define PFAN_PID           0x7701

/**
 * struct ventilo_data - designed to be the buffer as parameter for the module
 * driver's write function.
 * @n: number of displays, eight maximum, [0 ; 7]
 * @effects: the configuration effects for the eight fan displays,
 * contains three options for display effects.
 *	- 1: open effect option  [0 ; 6] <-- range of values
 *	- 2: close effect option [0 ; 6]
 *	- 3: turn effect option  (0, 4, 6)
 * @displays: the eight bitmap displays, each one contains 1716 bytes or
 *	156 * 11 which represents 156 columns of 11 leds each.
 *	A 'display' means a whole ventilo's display.
 */
struct pfan_data {
	unsigned char n;
	unsigned char effects[8][3];
	unsigned char **images;
};

#endif /* DATA_H_ */

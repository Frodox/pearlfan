/*
 * The GPL License (GPLv3)
 *
 * Copyright © 2016 Thomas "Ventto" Venriès  <thomas.venries@gmail.com>
 *
 * Pearlfan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pearlfan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Pearlfan.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CONVERT_H_
#define CONVERT_H_

#include "defutils.h"

void pfan_draw_point(int xpos,
                     int ypos,
                     unsigned short display[PFAN_MAX_W]);

void pfan_draw_image(unsigned char *raster,
                     unsigned short display[PFAN_MAX_W]);

void pfan_draw_char(int xpos,
                    int c,
                    unsigned short display[PFAN_MAX_W]);

void pfan_draw_text(char *text,
                    int lenght,
                    int lspace,
                    unsigned short display[PFAN_MAX_W]);

int pfan_draw_paragraph(char *text,
                        unsigned short display[PFAN_MAX_DISPLAY][PFAN_MAX_W]);

#endif /* !CONVERT_H_ */

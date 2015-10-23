/*
 * bsync.h
 *
 * Copyright (C) 2015 - Francesco Giudici <francesco.giudici@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>
#include <syslog.h>

#define BVER    "1.0"
#define INDIR   "/sys/class/backlight/acpi_video0/"
#define OUTDIR  "/sys/class/backlight/intel_backlight/"

typedef enum {
	BR_UNKNOWN,
	BR_LINEAR,
	BR_BISECT
} bri_function;

typedef struct virtual_display_data {
	char *sys_dir;
	int maxb;
	int curb;
	int *bmap;
} vdisplay_data;

void daemonize();
void verbose_quit(const char *msg);
void comp_brifunction(vdisplay_data *src, vdisplay_data *dst, bri_function bri_f);
bri_function f_t2num(char *optarg);
int write_brightness(const char *path, const int bound);
int retrieve_bound(const char *path, const char *param);
int transform_br(int src_bri, int dst_max);
void print_usage();


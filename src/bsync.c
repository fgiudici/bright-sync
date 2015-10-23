/*
 * Brightness Sync - bsync
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

#include "bsync.h"

void daemonize() {
	pid_t pid;

	if ((pid = fork()) == -1)
		verbose_quit("fork failed");

	if (pid != 0)
		exit(0);

	if (setsid() == -1)
		verbose_quit("setsid failed");

	if (chdir("/var/log") == -1)
		verbose_quit("chdir failed");

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

int main (int argc, char** argv) {
	int ch, act_br, dmode = 1;
	bri_function bf = BR_BISECT;
	vdisplay_data src, dst;
	int inotifyFd, wd;
	char temp[256];
	ssize_t numRead;

	src.sys_dir = dst.sys_dir = NULL;

	while ((ch = getopt(argc, argv, "i:o:Ff:")) != -1) {
		switch (ch) {
			case 'i': src.sys_dir = optarg; break;
			case 'o': dst.sys_dir = optarg; break;
			case 'F': dmode = 0; openlog(NULL, LOG_PERROR, 0); break;
			case 'f': bf = f_t2num(optarg);
				if (bf == BR_UNKNOWN) {
					fprintf(stderr, "ERR: unknown function %s\n\n", optarg);
					print_usage();
					exit(0);
				}
				break;
			case 'h':
			default: print_usage(); exit(0);
		}
	}

	if (src.sys_dir == NULL)
		src.sys_dir = strdup(INDIR);
	if (dst.sys_dir == NULL)
		dst.sys_dir = strdup(OUTDIR);

	syslog(LOG_NOTICE, "starting daemon - target: %s\n", dst.sys_dir);
	syslog(LOG_NOTICE, "                  source: %s\n", src.sys_dir);

	/* retrieve max brightnesses */
	if ((src.maxb = retrieve_bound(src.sys_dir, "max_brightness")) < 0)
		verbose_quit("Unable to retrieve src max brightness");
	if ((dst.maxb = retrieve_bound(dst.sys_dir, "max_brightness")) < 0)
		verbose_quit("Unable to retrieve dst max_brightness");
	if ((src.curb = retrieve_bound(src.sys_dir, "brightness")) < 0)
		verbose_quit("Unable to retrieve src brightness");

	comp_brifunction(&src, &dst, bf);

	if ((inotifyFd = inotify_init()) == -1)
		verbose_quit("inotify_init failed");

	snprintf(temp, 255, "%s%s", src.sys_dir, "brightness");
	if ((wd = inotify_add_watch(inotifyFd, temp, IN_MODIFY)) == -1)
		verbose_quit("inotify_add_watch failed");

	if (dmode)
		daemonize();

	/* Init brightness */
	write_brightness(dst.sys_dir, dst.bmap[src.curb]);

	while (1) {
		/* Blocking read */
		numRead = read(inotifyFd, temp, 256);
		if (numRead == 0)
			verbose_quit("read from inotify fd returned 0");
		if (numRead == -1)
			verbose_quit("read from inotify fd error");

		if ((act_br = retrieve_bound(src.sys_dir, "brightness")) < 0)
			verbose_quit("unable to retrieve actual brightness");
		if (act_br == src.curb)
			continue;

		src.curb = act_br;
		write_brightness(dst.sys_dir, dst.bmap[act_br]);
	}
}

bri_function f_t2num(char *optarg) {
	if (!strcmp(optarg, "linear"))
		return BR_LINEAR;
	else if (!strcmp(optarg, "bisect"))
		return BR_BISECT;
	else
		return BR_UNKNOWN;
}

void comp_brifunction(vdisplay_data *src, vdisplay_data *dst, bri_function bri_f) {
	int i;

	if (src->maxb > 100)
		verbose_quit("Source brightness range too big");

	dst->bmap = calloc((src->maxb + 1), sizeof(int));
	dst->bmap[src->maxb] = dst->maxb;
	dst->bmap[0] = 0;

	switch (bri_f) {
		case BR_UNKNOWN:
		case BR_LINEAR:
			for (i = 1; i < src->maxb; i++)
				dst->bmap[i] = ((dst->maxb / src->maxb) * i);
			break;
		case BR_BISECT:
			for (i = (src->maxb - 1); i > 0; i--)
				dst->bmap[i] = (dst->bmap[i+1] / 2);
			break;
	}
}

void verbose_quit(const char *msg) {
	syslog(LOG_ERR, "%s.\n", msg);
	exit(0);
}

int write_brightness(const char *path, const int bound) {
	FILE *temp_f;
	char temp_s[256];

	snprintf(temp_s, 255, "%sbrightness", path);
	if ((temp_f = fopen(temp_s, "w")) == NULL)
		return -1;

	fprintf(temp_f, "%d", bound);
	fclose(temp_f);

	syslog(LOG_DEBUG, "written %d on %s.\n", bound, temp_s);
	return 0;
}

int retrieve_bound(const char *path, const char *param) {
	char temp_s[256];
	int bound;
	FILE *temp_f;

	snprintf(temp_s, 255, "%s%s", path, param);
	if ((temp_f = fopen(temp_s, "r")) == NULL)
		return -1;

	if ((fscanf(temp_f, "%d", &bound)) != 1)
		return -2;
	fclose(temp_f);

	return bound;
}

void print_usage() {
	printf("bsync v.%s:\n",BVER);
	printf(" bsync [-F] [-i <input directory>] [-o <output directory>] [-f <conversion function>]\n");
	printf("\t-h:\tprint this help and exit\n");
	printf("\t-F:\trun in foreground\n");
	printf("\t-i:\tread brightness value from <input directory>\n");
	printf("\t-o:\twrite brightness value to <output directory>\n");
	printf("\t-f:\tspecify brightness conversion funcion (linear, bisect)\n");
}



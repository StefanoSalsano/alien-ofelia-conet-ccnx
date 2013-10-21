/**
 * @file ccnd_main.c
 *
 * A CCNx program.
 *
 * Copyright (C) 2009-2011 Palo Alto Research Center, Inc.
 *
 * This work is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 * This work is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details. You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <signal.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ccnd_private.h"
#include "conet/conet.h"
static int stdiologger(void *loggerdata, const char *format, va_list ap) {
	FILE *fp = (FILE *) loggerdata;
	return (vfprintf(fp, format, ap));
}

int main(int argc, char **argv) {
	struct ccnd_handle *h;

	char line[256];
	FILE* file;

	if ((file = fopen("./conet.conf", "r")) == NULL) {
		fprintf(stderr,
				"Manca il file conet.conf!!!!! utilizzo parametri di default\n");
		conet_ifname = CONET_IFNAME;
		fclose(file);
		return -1;
	} else {
		while (fgets(line, 256, file) != NULL) {
			char par[256], val[256];

			if (line[0] == '#')
				continue;

			if (sscanf(line, "%s = %s", par, val) != 2) {
				//			fprintf(stderr, "Syntax error, line %d\n", linenum);
				continue;
			}
			if (strcmp(par, "if_name") == 0)
				conet_ifname = val;

			printf("conet.conf: %s = %s \n", par,
					val);
		}
		fclose(file);
	}
	if (argc > 1) {
		USE_CONET = atoi(argv[1]); //it affects propagate_interest in conet.c
		if (argc > 2) {
			CONET_PREFETCH = atoi(argv[2]);
		}
		if (argc > 3) {
			//    		TAG_VLAN=atoi(argv[3]);  //TAG_VLAN
			int temp = atoi(argv[3]); //min pack len 0 => default in conet.h
			if (temp != 0) {
				MIN_PACK_LEN = atoi(argv[3]);
			}
		}
		if (argc > 4) {
			int temp = atoi(argv[4]); //slow start threshold 0 = default in conet.h
			if (temp != 0) {
				SS_THRESH = atoi(argv[4]);
			}
		}
		if (argc > 5) {
			USER_PARAM_1 = atoi(argv[5]);
		}
	}
	signal(SIGPIPE, SIG_IGN);
	h = ccnd_create(argv[0], stdiologger, stderr);
	if (h == NULL)
		exit(1);
	ccnd_run(h);
	ccnd_msg(h, "exiting.");
	ccnd_destroy(&h);
	exit(0);
}


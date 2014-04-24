/*
 * gadgetd.c
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <gadgetd-config.h>
#include <gadgetd-create.h>
#include <gadgetd-common.h>

#include <gio/gio.h>
#include <glib/gprintf.h>

/* default path for config file */
#define CONFIG_FILE	 "/etc/gadgetd/gadgetd.config"
#define CONFIGFS_MNT	 "/sys/kernel/config"

struct gd_config config;

static void
usage() {

	fprintf(stdout,
		"\nUsage: gadgetd [option]...\n"
		"\n"
		"  -c [file path] custom config file location\n"
		"  -h display this help screen\n"
		"\n");
	exit(GD_SUCCESS);
}

static int
parse_cmdline(int argc, char * const argv[], struct gd_config *pconfig)
{
	int opt = 0;
	struct stat st;
	extern char *optarg;
	extern int optind, optopt;

	while((opt = getopt(argc, argv, "c:h")) != -1) {
		switch (opt) {
		case 'c':
			pconfig->gd_config_file_path = strdup(optarg);
			if(pconfig->gd_config_file_path == NULL) {
				ERROR("\nUnknown error\n");
				exit(GD_ERROR_OTHER_ERROR);
			}
			if (stat(optarg, &st) < 0 ) {
				ERROR("\nBad filename :""%s error %s \n",
					optarg, strerror(errno));
				exit(GD_ERROR_FILE_OPEN_FAILED);
			}
			break;
		case 'h':
			usage();
		default:
			usage();
		}
	}

	return 0;
}

static void
gd_free_config(struct gd_config *config)
{
	free(config->g_attrs);
	free(config->g_strs);
	free(config->cfg_strs);
	free(config->gd_config_file_path);
	free(config->configfs_mnt);
}

static int
init_config_attrs(struct gd_config *pconfig)
{
	int g_ret = GD_SUCCESS;

	pconfig->g_attrs = malloc(sizeof(usbg_gadget_attrs));
	pconfig->g_strs = malloc(sizeof(usbg_gadget_strs));
	pconfig->cfg_strs = malloc(sizeof(usbg_config_strs));

	if(pconfig->g_attrs == NULL || pconfig->g_strs == NULL
	   || pconfig->cfg_strs == NULL) {
		ERROR("ERROR allocating memory");
		free(pconfig->g_attrs);
		free(pconfig->g_strs);
		free(pconfig->cfg_strs);
		g_ret = GD_ERROR_NO_MEM;
	}

	pconfig->gd_config_file_path = NULL;
	pconfig->configfs_mnt = NULL;

	return g_ret;
}

static int
gd_read_config(int argc, char * const argv[], struct gd_config *pconfig)
{
	int g_ret = GD_SUCCESS;

	parse_cmdline(argc, argv, pconfig);

	if (pconfig->gd_config_file_path == NULL) {
		pconfig->gd_config_file_path = strdup(CONFIG_FILE);
	}

        g_ret = gd_read_config_file(pconfig);
        if (g_ret != GD_SUCCESS) {
                ERROR("Error opening config file\n");
        }

	if (pconfig->configfs_mnt == NULL){
		pconfig->configfs_mnt = strdup(CONFIGFS_MNT);
	}

	return g_ret;
}


int
main(int argc, char **argv)
{
	int g_ret = GD_SUCCESS;

	g_ret = init_config_attrs(&config);
	if (g_ret != GD_SUCCESS) {
                ERROR("Error alocating memory\n");
        }

	g_ret = gd_read_config(argc, argv, &config);
	if (g_ret != GD_SUCCESS) {
                ERROR("Error reading default config\n");
        }

	gd_free_config(&config);
	return g_ret;
}

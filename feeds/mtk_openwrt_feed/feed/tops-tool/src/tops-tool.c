// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <Alvin.Kuo@mediatek.com>
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "dump.h"

static void print_usage(void)
{
	printf("Usage:\n");
	printf(" tops-tool [CMD] [DUMP_DIR]\n");
	printf(" [CMD] are:\n");
	printf("    save_dump   save dump data as file in directory [DUMP_DIR]\n");
	printf(" [DUMP_DIR] is directory of dump file\n");
}

static int verify_parameters(int argc,
			     char *argv[])
{
	char *cmd;

	if (argc < 2) {
		fprintf(stderr, DUMP_LOG_FMT("missing cmd\n"));
		return -EINVAL;
	}

	cmd = argv[1];
	if (!strncmp(cmd, "save_dump", 9)) {
		if (argc < 3) {
			fprintf(stderr, DUMP_LOG_FMT("too few parameters\n"));
			return -EINVAL;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	char *cmd;

	ret = verify_parameters(argc, argv);
	if (ret) {
		print_usage();
		goto error;
	}

	cmd = argv[1];
	if (!strncmp(cmd, "save_dump", 9)) {
		ret = tops_save_dump_data(argv[2]);
		if (ret) {
			fprintf(stderr,
				DUMP_LOG_FMT("cmd %s: save dump data fail(%d)\n"),
					     cmd, ret);
			goto error;
		}
	} else {
		fprintf(stderr, DUMP_LOG_FMT("unsupported cmd %s\n"), cmd);
		goto error;
	}

error:
	return ret;
}

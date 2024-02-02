# SPDX-License-Identifier: GPL-2.0-or-later */
#
# Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
#
# Author: Alvin Kuo <Alvin.Kuo@mediatek.com>
#

PROJECT := tops-tool
OBJECTS := tops-tool.o dump.o

all: $(PROJECT)

$(PROJECT): $(OBJECTS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY : clean
clean:
	rm -f $(PROJECT) *.o

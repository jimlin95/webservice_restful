# Copyright (c) 2014 Cesanta Software
# All rights reserved

PROG = app
CFLAGS = -W -Wall -lm -I../mongoose/ -I ../cJSON/ -pthread -g -O0 $(CFLAGS_EXTRA)
SOURCES = $(PROG).c ../mongoose/mongoose.c ../cJSON/cJSON.c

$(PROG): $(SOURCES)
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

clean:
	rm -rf $(PROG) *.exe *.dSYM *.obj *.exp .*o *.lib

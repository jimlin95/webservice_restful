# Copyright (c) 2014 Cesanta Software
# All rights reserved

PROG = app
CFLAGS = -W -Wall -lm -lsqlite3 -I../mongoose/ -I ../cJSON/ -I ../sqlite/ -pthread -g -O0 -L../sqlite/.libs $(CFLAGS_EXTRA)
SOURCES = $(PROG).c ../mongoose/mongoose.c ../cJSON/libcjson.a

$(PROG): $(SOURCES)
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

clean:
	rm -rf $(PROG) *.exe *.dSYM *.obj *.exp .*o *.lib

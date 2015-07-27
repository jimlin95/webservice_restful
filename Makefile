# Copyright (c) 2014 Cesanta Software
# All rights reserved

PROG = restful_api
CFLAGS = -W -Wall -I../mongoose/ -pthread -g -O0 $(CFLAGS_EXTRA)
SOURCES = $(PROG).c ../mongoose/mongoose.c

$(PROG): $(SOURCES)
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

clean:
	rm -rf $(PROG) *.exe *.dSYM *.obj *.exp .*o *.lib

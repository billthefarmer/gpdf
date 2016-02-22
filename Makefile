# Mekfile
#

GCC    = gcc
CFLAGS = -g -W -Wall -std=gnu99 -Iinclude -Llibs -lhpdf

all:	gpdf

gpdf:	gpdf.c getline.c

clean:
	rm *.exe *.o

%:	%.c
	$(GCC) -o $@ $^ $(CFLAGS)

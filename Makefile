# Mekfile for gpdf
#

GCC    = gcc

ifeq ($(OS), Windows_NT)
CFLAGS = -g -W -Wall -std=gnu99 -Iinclude -Llib -lhpdf
else
CFLAGS = -g -W -Wall -std=gnu99 -Iinclude -lhpdf
endif

all:	gpdf

ifeq ($(OS), Windows_NT)
gpdf:	gpdf.c getline.c
else
gpdf:	gpdf.c
endif

clean:
	rm *.exe *.o

%:	%.c
	$(GCC) -o $@ $^ $(CFLAGS)

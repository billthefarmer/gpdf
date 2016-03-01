# Mekfile for gpdf
#

GCC    = gcc

ifeq ($(OS), Windows_NT)
  win64 = $(shell which gcc | grep 64)
  ifneq ($(win64)x, x)
    CFLAGS = -g -W -Wall -std=gnu99 -Iinclude -Llib64 -lhpdf

  else
    CFLAGS = -g -W -Wall -std=gnu99 -Iinclude -Llib32 -lhpdf
  endif

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

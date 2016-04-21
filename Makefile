# Mekfile for gpdf
#

GCC    = gcc
G++    = g++

ifeq ($(OS), Windows_NT)
  win64 = $(shell which gcc | grep 64)
  ifneq ($(win64)x, x)
    CFLAGS = -g -W -Wall -std=gnu++11 -Iinclude -Llib64 -lhpdf

  else
    CFLAGS = -g -W -Wall -std=gnu++11 -Iinclude -Llib32 -lhpdf
  endif

else
  CFLAGS = -g -W -Wall -std=c++11 -Iinclude -lhpdf
endif

all:	gpdf

ifeq ($(OS), Windows_NT)
gpdf:	gpdf.cpp getline.c
else
gpdf:	gpdf.cpp
endif

clean:
	rm *.exe *.opp

%:	%.cpp
	$(G++) -o $@ $^ $(CFLAGS)

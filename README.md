# Gpdf
## GEDCOM genealogy pdf chart creation

Read and parse a GEDCOM format text file into memory and produce a
large format pdf output file. This program was developed and tested
using GEDCOM output from the Webtrees geneology software. It has also
been tested with the allged.ged test file which contains masses of
data which is ignored.

I initially attempted to use the
[GEDCOM parser library](http://gedcom-parse.sourceforge.net) for
parsing but could only successfuly build it on 32 bit linux. Next I
tried [GHOSTS](http://www.nongnu.org/ghosts/users/index.html) with
similar results. So I decided to write the parser myself as the GEDCOM
format is almost self documenting once you look at. This means that it
will only handle ASCII and a subset of UTF-8 text. The test data was
produced on [Webtrees](https://www.webtrees.net/index.php/en).

I decided to use [libHaru ](http://libharu.org) to produce the pdf
output. This will build successfully after the build files are
patched. The program is written in generic C, so should work on any
platform that libHaru will build on. To build libHaru on windows
requires zlib and libpng, which can be found in MingW32 and
gnuwin32. To build libHaru on Mac OSX requires libpng, which can be
built and installed. On Linux, just install zlib and libpng if they
are not already installed. On Linux Mint thats:
```
$ sudo apt install libz-dev libpng-dev
```
I discovered that Mingw32 does not contain `getline()`, which is a bit
essential. I discovered an implementation online. This is not needed
on linux and not on OSX either.

To run on windows you will need to extricate libpng and zlib from
Mingw32 and put them in the execution folder with libHaru.

To use the program, use the -w switch for the initial run like this:
```
$ gpdf -w <infile.ged>
```
This will produce an A3 pdf file with a grid of slots with x, y
positions, and a text file with a list of ids, two zeros and the
name for each individual in the file. Like this:

![billthefarmer.github.io/images/gpdf/slots.png](billthefarmer.github.io/images/gpdf/slots.png)
```
   0  posn suggested
   0  x  y    x      Name
   1  0  0    1      Jane /Doe/
   2  0  0    1      John /Doe/
...
```
The suggested x coordinate in the file is based on the calculated
generation of the individual based on the number of generations of
ancestors. It won't always be correct.  Fill in the x, y positions
for each individual in the file and run the program again without the
-w switch. This will produce a pdf file with the chart.

You can to test runs with only a few positions filled in to see what
it looks like. Individuals with two zeros for the position won't
appear.

![billthefarmer.github.io/images/gpdf/allged.png](billthefarmer.github.io/images/gpdf/allged.png)

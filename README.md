# Gpdf
## GEDCOM genealogy pdf chart creation

Read and parse a GEDCOM format text file into memory and produce a
large format pdf output file.

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
platform that libHaru will build on.

I discovered that Mingw32 does not contain `getline()`, which is a bit
essential. I discovered an implementation online. This is not needed
on linux and probably not on OSX either.

To run on windows you will need to extricate libpng and zlib from
Mingw32 and put them in the execution folder.

To use the program, use the -w switch for the initial run like this:
```
$ gpdf -w <infile.ged>
```
This will produce an A3 pdf file with a grid of slots with x, y
coordinates, and a text file with a list of ids, two zeros and the
name for each individual in the file. Like this:
```
1  0 0 Jane /Doe/
2  0 0 John /Doe/
...
```
Fill in the x, y coordinates for each individual in the file and run
the program again without the -w switch. This will produce a pdf file
with the chart, overwriting the grid.

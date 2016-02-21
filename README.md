# Gpdf
## GEDCOM genealogy pdf chart creation

Read and parse a GEDCOM format text file into memory and produce a large format pdf output file.

I initially attempted to use the [GEDCOM parser library](http://gedcom-parse.sourceforge.net) for parsing but could only successfuly build it on 32 bit linux. Next I tried [GHOSTS](http://www.nongnu.org/ghosts/users/index.html) with similar results. So I decided to write the parser myself as the GEDCOM format is almost self documenting one you look at. This means that it will only handle ASCII and a subset of UTF-8 text. The test data was produced on [Webtrees](https://www.webtrees.net/index.php/en).

I decided to use [libHaru ](http://libharu.org) to produce the pdf output. This will build successfully after the build files are patched. The program is written in generic C, so should work on any platform that libHaru  will build on.

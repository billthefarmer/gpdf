////////////////////////////////////////////////////////////////////////////////
//
//  Gpdf - Convert GedCOM file tp pdf chart.
//
//  Copyright (C) 2016  Bill Farmer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//  Bill Farmer  william j farmer [at] yahoo [dot] co [dot] uk.
//
///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdbool.h>

#include "hpdf.h"
#include "gpdf.h"

// static const int pagesizes[5][2] =
//     {{840, 1188},
//      {594, 840},
//      {420, 594},
//      {297, 420},
//      {210, 297}};

// static const float muliplier = 72.0 / 25.4;

indi inds[SIZE_INDS] = {};
fam  fams[SIZE_FAMS] = {};

indi *indp;
fam  *famp;

int state = STATE_NONE;
int date = DATE_NONE;
int plac = PLAC_NONE;

int fmss = 0;
int chln = 0;
int gens = 0;

int indindex = 1;
int famindex = 1;

bool writetext = false;
bool boldnames = false;

char file[SIZE_NAME];

float fs = SIZE_FONT;
jmp_buf env;

int main(int argc, char *argv[])
{
    int c;

    // Check args

    opterr = 0;

    while ((c = getopt(argc, argv, "wf:")) != -1)
	switch (c)
	{
	case 'w':
	    writetext = true;
	    break;

	case 'f':
	    fs = atof(optarg);
	    break;

	case '?':
	    if (optopt == 'f')
		fprintf (stderr, "Option -%c requires an argument\n", optopt);

	    else if (isprint (optopt))
		fprintf (stderr, "Unknown option `-%c'\n", optopt);

	    else
		fprintf (stderr,
			 "Unknown option character `\\x%x'\n",
			 optopt);
	    return GPDF_ERROR;

	default:
	    return GPDF_ERROR;
	}

    if (argv[optind] == NULL)
    {
	fprintf(stderr, "Usage: %s [-w] [-f fontsize] <infile>\n\n",
		argv[0]);
	fprintf(stderr, "  -w - write text file and layout page\n");
	// fprintf(stderr, "  -b - surnames in bold text\n");
	// fprintf(stderr, "  -p - set page size A0 -- A4\n");
	fprintf(stderr, "  -f - set font size in points (1/72 inch)\n");

	return GPDF_ERROR;
    }

    int result;

    // Parse the input file

    result = parse_gedcom_file(argv[optind]);

    if (result != GPDF_SUCCESS)
    {
	fprintf(stderr, "gpdf: Couldn't parse %s\n", argv[optind]);
	return GPDF_ERROR;
    }

    // Find generations in data

    find_generations();

    // If writing text file

    if (writetext)
	write_textfile();

    // Draw the tree

    draw_pdf();

    return GPDF_SUCCESS;
}

int find_individual(char *xref)
{
    for (int i = 1; i < indindex; i++)
    {
	// If found return id

	if ((inds[i].id > 0) && (strcmp(inds[i].xref, xref) == 0))
	{
	    return inds[i].id;
	}
    }

    // Else use next slot, save xref and return id

    inds[indindex].id = indindex;
    strcpy(inds[indindex].xref, xref);
    return inds[indindex++].id;
}

int find_family(char *xref)
{
    for (int i = 1; i < famindex; i++)
    {
	// If found return id

	if ((fams[i].id > 0) && (strcmp(fams[i].xref, xref) == 0))
	{
	    return fams[i].id;
	}
    }

    // Else use next slot, save xref and return id

    fams[famindex].id = famindex;
    strcpy(fams[famindex].xref, xref);
    return fams[famindex++].id;
}

int parse_gedcom_file(char *filename)
{
    FILE *infile = NULL;;
    char line[256];
    char *linep = line;
    size_t size = sizeof(line);
    int status;

    // Open the file

    infile = fopen(filename, "r");

    if (infile == NULL)
	return GPDF_ERROR;

    // Get lines

    while (getline(&linep, &size, infile) != -1)
    {
	int type;
	char first[64] = {0};
	char second[64] = {0};

	// parse fields

	sscanf(line, "%d %63s %63[0-9a-zA-Z /@-]", &type, first, second);

	// Check record type

	switch (type)
	{
	case TYPE_OBJECT:
	    status = object(first, second);
	    break;

	case TYPE_ATTR:
	    status = attrib(first, second);
	    break;

	case TYPE_ADDN:
	    status = additn(first, second);
	    break;
	}

	if (status != GPDF_SUCCESS)
	{
	    fclose(infile);
	    return GPDF_ERROR;
	}
    }

    fclose(infile);

    return GPDF_SUCCESS;
}

int object(char *first, char *second)
{
    // Head

    if (strcmp(first, "HEAD") == 0)
    {
	state = STATE_HEAD;
    }

    // Individual

    else if (strcmp(second, "INDI") == 0)
    {
	int id = 0;
	char xref[SIZE_XREF];

	sscanf(first, "@%31[0-9A-Za-z_]@", xref);

	id = find_individual(xref);

	if (id == 0)
	{
	    fprintf(stderr, "gpdf: Can't find '%s'\n", first);
	    return GPDF_ERROR;
	}

	indp = &inds[id];
	indp->id = id;
	state = STATE_INDI;
	fmss = 0;
    }

    // Family

    else if (strcmp(second, "FAM") == 0)
    {
	int id = 0;
	char xref[SIZE_XREF];

	sscanf(first, "@%31[0-9A-Za-z_]@", xref);

	id = find_family(xref);

	if (id == 0)
	{
	    fprintf(stderr, "gpdf: Can't find '%s'\n", first);
	    return GPDF_ERROR;
	}

	famp = &fams[id];
	famp->id = id;
	state = STATE_FAM;
	chln = 0;
    }

    return GPDF_SUCCESS;
}

int attrib(char *first, char *second)
{
    switch (state)
    {
	// Head

    case STATE_HEAD:
	if (strcmp(first, "FILE") == 0)
	{
	    strncpy(file, second, sizeof(file) - 1);
	}
	break;

	// Individual

    case STATE_INDI:
	if (strcmp(first, "NAME") == 0)
	{
	    strncpy(indp->name, second, SIZE_NAME - 1);
	}

	else if (strcmp(first, "SEX") == 0)
	{
	    strncpy(indp->sex, second, SIZE_SEX - 1);
	}

	else if (strcmp(first, "BIRT") == 0)
	{
	    date = DATE_BIRT;
	    plac = PLAC_BIRT;
	}

	else if (strcmp(first, "DEAT") == 0)
	{
	    indp->deat.yes = true;
	    date = DATE_DEAT;
	    plac = PLAC_DEAT;
	}

	else if (strcmp(first, "CHAN") == 0)
	{
	    date = DATE_NONE;
	    plac = PLAC_NONE;
	}

	else if (strcmp(first, "FAMC") == 0)
	{
	    int id;
	    char xref[SIZE_XREF];

	    sscanf(second, "@%31[0-9A-Za-z_]@", xref);

	    id = find_family(xref);

	    if (id == 0)
	    {
		fprintf(stderr, "gpdf: Can't find '%s'\n", second);
		return GPDF_ERROR;
	    }

	    indp->famc = &fams[id];
	}

	else if (strcmp(first, "FAMS") == 0)
	{
	    int id;
	    char xref[SIZE_XREF];

	    sscanf(second, "@%31[0-9A-Za-z_]@", xref);

	    id = find_family(xref);

	    if (id == 0)
	    {
		fprintf(stderr, "gpdf: Can't find '%s'\n", second);
		return GPDF_ERROR;
	    }

	    indp->fams[fmss++] = &fams[id];
	}

	else if (strcmp(first, "NCHI") == 0)
	{
	    indp->nchi = atoi(second);
	}

	else if (strcmp(first, "OCCU") == 0)
	{
	    strncpy(indp->occu, second, SIZE_OCCU - 1);
	}
	break;

	// Family

    case STATE_FAM:
	if (strcmp(first, "HUSB") == 0)
	{
	    int id;
	    char xref[SIZE_XREF];

	    sscanf(second, "@%31[0-9A-Za-z_]@", xref);

	    id = find_individual(xref);

	    if (id == 0)
	    {
		fprintf(stderr, "gpdf: Can't find '%s'\n", second);
		return GPDF_ERROR;
	    }

	    famp->husb = &inds[id];
	}

	else if (strcmp(first, "WIFE") == 0)
	{
	    int id;
	    char xref[SIZE_XREF];

	    sscanf(second, "@%31[0-9A-Za-z_]@", xref);

	    id = find_individual(xref);

	    if (id == 0)
	    {
		fprintf(stderr, "gpdf: Can't find '%s'\n", second);
		return GPDF_ERROR;
	    }

	    famp->wife = &inds[id];
	}

	else if (strcmp(first, "CHIL") == 0)
	{
	    int id;
	    char xref[SIZE_XREF];

	    sscanf(second, "@%31[0-9A-Za-z_]@", xref);

	    id = find_individual(xref);

	    if (id == 0)
	    {
		fprintf(stderr, "gpdf: Can't find '%s'\n", second);
		return GPDF_ERROR;
	    }

	    famp->chil[++chln] = &inds[id];
	}

	else if (strcmp(first, "CHAN") == 0)
	{
	    date = DATE_NONE;
	    plac = PLAC_NONE;
	}

	else if (strcmp(first, "MARR") == 0)
	{
	    famp->marr.yes = true;
	    date = DATE_MARR;
	    plac = PLAC_MARR;
	}

	else if (strcmp(first, "DIV") == 0)
	{
	    famp->div.yes = true;
	    date = DATE_DIV;
	    plac = PLAC_DIV;
	}
	break;
    }

    return GPDF_SUCCESS;
}

int additn(char *first, char *second)
{
    switch (state)
    {
	// Individual

    case STATE_INDI:
	if (strcmp(first, "GIVN") == 0)
	{
	    strncpy(indp->givn, second, SIZE_GIVN - 1);
	}

	else if (strcmp(first, "SURN") == 0)
	{
	    strncpy(indp->surn, second, SIZE_SURN - 1);
	}

	else if (strcmp(first, "NICK") == 0)
	{
	    strncpy(indp->nick, second, SIZE_NICK - 1);
	}

	else if (strcmp(first, "_MARNM") == 0)
	{
	    strncpy(indp->marn, second, SIZE_NAME - 1);
	}

	else if (strcmp(first, "DATE") == 0)
	{
	    switch (date)
	    {
	    case DATE_BIRT:
		strncpy(indp->birt.date, second, SIZE_DATE - 1);
		break;

	    case DATE_DEAT:
		strncpy(indp->deat.date, second, SIZE_DATE - 1);
		break;
	    }
	}

	else if (strcmp(first, "PLAC") == 0)
	{
	    switch (plac)
	    {
	    case PLAC_BIRT:
		strncpy(indp->birt.plac, second, SIZE_PLAC - 1);
		break;

	    case PLAC_DEAT:
		strncpy(indp->deat.plac, second, SIZE_PLAC - 1);
		break;
	    }
	}
	break;

	// Family

    case STATE_FAM:
	{
	    if (strcmp(first, "DATE") == 0)
	    {
		switch (date)
		{
		case DATE_MARR:
		    strncpy(famp->marr.date, second, SIZE_DATE - 1);
		    break;

		case DATE_DIV:
		    strncpy(famp->div.date, second, SIZE_DATE - 1);
		    break;
		}
	    }

	    else if (strcmp(first, "PLAC") == 0)
	    {
		switch (plac)
		{
		case PLAC_MARR:
		    strncpy(famp->marr.plac, second, SIZE_PLAC - 1);
		    break;

		case PLAC_DIV:
		    strncpy(famp->div.plac, second, SIZE_PLAC - 1);
		    break;
		}
		break;
	    }
	}
    }

    return GPDF_SUCCESS;
}

int find_generations()
{
    // Iterate through the individuals

    for (int i = 1; i < SIZE_INDS; i++)
    {
	if (inds[i].id > 0)
	{
	    // If they have parents

	    if (inds[i].famc != NULL)
	    {
		int n = 0;
		fam *famp = inds[i].famc;

		// Follow the links

		while (famp != NULL)
		{
		    // Husband

		    if ((famp->husb != NULL) &&
			(famp->husb->famc != NULL))
			famp = famp->husb->famc;

		    // Wife

		    else if (famp->wife != NULL)
			famp = famp->wife->famc;

		    // Give up

		    else
			famp = NULL;

		    // Count the generations

		    n++;
		}

		inds[i].gens = n;

		// Remember generations

		if (gens < n)
		    gens = n;
	    }
	}
    }

    for (int i = 1; i < SIZE_INDS; i++)
    {
	if (inds[i].id > 0)
	{
	    // If male

	    if (inds[i].sex[0] == 'M')
	    {
		// See if a wife has more generations

		for (int j = 0; j < SIZE_FMSS; j++)
		{
		    if ((inds[i].fams[j] != NULL) &&
			(inds[i].fams[j]->wife != NULL) &&
			(inds[i].fams[j]->wife->gens > inds[i].gens))
			inds[i].gens = inds[i].fams[j]->wife->gens;
		}
	    }

	    else
	    {
		// See if a husband has more generations

		for (int j = 0; j < SIZE_FMSS; j++)
		{
		    if ((inds[i].fams[j] != NULL) &&
			(inds[i].fams[j]->husb != NULL) &&
			(inds[i].fams[j]->husb->gens > inds[i].gens))
			inds[i].gens = inds[i].fams[j]->husb->gens;
		}
	    }

	    // Calculate x position on page

	    inds[i].posn.x = gens - inds[i].gens;
	}
    }

    return GPDF_SUCCESS;
}

int write_textfile()
{
    char filename[256];
    FILE *textfile;

    strcpy(filename, file);
    strcat(filename, ".txt");

#ifndef __MINGW32__
    textfile = fopen(filename, "wx");
#else
    // File open mode 'wx' not supported by windows, although in POSIX
    // accorrding to Gnu C Library docs.

    textfile = fopen(filename, "r");

    if (textfile != NULL)
    {
	fprintf(stderr, "Not overwriting '%s'\n", filename);
	return GPDF_SUCCESS;
    }

    fclose(textfile);

    textfile = fopen(filename, "w");
#endif
    if (textfile == NULL)
    {
	fprintf(stderr, "gpdf: cant't write to %s\n", filename);
	return GPDF_SUCCESS;
    }

    fprintf(textfile, "   0  posn suggested\n");
    fprintf(textfile, "   0  x  y    x      Name\n");

    for (int i = 1; i < SIZE_INDS; i++)
    {
	if (inds[i].id > 0)
	{
	    fprintf(textfile, "%4d  0  0    %1.0f      %s\n",
		    inds[i].id, inds[i].posn.x, inds[i].name);
	}
    }

    fclose(textfile);

    return GPDF_SUCCESS;
}

int read_textfile()
{
    char filename[256];
    char line[256];
    char *linep = line;
    size_t size = sizeof(line);
    FILE *textfile;

    strcpy(filename, file);
    strcat(filename, ".txt");

    textfile = fopen(filename, "r");

    if (textfile == NULL)
    {
	fprintf(stderr, "gpdf: can't read %s\n", filename);
	return GPDF_ERROR;
    }

    while (getline(&linep, &size, textfile) != -1)
    {
	int id = 0;
	float x = 0;
	float y = 0;

	// parse fields

	sscanf(line, " %d %f %f", &id, &x, &y);

	if (id > 0)
	{
	    inds[id].posn.x = x;
	    inds[id].posn.y = y;
	}
    }

    fclose(textfile);

    return GPDF_SUCCESS;
}

// Error handler from examples

void error_handler(HPDF_STATUS error_no, HPDF_STATUS   detail_no,
		   void *user_data __attribute__ ((unused)))
{
    printf ("gpdf: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
	    (HPDF_UINT)detail_no);

    longjmp(env, 1);
}

// Draw individual info

int draw_individuals(HPDF_Page page, HPDF_Font font, HPDF_Font bold,
		     int fs, float height, float slotwidth, float slotheight)
{
    HPDF_Page_SetFontAndSize(page, font, fs);
    HPDF_Page_BeginText(page);

    for (int i = 0; i < SIZE_INDS; i++)
    {
	if (inds[i].id > 0)
	{
	    if ((inds[i].posn.x > 0) || (inds[i].posn.y > 0))
	    {
		float x = (SIZE_MARG * 4) + (inds[i].posn.x * slotwidth);
		float y = height - (SIZE_MARG * 2) -
		    (inds[i].posn.y * slotheight);

		// Name

		if (inds[i].givn[0] == '\0')
		{
		    char *givn;
		    char *surn;
		    char *endn;

		    // Name

		    givn = inds[i].name;
		    surn = strchr(inds[i].name, '/');
		    if (surn != NULL)
			*surn = '\0';

		    surn += 1;
		    endn = strchr(surn, '/');
		    if (endn != NULL)
			*endn = '\0';

		    HPDF_Page_TextOut(page, x, y, givn);
		    HPDF_Page_ShowText(page, " ");

		    HPDF_Page_SetFontAndSize(page, bold, fs);
		    HPDF_Page_ShowText(page, surn);
		    HPDF_Page_SetFontAndSize(page, font, fs);
		}

		else
		{
		    // Given names surname

		    HPDF_Page_TextOut(page, x, y, inds[i].givn);
		    if (inds[i].nick[0] != '\0')
		    {
			HPDF_Page_ShowText(page, " '");
			HPDF_Page_ShowText(page, inds[i].nick);
			HPDF_Page_ShowText(page, "' ");
		    }

		    else
			HPDF_Page_ShowText(page, " ");

		    HPDF_Page_SetFontAndSize(page, bold, fs);
		    HPDF_Page_ShowText(page, inds[i].surn);
		    HPDF_Page_SetFontAndSize(page, font, fs);
		}

		// Birth

		if ((inds[i].birt.date[0] != '\0') &&
		    (inds[i].birt.plac[0] != '\0'))
		{
		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    HPDF_Page_ShowText(page, "b   ");
		    HPDF_Page_ShowText(page, inds[i].birt.date);
		    HPDF_Page_ShowText(page, " ");
		    HPDF_Page_ShowText(page, inds[i].birt.plac);
		}

		else if (inds[i].birt.date[0] != '\0')
		{
		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    HPDF_Page_ShowText(page, "b   ");
		    HPDF_Page_ShowText(page, inds[i].birt.date);	
		}

		else if (inds[i].birt.plac[0] != '\0')
		{
		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    HPDF_Page_ShowText(page, "b   ");
		    HPDF_Page_ShowText(page, inds[i].birt.plac);	
		}

		// Occupation

		if (inds[i].occu[0] != '\0')
		{
		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    HPDF_Page_ShowText(page, "o   ");
		    HPDF_Page_ShowText(page, inds[i].occu);
		}

		// Marriages and divorces

		if (inds[i].sex[0] == 'F')
		{
		    for (int j = 0; j < SIZE_FMSS; j++)
		    {
			if (inds[i].fams[j] != NULL)
			{
			    fam *famp = inds[i].fams[j];

			    if ((famp->marr.date[0] != '\0') &&
				(famp->marr.plac[0] != '\0'))
			    {
				HPDF_Page_MoveToNextLine(page);
				HPDF_Page_MoveTextPos(page, 0, -fs);
				HPDF_Page_ShowText(page, "m  ");
				HPDF_Page_ShowText(page, famp->marr.date);
				HPDF_Page_ShowText(page, " ");
				HPDF_Page_ShowText(page, famp->marr.plac);
			    }

			    else if (famp->marr.date[0] != '\0')
			    {
				HPDF_Page_MoveToNextLine(page);
				HPDF_Page_MoveTextPos(page, 0, -fs);
				HPDF_Page_ShowText(page, "m  ");
				HPDF_Page_ShowText(page, famp->marr.date);	
			    }

			    else if (famp->marr.plac[0] != '\0')
			    {
				HPDF_Page_MoveToNextLine(page);
				HPDF_Page_MoveTextPos(page, 0, -fs);
				HPDF_Page_ShowTextNextLine(page, "m  ");
				HPDF_Page_ShowText(page, famp->marr.plac);	
			    }

			    // else if (famp->marr.yes)
			    // {
			    //     HPDF_Page_MoveToNextLine(page);
			    //     HPDF_Page_MoveTextPos(page, 0, -fs);
			    //     HPDF_Page_ShowText(page, "m");
			    // }

			    if ((famp->div.date[0] != '\0') &&
				(famp->div.plac[0] != '\0'))
			    {
				HPDF_Page_MoveToNextLine(page);
				HPDF_Page_MoveTextPos(page, 0, -fs);
				if ((famp->marr.date[0] == '\0') &&
				    (famp->marr.plac[0] == '\0'))
				    HPDF_Page_ShowText(page, "m, dv ");

				else
				    HPDF_Page_ShowText(page, "dv ");
				HPDF_Page_ShowText(page, famp->div.date);
				HPDF_Page_ShowText(page, " ");
				HPDF_Page_ShowText(page, famp->div.plac);
			    }

			    else if (famp->div.date[0] != '\0')
			    {
				HPDF_Page_MoveToNextLine(page);
				HPDF_Page_MoveTextPos(page, 0, -fs);
				if ((famp->marr.date[0] == '\0') &&
				    (famp->marr.plac[0] == '\0'))
				    HPDF_Page_ShowText(page, "m, dv ");

				else
				    HPDF_Page_ShowText(page, "dv ");
				HPDF_Page_ShowText(page, famp->div.date);	
			    }

			    else if (famp->div.plac[0] != '\0')
			    {
				HPDF_Page_MoveToNextLine(page);
				HPDF_Page_MoveTextPos(page, 0, -fs);
				if ((famp->marr.date[0] == '\0') &&
				    (famp->marr.plac[0] == '\0'))
				    HPDF_Page_ShowText(page, "m, dv ");

				else
				    HPDF_Page_ShowTextNextLine(page, "dv ");
				HPDF_Page_ShowText(page, famp->div.plac);	
			    }

			    // else if (famp->div.yes)
			    // {
			    //     HPDF_Page_ShowText(page, ", d");
			    // }
			}
		    }
		}

		// Children

		if (inds[i].nchi > 0)
		{
		    char s[16];

		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    sprintf(s, "c   %d", inds[i].nchi);
		    HPDF_Page_ShowText(page, s);
		}

		// Death

		if ((inds[i].deat.date[0] != '\0') &&
		    (inds[i].deat.plac[0] != '\0'))
		{
		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    HPDF_Page_ShowText(page, "d   ");
		    HPDF_Page_ShowText(page, inds[i].deat.date);
		    HPDF_Page_ShowText(page, " ");
		    HPDF_Page_ShowText(page, inds[i].deat.plac);
		}

		else if (inds[i].deat.date[0] != '\0')
		{
		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    HPDF_Page_ShowText(page, "d   ");
		    HPDF_Page_ShowText(page, inds[i].deat.date);	
		}

		else if (inds[i].deat.plac[0] != '\0')
		{
		    HPDF_Page_MoveToNextLine(page);
		    HPDF_Page_MoveTextPos(page, 0, -fs);
		    HPDF_Page_ShowText(page, "d   ");
		    HPDF_Page_ShowText(page, inds[i].deat.plac);	
		}

		// else if (inds[i].deat.yes)
		// {
		// 	HPDF_Page_MoveToNextLine(page);
		// 	HPDF_Page_MoveTextPos(page, 0, -fs);
		// 	HPDF_Page_ShowTextNextLine(page, "d");
		// }
	    }
	}
    }

    HPDF_Page_EndText(page);

    return GPDF_SUCCESS;
}

// Draw family lines

int draw_family_lines(HPDF_Page page, float height,
		      float slotwidth, float slotheight)
{
    // Draw individual famc and fams connections

    for (int i = 0; i < SIZE_INDS; i++)
    {
	if (inds[i].id > 0)
	{
	    if ((inds[i].posn.x > 0) || (inds[i].posn.y > 0))
	    {
		float x = (SIZE_MARG * 4) + (inds[i].posn.x * slotwidth);
		float y = height - (SIZE_MARG * 2) -
		    (inds[i].posn.y * slotheight);

		if (inds[i].famc != NULL)
		{
		    HPDF_Page_MoveTo(page, x + slotwidth - (SIZE_MARG * 2), y);
		    HPDF_Page_LineTo(page, x + slotwidth - SIZE_MARG, y);
		    HPDF_Page_Stroke(page);
		}

		if (inds[i].fams[0] != NULL)
		{
		    HPDF_Page_MoveTo(page, x, y);
		    HPDF_Page_LineTo(page, x - SIZE_MARG, y);
		    HPDF_Page_Stroke(page);
		}
	    }
	}
    }

    // Draw lines from wife to chilren

    for (int i = 1; i < SIZE_FAMS; i++)
    {
	if ((fams[i].wife != NULL) &&
	    ((fams[i].wife->posn.x > 0) || (fams[i].wife->posn.y > 0)))
	{
	    float wx = (SIZE_MARG * 3) + (fams[i].wife->posn.x * slotwidth);
	    float wy = height - (SIZE_MARG * 2) -
		(fams[i].wife->posn.y * slotheight);

	    for (int j = 1; j < SIZE_CHLN; j++)
	    {
		if ((fams[i].chil[j] != NULL) &&
		    ((fams[i].chil[j]->posn.x > 0) ||
		     (fams[i].chil[j]->posn.y > 0)))
		{
		    float cx = (SIZE_MARG * 3) + slotwidth +
			(fams[i].chil[j]->posn.x * slotwidth);
		    float cy = height - (SIZE_MARG * 2) -
			(fams[i].chil[j]->posn.y * slotheight);

		    HPDF_Page_MoveTo(page, wx, wy);
		    HPDF_Page_LineTo(page, cx, cy);
		    HPDF_Page_Stroke(page);
		}
	    }
	}

	// Draw lines from husband to children

	if ((fams[i].husb != NULL) &&
	    ((fams[i].husb->posn.x > 0) || (fams[i].husb->posn.y > 0)))
	{
	    float hx = (SIZE_MARG * 3) + (fams[i].husb->posn.x * slotwidth);
	    float hy = height - (SIZE_MARG * 2) -
		(fams[i].husb->posn.y * slotheight);

	    for (int j = 1; j < SIZE_CHLN; j++)
	    {
		if ((fams[i].chil[j] != NULL) &&
		    ((fams[i].chil[j]->posn.x > 0) ||
		     (fams[i].chil[j]->posn.y > 0)))
		{
		    float cx = (SIZE_MARG * 3) + slotwidth +
			(fams[i].chil[j]->posn.x * slotwidth);
		    float cy = height - (SIZE_MARG * 2) -
			(fams[i].chil[j]->posn.y * slotheight);

		    HPDF_Page_MoveTo(page, hx, hy);
		    HPDF_Page_LineTo(page, cx, cy);
		    HPDF_Page_Stroke(page);
		}
	    }
	}

	// Draw lines from wife to husband

	if ((fams[i].wife != NULL) && (fams[i].husb != NULL) &&
	    ((fams[i].wife->posn.x > 0) || (fams[i].wife->posn.y > 0)) &&
	    ((fams[i].husb->posn.x > 0) || (fams[i].husb->posn.y > 0)))
	{
	    float wx = (SIZE_MARG * 3) + (fams[i].wife->posn.x * slotwidth);
	    float wy = height - (SIZE_MARG * 2) -
		(fams[i].wife->posn.y * slotheight);

	    float hx = (SIZE_MARG * 3) + (fams[i].husb->posn.x * slotwidth);
	    float hy = height - (SIZE_MARG * 2) -
		(fams[i].husb->posn.y * slotheight);

	    HPDF_Page_MoveTo(page, hx, hy);
	    HPDF_Page_LineTo(page, wx, wy);
	    HPDF_Page_Stroke(page);
	}
    }

    return GPDF_SUCCESS;
}

// Draw the chart

int draw_pdf()
{
    HPDF_Doc  pdf;
    HPDF_Page page;
    HPDF_Font font;
    HPDF_Font bold;
    HPDF_REAL height;
    HPDF_REAL width;

    char filename[256];
    char title[256];

    strcpy(title, file);
    title[0] = toupper(title[0]);
    strcat(title, " Family Tree");

    pdf = HPDF_New(error_handler, NULL);
    if (pdf == NULL)
    {
        fprintf(stderr, "gpdf: cannot create PdfDoc object\n");
        return GPDF_ERROR;
    }

    if (setjmp(env))
    {
        HPDF_Free(pdf);
        return GPDF_ERROR;
    }

    // Add a new page object
    page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A3, HPDF_PAGE_LANDSCAPE);

    height = HPDF_Page_GetHeight(page);
    width  = HPDF_Page_GetWidth(page);

    // Draw the border of the page
    HPDF_Page_SetLineWidth(page, 0.6);
    HPDF_Page_Rectangle(page, SIZE_MARG, SIZE_MARG,
			width - (2 * SIZE_MARG), height - (2 * SIZE_MARG));

    font = HPDF_GetFont(pdf, FONT, NULL);
    bold = HPDF_GetFont(pdf, BOLD, NULL);

    if (writetext)
    {
	// Horizontal slots on the page

	float slotwidth = (width - (SIZE_MARG * 4)) / (gens + 1);
	float slotheight = fs * 6;

	for (int i = 1; i < gens + 1; i++)
	{
	    float x = (2 * SIZE_MARG) + (i * slotwidth);

	    HPDF_Page_MoveTo(page, x, SIZE_MARG);
	    HPDF_Page_LineTo(page, x, height - SIZE_MARG);
	}

	// Vertical slots on the page

	int slots = (height - (SIZE_MARG * 6)) / slotheight;
	for (int i = 1; i < slots; i++)
	{
	    float y = (4 * SIZE_MARG) + (i * slotheight);

	    HPDF_Page_MoveTo(page, SIZE_MARG, y);
	    HPDF_Page_LineTo(page, width - SIZE_MARG, y);
	}

	HPDF_Page_Stroke(page);
	HPDF_Page_SetFontAndSize(page, font, fs);
	HPDF_Page_BeginText(page);

	for (int i = 0; i < slots; i++)
	{
	    for (int j = 0; j < (gens + 1); j++)
	    {
		char s[16];

		float x = (3 * SIZE_MARG) + (j * slotwidth);
		float y = height - (6 * SIZE_MARG) - (i * slotheight);
		sprintf(s, "%d, %d", j, i);
		HPDF_Page_TextOut(page, x, y, s);
	    }
	}

	HPDF_Page_EndText(page);
	HPDF_SaveToFile(pdf, "slots.pdf");
    }

    else
    {
	int result;
	HPDF_REAL tw;

	result = read_textfile();

	if (result != GPDF_SUCCESS)
	    return GPDF_ERROR;

	HPDF_Page_Rectangle(page, width - 210, SIZE_MARG, 200, 22);
	HPDF_Page_Stroke(page);

	// Draw the title of the page (with positioning center).
	HPDF_Page_SetFontAndSize(page, font, 18);

	tw = HPDF_Page_TextWidth(page, title);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, (width - 110) - tw / 2, 14, title);
	HPDF_Page_EndText(page);

	float slotheight = fs * 6;
	float slotwidth = (width - (SIZE_MARG * 6)) / (gens + 1);

	draw_individuals(page, font, bold, fs, height, slotwidth, slotheight);
	draw_family_lines(page, height, slotwidth, slotheight);

	strcpy(filename, file);
	strcat(filename, ".pdf");

	// Save file

	HPDF_SaveToFile(pdf, filename);
    }

    return GPDF_SUCCESS;
}

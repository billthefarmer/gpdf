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

static const int pagesizes[5][2] =
    {{840, 1188},
     {594, 840},
     {420, 594},
     {297, 420},
     {210, 297}};

static const float muliplier = 72.0 / 25.4;

indi inds[SIZE_INDS] = {};
fam  fams[SIZE_FAMS] = {};

indi *indp;
fam  *famp;

int stat = STAT_NONE;
int date = DATE_NONE;
int plac = PLAC_NONE;

int fmss = 0;
int chln = 0;
int gens = 0;

bool writetext = false;
bool boldnames = false;

char *pagesize;
char *fontsize;

char file[SIZE_NAME];

int main(int argc, char *argv[])
{
    int c;

    // Check args

    if (argc < 2)
    {
	fprintf(stderr, "Usage: %s [-w] [-p pagesize] [-f fontsize] <infile>\n\n",
	       argv[0]);
	fprintf(stderr, "  -w - write text file and layout page\n");
	fprintf(stderr, "  -b - surnames in bold text\n");
	fprintf(stderr, "  -p - set page size A0 -- A4\n");
	fprintf(stderr, "  -f - set font size in 1/72nd inch\n");

	return GPDF_ERROR;
    }

    opterr = 0;

    while ((c = getopt(argc, argv, "wp:f:")) != -1)
	switch (c)
	{
	case 'w':
	    writetext = true;
	    break;

	case 'p':
	    pagesize = optarg;
	    break;

	case 'c':
	    fontsize = optarg;
	    break;

	case '?':
	    if ((optopt == 'p') || (optopt = 'f'))
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

    // Print the tree

    print_pdf();

    return GPDF_SUCCESS;
}

int parse_gedcom_file(char *filename)
{
    FILE *infile = NULL;;
    char buffer[256];
    char *bufferp = buffer;
    size_t size = sizeof(buffer);

    // Open the file

    infile = fopen(filename, "r");

    if (infile == NULL)
	return GPDF_ERROR;

    // Get lines

    while (getline(&bufferp, &size, infile) != -1)
    {
	int type;
	char first[64] = {0};
	char second[64] = {0};

	// parse fields

	sscanf(buffer, "%d %63s %63[0-9a-zA-Z /@-]", &type, first, second);

	// Check record type

	switch (type)
	{
	case TYPE_OBJECT:
	    object(first, second);
	    break;

	case TYPE_ATTR:
	    attrib(first, second);
	    break;

	case TYPE_ADDN:
	    additn(first, second);
	    break;
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
	stat = STAT_HEAD;
    }

    // Individual

    else if (strcmp(second, "INDI") == 0)
    {
	int id;

	sscanf(first, "@I%d@", &id);
	indp = &inds[id];
	indp->id = id;
	stat = STAT_INDI;
	fmss = 0;
    }

    // Family

    else if (strcmp(second, "FAM") == 0)
    {
	int id;

	sscanf(first, "@F%d@", &id);
	famp = &fams[id];
	famp->id = id;
	stat = STAT_FAM;
	chln = 0;
   }

    return GPDF_SUCCESS;
}

int attrib(char *first, char *second)
{
    switch (stat)
    {
	// Head

    case STAT_HEAD:
	if (strcmp(first, "FILE") == 0)
	{
	    strncpy(file, second, sizeof(file) - 1);
	}
	break;

	// Individual

    case STAT_INDI:
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

	    sscanf(second, "@F%d@", &id);
	    indp->famc = &fams[id];
	}

	else if (strcmp(first, "FAMS") == 0)
	{
	    int id;

	    sscanf(second, "@F%d@", &id);
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

    case STAT_FAM:
	if (strcmp(first, "HUSB") == 0)
	{
	    int id;

	    sscanf(second, "@I%d@", &id);
	    famp->husb = &inds[id];
	}

	else if (strcmp(first, "WIFE") == 0)
	{
	    int id;

	    sscanf(second, "@I%d@", &id);
	    famp->wife = &inds[id];
	}

	else if (strcmp(first, "CHIL") == 0)
	{
	    int id;

	    sscanf(second, "@I%d@", &id);
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
	break;
    }

    return GPDF_SUCCESS;
}

int additn(char *first, char *second)
{
    switch (stat)
    {
	// Individual

    case STAT_INDI:
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

    case STAT_FAM:
	{
	    if (strcmp(first, "DATE") == 0)
	    {
		switch (date)
		{
		case DATE_MARR:
		    strncpy(famp->marr.date, second, SIZE_DATE - 1);
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

    return GPDF_SUCCESS;
}

int write_textfile()
{
    char filename[256];
    FILE *textfile;

    strcpy(filename, file);
    strcat(filename, ".txt");

    textfile = fopen(filename, "w");

    if (textfile == NULL)
    {
	fprintf(stderr, "gpdf: cant't write to %s\n", filename);
	return GPDF_ERROR;
    }

    for (int i = 1; i < SIZE_INDS; i++)
    {
	if (inds[i].id > 0)
	{
	    fprintf(textfile, "%2d %-36s 0  0\n", inds[i].id, inds[i].name);
	}
    }

    fclose(textfile);

    return GPDF_SUCCESS;
}

// Use helvetica

#define FONT "Helvetica"
#define BOLD "Helvetica-Bold"

int fs = SIZE_FONT;
jmp_buf env;

// Error handler from examples

void error_handler(HPDF_STATUS error_no, HPDF_STATUS   detail_no,
		   void *user_data __attribute__ ((unused)))
{
    printf ("gpdf: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
	    (HPDF_UINT)detail_no);

    longjmp(env, 1);
}

// Print individual info

int print_indi(HPDF_Page page, HPDF_Font font, HPDF_Font bold,
	       int fs, int x, int y, int id)
{
    HPDF_Page_SetFontAndSize(page, font, fs);
    HPDF_Page_BeginText(page);

    // Name

    HPDF_Page_TextOut(page, x, y, inds[id].givn);
    HPDF_Page_ShowText(page, " ");
    HPDF_Page_SetFontAndSize(page, bold, fs);
    HPDF_Page_ShowText(page, inds[id].surn);
    HPDF_Page_SetFontAndSize(page, font, fs);

    // Birth

    if ((inds[id].birt.date[0] != '\0') &&
	(inds[id].birt.plac[0] != '\0'))
    {
	HPDF_Page_MoveToNextLine(page);
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowText(page, "b   ");
	HPDF_Page_ShowText(page, inds[id].birt.date);
	HPDF_Page_ShowText(page, " ");
	HPDF_Page_ShowText(page, inds[id].birt.plac);
    }

    else if (inds[id].birt.date[0] != '\0')
    {
	HPDF_Page_MoveToNextLine(page);
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowText(page, "b   ");
	HPDF_Page_ShowText(page, inds[id].birt.date);	
    }

    else if (inds[id].birt.plac[0] != '\0')
    {
	HPDF_Page_MoveToNextLine(page);
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowText(page, "b   ");
	HPDF_Page_ShowText(page, inds[id].birt.plac);	
    }

    // Marriages

    if (inds[id].sex[0] == 'F')
    {
	for (int i = 0; i < SIZE_FMSS; i++)
	{
	    if (inds[id].fams[i] != NULL)
	    {
		fam *famp = inds[id].fams[i];

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
	    }
	}
    }

    // Death

    if ((inds[id].deat.date[0] != '\0') &&
	(inds[id].deat.plac[0] != '\0'))
    {
	HPDF_Page_MoveToNextLine(page);
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowText(page, "d   ");
	HPDF_Page_ShowText(page, inds[id].deat.date);
	HPDF_Page_ShowText(page, " ");
	HPDF_Page_ShowText(page, inds[id].deat.plac);
    }

    else if (inds[id].deat.date[0] != '\0')
    {
	HPDF_Page_MoveToNextLine(page);
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowText(page, "d   ");
	HPDF_Page_ShowText(page, inds[id].deat.date);	
    }

    else if (inds[id].deat.plac[0] != '\0')
    {
	HPDF_Page_MoveToNextLine(page);
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowText(page, "d   ");
	HPDF_Page_ShowText(page, inds[id].deat.plac);	
    }

    // else if (inds[id].deat.yes)
    // {
    // 	HPDF_Page_MoveToNextLine(page);
    // 	HPDF_Page_MoveTextPos(page, 0, -fs);
    // 	HPDF_Page_ShowTextNextLine(page, "d");
    // }

    HPDF_Page_EndText(page);

    return GPDF_SUCCESS;
}

// Print the chart

int print_pdf()
{
    HPDF_Doc  pdf;
    HPDF_Page page;
    HPDF_Font font;
    HPDF_Font bold __attribute__ ((unused));
    HPDF_REAL tw;
    HPDF_REAL height;
    HPDF_REAL width;

    char filename[256];
    char title[256];

    if (writetext)
    {
	int result;

	result = write_textfile();

	if (result != GPDF_SUCCESS)
	    return GPDF_ERROR;
    }

    strcpy(title, file);
    title[0] = toupper(title[0]);
    strcat(title, " Family Tree");

    strcpy(filename, file);
    strcat(filename, ".pdf");
    
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

    /* Add a new page object. */
    page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A3, HPDF_PAGE_LANDSCAPE);

    height = HPDF_Page_GetHeight(page);
    width  = HPDF_Page_GetWidth(page);

    /* Print the lines of the page. */
    HPDF_Page_SetLineWidth(page, 0.6);
    HPDF_Page_Rectangle(page, SIZE_MARG, SIZE_MARG,
			width - (2 * SIZE_MARG), height - (2 * SIZE_MARG));

    font = HPDF_GetFont(pdf, "Helvetica", NULL);
    bold = HPDF_GetFont(pdf, "Helvetica-Bold", NULL);

    if (!writetext)
    {
	HPDF_Page_Rectangle(page, width - 210, SIZE_MARG, 200, 22);
	HPDF_Page_Stroke(page);

	/* Print the title of the page (with positioning center). */
	HPDF_Page_SetFontAndSize(page, font, 18);

	tw = HPDF_Page_TextWidth(page, title);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, (width - 110) - tw / 2, 14, title);
	HPDF_Page_EndText(page);
    }

    else
    {
	// Horizontal slots on the page

	int slotsize = (width - (SIZE_MARG * 4)) / (gens + 1);
	for (int i = 1; i < gens + 1; i++)
	{
	    int x = (2 * SIZE_MARG) + (i * slotsize);

	    HPDF_Page_MoveTo(page, x, SIZE_MARG);
	    HPDF_Page_LineTo(page, x, height - SIZE_MARG);
	}

	// Vertical slots on the page

	int slots = (height - (SIZE_MARG * 4)) / (fs * 4);
	for (int i = 1; i < slots; i++)
	{
	    int y = (4 * SIZE_MARG) + (i * fs * 4);

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

		int x = (3 * SIZE_MARG) + (j * slotsize);
		int y = (6 * SIZE_MARG) + (i * fs * 4);
		sprintf(s, "%d, %d", j, i);
		HPDF_Page_TextOut(page, x, y, s);
	    }
	}

	HPDF_Page_EndText(page);
    }

    // Save file

    HPDF_SaveToFile(pdf, filename);

    return GPDF_SUCCESS;
}

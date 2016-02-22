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

#include "hpdf.h"
#include "gpdf.h"

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

char file[SIZE_NAME];

int main(int argc, char *argv[])
{
    int result;

    // Check args

    if (argc < 2)
    {
	printf("Usage: %s <infile> [args...]\n", argv[0]);
	return GPDF_ERROR;
    }

    // Parse the input file

    result = parse_gedcom_file(argv[1]);

    if (result != 0)
    {
	printf("gpdf: Couldn't parse %s\n", argv[1]);
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
	    indp->deat.flag = TRUE;
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
	    famp->marr.flag = TRUE;
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
	       int x, int y, int id)
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
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowTextNextLine(page, "b  ");
	HPDF_Page_ShowText(page, inds[id].birt.date);
	HPDF_Page_ShowText(page, " ");
	HPDF_Page_ShowText(page, inds[id].birt.plac);
    }

    else if (inds[id].birt.date[0] != '\0')
    {
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowTextNextLine(page, "b  ");
	HPDF_Page_ShowText(page, inds[id].birt.date);	
    }

    else if (inds[id].birt.plac[0] != '\0')
    {
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowTextNextLine(page, "b  ");
	HPDF_Page_ShowText(page, inds[id].birt.plac);	
    }

    // Marriages

    for (int i = 0; i < SIZE_FMSS; i++)
    {
	if (inds[id].fams[i] != NULL)
	{
	    fam *famp = inds[id].fams[i];

	    if ((famp->marr.date[0] != '\0') &&
		(famp->marr.plac[0] != '\0'))
	    {
		HPDF_Page_MoveTextPos(page, 0, -fs);
		HPDF_Page_ShowTextNextLine(page, "m  ");
		HPDF_Page_ShowText(page, famp->marr.date);
		HPDF_Page_ShowText(page, " ");
		HPDF_Page_ShowText(page, famp->marr.plac);
	    }

	    else if (famp->marr.date[0] != '\0')
	    {
		HPDF_Page_MoveTextPos(page, 0, -fs);
		HPDF_Page_ShowTextNextLine(page, "m  ");
		HPDF_Page_ShowText(page, famp->marr.date);	
	    }

	    else if (famp->marr.plac[0] != '\0')
	    {
		HPDF_Page_MoveTextPos(page, 0, -fs);
		HPDF_Page_ShowTextNextLine(page, "m  ");
		HPDF_Page_ShowText(page, famp->marr.plac);	
	    }

	    else if (famp->marr.flag)
	    {
		HPDF_Page_MoveTextPos(page, 0, -fs);
		HPDF_Page_ShowTextNextLine(page, "m");
	    }
	}
    }

    // Death

    if ((inds[id].deat.date[0] != '\0') &&
	(inds[id].deat.plac[0] != '\0'))
    {
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowTextNextLine(page, "d  ");
	HPDF_Page_ShowText(page, inds[id].deat.date);
	HPDF_Page_ShowText(page, " ");
	HPDF_Page_ShowText(page, inds[id].deat.plac);
    }

    else if (inds[id].deat.date[0] != '\0')
    {
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowTextNextLine(page, "d  ");
	HPDF_Page_ShowText(page, inds[id].deat.date);	
    }

    else if (inds[id].deat.plac[0] != '\0')
    {
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowTextNextLine(page, "d  ");
	HPDF_Page_ShowText(page, inds[id].deat.plac);	
    }

    else if (inds[id].deat.flag)
    {
	HPDF_Page_MoveTextPos(page, 0, -fs);
	HPDF_Page_ShowTextNextLine(page, "d");
    }

    HPDF_Page_EndText(page);

    return GPDF_SUCCESS;
}

// Print the chart

int print_pdf()
{
    HPDF_Doc  pdf;
    HPDF_Page page;
    HPDF_Font font;
    HPDF_Font bold;
    HPDF_REAL tw;
    HPDF_REAL height;
    HPDF_REAL width;

    char filename[256];
    char title[256];

    strcpy(title, file);
    title[0] = toupper(title[0]);
    strcat(title, " Family Tree");

    strcpy(filename, file);
    strcat(filename, ".pdf");

    pdf = HPDF_New(error_handler, NULL);
    if (pdf == NULL)
    {
        printf("gpdf: cannot create PdfDoc object\n");
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
    HPDF_Page_Rectangle(page, width - 210, SIZE_MARG, 200, 22);
    HPDF_Page_Stroke(page);

    /* Print the title of the page (with positioning center). */
    font = HPDF_GetFont(pdf, "Helvetica", NULL);
    bold = HPDF_GetFont(pdf, "Helvetica-Bold", NULL);
    HPDF_Page_SetFontAndSize(page, font, 18);

    tw = HPDF_Page_TextWidth(page, title);
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, (width - 110) - tw / 2, 14, title);
    HPDF_Page_EndText(page);

    // Horizontal slots on the page

    int slots[gens + 1];
    for (int i = 0; i <= gens; i++)
	slots[i] = 0;

    int slot = (width - 40) / (gens + 1);
    for (int i = 1; i < SIZE_INDS; i++)
    {
	if (inds[i].id > 0)
	{
	    int x = (2 * SIZE_MARG) + (inds[i].posn.x * slot);
	    int y = (height - (2 * SIZE_MARG) - fs) -
		(fs * 6 * slots[inds[i].posn.x]++);

	    print_indi(page, font, bold, x, y, inds[i].id);
	}
    }

    // Save file

    HPDF_SaveToFile(pdf, "test.pdf");

    return GPDF_SUCCESS;
}

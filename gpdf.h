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

#ifndef GPDF_H
#define GPDF_H

// Use helvetica

#define FONT "Helvetica"
#define BOLD "Helvetica-Bold"

typedef enum
    {SIZE_INDS = 256,
     SIZE_LINE = 256,
     SIZE_FAMS = 128,
     SIZE_NAME = 64,
     SIZE_GIVN = 32,
     SIZE_SURN = 32,
     SIZE_PLAC = 32,
     SIZE_OCCU = 32,
     SIZE_XREF = 32,
     SIZE_NICK = 16,
     SIZE_DATE = 16,
     SIZE_CHLN = 16,
     SIZE_GENS = 16,
     SIZE_FMSS = 4,
     SIZE_SEX  = 4}
    gpdf_size_t;

typedef enum
    {SIZE_A0,
     SIZE_A1,
     SIZE_A2,
     SIZE_A3,
     SIZE_A4}
    gpdf_page_size_t;

typedef enum
    {SIZE_PAGE    = SIZE_A3,
     SIZE_FONT    = 8,
     SIZE_MARGIN  = 20,
     SIZE_INSET   = 10}
    gpdf_page_t;

typedef enum
    {GPDF_SUCCESS,
     GPDF_ERROR}
    gpdf_return_t;

typedef enum
    {TYPE_OBJECT,
     TYPE_ATTR,
     TYPE_ADDN}
    gpdf_type_t;

typedef enum
    {STATE_HEAD,
     STATE_INDI,
     STATE_FAML,
     STATE_NONE = -1}
    gpdf_state_t;

typedef enum
    {DATE_BIRT,
     DATE_DEAT,
     DATE_MARR,
     DATE_DIVC,
     DATE_NONE = -1}
    gpdf_date_t;

typedef enum
    {PLAC_BIRT,
     PLAC_DEAT,
     PLAC_MARR,
     PLAC_DIVC,
     PLAC_NONE = -1}
    gpdf_plac_t;

typedef struct
{
    bool yes;
    char date[SIZE_DATE];
    char plac[SIZE_PLAC];
} birt, deat, marr, divc;

typedef struct
{
    float x, y;
} coord;

typedef struct indi_s
{
    int id;
    int gens;
    int nchi;
    coord posn;
    char xref[SIZE_XREF];
    char name[SIZE_NAME];
    char givn[SIZE_GIVN];
    char surn[SIZE_GIVN];
    char marn[SIZE_NAME];
    char nick[SIZE_NICK];
    char occu[SIZE_OCCU];
    char sex[SIZE_SEX];
    birt birt;
    deat deat;
    struct fam_s *famc;
    struct fam_s *fams[SIZE_FMSS];
} indi;

typedef struct fam_s
{
    int id;
    indi *husb;
    indi *wife;
    char xref[SIZE_XREF];
    marr marr;
    divc divc;
    indi *chil[SIZE_CHLN];
} faml;

// Functions

int parse_gedcom_file(char *);
int find_generations();
int read_textfile();
int write_textfile();
int draw_pdf();
int object(char *, char *);
int attrib(char *, char *);
int additn(char *, char *);

#ifdef __MINGW32__
int getline(char **, size_t *, FILE *);
#endif

#endif

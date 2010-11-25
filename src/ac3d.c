/*

ORBIT, a freeware space combat simulator
Copyright (C) 1999  Steve Belczyk <steve1@genesis.nred.ma.us>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "orbit.h"

/*
 *  Stuff to load AC3D files
 *
 *  Most of this is based on ac-to-gl, by Steve Baker <sjbaker1@airmail.net>
 */

int DoMaterial (char *s);
int DoObject (char *s);
int DoACName (char *s);
int DoData (char *s);
int DoTexture (char *s);
int DoTexrep (char *s);
int DoRot (char *s);
int DoLoc (char *s);
int DoUrl (char *s);
int DoNumvert (char *s);
int DoNumsurf (char *s);
int DoSurf (char *s);
int DoMat (char *s);
int DoRefs (char *s);
int DoKids (char *s);

int DoObjWorld (char *s);
int DoObjPoly (char *s);
int DoObjGroup (char *s);
void SkipSpaces (char **s);
void SkipQuotes (char **s);
void RemoveQuotes (char *s);
void load_sgi_texture (char *s);

#define MAX_TEXTURES 1000    /* This *ought* to be enough! */
char *texture_fnames [MAX_TEXTURES];

int acsmooth;		/* Smooth surface? */
int acsurfpass;		/* Pass through surface */
#define PASS_NORMAL (0)	/* Get normals */
#define PASS_RENDER (1) /* Render */

struct
{
	int id;			/* Texture id */
	char name[64];	/* Name */
} tex[MAX_TEXTURES];
int ntex;

struct ACVertex
{
	float v[3];     /* Vertex coords */
	float normal[3]; /* Normal */
	float n;        /* Number of normals to average */
} *vtab;

int num_materials = 0;
int num_textures = 0;
int nv;			/* Number of vertices in vtab */

int last_flags = -1;
int last_mat;
int last_num_kids = -1;
int current_flags = -1;
int texture_enabled = 0;
int need_texture = 0;
int firsttime2;

#define PARSE_CONT   0
#define PARSE_POP    1

int matlist[1024];
int modelnum;
FILE *acfd;

struct Tag
{
	char *token;
	int (*func) (char *s);
};

struct Tag top_tags [] =
{
	{"MATERIAL", DoMaterial},
	{"OBJECT"  , DoObject  },
};

InitTextures()
{
	int t;

	for (t=0; t<MAX_TEXTURES; t++)
	{
		tex[t].id = (-1);
		tex[t].name[0] = 0;
	}

	ntex = 0;
}

LoadAC3D (name)
char *name;
{
	char buffer[1024], *s, fn[128];
	int firsttime, m;

	/* See if it's already loaded */
	for (m=0; m<NMODELS; m++)
	{
		if (!strcmp (model[m].name, name)) return (m);
	}

	Log ("LoadAC3D: Loading model %s", name);

	/* Find an unused model index */
	m = modelnum = FindModel();
	strcpy (model[m].name, name);
	model[m].in_use = 1;

	/* Init bounding box */
	model[m].lobound[0] = model[m].lobound[1] = model[m].lobound[2] = 0.0;
	model[m].hibound[0] = model[m].hibound[1] = model[m].hibound[2] = 0.0;

	/* Get a list id */
	model[m].list = glGenLists (1);

	/* Construct file name */
	sprintf (fn, "models/%s", name);

	/* Open it */
	if (NULL == (acfd = fopen (fn, "rt")))
	{
		Log ("LoadAC3D: Can't open %s, giving up!", fn);
		return (-1);
	}

	firsttime = 1;
	firsttime2 = 1;

	num_materials = 0;
	num_textures = 0;
	vtab = NULL;
	last_flags = -1 ;
	last_num_kids = -1 ;
	current_flags = -1 ;
	texture_enabled = 0;
	need_texture = 0;
	last_mat = -1;

	/* Look for and define textures */
	ScanForTextures();

	while (fgets (buffer, 1024, acfd) != NULL)
	{
		s = buffer ;

		/* Skip leading whitespace */
		SkipSpaces (&s);

		/* Skip blank lines and comments */
		if ( *s < ' ' && *s != '\t' ) continue;
		if ( *s == '#' || *s == ';' ) continue;

		if (firsttime)
		{
			firsttime = 0;

			if (strncmp (s, "AC3D", 4) != 0)
			{
				Log ("LoadAC3d: %s is not an AC3D format file", fn);
				return (-1);
			}
		}
		else
		{
			Search (top_tags, s);
		}
	}

	glShadeModel (GL_FLAT);
	glEndList();
	fclose (acfd);

	Log ("LoadAC3D: Lo bounds: %lf %lf %lf", model[m].lobound[0],
			model[m].lobound[1], model[m].lobound[2]);
	Log ("LoadAC3D: Hi bounds: %lf %lf %lf", model[m].hibound[0],
			model[m].hibound[1], model[m].hibound[2]);

	/* All is well */
	return m;
}

ScanForTextures()
/*
 *  Look through the AC3D file for all "texture" lines
 */
{
	char buf[1024], fn[256];

	/* Look for "texture" lines */
	while (NULL != fgets (buf, 1024, acfd))
	{
		if (1 == sscanf (buf, "texture %s", fn))
		{
			RemoveQuotes (fn);
			LoadTexture (fn);
		}
	}

	/* Rewind the file */
	fseek (acfd, 0, SEEK_SET);
}

LoadTexture (s)
char *s;
/*
 *  Load a texture file
 */
{
	char fn[256];

	Log ("LoadTexture: Loading texture %s", s);

	/* Construct file name */
	sprintf (fn, "models/%s", s);

	/* Get a texture id */
	glGenTextures (1, &tex[ntex].id);
	strcpy (tex[ntex].name, s);

	/* Define the texture */
	glBindTexture (GL_TEXTURE_2D, tex[ntex].id);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	load_sgi_texture (fn);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	ntex++;
}

void ACNormalize (float *v)
{
	float mag;

	mag = sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	if (mag == 0.0f)
	{
		v[0] = v[1] = 0.0f;
		v[2] = 1.0f;
		return;
	}

	v[0] /= mag;
	v[1] /= mag;
	v[2] /= mag;
}

void CrossProduct (float *dst, float *a, float *b)
{
	dst[0] = a[1] * b[2] - a[2] * b[1];
	dst[1] = a[2] * b[0] - a[0] * b[2];
	dst[2] = a[0] * b[1] - a[1] * b[0];
}

void MakeNormal (float *dst, float *a, float *b, float *c)
{
	float ab[3];
	float ac[3];

	ab[0] = b[0] - a[0];
	ab[1] = b[1] - a[1];
	ab[2] = b[2] - a[2];
	ACNormalize (ab);

	ac[0] = c[0] - a[0];
	ac[1] = c[1] - a[1];
	ac[2] = c[2] - a[2];
	ACNormalize (ac);

	CrossProduct (dst, ab, ac);
	ACNormalize (dst);
}
 
void SkipSpaces (char **s)
{
	while (**s == ' ' || **s == '\t') (*s)++;
}

void SkipQuotes (char **s)
{
	char *t;

	SkipSpaces (s);

	if (**s == '\"')
	{
		(*s)++;
		t = *s;
		while (*t != '\0' && *t != '\"') t++;

		if ( *t != '\"' )
			Log ("SkipQuotes: Mismatched double-quote in '%s'\n", *s);

		*t = '\0';
	}
	else
	{
		Log ("SkipQuuotes: Expected double-quote in '%s'", *s);
	}
}

void RemoveQuotes (char *s)
/*
 *  Remove quotes from string
 */
{
	char *t;
	int i, j, len;

	len = strlen (s);
	t = (char *) malloc (len+1);
	strcpy (t, s);
	j = 0;

	for (i=0; i<len; i++)
	{
		if (t[i] != '"')
		{
			s[j++] = t[i];
		}
	}
	s[j] = 0;

	free (t);
}

int Search (struct Tag *tags, char *s)
{
	int i;

	SkipSpaces (&s);

	for (i=0; tags[i].token != NULL; i++)
	{
		if (!strncasecmp (tags[i].token, s, strlen (tags[i].token)))
		{
			s += strlen (tags[i].token);
			SkipSpaces (&s);
			return (*(tags[i].func))(s);
		}
	}

	Log ("Search: Unrecognised token '%s'", s);
	exit (1);
	return 0;
}

struct Tag object_tags[] =
{
	{"name"   , DoACName},
	{"data"   , DoData},
	{"texture", DoTexture},
	{"texrep" , DoTexrep},
	{"rot"    , DoRot},
	{"loc"    , DoLoc},
	{"url"    , DoUrl},
	{"numvert", DoNumvert},
	{"numsurf", DoNumsurf},
	{"kids"   , DoKids},
	{NULL, NULL }
};

struct Tag surf_tag[] =
{
	{"SURF", DoSurf},
	{NULL, NULL}
};

struct Tag surface_tags[] =
{
	{"mat" , DoMat },
	{"refs", DoRefs},
	{NULL, NULL}
};

struct Tag obj_type_tags[] =
{
	{"world", DoObjWorld},
	{"poly" , DoObjPoly },
	{"group", DoObjGroup},
	{NULL, NULL}
};

#define OBJ_WORLD  0
#define OBJ_POLY   1
#define OBJ_GROUP  2

int DoObjWorld (char *s)
{
	return OBJ_WORLD;
} 

int DoObjPoly (char *s)
{
	return OBJ_POLY;
}

int DoObjGroup (char *s)
{
	return OBJ_GROUP;
}

void DoBegin (int flags)
{
	if (last_flags == -1) last_flags = ~flags;

	if (((flags>>4) & 0x01) != ((last_flags>>4) & 0x01))
	{
		if ((flags>>4) & 0x01)
		{
			/* Smooth Shaded */
			acsmooth = 1;
		}
		else
		{
			/* Flat Shaded */
			acsmooth = 0;
		}
	}
/**
	if (((flags>>4) & 0x02) != ((last_flags>>4) & 0x02))
	{
		if ((flags>>4) & 0x02)
		{
			if (acsurfpass == PASS_RENDER) glDisable (GL_CULL_FACE);
		}
		else
		{
			if (acsurfpass == PASS_RENDER) glEnable (GL_CULL_FACE);
		}
	}

	if ((last_flags & 0x0F) == 0  && (flags & 0x0F) != 0)
	{
		if (acsurfpass == PASS_RENDER) glDisable (GL_LIGHTING);
	}
	else if ((last_flags & 0x0F) != 0  && (flags & 0x0F) == 0)
	{
		if (acsurfpass == PASS_RENDER) glEnable (GL_LIGHTING);
	}
**/
	last_flags = flags;

	switch (flags & 0x0F)
	{
	case 0:	if (acsurfpass == PASS_RENDER) glBegin (GL_POLYGON);
		break;

	case 1:	if (acsurfpass == PASS_RENDER) glBegin (GL_LINE_LOOP);
		break;

	case 2:	if (acsurfpass == PASS_RENDER) glBegin (GL_LINE_STRIP);
		break;

	default:
		Log ("DoBegin: Illegal surface type 0x%02x", flags);
		break;
	}
}

int DoMaterial (char *s)
{
	char name[1024], *nm;
	float rgb[3], amb[3], emis[3], spec[3], trans;
	int shi;

	if (15 != sscanf (s,
		"%s rgb %f %f %f amb %f %f %f emis %f %f %f spec %f %f %f shi %d trans %f",
		name, &rgb[0], &rgb[1], &rgb[2], &amb[0], &amb[1], &amb[2],
		&emis[0], &emis[1], &emis[2], &spec[0], &spec[1], &spec[2], &shi, &trans))
	{
		Log ("DoMaterial: Can't parse this MATERIAL: %s", s);
	}
	else
	{
		nm = name;
		SkipQuotes (&nm);

		/* Get a list for this material */
		matlist[num_materials] = glGenLists (1);

		glNewList (matlist[num_materials], GL_COMPILE );
		glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, amb);
		glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, rgb);
		glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, spec);
		glMaterialfv (GL_FRONT_AND_BACK, GL_EMISSION, emis);
		glMaterialf  (GL_FRONT_AND_BACK, GL_SHININESS, shi);
		glEndList();
	}
	num_materials++;

	return PARSE_CONT;
}

int DoObject (char *s)
{
	int i, obj_type, num_kids;
	char buffer[1024];

	if (firsttime2)
	{
		glNewList (model[modelnum].list, GL_COMPILE);
		glShadeModel (GL_SMOOTH);
		firsttime2 = 0;
	}

	obj_type = Search (obj_type_tags, s);  

	switch (obj_type)
	{
	case OBJ_WORLD:
		break;

	case OBJ_POLY:
		break;

	case OBJ_GROUP:
		break;
	}

	need_texture = 0;

	glPushMatrix();

	while (NULL != fgets (buffer, 1024, acfd))
	{
		if (Search (object_tags, buffer) == PARSE_POP) break;
	}

	num_kids = last_num_kids;

	for (i=0; i<num_kids; i++)
	{
		fgets (buffer, 1024, acfd);
		Search (top_tags, buffer);
	}

	glPopMatrix();

	return PARSE_CONT;
}

int DoACName (char *s)
{
	SkipQuotes (&s);
	return PARSE_CONT;
}

int DoData (char *s)
{
	int i, len;

	len = strtol (s, NULL, 0);
	Log ("DoData: WARNING - data string encountered");
	for (i=0; i<len; i++) fgetc (acfd);

	return PARSE_CONT;
}

int GetTexture (char *s)
{
	int t;

	for (t=0; t<ntex; t++)
	{
		if (!strcasecmp (s, tex[t].name)) return t;
	}

	return 0;
}

int DoTexture (char *s)
{
	static int last_tex = -1;
	int t;

	SkipQuotes (&s);

	t = GetTexture (s);

	if (t != last_tex)
	{
		glBindTexture (GL_TEXTURE_2D, tex[t].id);
 
		last_tex = t;
	}

	need_texture = 1;

	return PARSE_CONT;
}

int DoTexrep (char *s)
{
	float texrep[2];

	if (2 != sscanf (s, "%f %f", &texrep[0], &texrep[1]))
		Log ("DoTextrep: Illegal texrep record: %s", s);

	return PARSE_CONT;
}

int DoRot (char *s)
{
	float mat[4][4];

	mat[0][3] = mat[1][3] = mat[2][3] = mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;
	mat[3][3] = 1.0f; 

	if (9 != sscanf (s, "%f %f %f %f %f %f %f %f %f",
		&mat[0][0], &mat[0][1], &mat[0][2],
		&mat[1][0], &mat[1][1], &mat[1][2],
		&mat[2][0], &mat[2][1], &mat[2][2]))
	{
		Log ("DoRot: Illegal rot record: %s", s);
	}

	glMultMatrixf (mat);

	return PARSE_CONT ;
}

int DoLoc (char *s)
{
	float loc[3];

	if (3 != sscanf (s, "%f %f %f", &loc[0], &loc[1], &loc[2]))
	{
		Log ("DoLoc: Illegal loc record: %s", s);
	}

	loc[0] /= 100.0;
	loc[1] /= 100.0;
	loc[2] /= 100.0;
	glTranslatef (loc[0], loc[1], loc[2]);

	return PARSE_CONT;
}

int DoUrl (char *s)
{
	SkipQuotes (&s);
	return PARSE_CONT;
}

int DoNumvert (char *s)
{
	char buffer[1024];
	int i;

	nv = strtol (s, NULL, 0);
 
	if (vtab != NULL) free (vtab) ;

	vtab = (struct ACVertex *) malloc (3*nv*sizeof(struct ACVertex));

	for (i=0; i<nv; i++)
	{
		fgets (buffer, 1024, acfd);

		if (3 != sscanf (buffer, "%f %f %f", &vtab[i].v[0], &vtab[i].v[1], &vtab[i].v[2]))
		{
			Log ("DoNumvert: Illegal vertex record: %s", buffer);
		}
		vtab[i].v[0] /= 100.0;
		vtab[i].v[1] /= 100.0;
		vtab[i].v[2] /= 100.0;

		vtab[i].normal[0] = vtab[i].normal[1] = vtab[i].normal[2] = 0.0;
		vtab[i].n = 0.0;

		if (vtab[i].v[0] < model[modelnum].lobound[0]) model[modelnum].lobound[0] = vtab[i].v[0];
		if (vtab[i].v[1] < model[modelnum].lobound[1]) model[modelnum].lobound[1] = vtab[i].v[1];
		if (vtab[i].v[2] < model[modelnum].lobound[2]) model[modelnum].lobound[2] = vtab[i].v[2];
		if (vtab[i].v[0] > model[modelnum].hibound[0]) model[modelnum].hibound[0] = vtab[i].v[0];
		if (vtab[i].v[1] > model[modelnum].hibound[1]) model[modelnum].hibound[1] = vtab[i].v[1];
		if (vtab[i].v[2] > model[modelnum].hibound[2]) model[modelnum].hibound[2] = vtab[i].v[2];
	}

	return PARSE_CONT;
}

int DoNumsurf (char *s)
{
	int i, ns, v;
	long pos, p;
	char buffer[1024];

	ns = strtol (s, NULL, 0);

	/* Do first pass to average normals */
	pos = ftell (acfd);
	acsurfpass = PASS_NORMAL;
	for (i=0; i<ns; i++)
	{
		p = ftell (acfd);
		fgets (buffer, 1024, acfd);
		Search (surf_tag, buffer);
	}

	/* Back to beginning of object */
	fseek (acfd, pos, SEEK_SET);

	/* Average normals */
	for (v=0; v<nv; v++)
	{
		vtab[v].normal[0] /= vtab[v].n;
		vtab[v].normal[1] /= vtab[v].n;
		vtab[v].normal[2] /= vtab[v].n;
	}

	/* Now render */
	acsurfpass = PASS_RENDER;
	last_mat = (-1);

	for (i=0; i<ns; i++)
	{
		p = ftell (acfd);
		fgets (buffer, 1024, acfd);
		Search (surf_tag, buffer);
	}

	return PARSE_CONT;
}

int DoSurf (char *s)
{
	char buffer[1024];

	current_flags = strtol (s, NULL, 0);

	while (NULL != fgets (buffer, 1024, acfd))
	{
		if (Search (surface_tags, buffer) == PARSE_POP) break;
	}

	return PARSE_CONT ;
}

int DoMat (char *s)
{
	int mat;

	mat = strtol (s, NULL, 0);

	if (mat != last_mat)
	{
		if (acsurfpass == PASS_RENDER) glCallList (matlist[mat]);
		last_mat = mat;
	}

	return PARSE_CONT ;
}

int DoRefs (char *s)
{
	int i;
	int nrefs;
	char buffer[1024] ;
	int *vlist;
	float *tlistu, *tlistv, nrm[3];

	nrefs = strtol (s, NULL, 0);

	if (nrefs == 0) return PARSE_POP;

	if (need_texture && !texture_enabled)
	{
		if (acsurfpass == PASS_RENDER) glEnable (GL_TEXTURE_2D);
		texture_enabled = 1;
	}
	else if (!need_texture && texture_enabled)
	{
		if (acsurfpass == PASS_RENDER) glDisable (GL_TEXTURE_2D);
		texture_enabled = 0;
	}

	vlist  = (int *) malloc (sizeof(int)*nrefs);
	tlistu = (float *) malloc (sizeof(float)*nrefs) ;
	tlistv = (float *) malloc (sizeof(float)*nrefs) ;
 
	DoBegin (current_flags);

	for (i=0; i<nrefs; i++)
	{
		fgets (buffer, 1024, acfd);

		if (3 != sscanf (buffer, "%d %f %f", &vlist[i], &tlistu[i], &tlistv[i]))
		{
			Log ("DoRefs: Illegal ref record: %s", buffer);
			exit (1);
		}
	}

	if (nrefs >= 3)
	{
		MakeNormal (nrm, vtab[vlist[0]].v, vtab[vlist[1]].v, vtab[vlist[2]].v);

		if (!acsmooth && (acsurfpass == PASS_RENDER))
		{
			glNormal3fv (nrm);
		}
	}

	for (i=0; i<nrefs; i++)
	{
		/* Maintain normal first pass */
		if (acsmooth && (acsurfpass == PASS_NORMAL))
		{
			MaintainNormal (vlist[i], nrm);
		}

		/* Render second pass */
		if (acsmooth && (acsurfpass == PASS_RENDER))
		{
			glNormal3fv (vtab[vlist[i]].normal);
		}

		if (acsurfpass == PASS_RENDER)
		{
			glTexCoord2f (tlistu[i], tlistv[i]);
			glVertex3fv (vtab[vlist[i]].v);
		}
	}

	free (vlist);
	free (tlistu);
	free (tlistv);

	if (acsurfpass == PASS_RENDER) glEnd();

	return PARSE_POP;
}

int DoKids (char *s)
{
	last_num_kids = strtol (s, NULL, 0);
	return PARSE_POP;
}

MaintainNormal (i, n)
int i;
float n[3];
/*
 *  Maintain a vertex normal
 */
{
	vtab[i].n += 1.0;

	vtab[i].normal[0] += n[0];
	vtab[i].normal[1] += n[1];
	vtab[i].normal[2] += n[2];
}

/** ------------- The rest loads SGI texture files --------------- **/

/* Some magic constants in the file header. */

#define SGI_IMG_MAGIC           0x01DA
#define SGI_IMG_SWABBED_MAGIC   0xDA01   /* This is how it appears on a PC */
#define SGI_IMG_VERBATIM        0
#define SGI_IMG_RLE             1

static int            sgi_max ;
static int            sgi_min ;
static int            sgi_colormap ;
static unsigned short sgi_magic ;
static char           sgi_type ;
static char           sgi_bpp ;
static unsigned int  *sgi_start ;
static int           *sgi_leng ;
static unsigned char *sgi_temp ;
static int            sgi_swapped ;
static FILE          *sgi_fd ;
static unsigned short sgi_dim ;
static unsigned short sgi_xsize ;
static unsigned short sgi_ysize ;
static unsigned short sgi_zsize ;

static void makeConsistentHeader ()
{
  /*
    Sanity checks - and a workaround for buggy RGB files generated by
    the MultiGen Paint program because it will sometimes get confused
    about the way to represent maps with more than one component.

    eg   Y > 1, Number of dimensions == 1
         Z > 1, Number of dimensions == 2
  */

  if ( sgi_ysize > 1 && sgi_dim < 2 ) sgi_dim = 2 ;
  if ( sgi_zsize > 1 && sgi_dim < 3 ) sgi_dim = 3 ;
  if ( sgi_dim < 1 ) sgi_ysize = 1 ;
  if ( sgi_dim < 2 ) sgi_zsize = 1 ;
  if ( sgi_dim > 3 ) sgi_dim   = 3 ;
  if ( sgi_zsize < 1 && sgi_ysize == 1 ) sgi_dim = 1 ;
  if ( sgi_zsize < 1 && sgi_ysize != 1 ) sgi_dim = 2 ;
  if ( sgi_zsize >= 1 ) sgi_dim = 3 ;

  /*
    A very few SGI image files have 2 bytes per component - this
    tool cannot deal with those kinds of files. 
  */

  if ( sgi_bpp == 2 )
  {
    Log ("makeConsistentHeader: Can't work with SGI images with %d bpp", sgi_bpp);
    exit (1);
  }

  sgi_bpp = 1 ;
  sgi_min = 0 ;
  sgi_max = 255 ;
  sgi_magic = SGI_IMG_MAGIC ;
  sgi_colormap = 0 ;
}

static void swab_short ( unsigned short *x )
{
  if ( sgi_swapped )
    *x = (( *x >>  8 ) & 0x00FF ) | 
         (( *x <<  8 ) & 0xFF00 ) ;
}

static void swab_int ( unsigned int *x )
{
  if ( sgi_swapped )
    *x = (( *x >> 24 ) & 0x000000FF ) | 
         (( *x >>  8 ) & 0x0000FF00 ) | 
         (( *x <<  8 ) & 0x00FF0000 ) | 
         (( *x << 24 ) & 0xFF000000 ) ;
}

static void swab_int_array ( int *x, int leng )
{
  int i;

  if ( ! sgi_swapped )
    return ;

  for ( i = 0 ; i < leng ; i++ )
  {
    *x = (( *x >> 24 ) & 0x000000FF ) | 
         (( *x >>  8 ) & 0x0000FF00 ) | 
         (( *x <<  8 ) & 0x00FF0000 ) | 
         (( *x << 24 ) & 0xFF000000 ) ;
    x++ ;
  }
}


static unsigned char readByte ()
{
  unsigned char x ;
  fread ( & x, sizeof(unsigned char), 1, sgi_fd ) ;
  return x ;
}

static unsigned short readShort ()
{
  unsigned short x ;
  fread ( & x, sizeof(unsigned short), 1, sgi_fd ) ;
  swab_short ( & x ) ;
  return x ;
}

static unsigned int readInt ()
{
  unsigned int x ;
  fread ( & x, sizeof(unsigned int), 1, sgi_fd ) ;
  swab_int ( & x ) ;
  return x ;
}


static void getRow ( unsigned char *buf, int y, int z )
{
    unsigned char pixel, count ;

  if ( y >= sgi_ysize ) y = sgi_ysize - 1 ;
  if ( z >= sgi_zsize ) z = sgi_zsize - 1 ;

  fseek ( sgi_fd, sgi_start [ z * sgi_ysize + y ], SEEK_SET ) ;

  if ( sgi_type == SGI_IMG_RLE )
  {
    unsigned char *tmpp = sgi_temp ;
    unsigned char *bufp = buf ;

    fread ( sgi_temp, 1, sgi_leng [ z * sgi_ysize + y ], sgi_fd ) ;

    while ( 1 )
    {
      pixel = *tmpp++ ;

      count = ( pixel & 0x7f ) ;

      if ( count == 0 )
	break ;

      if ( pixel & 0x80 )
      {
        while ( count-- )
	  *bufp++ = *tmpp++ ;
      }
      else
      {
        pixel = *tmpp++ ;

	while ( count-- )
          *bufp++ = pixel ;
      }
    }
  }
  else
    fread ( buf, 1, sgi_xsize, sgi_fd ) ;
}


static void getPlane ( unsigned char *buf, int z )
{
  int y;
  if ( sgi_fd == NULL )
    return ;

  if ( z >= sgi_zsize ) z = sgi_zsize - 1 ;

  for ( y = 0 ; y < sgi_ysize ; y++ )
    getRow ( & buf [ y * sgi_xsize ], y, z ) ;
}



static void getImage ( unsigned char *buf )
{
  int y, z;

  if ( sgi_fd == NULL )
    return ;

  for (y = 0 ; y < sgi_ysize ; y++ )
    for (z = 0 ; z < sgi_zsize ; z++ )
      getRow ( & buf [ ( z * sgi_ysize + y ) * sgi_xsize ], y, z ) ;
}

void load_sgi_texture ( char *fname )
{
  int i, tablen, x, y, l, maxlen;
  GLubyte *texels [ 20 ], *ptr ;
  unsigned char *rbuf, *gbuf, *bbuf, *abuf;
  int lev, map_level;
  int j, x2, y2, c;

  sgi_dim   = 0 ;
  sgi_start = NULL ;
  sgi_leng  = NULL ;
  sgi_temp  = NULL ;

  sgi_swapped = 0 ;

  sgi_fd = fopen ( fname, "rb" ) ;

  if ( sgi_fd == NULL )
  {
    Log ("load_sgi_texture: Failed to open '%s' for reading.", fname ) ;
    return ;
  }

  sgi_magic = readShort () ;

  if ( sgi_magic != SGI_IMG_MAGIC && sgi_magic != SGI_IMG_SWABBED_MAGIC )
  {
    Log ("load_sgi_texture: %s: Unrecognised magic number 0x%04x", fname, sgi_magic ) ;
    exit ( 1 ) ;
  }

  if ( sgi_magic == SGI_IMG_SWABBED_MAGIC )
  {
    sgi_swapped = 1 ;
    swab_short ( & sgi_magic ) ;
  }

  sgi_type  = readByte  () ;
  sgi_bpp   = readByte  () ;
  sgi_dim   = readShort () ;

  /*
    This is a backstop test - if for some reason the magic number isn't swabbed, this
    test will still catch a swabbed file. Of course images with more than 256 dimensions
    are not catered for :-)
  */

  if ( sgi_dim > 255 )
  {
    Log ("load_sgi_texture: %s: Bad swabbing?!?", fname ) ;
    sgi_swapped = ! sgi_swapped ;
    swab_short ( & sgi_dim ) ;
    sgi_magic = SGI_IMG_MAGIC ;
  }

  sgi_xsize = readShort () ;
  sgi_ysize = readShort () ;
  sgi_zsize = readShort () ;
  sgi_min   = readInt   () ;  
  sgi_max   = readInt   () ;  
              readInt   () ;  /* Dummy field */

  for (i = 0 ; i < 80 ; i++ )
    readByte () ;         /* Name field */

  sgi_colormap = readInt () ;

  for (i = 0 ; i < 404 ; i++ )
    readByte () ;         /* Dummy field */

  makeConsistentHeader () ;

  tablen = sgi_ysize * sgi_zsize ;
  sgi_start = (unsigned int *) malloc (tablen*sizeof(unsigned int)) ;
  sgi_leng  = (int *) malloc (sizeof(int)*tablen) ;

  if ( sgi_type == SGI_IMG_RLE )
  {
    fread ( sgi_start, sizeof ( unsigned int ), tablen, sgi_fd ) ;
    fread ( sgi_leng , sizeof (  int ), tablen, sgi_fd ) ;
    swab_int_array ( (int *) sgi_start, tablen ) ;
    swab_int_array ( (int *) sgi_leng , tablen ) ;

    maxlen = 0 ;

    for (i = 0 ; i < tablen ; i++ )
      if ( sgi_leng [ i ] > maxlen )
        maxlen = sgi_leng [ i ] ;

    sgi_temp = (unsigned char *) malloc (maxlen) ;
  }
  else
  {
    sgi_temp = NULL ;

    for (i = 0 ; i < sgi_zsize ; i++ )
      for (j = 0 ; j < sgi_ysize ; j++ )
      {
        sgi_start [ i * sgi_ysize + j ] = sgi_xsize * ( i * sgi_ysize + j ) + 512 ;
        sgi_leng  [ i * sgi_ysize + j ] = sgi_xsize ;
      }
  }

  if ( sgi_zsize <= 0 || sgi_zsize > 4 )
  {
    Log ("load_sgi_texture: %s: Not an int/inta/rgb/rgba image?!?", fname ) ;
    exit ( 1 ) ;
  }


  for (i = 0 ; i < 20 ; i++ )
    texels [ 0 ] = NULL ;

  texels [ 0 ] = (GLubyte *) malloc (sizeof(GLubyte)* sgi_xsize * sgi_ysize * sgi_zsize ) ;

  ptr = texels [ 0 ] ;

  rbuf = malloc (sgi_xsize);
  gbuf = (sgi_zsize>1) ? malloc(sgi_xsize): (unsigned char *) NULL ;
  bbuf = (sgi_zsize>2) ? malloc(sgi_xsize): (unsigned char *) NULL ;
  abuf = (sgi_zsize>3) ? malloc(sgi_xsize): (unsigned char *) NULL ;

  for ( y = 0 ; y < sgi_ysize ; y++ )
  {
    switch ( sgi_zsize )
    {
      case 1 :
	getRow ( rbuf, y, 0 ) ;

	for ( x = 0 ; x < sgi_xsize ; x++ )
	  *ptr++ = rbuf [ x ] ;

	break ;

      case 2 :
	getRow ( rbuf, y, 0 ) ;
	getRow ( gbuf, y, 1 ) ;

	for (x = 0 ; x < sgi_xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	}
	break ;

      case 3 :
        getRow ( rbuf, y, 0 ) ;
	getRow ( gbuf, y, 1 ) ;
	getRow ( bbuf, y, 2 ) ;

	for (x = 0 ; x < sgi_xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	  *ptr++ = bbuf [ x ] ;
	}
	break ;

      case 4 :
        getRow ( rbuf, y, 0 ) ;
	getRow ( gbuf, y, 1 ) ;
	getRow ( bbuf, y, 2 ) ;
	getRow ( abuf, y, 3 ) ;

	for (x = 0 ; x < sgi_xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	  *ptr++ = bbuf [ x ] ;
	  *ptr++ = abuf [ x ] ;
	}
	break ;
    }
  }

  for ( lev = 0 ; (( sgi_xsize >> (lev+1) ) != 0 ||
                   ( sgi_ysize >> (lev+1) ) != 0 ) ; lev++ )
  {
    /* Suffix '1' is the higher level map, suffix '2' is the lower level. */

    int l1 =  lev  ;
    int l2 = lev+1 ;
    int w1 = sgi_xsize >> l1 ;
    int h1 = sgi_ysize >> l1 ;
    int w2 = sgi_xsize >> l2 ;
    int h2 = sgi_ysize >> l2 ;

    if ( w1 <= 0 ) w1 = 1 ;
    if ( h1 <= 0 ) h1 = 1 ;
    if ( w2 <= 0 ) w2 = 1 ;
    if ( h2 <= 0 ) h2 = 1 ;

    texels[l2] = (GLubyte *) malloc (sizeof(GLubyte)*  w2 * h2 * sgi_zsize ) ;

    for (x2 = 0 ; x2 < w2 ; x2++ )
      for (y2 = 0 ; y2 < h2 ; y2++ )
        for (c = 0 ; c < sgi_zsize ; c++ )
        {
          int x1   = x2 + x2 ;
          int x1_1 = ( x1 + 1 ) % w1 ;
          int y1   = y2 + y2 ;
          int y1_1 = ( y1 + 1 ) % h1 ;

	  int t1 = texels [ l1 ] [ (y1   * w1 + x1  ) * sgi_zsize + c ] ;
	  int t2 = texels [ l1 ] [ (y1_1 * w1 + x1  ) * sgi_zsize + c ] ;
	  int t3 = texels [ l1 ] [ (y1   * w1 + x1_1) * sgi_zsize + c ] ;
	  int t4 = texels [ l1 ] [ (y1_1 * w1 + x1_1) * sgi_zsize + c ] ;

          texels [ l2 ] [ (y2 * w2 + x2) * sgi_zsize + c ] =
                                           ( t1 + t2 + t3 + t4 ) / 4 ;
        }
  }

  texels [ lev+1 ] = NULL ;

  if (rbuf != NULL) free (rbuf)  ;
  if (gbuf != NULL) free (gbuf)  ;
  if (bbuf != NULL) free (bbuf)  ;
  if (abuf != NULL) free (abuf)  ;

  if ( ! ((sgi_xsize & (sgi_xsize-1))==0) ||
       ! ((sgi_ysize & (sgi_ysize-1))==0) )
  {
    Log ("load_sgi_texture: %s: Map is not a power-of-two in size!", fname ) ;
    exit ( 1 ) ;
  }

  glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 ) ;

  map_level = 0 ;

  if ( sgi_xsize > 256 || sgi_ysize > 256 )
  {
    while ( sgi_xsize > 256 || sgi_ysize > 256 )
    {
      if (texels[0] != NULL) free (texels[0]) ;
      sgi_xsize >>= 1 ;
      sgi_ysize >>= 1 ;

      for (l = 0 ; texels [ l ] != NULL ; l++ )
	texels [ l ] = texels [ l+1 ] ;
    }
  }

  for (i = 0 ; texels [ i ] != NULL ; i++ )
  {
    int w = sgi_xsize>>i ;
    int h = sgi_ysize>>i ;

    if ( w <= 0 ) w = 1 ;
    if ( h <= 0 ) h = 1 ;

    glTexImage2D  ( GL_TEXTURE_2D,
                     map_level, sgi_zsize, w, h, 0 /* Border */,
                            (sgi_zsize==1)?GL_LUMINANCE:
                            (sgi_zsize==2)?GL_LUMINANCE_ALPHA:
                            (sgi_zsize==3)?GL_RGB:
                                           GL_RGBA,
                            GL_UNSIGNED_BYTE, (GLvoid *) texels[i] ) ;
    map_level++ ;
  }

  fclose (sgi_fd);
}


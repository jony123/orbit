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
 *  Routines to manipulate object models.
 *  Models are stored as "triangle" files in the "models" directory.
 */

InitModels()
/*
 *  Set up models structures
 */
{
	int m;

	for (m=0; m<NMODELS; m++)
	{
		model[m].in_use = 0;
		model[m].name[0] = 0;
		model[m].list = (-1);
	}
}

ResetModels()
/*
 *  Get rid of all models
 */
{
	int m;

	for (m=0; m<NMODELS; m++)
	{
		model[m].in_use = 0;
		model[m].name[0] = 0;
		if (glIsList (model[m].list)) glDeleteLists (model[m].list, 1);
		model[m].list = (-1);
	}
}

int FindModel ()
/*
 *  Find an unused model
 */
{
	int m;

	for (m=0; m<NMODELS; m++)
	{
		if (!model[m].in_use) return (m);
	}

	Log ("FindModel: Out of models!  Increase NMODELS in orbit.h!");
	FinishSound();
	CloseLog();
	exit (0);
}

struct tri_struct
{
	int v1, v2, v3;
	int hex;
	int done;
	double norm[3];
} *tri;

struct vert_struct
{
	double v[3];
} *vert;

int ntris;
int nverts;
int verts;
int parity;
int orphans;
int length;
int stripped;
int lasthex;

int LoadModel (name)
char *name;
/*
 *  Load a triangle file, make display list, return list ID
 */
{
	char fn[32];
	FILE *fd;
	int hex, m, len;
	double v1[3], v2[3], v3[3], n1[3], n2[3], norm[3];

	/* Hand off if it's an AC3D model */
	len = strlen (name);
	if ( (len > 3) && (!strcasecmp (&name[len-2], "ac")) )
	{
		return (LoadAC3D (name));
	}

	/* See if it's already loaded */
	for (m=0; m<NMODELS; m++)
	{
		if (!strcmp (model[m].name, name)) return (m);
	}

	Log ("LoadModel: Loading model %s", name);

	/* Allocate space for triangles and vertices */
	if (NULL == (tri = (struct tri_struct *)
		malloc (4096 * sizeof (struct tri_struct))))
	{
		OutOfMemory();
	}
	if (NULL == (vert = (struct vert_struct *)
		malloc (4096 * sizeof (struct vert_struct))))
	{
		OutOfMemory();
	}

	/* Find an unused model index */
	m = FindModel();
	strcpy (model[m].name, name);
	model[m].in_use = 1;

	/* Get a list id */
	model[m].list = glGenLists (1);
	glNewList (model[m].list, GL_COMPILE);

	/* Construct file name */
	sprintf (fn, "models/%s", name);

	/* Open it */
	if (NULL == (fd = fopen (fn, "r")))
	{
		Log ("LoadModel: Can't open %s, giving up!", fn);
		return (-1);
	}

	ntris = 0;
	verts = 0;
	nverts = 0;
	orphans = 0;
	lasthex = -1;

	/* Read them triangles */
	while (10 == fscanf (fd, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %i",
		&v1[0], &v1[1], &v1[2], &v2[0], &v2[1], &v2[2],
		&v3[0], &v3[1], &v3[2], &hex))
	{
		Vdiv (v1, v1, 100.0);
		Vdiv (v2, v2, 100.0);
		Vdiv (v3, v3, 100.0);

		/* Compute normal */
		Vsub (n1, v3, v2);
		Normalize (n1);
		Vsub (n2, v1, v2);
		Normalize (n2);
		Crossp (norm, n1, n2);
		Normalize (norm);

		/* Make triangle structure */
		tri[ntris].v1 = AddVertex (v1);
		tri[ntris].v2 = AddVertex (v2);
		tri[ntris].v3 = AddVertex (v3);

		Vset (tri[ntris].norm, norm);
		tri[ntris].done = 0;
		tri[ntris].hex = hex;

		ntris++;
	}
	fclose (fd);

	/* Make models about the same size in network games */
	if (am_client || am_server) ScaleModel (m);

	/* Make into triangle strips */
	MakeStrips();

	glEndList();

	/* Construct bounding box */
	ModelBounds (m);

	free (tri);
	free (vert);

	return (m);
}

MakeStrips()
/*
 *  Since the triangle model consists of sparse triangles and we
 *  want to use triangle strips for performance, this routine slices
 *  and dices the triangles to make them into strips where it can.
 *  The result is not optimal, but it typically cuts the number of
 *  transformed vertices in half and sometimes does much better.
 */
{
	int t, a;

	Log ("MakeStrips: Total triangles: %d", ntris);
	Log ("MakeStrips: Total vertices: %d", nverts);

	/* While there are more */
	while ((-1) != (t = FindTriangle()))
	{
		verts += 3;
		parity = 0;
		stripped = 0;
		length = 1;

		glBegin (GL_TRIANGLE_STRIP);

		/* While there is an adjacent triangle */
		while ((-1) != (a = FindAdjacent(t)))
		{
			if (length == 1)
			{
				Normal (t);
				Vertex (tri[t].v1);
				Vertex (tri[t].v2);
				Vertex (tri[t].v3);
			}
			Normal (a);
			Vertex (tri[a].v3);
			stripped++;
			tri[t].done = 1;
			tri[a].done = 1;
			t = a;
			verts++;
			parity = !parity;
			length++;
		}

		if (!stripped)
		{
			flop (t);
			while ((-1) != (a = FindAdjacent(t)))
			{
				if (length == 1)
				{
					Normal (t);
					Vertex (tri[t].v1);
					Vertex (tri[t].v2);
					Vertex (tri[t].v3);
				}
				Normal (a);
				Vertex (tri[a].v3);
				stripped++;
				tri[t].done = 1;
				tri[a].done = 1;
				t = a;
				verts++;
				parity = !parity;
				length++;
			}
		}

		if (!stripped)
		{
			flop (t);
			while ((-1) != (a = FindAdjacent(t)))
			{
				if (length == 1)
				{
					Normal (t);
					Vertex (tri[t].v1);
					Vertex (tri[t].v2);
					Vertex (tri[t].v3);
				}
				Normal (a);
				Vertex (tri[a].v3);
				stripped++;
				tri[t].done = 1;
				tri[a].done = 1;
				t = a;
				verts++;
				parity = !parity;
				length++;
			}
		}

		if (!stripped)
		{
			Normal (t);
			Vertex (tri[t].v1);
			Vertex (tri[t].v2);
			Vertex (tri[t].v3);

			/* We get here if triangle t couldn't be put
			   in a triangle strip */
			tri[t].done = 1;
			orphans++;
		}

		glEnd();
	}

	Log ("MakeStrips: Rendered vertices: %d (would have been %d, bare minimum %d)",
		verts, 3*ntris, nverts);
	Log ("MakeStrips: Orphaned triangles: %d", orphans);
}

int FindTriangle()
{
	int t;

	for (t = 0; t<ntris; t++)
	{
		if (!tri[t].done) return (t);
	}
	return (-1);
}

int FindAdjacent (t)
int t;
{
	int a, i;

	for (a=0; a<ntris; a++)
	{
		if ( (a != t) && (!tri[a].done) )
		{
		    if (parity == 0)
		    {
			for (i=0; i<3; i++)
			{
				if ( (tri[t].v2 == tri[a].v2) &&
				     (tri[t].v3 == tri[a].v1) &&
				     (tri[t].hex == tri[a].hex) )
					return (a);
				flop (a);
			}
		    }
		    else
		    {
			for (i=0; i<3; i++)
			{
				if ( (tri[t].v3 == tri[a].v2) &&
				     (tri[t].v1 == tri[a].v1) &&
				     (tri[t].hex == tri[a].hex) )
					return (a);
				flop (a);
			}
		    }
		}
	}
	return (-1);
}

flop (t)
int t;
{
	int v;

	v = tri[t].v1;
	tri[t].v1 = tri[t].v2;
	tri[t].v2 = tri[t].v3;
	tri[t].v3 = v;
}

int Veq (v1, v2)
int v1, v2;
{
	return (v1 == v2);
}

int AddVertex (v)
double v[3];
{
	int i;

	for (i=0; i<nverts; i++)
	{
		if ( (v[0] == vert[i].v[0]) &&
		     (v[1] == vert[i].v[1]) &&
		     (v[2] == vert[i].v[2]) ) return (i);
	}

	Vset (vert[nverts].v, v);
	nverts++;

	return (nverts-1);
}

Vertex (v)
int v;
{
	glVertex3d (vert[v].v[0], vert[v].v[1], vert[v].v[2]);
}

Normal (t)
int t;
{
	float color[3];

	if (tri[t].hex != lasthex)
	{
		lasthex = tri[t].hex;
		color[0] = ((float) (tri[t].hex / 65536)) / 256.0;
		color[1] = ((float) ((tri[t].hex % 65536) / 256)) / 256.0;
		color[2] = ((float) (tri[t].hex % 256)) / 256.0;
		glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, color);
		glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, color);
	}
	glNormal3dv (tri[t].norm);
}

ModelBounds (m)
int m;
/*
 *  Construct bounding box of model m
 */
{
	int i, v;
	double mm;

	for (i=0; i<3; i++)
	{
		model[m].lobound[i] = model[m].hibound[i] = vert[0].v[i];
	}

	for (v=1; v<nverts; v++)
	{
		for (i=0; i<3; i++)
		{
			if (vert[v].v[i] < model[m].lobound[i])
				model[m].lobound[i] = vert[v].v[i];

			if (vert[v].v[i] > model[m].hibound[i])
				model[m].hibound[i] = vert[v].v[i];
		}
	}

	Log ("ModelBounds: Lo bounds: %lf %lf %lf", model[m].lobound[0],
			model[m].lobound[1], model[m].lobound[2]);
	Log ("ModelBounds: Hi bounds: %lf %lf %lf", model[m].hibound[0],
			model[m].hibound[1], model[m].hibound[2]);

	/* Figure radius of bounding sphere */
	model[m].radius = Mag (model[m].lobound);
	if ((mm = Mag (model[m].hibound)) > model[m].radius)
		model[m].radius = mm;
	model[m].radius2 = model[m].radius * model[m].radius;
}

ScaleModel (m)
/*
 *  Make models a uniform size
 */
{
	double ratio;
	int v, i;

	/* Compute existing model bounds */
	ModelBounds (m);

	if (model[m].radius == 0.0) return;

	/* Want everything to have a radius of, say, 0.030 */
	ratio = model[m].radius / 0.030;

	/* Scale all the vertices */
	for (v=0; v<nverts; v++)
	{
		for (i=0; i<3; i++)
		{
			vert[v].v[i] /= ratio;
		}
	}
}

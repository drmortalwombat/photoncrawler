#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "vector3d.h"
#include "rgbimage.h"

Vector3 backgroundColor = {{0.7, 0.7, 1.0}};
Vector3  SkyBaseColor = {{0.0, 0.0, 0.8}};
Vector3  SkyHorizonColor = {{0.4, 0.4, 0.0}};

Vector3 White = {{1.0, 1.0, 1.0}};

float	width, height;
Vector3	viewP, viewZ, viewX, viewY;


float infinity = 10000000;
float epsilon = 0.01;

struct Material
{
	Vector3	color1, color2;
	float	size, mirror;

	Vector3 (* colorat)(Material * mat, const Vector3 * pos);
};

Vector3 solidmat_colorat(Material * mat, const Vector3 * pos)
{
	return mat->color1;
}

Vector3 mirrormat_colorat(Material * mat, const Vector3 * pos)
{
	return White;
}

Vector3 checkermat_colorat(Material * mat, const Vector3 * pos)
{
	int	ix = (int)floor(pos->v[0] / mat->size);
	int	iy = (int)floor(pos->v[1] / mat->size);
	int	iz = (int)floor(pos->v[2] / mat->size);
			
	if ((ix ^ iy ^ iz) & 1)
		return mat->color1;
	else
		return mat->color2;
}

void solidmat_init(Material * mat, const Vector3 * color)
{
	mat->color1 = *color;
	mat->colorat = solidmat_colorat;
	mat->mirror = 0.0;
}

void checkermat_init(Material * mat, const Vector3 * color1, const Vector3 * color2, float size)
{
	mat->color1 = *color1;
	mat->color2 = *color2;
	mat->size = size;
	mat->colorat = checkermat_colorat;
	mat->mirror = 0.0;
}

void mirrormat_init(Material * mat, float mirror)
{
	mat->colorat = mirrormat_colorat;
	mat->mirror = mirror;
}

struct Solid
{
	Vector3			pos, norm;
	float			radius;
	Material	*	material;

	Solid	*	next;

	float (* distance)(Solid * solid, const Vector3 * pos, const Vector3 * dir);
	Vector3 (* normal)(Solid * solid, const Vector3 * pos);
	Vector3 (* colorat)(Solid * solid, const Vector3 * pos);
};

Vector3 solid_colorat(Solid * solid, const Vector3 * pos)
{
	return solid->material->colorat(solid->material, pos);
}

float sphere_distance(Solid * solid, const Vector3 * pos, const Vector3 * dir)
{
	Vector3	sc;
	vec3_diff(&sc, pos, &solid->pos);

	float	q = vec3_vmul(&sc, &sc) - solid->radius * solid->radius;
	float	p = 2 * vec3_vmul(&sc, dir);

	float	det = p * p * 0.25 - q;
	
	if (det >= 0)
		return - 0.5 * p - sqrt(det);

	return -infinity;
}

Vector3 sphere_normal(Solid * solid, const Vector3 * pos)
{
	Vector3	sc;
	vec3_diff(&sc, pos, &solid->pos);
	vec3_norm(&sc);

	return sc;
}

void sphere_init(Solid * solid, const Vector3 * center, float radius, Material * mat)
{
	solid->pos = *center;
	solid->radius = radius;
	solid->material = mat;
	solid->distance = sphere_distance;
	solid->normal = sphere_normal;
	solid->colorat = solid_colorat;
}

float halfspace_distance(Solid * solid, const Vector3 * pos, const Vector3 * dir)
{
	float nd = vec3_vmul(dir, &solid->norm);
			
	if (nd != 0)
	{
		Vector3	sc;
		vec3_diff(&sc, &solid->pos, pos);
		return vec3_vmul(&sc, &solid->norm) / nd;
	}

	return -infinity;
}

Vector3 halfspace_normal(Solid * solid, const Vector3 * pos)
{
	return solid->norm;
}

void halfspace_init(Solid * solid, const Vector3 * pos, const Vector3 * norm, Material * mat)
{
	solid->pos = *pos;
	solid->norm = *norm;
	solid->material = mat;
	solid->distance = halfspace_distance;
	solid->normal = halfspace_normal;
	solid->colorat = solid_colorat;
}

struct Scene
{
	Solid	*	solids;
	Vector3		light;
}

void scene_init(Scene * scene)
{
	scene->solids = nullptr;
	scene->light.v[0] = -2; scene->light.v[1] = 1; scene->light.v[2] = -1;
	vec3_norm(&scene->light);	
}

void scene_add_solid(Scene * scene, Solid * solid)
{
	solid->next = scene->solids;
	scene->solids = solid;
}

bool scene_intersects(Scene * scene, const Vector3 * pos, const Vector3 * dir, Solid * exclude)
{
	Solid * 	shape = scene->solids;

	while (shape)
	{
		if (shape != exclude)
		{
			float	sd = shape->distance(shape, pos, dir);
//			printf("");
//			printf("%f, %f, %f -> %f\n", pos->v[0], pos->v[1], pos->v[2], sd);
			if (sd > epsilon)
				return true;
		}
		shape = shape->next;
	}

	return false;
}

Vector3 scene_trace(Scene * scene, const Vector3 * pos, const Vector3 * dir)
{
	float		distance = infinity;
	Solid * 	shape = scene->solids, * nearest = nullptr;

	while (shape)
	{
		float	sd = shape->distance(shape, pos, dir);
		if (sd > 0 && sd < distance) {
			nearest = shape;
			distance = sd;
		}
		shape = shape->next;
	}

	if (nearest) 
	{
		Vector3	at, norm;
		vec3_lincomb(&at, pos, distance, dir);
		norm = nearest->normal(nearest, &at);

		if (nearest->material->mirror > 0) 
		{
			Vector3	ndir, nat;

			vec3_lincomb(&ndir, dir, -2.0 * vec3_vmul(dir, &norm), &norm);
			vec3_lincomb(&nat, &at, epsilon, &ndir);

			Vector3	color = scene_trace(scene, &nat, &ndir);
			vec3_scale(&color, nearest->material->mirror);

			return color;
		}
		else
		{
			Vector3	color = nearest->colorat(nearest, &at);

			float	light = 0.1;

			if (!scene_intersects(scene, &at, &scene->light, nearest)) 
			{
		

				float lcos = vec3_vmul(&scene->light, &norm);
				if (lcos > 0)
					light += 0.9 * lcos;
			}

			vec3_scale(&color, light);

			return color;
		}
	}
	else
	{
		Vector3	color;
		vec3_lincomb(&color, &SkyBaseColor, 1.0 - dir->v[1], &SkyHorizonColor);
		return color;
	}
}



void trace_start(Scene * scene)
{
	width = 320.0;
	height = 200.0;
				
	float	vof = tan(90 * PI / 360);
			
	Matrix3	viewm;
	mat3_set_rotate_x(&viewm, -0.22);
//	mat3_ident(&viewm);
#if 0
	printf("M: %f %f %f   %f %f %f   %f %f %f\n", 
		viewm.m[0], 
		viewm.m[1], 
		viewm.m[2], 
		viewm.m[3], 
		viewm.m[4], 
		viewm.m[5], 
		viewm.m[6], 
		viewm.m[7], 
		viewm.m[8]);
#endif
	viewP.v[0] = 0; viewP.v[1] = 50; viewP.v[2] = -200;
	viewZ.v[0] = 0; viewZ.v[1] = 0; viewZ.v[2] = 1;
	viewX.v[0] = vof; viewX.v[1] = 0; viewX.v[2] = 0;
	viewY.v[0] = 0; viewY.v[1] = vof; viewY.v[2] = 0;

	vec3_mmul(&viewX, &viewm, &viewX);
	vec3_mmul(&viewY, &viewm, &viewY);
	vec3_mmul(&viewZ, &viewm, &viewZ);
#if 0
	printf("X: %f %f %f\n", viewX.v[0], viewX.v[1], viewX.v[2]);
	printf("Y: %f %f %f\n", viewY.v[0], viewY.v[1], viewY.v[2]);
#endif
}

Vector3 trace_pixel(Scene * scene, int ix, int iy)
{
	float	tx = (float)ix, ty = (float)iy;

	float dx = (tx - width / 2) / width;
	float dy = (height / 2 - ty) / width;

	Vector3	vn;

	vec3_lincomb2(&vn, &viewZ, dx, &viewX, dy, &viewY);

	vec3_norm(&vn);

	Vector3	c = scene_trace(scene, &viewP, &vn);

	return c;
}

static char cscale(float f)
{
	int fi = 16 + (int)(f * 224);
	if (fi < 0)
		return 0;
	else if (fi > 255)
		return 255;
	else
		return fi;
}

void trace_frame(Scene * scene)
{
	for(char iy=0; iy<25; iy++)
	{
		for(char ix=0; ix<40; ix++)
		{
			for(char y=0; y<8; y++)
			{
				for(char x=0; x<4; x++)
				{
					Vector3	c = trace_pixel(scene, ix * 8 + 2 * x, iy * 8 + y);

					rgbblock[y][x].r = cscale(c.v[0]);
					rgbblock[y][x].g = cscale(c.v[1]);
					rgbblock[y][x].b = cscale(c.v[2]);
				}
			}

			rgbimg_noiseblock();

			rgbimg_buildpal();

			rgbimg_mapblock();
			rgbimg_putblock(ix, iy);			
		}
	}
}


int main(void)
{
	for(char i=0; i<16; i++)
	{
		basepalette[i].r = (basepalette[i].r >> 2) + 8;
		basepalette[i].g = (basepalette[i].g >> 2) + 8;
		basepalette[i].b = (basepalette[i].b >> 2) + 8;
	}

	rgbimg_begin();

	Scene		scene;
	Solid		ground, sphere1, sphere2, sphere3;
	Material	smat1, smat2, cmat, mmat;
	Vector3		pos, norm, color1, color2;

	scene_init(&scene);

	color1.v[0] = 0.0; color1.v[1] = 1.0; color1.v[2] = 0.0;
	solidmat_init(&smat1, &color1);

	color1.v[0] = 1.0; color1.v[1] = 0.0; color1.v[2] = 0.0;
	solidmat_init(&smat2, &color1);

	color1.v[0] = 1.0; color1.v[1] = 1.0; color1.v[2] = 1.0;
	color2.v[0] = 0.2; color2.v[1] = 0.2; color2.v[2] = 0.2;	
	checkermat_init(&cmat, &color1, &color2, 10);

	mirrormat_init(&mmat, 0.9);

	pos.v[0] = -35; pos.v[1] = 0; pos.v[2] = 0;
	sphere_init(&sphere1, &pos, 30, &smat1);
	scene_add_solid(&scene, &sphere1);

	pos.v[0] = 40; pos.v[1] = 10; pos.v[2] = 5;
	sphere_init(&sphere2, &pos, 40, &smat2);
	scene_add_solid(&scene, &sphere2);

	pos.v[0] = 0; pos.v[1] = -30; pos.v[2] = 0;
	norm.v[0] = 0; norm.v[1] = 1; norm.v[2] = 0;
	halfspace_init(&ground, &pos, &norm, &cmat);
	scene_add_solid(&scene, &ground);

	pos.v[0] = 10; pos.v[1] = 20; pos.v[2] = 80;
	sphere_init(&sphere3, &pos, 40, &mmat);
	scene_add_solid(&scene, &sphere3);

	trace_start(&scene);
	trace_frame(&scene);

#if 0

	for(char iy=0; iy<25; iy++)
	{
		for(char ix=0; ix<40; ix++)
		{
			for(char y=0; y<8; y++)
			{
				for(char x=0; x<4; x++)
				{
					int	tx = ix * 4 + x, ty = iy * 8 + y;
#if 1
					rgbblock[y][x].r = 0;
					rgbblock[y][x].g = tx * 8 / 5;
					rgbblock[y][x].b = ty * 32 /25 ;
#else
					float	fx = (float)(x + 4 * ix) / 32 - 1.0;
					float	fy = (float)(y + 8 * iy) / 64 - 1.0;

					float	r = sqrt(fx * fx + fy * fy);

					rgbblock[y][x].r = 128 + 127 * sin((float)(y + 8 * iy) * 3.14 / 64);
					rgbblock[y][x].g = 255 * r;
					rgbblock[y][x].b = 128 + 127 * sin((float)(x + 4 * ix) * 3.14 / 32);
#endif					
				}
			}

			rgbimg_noiseblock();

			rgbimg_buildpal();

			rgbimg_mapblock();
			rgbimg_putblock(ix, iy);

		}
	}
#endif

	getch();

	rgbimg_end();

	return 0;
}
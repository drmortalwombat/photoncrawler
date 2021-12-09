#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "vector3d.h"
#include "rgbimage.h"

Vector3	surfaceBase = {{0, -20, 0}};
Vector3 surfaceNorm = {{0, 1, 0}};
Vector3 surfaceColor = {{1, 0, 0}};

Vector3 sphereCenter = {{0, 0, 0}};
float sphereRadius = 30;
Vector3 sphereColor = {{0, 1, 0}};

Vector3 backgroundColor = {{0, 0, 0}};

float	width, height;
Vector3	viewP, viewZ, viewX, viewY;

float infinity = 10000000;

Vector3 trace(Vector3 * pos, Vector3 * dir) 
{
	float	surfaceDistance = -infinity;
	float	sphereDistance = -infinity;

	float	nd = vec3_vmul(dir, &surfaceNorm);
	
	if (nd != 0)
	{
		Vector3	vd;
		vec3_diff(&vd, &surfaceBase, pos);		
		surfaceDistance = vec3_vmul(&vd, &surfaceNorm) / nd;
	}
	
	Vector3	sc;
	vec3_diff(&sc, pos, &sphereCenter);
	float	q = vec3_vmul(&sc, &sc) - sphereRadius * sphereRadius;
	float	p = 2 * vec3_vmul(&sc, dir);
	float	det = p * p * 0.25 - q;
	if (det >= 0)
		sphereDistance = - 0.5 * p - sqrt(det);
	
	if (surfaceDistance > 0 && (surfaceDistance < sphereDistance || sphereDistance < 0))
		return surfaceColor;
	else if (sphereDistance > 0)
		return sphereColor;
	else
		return backgroundColor;
}

void trace_start()
{
	width = 320.0;
	height = 200.0;
				
	float	vof = tan(90 * PI / 360);
			
	viewP.v[0] = 0; viewP.v[1] = 50; viewP.v[2] = -200;
	viewZ.v[0] = 0; viewZ.v[1] = 0; viewZ.v[2] = 1;
	viewX.v[0] = vof; viewX.v[1] = 0; viewX.v[2] = 0;
	viewY.v[0] = 0; viewY.v[1] = vof; viewY.v[2] = 0;
}

Vector3 trace_pixel(char ix, char iy)
{
	float	tx = (float)ix, ty = (float)iy;

	float dx = (tx - width / 2) / width;
	float dy = (height / 2 - ty) / width;

	Vector3	vn;

	vec3_lincomb2(&vn, &viewZ, dx, &viewX, dy, &viewY);

	vec3_norm(&vn);

	Vector3	c = trace(&viewP, &vn);

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
void trace_frame()
{
	for(char iy=0; iy<25; iy++)
	{
		for(char ix=0; ix<40; ix++)
		{
			for(char y=0; y<8; y++)
			{
				for(char x=0; x<4; x++)
				{
					Vector3	c = trace_pixel(ix * 8 + 2 * x, iy * 8 + y);

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

	trace_start();
	trace_frame();

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
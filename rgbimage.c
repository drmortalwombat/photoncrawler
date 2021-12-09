#include "rgbimage.h"
#include <c64/vic.h>

void rgbimg_begin(void)
{
	vic.ctrl1 = VIC_CTRL1_BMM | VIC_CTRL1_DEN | VIC_CTRL1_RSEL | 3;
	vic.ctrl2 = VIC_CTRL2_MCM | VIC_CTRL2_CSEL;
	
	vic.color_back = VCOL_BLACK;
	vic.color_border = VCOL_BLACK;

	vic_setbank(3);
	vic.memptr = 0x28;
}

void rgbimg_end(void)
{
	vic.ctrl1 = VIC_CTRL1_DEN | VIC_CTRL1_RSEL | 3;
	vic.ctrl2 = VIC_CTRL2_CSEL;
	
	vic.color_back = VCOL_BLACK;

	vic_setbank(0);
	vic.memptr = 0x14;
}

#define Screen	((byte *)0xc800)
#define Color	((byte *)0xd800)
#define Hires	((byte *)0xe000)

char	block[8][4], color[4];
RGBA	rgbblock[8][4];
RGBA	rgbpalette[4];

RGBA	basepalette[16] = 
{
	{ 0x00, 0x00, 0x00 },
	{ 0xFF, 0xFF, 0xFF },
	{ 0x68, 0x37, 0x2B },
	{ 0x70, 0xA4, 0xB2 },
	{ 0x6F, 0x3D, 0x86 },
	{ 0x58, 0x8D, 0x43 },
	{ 0x35, 0x28, 0x79 },
	{ 0xB8, 0xC7, 0x6F },
	{ 0x6F, 0x4F, 0x25 },
	{ 0x43, 0x39, 0x00 },
	{ 0x9A, 0x67, 0x59 },
	{ 0x44, 0x44, 0x44 },
	{ 0x6C, 0x6C, 0x6C },
	{ 0x9A, 0xD2, 0x84 },
	{ 0x6C, 0x5E, 0xB5 },
	{ 0x95, 0x95, 0x95 }
};

void rgbimg_putblock(char x, char y)
{
	char * hp = Hires + 320 * y + 8 * x;
	for(char i=0; i<8; i++)
		hp[i] = (((((block[i][0] << 2) | block[i][1]) << 2) | block[i][2]) << 2) | block[i][3];
	Screen[x + 40 * y] = (color[1] << 4) | color[2];
	Color[x + 40 * y] = color[3];
}

static inline char dist(char a, char b)
{
	return a > b ? a - b : b - a;	
}

char rgbimg_mapcolor(RGBA c)
{
	char	ei = dist(c.r, rgbpalette[0].r) + dist(c.g, rgbpalette[0].g) + dist(c.b, rgbpalette[0].b);
	char	ci = 0;

	for(char i=1; i<4; i++)
	{
		char e = dist(c.r, rgbpalette[i].r) + dist(c.g, rgbpalette[i].g) + dist(c.b, rgbpalette[i].b);
		if (e < ei)
		{
			ei = e;
			ci = i;
		}
	}

	return ci;
}

static unsigned mapcolordist(RGBA c, char n)
{
	char	ei = dist(c.r, rgbpalette[0].r) + dist(c.g, rgbpalette[0].g) + dist(c.b, rgbpalette[0].b);

	for(char i=1; i<n; i++)
	{
		char e = dist(c.r, rgbpalette[i].r) + dist(c.g, rgbpalette[i].g) + dist(c.b, rgbpalette[i].b);
		if (e < ei)
			ei = e;
	}

	return ei;
}

char paldist[16][32];
char	pdist[32];

void rgbimg_buildpal(void)
{
	for(char j=0; j<16; j++)
	{
		for(char i=0; i<32; i++)
		{
			paldist[j][i] = 
				dist(rgbblock[0][i].r, basepalette[j].r) + 
				dist(rgbblock[0][i].g, basepalette[j].g) +
				dist(rgbblock[0][i].b, basepalette[j].b);
		}
	}

	for(char i=0; i<32; i++)
		pdist[i] = paldist[0][i];

	color[0] = 0;
	rgbpalette[0] = basepalette[0];

	for(char i=1; i<4; i++)
	{
		unsigned	dmax = 65535;
		char		jmax = 0;

		for(char j=1; j<16; j++)
		{			
			unsigned	dsum = 0;
			for(char k=0; k<32; k++)
			{
				char 	ei = pdist[k];
				char	e = paldist[j][k];

				if (e < ei)
					ei = e;

				dsum += ei;
			}
			if (dsum < dmax)
			{
				dmax = dsum;
				jmax = j;
			}
		}

		color[i] = jmax;
		rgbpalette[i] = basepalette[jmax];

		for(char k=0; k<32; k++)
		{
			char 	ei = pdist[k];
			char	e = paldist[jmax][k];

			if (e < ei)
				pdist[k] = e;
		}
	}	
}

static char correct1(char a, char m, char d)
{
	int	k = (int)d + a - m;

	if (k < 0)
		return 0;
	else if (k > 255)
		return 255;
	else
		return k;
}


static char correct2(char a, char m, char d)
{
	int	k = ((int)d + d + a - m) >> 1;

	if (k < 0)
		return 0;
	else if (k > 255)
		return 255;
	else
		return k;
}

static char correct4(char a, char m, char d)
{
	int	k = ((int)d * 4 + a - m) >> 2;

	if (k < 0)
		return 0;
	else if (k > 255)
		return 255;
	else
		return k;
}

static inline char noise(char a)
{
	return (a + (rand() & 63)) >> 2;
}

void rgbimg_noisergb(RGBA * rgba)
{
	rgba->r = noise(rgba->r);
	rgba->g = noise(rgba->g);
	rgba->b = noise(rgba->b);
}

void rgbimg_noiseblock(void)
{
	for(char y=0; y<8; y++)
	{
		for(char x=0; x<4; x++)
		{		
			rgbimg_noisergb(&(rgbblock[y][x]));
		}
	}
}	

void rgbimg_mapblock(void)
{
	for(char y=0; y<8; y+=2)
	{
		for(char x=0; x<4; x+=2)
		{			
			RGBA	rgb0 = rgbblock[y + 0][x + 0];
			RGBA	rgb1 = rgbblock[y + 1][x + 1];
			RGBA	rgb2 = rgbblock[y + 0][x + 1];
			RGBA	rgb3 = rgbblock[y + 1][x + 0];

			char c0 = rgbimg_mapcolor(rgb0);
			block[y + 0][x + 0] = c0;

			rgb1.r = correct2(rgb0.r, rgbpalette[c0].r, rgb1.r);
			rgb1.g = correct2(rgb0.g, rgbpalette[c0].g, rgb1.g);
			rgb1.b = correct2(rgb0.b, rgbpalette[c0].b, rgb1.b);

			rgb2.r = correct4(rgb0.r, rgbpalette[c0].r, rgb2.r);
			rgb2.g = correct4(rgb0.g, rgbpalette[c0].g, rgb2.g);
			rgb2.b = correct4(rgb0.b, rgbpalette[c0].b, rgb2.b);

			rgb3.r = correct4(rgb0.r, rgbpalette[c0].r, rgb3.r);
			rgb3.g = correct4(rgb0.g, rgbpalette[c0].g, rgb3.g);
			rgb3.b = correct4(rgb0.b, rgbpalette[c0].b, rgb3.b);

			char c1 = rgbimg_mapcolor(rgb1);
			block[y + 1][x + 1] = c1;

			rgb2.r = correct2(rgb1.r, rgbpalette[c1].r, rgb2.r);
			rgb2.g = correct2(rgb1.g, rgbpalette[c1].g, rgb2.g);
			rgb2.b = correct2(rgb1.b, rgbpalette[c1].b, rgb2.b);

			rgb3.r = correct2(rgb1.r, rgbpalette[c1].r, rgb3.r);
			rgb3.g = correct2(rgb1.g, rgbpalette[c1].g, rgb3.g);
			rgb3.b = correct2(rgb1.b, rgbpalette[c1].b, rgb3.b);

			char c2 = rgbimg_mapcolor(rgb2);
			block[y + 0][x + 1] = c2;

			rgb3.r = correct1(rgb2.r, rgbpalette[c2].r, rgb3.r);
			rgb3.g = correct1(rgb2.g, rgbpalette[c2].g, rgb3.g);
			rgb3.b = correct1(rgb2.b, rgbpalette[c2].b, rgb3.b);

			char c3 = rgbimg_mapcolor(rgb3);
			block[y + 1][x + 0] = c3;
		}
	}
}


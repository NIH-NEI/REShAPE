#define __rsGeom_main__
#include "rsGeom.h"

// ---------- Particle -------------

uint64 Particle::update_from_fill()
{
	if (fill.size() > 0) {
		x0 = fill[0].xl;
		y0 = fill[0].y;
	}
	else {
		x0 = y0 = 0;
	}
	bnd = fill_boundary(fill);
	area = fill_area(fill);
	return area;
}

bool Particle::IsInside(int x, int y)
{
	if (!bnd.IsInside(x, y)) return false;
	for (HSeg& hs : fill) {
		if (hs.y != y) continue;
		if (x >= hs.xl && x <= hs.xr) return true;
	}
	return false;
}

Point Particle::center_mass()
{
	double xc = 0., yc = 0.;
	int npt = 0;
	for (HSeg& hs : fill) {
		int y = hs.y;
		for (int x = hs.xl; x <= hs.xr; x++) {
			xc += x;
			yc += y;
			++npt;
		}
	}
	if (npt == 0) return Point(0, 0);
	return Point(int(xc / npt), int(yc / npt));
}

uint64 Particle::overlay_area(std::vector<HSeg>& other_fill)
{
	return fill_overlay_area(fill, other_fill);
}
uint64 Particle::overlay_area(Particle& other)
{
	if (!bnd.intersects(other.bnd)) return 0;
	return fill_overlay_area(fill, other.fill);
}

void Particle::translate(int x, int y)
{
	x0 += x;
	y0 += y;
	bnd.xmin += x;
	bnd.xmax += x;
	bnd.ymin += y;
	bnd.ymax += y;
	for (HSeg& hs : fill) {
		hs.y += y;
		hs.xl += x;
		hs.xr += x;
	}
}

//----------------------------- Global utility functions --------------------------------

Boundary fill_boundary(std::vector<HSeg>& fill)
{
	Boundary b(0, 0, 0, 0);
	if (fill.size() == 0) return b;
	HSeg hs0 = fill[0];
	b.ymin = b.ymax = hs0.y;
	b.xmin = hs0.xl;
	b.xmax = hs0.xr;
	for (HSeg& s : fill) {
		if (b.ymin > s.y) b.ymin = s.y;
		if (b.ymax < s.y) b.ymax = s.y;
		if (b.xmin > s.xl) b.xmin = s.xl;
		if (b.xmax < s.xr) b.xmax = s.xr;
	}
	return b;
}

uint64 fill_overlay_area(std::vector<HSeg>& fill, std::vector<HSeg>& other_fill)
{
	uint64 a = 0;
	for (HSeg& hs0 : fill) {
		for (HSeg& hs1 : other_fill) {
			if (hs0.y != hs1.y) continue;
			int xmin = hs0.xl < hs1.xl ? hs1.xl : hs0.xl;
			int xmax = hs0.xr > hs1.xr ? hs1.xr : hs0.xr;
			if (xmax < xmin) continue;
			a += uint64(xmax - xmin + 1);
		}
	}
	return a;
}

//--------------------- Global data --------------------------------

NbrPoint hood_pts[37] = {
	{0, 0},

	{0, -1},
	{1, 0},
	{0, 1},
	{-1, 0},

	{-1, -1},
	{1, -1},
	{1, 1},
	{-1, 1},

	{-1, -2},
	{0, -2},
	{1, -2},
	{2, -1},
	{2, 0},
	{2, 1},
	{1, 2},
	{0, 2},
	{-1, 2},
	{-2, 1},
	{-2, 0},
	{-2, -1},

	{-2, -2},
	{2, -2},
	{2, 2},
	{-2, 2},
	{-1, -3},
	{0, -3},
	{1, -3},
	{3, -1},
	{3, 0},
	{3, 1},
	{1, 3},
	{0, 3},
	{-1, 3},
	{-3, 1},
	{-3, 0},
	{-3, -1}
};


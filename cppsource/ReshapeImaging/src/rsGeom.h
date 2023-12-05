#ifndef rsGeom_h
#define rsGeom_h

#define _USE_MATH_DEFINES // for C++
#include <cmath>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>

#ifndef int64
typedef signed long long int64;
#endif
#ifndef uint64
typedef unsigned long long uint64;
#endif
#ifndef uint32
typedef unsigned int uint32;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uint8
typedef unsigned char uint8;
#endif

typedef std::unordered_map<std::string, std::string> Properties;

const double PI = 3.14159265;
const double NaN = std::nan(" ");

const int HOOD_SIZE_NEUMANN = 5;
const int HOOD_SIZE_MOORE = 9;
const int HOOD_SIZE_FATCROSS = 21;
const int HOOD_SIZE_RAD3 = 37;

struct Point
{
	int x, y;
	Point() : x(0), y(0) {}
	Point(int _x, int _y) : x(_x), y(_y) {}
	// Point(Point &p) : x(p.x), y(p.y) {}
	double dist(Point& p1) {
		double dx(x - p1.x);
		double dy(y - p1.y);
		return sqrt(dx * dx + dy * dy);
	}
	double dist(int _x, int _y) {
		double dx(x - _x);
		double dy(y - _y);
		return sqrt(dx * dx + dy * dy);
	}
	int idist(Point& p1) {
		int dx(x - p1.x); if (dx < 0) dx = -dx;
		int dy(y - p1.y); if (dy < 0) dy = -dy;
		return dx > dy ? dx : dy;
	}
	void swap(Point& p1) {
		x ^= p1.x; p1.x ^= x; x ^= p1.x;
		y ^= p1.y; p1.y ^= y; y ^= p1.y;
	}
	bool equals(Point& p1) {
		return x == p1.x && y == p1.y;
	}
};

struct NbrPoint
{
	int dx, dy;
};

struct Boundary
{
	int xmin, ymin, xmax, ymax;
	Boundary() {}
	Boundary(int _xmin, int _ymin, int _xmax, int _ymax) :
		xmin(_xmin), ymin(_ymin), xmax(_xmax), ymax(_ymax) {}
	//Boundary(const Boundary &bnd) :
	//	xmin(bnd.xmin), ymin(bnd.ymin), xmax(bnd.xmax), ymax(bnd.ymax) {}
	void expand(int bord = 1) {
		xmin -= bord; ymin -= bord;
		xmax += bord; ymax += bord;
	}
	void translate(int x0, int y0) {
		xmin += x0;
		xmax += x0;
		ymin += y0;
		ymax += y0;
	}
	void combo(Boundary& bnd) {
		if (xmin > bnd.xmin) xmin = bnd.xmin;
		if (ymin > bnd.ymin) ymin = bnd.ymin;
		if (xmax < bnd.xmax) xmax = bnd.xmax;
		if (ymax < bnd.ymax) ymax = bnd.ymax;
	}
	bool intersects(Boundary& bnd) {
		return xmin < bnd.xmax&& xmax > bnd.xmin &&
			ymin < bnd.ymax&& ymax > bnd.ymin;
	}
	bool IsInside(int x, int y) {
		return x >= xmin && x <= xmax && y >= ymin && y <= ymax;
	}
	bool IsInside(Point p) {
		return p.x >= xmin && p.x <= xmax && p.y >= ymin && p.y <= ymax;
	}
	bool contains(Boundary& b) {
		return xmin <= b.xmin && xmax >= b.xmax && ymin <= b.ymin && ymax >= b.ymax;
	}
	int minbdist(Boundary& b) {
		int d = b.ymin - ymin;
		int dd = xmax - b.xmax; if (dd < d) d = dd;
		dd = ymax - b.ymax; if (dd < d) d = dd;
		dd = b.xmin - xmin; if (dd < d) d = dd;
		if (d < 0) d = 0;
		return d;
	}
	long long area() { return ((long long)(ymax - ymin + 1)) * (xmax - xmin + 1); }
	Boundary intersection(Boundary& bnd) {
		Boundary b = bnd;
		if (b.xmin < xmin) b.xmin = xmin;
		if (b.xmax > xmax) b.xmax = xmax;
		if (b.ymin < ymin) b.ymin = ymin;
		if (b.ymax > ymax) b.ymax = ymax;
		return b;
	}
};

struct HSeg
{
	int y, xl, xr;
	HSeg() {}
	HSeg(int _y, int _xl, int _xr) :
		y(_y), xl(_xl), xr(_xr) {}
	bool less_than(HSeg& other) {
		if (y != other.y) return y < other.y;
		return xl < other.xl;
	}
};

struct Particle
{
	int x0, y0;
	uint64 area;
	Boundary bnd;
	std::vector<HSeg> fill;
	//
	Particle() {}
	//Particle(Particle &ptc) {
	//	copyfrom(ptc);
	//}
	void clear() {
		x0 = y0 = 0;
		bnd = Boundary(0, 0, 0, 0);
		fill.clear();
	}
	void copyfrom(Particle& ptc) {
		fill.resize(ptc.fill.size());
		for (size_t i = 0; i < ptc.fill.size(); i++)
			fill[i] = ptc.fill[i];
		x0 = ptc.x0;
		y0 = ptc.y0;
		bnd = ptc.bnd;
		area = ptc.area;
	}
	//
	bool empty() { return fill.size() == 0; }
	uint64 update_from_fill();
	bool IsInside(int x, int y);
	Point center_mass();
	uint64 overlay_area(std::vector<HSeg>& other_fill);
	uint64 overlay_area(Particle& other);
	double iou(Particle& other) {
		uint64 ovl = overlay_area(other);
		if (ovl == 0) return 0.;
		return double(ovl) / (double(area) + double(other.area) - ovl);
	}
	void translate(int x, int y);
};

//----------------------------- Global utility functions --------------------------------

inline uint64 fill_area(std::vector<HSeg>& fill) {
	uint64 a = 0;
	for (HSeg& s : fill) a += uint64(s.xr - s.xl + 1);
	return a;
}
Boundary fill_boundary(std::vector<HSeg>& fill);
uint64 fill_overlay_area(std::vector<HSeg>& fill, std::vector<HSeg>& other_fill);

#ifndef __rsGeom_main__
extern NbrPoint hood_pts[37];
#endif

#endif

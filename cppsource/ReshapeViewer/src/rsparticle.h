#ifndef rsparticle_h
#define rsparticle_h

#include <memory.h>
#include <cstddef>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

const double PI = 3.14159265;
const double NaN = std::nan(" ");

typedef unsigned char byte;
typedef unsigned long long uint64;

struct rsPoint
{
	uint64 x;
	uint64 y;
	rsPoint() {}
	rsPoint(uint64 x, uint64 y) { this->x = x; this->y = y; }
	inline uint64 sqdist(rsPoint & other)
	{
		uint64 dx = x > other.x ? x - other.x : other.x - x;
		uint64 dy = y > other.y ? y - other.y : other.y - y;
		return dx * dx + dy * dy;
	}
	inline int compare(rsPoint & other)
	{
		if (y < other.y) return -1;
		if (y > other.y) return 1;
		if (x < other.x) return -1;
		if (x > other.x) return 1;
		return 0;
	}
};

struct rsRect
{
	uint64 x0, x1, y0, y1;
	rsRect() {}
	rsRect(rsPoint &ul, rsPoint &lr) { this->x0 = ul.x;  this->x1 = lr.x; this->y0 = ul.y; this->y1 = ul.y; }
	int contains(rsPoint pt) { return pt.x >= this->x0 && pt.x <= this->x1 && pt.y >= this->y0 && pt.y <= this->y1; }
	int overlaps(const rsRect & other) {
		if (this->x1 < other.x0) return 0;
		if (this->x0 > other.x1) return 0;
		if (this->y1 < other.y0) return 0;
		if (this->y0 > other.y1) return 0;
		return 1;
	}
	rsRect intersect(const rsRect & other) {
		rsRect r;
		r.x0 = std::max(this->x0, other.x0);
		r.x1 = std::min(this->x1, other.x1);
		r.y0 = std::max(this->y0, other.y0);
		r.y1 = std::min(this->y1, other.y1);
		return r;
	}
};

std::ostream& operator << (std::ostream & out, rsRect &r);
std::ostream& operator << (std::ostream & out, rsPoint &p);

struct rsHSeg
{
	uint64 x0, x1;
	uint64 y;
	rsHSeg() {}
	rsHSeg(uint64 y, uint64 x0, uint64 x1) { this->y = y; this->x0 = x0; this->x1 = x1; }
};

struct rsParticlePar
{
	int xStart;
	int yStart;
// initial parameters (loaded from the input "Results" CSV)
	double Area;		// "Area"
	double Peri;		// "Perim."
	double EllipMaj;	// "Major"
	double ElipMin;		// "Minor"
	double AR;			// "AR"
	double Angle;		// "Angle"
	double Feret;		// "Feret"
	double MinFeret;	// "MinFeret"
	double FeretAngle;	// "FeretAngle"
	double Circ;		// "Circ."
	double Solidity;	// "Solidity"
	double Width;		// "Width"
	double Height;		// "Height"
// derived parameters (added to the output "Data" CSV)
	double AoP;			// "Area/Perim."
	double FeretAR;		// "Ferets AR"
	double Compactness;	// "Compactness"
	double Extent;		// "Extent"
	int neighbors;		// "Neighbors"  -- nn-dependent
	double A_hull;		// "Area Convext Hull"
	double P_hull;		// "Perim Convext Hull"
	double PoN;			// "PoN"  -- nn-dependent
	double Poly_SR;		// "PSR"  -- nn-dependent
	double Poly_AR;		// "PAR"  -- nn-dependent
	double Poly_Ave;	// "Poly_Ave"  -- nn-dependent
	double Hex_SR;		// "HSR"  -- nn-dependent
	double Hex_AR;		// "HAR"  -- nn-dependent
	double Hex_Ave;		// "Hex_Ave"  -- nn-dependent
};

struct rsDisplayPar
{
	size_t off;
	const char *label;
	const char *header;
	const char *text;
};

#ifndef NO_EXTERNS
extern rsDisplayPar visualParams[];
extern size_t numVisualParams;
#endif

class rsParticle
{
private:
	uint64 id;
	std::vector<rsHSeg> fill;
	rsPoint ul;
	rsPoint lr;
	uint64 xc, yc;
public:
	rsParticlePar p;
	double *parPtr(size_t poff) { return (double *)(((char *)(&p)) + poff);	}
	rsParticle(uint64 id = 0) { this->id = id; memset(&p, 0, sizeof p); }
	rsParticle(const rsParticle & other, const rsRect r);
	uint64 GetId() { return id; }
	void SetId(uint64 id) { this->id = id; }
	size_t GetFillSize() { return fill.size(); }
	rsHSeg GetFillAt(size_t pos) { return fill[pos]; }
	void AddFill(rsHSeg f) { fill.push_back(f); }
	rsPoint GetUL() { return ul; }
	rsPoint GetLR() { return lr; }
	uint64 GetXCenter() { return xc; }
	uint64 GetYCenter() { return yc; }
	void FixCenter() {
		if (fill.size() == 0) {
			xc = p.xStart;
			yc = p.yStart;
		}
	}
	void TranslateFill(uint64 x, uint64 y) {
		for (size_t i = 0; i < fill.size(); i++) {
			rsHSeg &f = fill[i];
			f.x0 += x;
			f.x1 += x;
			f.y += y;
		}
	}
	void validate();
	void clear() {
		fill.resize(0);
		ul.x = ul.y = lr.x = lr.y = 0;
	}
};

#endif

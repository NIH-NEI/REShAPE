
#ifndef rsImage_h
#define rsImage_h

#include <string.h>
#include <stddef.h>
#include <time.h>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cmath>

const double PI = 3.14159265;
const double NaN = std::nan(" ");

typedef unsigned char byte;
// typedef unsigned int uint64;
typedef unsigned long long uint64;

const uint64 EXPAND = 6;
const uint64 EXPANDSQ = 41;

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
	int contains(const rsPoint &pt) { return pt.x >= this->x0 && pt.x <= this->x1 && pt.y >= this->y0 && pt.y <= this->y1; }
	int overlaps(rsRect & other) {
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

struct rsHSeg
{
	uint64 x0, x1;
	uint64 y;
	rsHSeg() {}
	rsHSeg(uint64 y, uint64 x0, uint64 x1) { this->y = y; this->x0 = x0; this->x1 = x1; }
};

struct rsParticlePar
{
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
	const char *text;
};

#ifndef NO_EXTERNS
extern rsDisplayPar visualParams[];
extern size_t numVisualParams;
#endif

class rsParticleOrigin
{
	friend class rsParticle;
private:
	uint64 id;
	uint64 x;
	uint64 y;
	uint64 nn;
public:
	rsParticlePar p;

	rsParticleOrigin() {}
	void SetData(uint64 id, uint64 x, uint64 y, uint64 nn=0) {
		this->id = id;
		this->x = x;
		this->y = y;
		this->nn = nn;
	}
	void SetId(uint64 id) { this->id = id; }
	uint64 GetId() { return id; }
	uint64 GetX() { return x; }
	uint64 GetY() { return y; }
	uint64 GetNN() { return nn; }
};

class rsParticle
{
	friend class rsImage;
private:
	std::vector<rsHSeg> fill;
	std::vector<rsPoint> perim;
	rsPoint ul;
	rsPoint lr;
	rsRect outer;
	std::vector<uint64> nbIds;
public:
	rsParticleOrigin *orig;
	rsParticle() { orig = NULL; }
	rsParticle(rsParticleOrigin & orig) { this->orig = &orig; }
	// Translate constructor
	rsParticle(rsParticle &srcpt, int dx, int dy) { translate(srcpt, dx, dy); }

	void translate(rsParticle &srcpt, int dx, int dy);
	uint64 GetId() { return orig ? orig->GetId() : 0; }
	rsParticleOrigin *GetOrigin() { return orig; }
	size_t GetFillSize() { return fill.size(); }
	rsHSeg GetFillAt(size_t pos) { return fill[pos]; }
	void AddFill(rsHSeg f) { fill.push_back(f); }
	size_t GetPerimSize() { return perim.size(); }
	rsPoint & GetPerimAt(size_t pos) { return perim[pos]; }
	rsPoint GetPoint() { return orig ? rsPoint(orig->x, orig->y) : rsPoint(0, 0); }
	rsPoint GetUL() { return ul; }
	rsPoint GetLR() { return lr; }
	rsRect & GetOuter() { return outer; }
	uint64 GetArea();
	size_t GetNeighborCount() { return nbIds.size(); }
	uint64 GetNN() { return orig ? orig->GetNN() : 0; }
	void SetData(rsParticle &other)
	{
		orig->x = other.GetOrigin()->GetX();
		orig->y = other.GetOrigin()->GetY();
		fill = other.fill;
		perim = other.perim;
		ul = other.ul;
		lr = other.lr;
	}
//	uint64 minsqd(rsParticle & other);
	int closer_than(rsParticle &other, uint64 maxsqd);
	int IsNeighbor(uint64 id) {
		for (size_t i = 0; i < nbIds.size(); i++)
			if (id == nbIds[i]) return 1;
		return 0;
	}
	void AddNeighbor(uint64 id) { nbIds.push_back(id); }
	void clear()
	{
		fill.clear();
		perim.clear();
		nbIds.clear();
		if (orig) {
			lr.x = ul.x = orig->x;
			lr.y = ul.y = orig->y;
		}
		else {
			lr.x = ul.x = lr.y = ul.y = 0;
		}
	}
	uint64 intersect(rsParticle &other);
};

class rsImage
{
private:
	byte *buffer;
	int allocated;
	uint64 width;
	uint64 height;
	std::string format;

	void FindParticleSegment(rsParticle &pt, uint64 x, uint64 y, byte c);
	void FindPerimPoint(rsParticle &pt, uint64 x, uint64 y, byte c);

public:
	rsImage() { this->buffer = NULL; this->format = "tif"; }
	rsImage(byte *buffer, uint64 width, uint64 height);
	virtual ~rsImage();

	void set_data(byte *buffer, uint64 width, uint64 height) {
		if (this->buffer) delete[] this->buffer;
		this->buffer = buffer;
		this->width = width;
		this->height = height;
		this->allocated = 0;
	}

	byte *GetImportPointer() { return buffer; }
	uint64 GetWidth() { return width; }
	uint64 GetHeight() { return height; }

	void fill(byte bkg) {
		if (buffer) memset(buffer, bkg, width*height * sizeof(byte));
	}
	void sanitize(byte oc, byte nc);

	// Replace bkg pixels on the image border with fgd
	void FillBorder(byte bkg, byte fgd);

	void RescanParticle(rsParticle & opt, uint64 expand = EXPAND);
	rsParticle FindParticle(rsParticleOrigin & orig, uint64 expand=EXPAND);

	void PaintParticle(rsParticle & pt, byte c);
	uint64 ParticleArea(rsParticle & pt, byte c=0);

	std::string & GetFormat() { return format; }
	void SetFormat(std::string & format) { this->format = format; }
};

size_t first_overlap(std::vector<rsParticle> & particles, uint64 y);
void find_neighbors(std::vector<rsParticle> & particles, uint64 maxsqd, clock_t t_start);
void compute_particle_parameters(rsParticlePar &p);

#endif

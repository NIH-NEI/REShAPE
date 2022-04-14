#ifndef rsImage_h
#define rsImage_h

#include "rsGeom.h"

class Raster8 {
public:
	int w, h;
	uint8* buf;
	uint64 len;
	bool is_temp;
	Raster8(int w, int h, unsigned char* buf)
	{
		this->w = w;
		this->h = h;
		this->len = (uint64)(this->w) * this->h;
		if (!buf) {
			this->buf = new unsigned char[len];
			is_temp = true;
		}
		else {
			this->buf = buf;
			is_temp = false;
		}
	}
	~Raster8() { if (is_temp) delete[] buf; }
	Boundary getBoundary() { return Boundary(0, 0, w - 1, h - 1); }
	uint8* scanLine(int y) { return buf + ((uint64)(w)*y); }
	void fill(uint8 c) { memset(buf, c, len); }
	uint8 value(int x, int y) { return *(scanLine(y) + x); }
	void setValue(int x, int y, uint8 c) { *(scanLine(y) + x) = c; }
	void clip(Boundary& bnd, int bord = 0) {
		if (bnd.xmin < bord) bnd.xmin = bord;
		if (bnd.xmax >= w - bord) bnd.xmax = w - 1 - bord;
		if (bnd.ymin < bord) bnd.ymin = bord;
		if (bnd.ymax >= h - bord) bnd.ymax = h - 1 - bord;
	}
	void fillBorder(uint8 c, int bsz = 1);
	void replaceColor(uint8 oldc, uint8 newc) {
		uint8* p = buf;
		for (int64 i = 0; uint64(i) < len; i++) {
			if (*p == oldc) *p++ = newc;
			else ++p;
		}
	}
	void sanitize(uint8 newc, uint8 keep) {
		uint8* p = buf;
		for (int64 i = 0; uint64(i) < len; i++) {
			if (*p != keep) *p++ = newc;
			else ++p;
		}
	}

	void findParticleFill(std::vector<HSeg>& fill, int x0, int y0, uint8 newc);
	uint64 detectParticle(Particle& ptc, int x0, int y0, uint8 newc);
	void paintParticleFill(std::vector<HSeg>& fill, uint8 c);
	void paintParticle(Particle& ptc, uint8 c) { paintParticleFill(ptc.fill, c); }
	void paintParticleADD(Particle& ptc, int incr);

protected:
	inline void paintHSeg(HSeg& hs, unsigned char c) {
		unsigned char* p = scanLine(hs.y);
		for (int x = hs.xl; x <= hs.xr; x++) p[x] = c;
	}
	void findFillSegments(std::vector<HSeg>& fill, int y, int xl, int xr,
		unsigned char oldc, unsigned char newc);
};

#endif

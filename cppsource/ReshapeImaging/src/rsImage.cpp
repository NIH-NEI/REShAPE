#include "rsImage.h"

// ---------- Raster8 -------------

void Raster8::fillBorder(uint8 c, int bsz)
{
	int x, y;
	for (y = 0; y < bsz; y++) {
		memset(scanLine(y), c, w);
		memset(scanLine(h - 1 - y), c, w);
	}
	for (y = bsz; y < h - bsz; y++) {
		uint8* p = scanLine(y);
		for (x = 0; x < bsz; x++) {
			p[w - 1 - x] = p[x] = c;
		}
	}
}

void Raster8::findFillSegments(std::vector<HSeg>& fill, int y, int xl, int xr,
	uint8 oldc, uint8 newc)
{
	uint8* p = scanLine(y);
	if (p[xl] == oldc) {
		for (; xl >= 0; xl--) if (p[xl] != oldc) break;
		++xl;
	}
	if (p[xr] == oldc) {
		for (; xr < w; xr++) if (p[xr] != oldc) break;
		--xr;
	}
	HSeg hs;
	hs.y = y;
	bool is_in = false;
	for (int x = xl; x <= xr; x++) {
		if (p[x] == oldc) {
			if (!is_in) {
				is_in = true;
				hs.xl = x;
			}
			hs.xr = x;
		}
		else {
			if (is_in) {
				is_in = false;
				paintHSeg(hs, newc);
				fill.push_back(hs);
			}
		}
	}
	if (is_in) {
		paintHSeg(hs, newc);
		fill.push_back(hs);
	}
}

void Raster8::findParticleFill(std::vector<HSeg>& fill, int x0, int y0, uint8 newc)
{
	uint8* p = scanLine(y0);
	uint8 oldc = p[x0];
	int xl, xr;
	for (xl = x0; xl >= 0; xl--)
		if (p[xl] != oldc) break;
	++xl;
	for (xr = x0; xr < w; xr++)
		if (p[xr] != oldc) break;
	--xr;

	HSeg hs = HSeg(y0, xl, xr);
	paintHSeg(hs, newc);

	fill.push_back(hs);
	size_t idx = 0;
	while (idx < fill.size()) {
		hs = fill[idx];
		++idx;
		if (hs.y > 0) {
			findFillSegments(fill, hs.y - 1, hs.xl, hs.xr, oldc, newc);
		}
		if (hs.y < h - 1) {
			findFillSegments(fill, hs.y + 1, hs.xl, hs.xr, oldc, newc);
		}
	}
}

uint64 Raster8::detectParticle(Particle& ptc, int x0, int y0, uint8 newc)
{
	ptc.clear();
	findParticleFill(ptc.fill, x0, y0, newc);
	return ptc.update_from_fill();
}

void Raster8::paintParticleFill(std::vector<HSeg>& fill, uint8 c)
{
	for (HSeg& hs : fill) {
		uint8* p = scanLine(hs.y);
		for (int x = hs.xl; x <= hs.xr; x++) p[x] = c;
	}
}

void Raster8::paintParticleADD(Particle& ptc, int incr)
{
	for (HSeg& hs : ptc.fill) {
		uint8* p = scanLine(hs.y);
		for (int x = hs.xl; x <= hs.xr; x++) {
			int c = int(p[x]) + incr;
			p[x] = (uint8)c;
		}
	}
}
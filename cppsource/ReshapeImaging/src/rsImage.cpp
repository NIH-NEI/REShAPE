
#include <memory.h>
#define NO_EXTERNS
#include "rsImage.h"

rsDisplayPar visualParams[] = {
	{offsetof(rsParticlePar, Area), "Area", "Area"},
	{offsetof(rsParticlePar, Peri), "Peri", "Perimeter"},
	{offsetof(rsParticlePar, AoP), "AoP", "Area/Perimeter"},
	{offsetof(rsParticlePar, PoN), "PoN", "Perimeter/Neighbors"},
	{offsetof(rsParticlePar, EllipMaj), "EllipMaj", "Ellipse Major Axis"},
	{offsetof(rsParticlePar, ElipMin), "ElipMin", "Ellipse Minor Axis"},
	{offsetof(rsParticlePar, AR), "AR", "Ellipse Aspect Ratio"},
	{offsetof(rsParticlePar, Angle), "Angle", "Ellipse Angle"},
	{offsetof(rsParticlePar, Feret), "Feret", "Feret Major"},
	{offsetof(rsParticlePar, MinFeret), "MinFeret", "Feret Minor"},
	{offsetof(rsParticlePar, FeretAR), "FeretAR", "Feret Aspect Ratio"},
	{offsetof(rsParticlePar, FeretAngle), "FeretAngle", "Feret Angle"},
	{offsetof(rsParticlePar, Circ), "Circ", "Circularity"},
	{offsetof(rsParticlePar, Solidity), "Solidity", "Solidity"},
	{offsetof(rsParticlePar, Compactness), "Compactness", "Compactness"},
	{offsetof(rsParticlePar, Extent), "Extent", "Extent"},
	{offsetof(rsParticlePar, Poly_SR), "Poly_SR", "Polygon Side Ratio"},
	{offsetof(rsParticlePar, Poly_AR), "Poly_AR", "Polygon Area Ratio"},
	{offsetof(rsParticlePar, Poly_Ave), "Poly_Ave", "Polygonality Score"},
	{offsetof(rsParticlePar, Hex_SR), "Hex_SR", "Hexagon Side Ratio"},
	{offsetof(rsParticlePar, Hex_AR), "Hex_AR", "Hexagon Area Ratio"},
	{offsetof(rsParticlePar, Hex_Ave), "Hex_Ave", "Hexagonality Score"}

};
size_t numVisualParams = sizeof visualParams / sizeof(rsDisplayPar);

std::ostream& operator << (std::ostream & out, rsRect &r) {
	out << "rsRect(" << r.x0 << "-" << r.x1 << ", " << r.y0 << "-" << r.y1 << ")";
	return out;
}

void rsParticle::translate(rsParticle &srcpt, int dx, int dy)
{
	orig = srcpt.orig;
	if (orig) {
		orig->SetData(orig->GetId(), orig->GetX() + dx, orig->GetY() + dy, orig->GetNN());
	}
	size_t i;
	fill.resize(srcpt.fill.size());
	for (i = 0; i < fill.size(); i++) {
		rsHSeg & shs = srcpt.fill[i];
		rsHSeg & hs = fill[i];
		hs.x0 = shs.x0 + dx;
		hs.x1 = shs.x1 + dx;
		hs.y = shs.y + dy;
	}
	perim.resize(srcpt.perim.size());
	for (i = 0; i < perim.size(); i++) {
		perim[i].x = srcpt.perim[i].x + dx;
		perim[i].y = srcpt.perim[i].y + dy;
	}
	ul.x = srcpt.ul.x + dx;
	ul.y = srcpt.ul.y + dy;
	lr.x = srcpt.lr.x + dx;
	lr.y = srcpt.lr.y + dy;
	outer.x0 = srcpt.outer.x0 + dx;
	outer.x1 = srcpt.outer.x1 + dx;
	outer.y0 = srcpt.outer.y0 + dy;
	outer.y1 = srcpt.outer.y1 + dy;
	nbIds.resize(0);

}

uint64 rsParticle::GetArea()
{
	uint64 area = 0;
	for (size_t pos = 0; pos < GetFillSize(); pos++) {
		rsHSeg s = GetFillAt(pos);
		area += (s.x1 - s.x0 + 1);
	}
	return area;
}

uint64 rsParticle::intersect(rsParticle &other)
{
	uint64 area = 0;
	size_t pos, o_pos;

	for (pos = 0, o_pos = 0; pos < GetFillSize(); pos++) {
		rsHSeg s = GetFillAt(pos);
		for (; o_pos < other.GetFillSize(); o_pos++) {
			rsHSeg o_s = other.GetFillAt(o_pos);
			if (o_s.y >= s.y) break;
		}
		if (o_pos >= other.GetFillSize()) break;
		for (size_t j = o_pos; j < other.GetFillSize(); j++) {
			rsHSeg o_s = other.GetFillAt(j);
			if (o_s.y > s.y) break;
			uint64 x0 = o_s.x0;
			if (x0 < s.x0) x0 = s.x0;
			uint64 x1 = o_s.x1;
			if (x1 > s.x1) x1 = s.x1;
			if (x0 <= x1)
				area += (x1 - x0 + 1);
		}
	}
	return area;
}

/*
uint64 rsParticle::minsqd(rsParticle & other)
{
	uint64 sqd = 1000000000;
	rsRect rx = this->outer.intersect(other.outer);
	for (size_t j = 0; j < this->GetPerimSize(); j++) {
		const rsPoint & pt1 = this->GetPerimAt(j);
		if (!rx.contains(pt1)) continue;
		for (size_t i = 0; i < other.GetPerimSize(); i++) {
//			if (!rx.contains(pt2)) continue;
			uint64 s = pt1.sqdist(other.GetPerimAt(i));
			if (s < sqd) sqd = s;
		}
	}
	return sqd;
}
*/

int rsParticle::closer_than(rsParticle &other, uint64 maxsqd)
{
	rsRect rx = this->outer.intersect(other.outer);
	for (size_t j = 0; j < this->GetPerimSize(); j++) {
		rsPoint & pt1 = this->GetPerimAt(j);
		if (!rx.contains(pt1)) continue;
		for (size_t i = 0; i < other.GetPerimSize(); i++) {
			if(pt1.sqdist(other.GetPerimAt(i)) < maxsqd)
				return 1;
		}
	}
	return 0;
}

rsImage::rsImage(byte *buffer, uint64 width, uint64 height)
{
	format = "tif";
	size_t sz = width * height;
	this->buffer = new byte[sz];
	this->allocated = 1;
	this->width = width;
	this->height = height;
	memcpy(this->buffer, buffer, sz * sizeof(byte));
}

rsImage::~rsImage()
{
	if (allocated)
		delete[] this->buffer;
}

static inline byte * replace_color(byte *p, byte bkg, byte fgd) {
	if (bkg == *p) *p = fgd;
	++p;
	return p;
}

// Replace any color!=oc with nc
void rsImage::sanitize(byte oc, byte nc)
{
	if (!buffer) return;
	byte *p = buffer;
	uint64 size = width * height;
	for (uint64 i = 0; i < size; i++,p++) {
		if (*p != oc) *p = nc;
	}
}

void rsImage::FillBorder(byte bkg, byte fgd)
{
	uint64 row, col, off;
	byte *p;
	for (col = 0, p = buffer; col < width; col++)
		p = replace_color(p, bkg, fgd);
	for (row = 1; row < height - 1; row++) {
		off = row * width + (width - 1);
		p = replace_color(buffer + off, bkg, fgd);
		replace_color(p, bkg, fgd);
	}
	off = (height - 1)*width;
	for (col = 0, p = buffer + off; col < width; col++)
		p = replace_color(p, bkg, fgd);
}

void rsImage::FindParticleSegment(rsParticle &pt, uint64 x, uint64 y, byte c)
{
	if (pt.GetFillSize() > 500) {
		pt.clear();
		return;
	}
	byte *p = buffer + (y * width);
	if (p[x] != 0) return;

	if (y < pt.ul.y) pt.ul.y = y;
	if (y > pt.lr.y) pt.lr.y = y;

	uint64 x0, x1;
	for (x0 = x; x0 >= 0; x0--)
		if (p[x0] != 0) {
			++x0;
			break;
		}
	for (x1 = x + 1; x1 < width; x1++)
		if (p[x1] != 0) {
			--x1;
			break;
		}

	for (x = x0; x <= x1; x++)
		p[x] = c;
	pt.AddFill(rsHSeg(y, x0, x1));

	if (x0 < pt.ul.x) pt.ul.x = x0;
	if (x1 > pt.lr.x) pt.lr.x = x1;

	p = buffer + ((y - 1) * width);
	for (x = x0; x <= x1; x++) {
		if (p[x] == 0)
			FindParticleSegment(pt, x, y - 1, c);
	}

	p = buffer + ((y + 1) * width);
	for (x = x0; x <= x1; x++) {
		if (p[x] == 0)
			FindParticleSegment(pt, x, y + 1, c);
	}
}

void rsImage::FindPerimPoint(rsParticle &pt, uint64 x, uint64 y, byte c)
{
	byte *pp = buffer + (y * width);
	if (pp[x] != c) return;
	pp[x] = 0;

	int cnt = 0;
	uint64 xx, yy;
	for (xx=x-1; xx<=x+1; xx++)
		for (yy = y - 1; yy <= y + 1; yy++) {
			byte *p = buffer + (yy*width + xx);
			if (*p != 0 && *p != c) ++cnt;
		}

	if (cnt == 0) {
		pp[x] = c;
		return;
	}

	if (cnt > 3)
		pt.perim.push_back(rsPoint(x, y));

	for (xx = x - 1; xx <= x + 1; xx++)
		for (yy = y - 1; yy <= y + 1; yy++) {
			byte *p = buffer + (yy*width + xx);
			if (*p == c) FindPerimPoint(pt, xx, yy, c);
		}

}

void rsImage::RescanParticle(rsParticle & opt, uint64 expand)
{
	rsParticleOrigin norig;
	uint64 id = opt.GetId();
	uint64 ox = opt.GetOrigin()->GetX();
	uint64 oy = opt.GetOrigin()->GetY();
	uint64 oarea = opt.GetArea() / 8;
	if (oarea < 50) oarea = 50;
	norig.SetData(id, opt.GetOrigin()->GetX(), opt.GetOrigin()->GetY());
	byte *p;

	for (size_t row = 0; row < opt.GetFillSize(); row++) {
		rsHSeg hs = opt.GetFillAt(row);
		uint64 y = hs.y;
		p = buffer + (y * width);
		for (uint64 x = hs.x0; x <= hs.x1; x++)
		if (p[x] == 0) {
			norig.SetData(id, x, y);
			rsParticle pt = FindParticle(norig, expand);
			if (pt.GetArea() < oarea) {
				// std::cout << "Particle " << std::to_string(id) << " new area " << std::to_string(pt.GetArea()) << " vs " << std::to_string(oarea) << std::endl;
				// PaintParticle(pt, 0);
				break;
			}

			rsPoint ul = pt.GetUL();
			uint64 ny = ul.y;
			uint64 nx = 0xFFFFFFFF;
			for (size_t nr = 0; nr < pt.fill.size(); nr++) {
				rsHSeg nhs = pt.fill[nr];
				if (nhs.y != ny) continue;
				if (nhs.x0 < nx) nx = nhs.x0;
			}
			if (nx != ox || ny != oy) {
				norig.SetData(id, nx, ny);
				opt.SetData(pt);
				// PaintParticle(opt, 0x55);
			}
			return;
		}
	}
}

rsParticle rsImage::FindParticle(rsParticleOrigin & orig, uint64 expand)
{
	rsParticle pt(orig);
	pt.clear();

	// byte c = (orig.GetX() + orig.GetY()) % 200 + 20;
	byte c = 0xAA;
	FindParticleSegment(pt, orig.GetX(), orig.GetY(), c);

	uint64 y0 = pt.ul.y;
	uint64 x0 = pt.lr.x;
	for (size_t i = 0; i < pt.fill.size(); i++) {
		rsHSeg & sg = pt.fill[i];
		if (sg.y == y0)
			if (sg.x0 < x0) x0 = sg.x0;
	}
	FindPerimPoint(pt, x0, y0, c);

	pt.outer.x0 = pt.ul.x > expand ? pt.ul.x - expand : 0;
	pt.outer.x1 = pt.lr.x + expand < width ? pt.lr.x + expand : width - 1;
	pt.outer.y0 = pt.ul.y > expand ? pt.ul.y - expand : 0;
	pt.outer.y1 = pt.lr.y + expand < height ? pt.lr.y + expand : height - 1;

/* DEBUG
	for (size_t si = 0; si < pt.fill.size(); si++) {
		rsHSeg & seg = pt.fill[si];
		byte *p = buffer + (seg.y * width);
		for (uint64 x = seg.x0; x <= seg.x1; x++) p[x] = c;
	}
	for (size_t pi = 0; pi < pt.perim.size(); pi++) {
		rsPoint & pn = pt.perim[pi];
		buffer[(pn.y * width) + pn.x] = 0;
	}
*/

	return pt;
}

void rsImage::PaintParticle(rsParticle & pt, byte c)
{
	for (size_t si = 0; si < pt.fill.size(); si++) {
		rsHSeg & seg = pt.fill[si];
		byte *p = buffer + (seg.y * width);
		for (uint64 x = seg.x0; x <= seg.x1; x++) p[x] = c;
	}
}

uint64 rsImage::ParticleArea(rsParticle & pt, byte c)
{
	uint64 a = 0;
	for (size_t si = 0; si < pt.fill.size(); si++) {
		rsHSeg & seg = pt.fill[si];
		byte *p = buffer + (seg.y * width);
		for (uint64 x = seg.x0; x <= seg.x1; x++)
			if (p[x] == c) ++a;
	}
	return a;
}

size_t first_overlap(std::vector<rsParticle> & particles, uint64 y)
{
	size_t iLow = 0;
	size_t iHigh = particles.size();
	if (iHigh == 0) return 0;
	--iHigh;
	while (iLow < iHigh) {
		size_t iMid = (iLow + iHigh) / 2;
		if (particles[iMid].GetOuter().y1 < y)
			iLow = iMid + 1;
		else
			iHigh = iMid;
	}
	return iLow;
}

void find_neighbors(std::vector<rsParticle> & particles, uint64 maxsqd, clock_t t_start)
{
	size_t np = particles.size();
	if (np <= 1) return;
	double nextperc = 25.;
	for (size_t j = 0; j < np; j++) {
		rsParticle & pt1 = particles[j];
		rsRect & r1 = pt1.GetOuter();

		for (size_t i = first_overlap(particles, r1.y0); i < np; i++) {
			rsParticle & pt2 = particles[i];
			if (pt2.GetId() == pt1.GetId()) continue;
			rsRect & r2 = pt2.GetOuter();
			if (r2.y0 > r1.y1) break;
			if (!r2.overlaps(r1) || pt1.IsNeighbor(pt2.GetId())) continue;

			if (pt1.closer_than(pt2, maxsqd)) {
				pt1.AddNeighbor(pt2.GetId());
				pt2.AddNeighbor(pt1.GetId());
			}
		}

		double perc = j * 100. / particles.size();
		if (perc >= nextperc) {
			clock_t t_end = clock();
			double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
			std::cout << "<" << elapsed << "> " << j << " of " << particles.size() << " particles" << std::endl;
			nextperc += 10.;
		}
	}
}

void compute_particle_parameters(rsParticlePar &p)
{
	p.AoP = p.Area / p.Peri;
	p.FeretAR = p.Feret / p.MinFeret;
	p.Compactness = sqrt((4. / PI) * p.Area) / p.EllipMaj;
	p.Extent = p.Area / ((p.Width)*(p.Height));
	p.A_hull = p.Area / p.Solidity;
	p.P_hull = 6. * sqrt(p.A_hull / (1.5*sqrt(3.)));
	if (p.neighbors > 0)
		p.PoN = p.Peri / p.neighbors;
	else
		p.PoN = NaN;
	p.Poly_SR = p.Poly_AR = p.Poly_Ave = p.Hex_SR = p.Hex_AR = p.Hex_Ave = NaN;
	if (p.neighbors < 3) return;

	// Polygonality metrics calculated based on the number of sides of the polygon
	p.Poly_SR = 1. - sqrt((1. - (p.PoN / (sqrt((4. * p.Area) / (p.neighbors * (1. / (tan(PI / p.neighbors))))))))*(1. - (p.PoN / (sqrt((4. * p.Area) / (p.neighbors * (1. / (tan(PI / p.neighbors)))))))));
	p.Poly_AR = 1. - sqrt((1. - (p.Area / (0.25*p.neighbors * p.PoN * p.PoN * (1. / (tan(PI / p.neighbors))))))*(1. - (p.Area / (0.25*p.neighbors * p.PoN * p.PoN * (1. / (tan(PI / p.neighbors)))))));
	p.Poly_Ave = 10. * (p.Poly_SR + p.Poly_AR) / 2.;

	//Hexagonality metrics calculated based on a convex, regular, hexagon
	int ib, ic, jv;

	// Unique area calculations from the derived and primary measures above
	double apoth1 = sqrt(3.)*p.Peri / 12.;
	double apoth2 = sqrt(3.)*p.Feret / 4.;
	double apoth3 = p.MinFeret / 2.;
	double side1 = p.Peri / 6.;
	double side2 = p.Feret / 2.;
	double side3 = p.MinFeret / sqrt(3.);
	double side4 = p.P_hull / 6.;

	double Area_uniq[11];
	Area_uniq[0] = 0.5*(3. * sqrt(3.))*side1*side1;
	Area_uniq[1] = 0.5*(3. * sqrt(3.))*side2*side2;
	Area_uniq[2] = 0.5*(3. * sqrt(3.))*side3*side3;
	Area_uniq[3] = 3. * side1*apoth2;
	Area_uniq[4] = 3. * side1*apoth3;
	Area_uniq[5] = 3. * side2*apoth3;
	Area_uniq[6] = 3. * side4*apoth1;
	Area_uniq[7] = 3. * side4*apoth2;
	Area_uniq[8] = 3. * side4*apoth3;
	Area_uniq[9] = p.A_hull;
	Area_uniq[10] = p.Area;

	double Area_ratio = 0.;
	jv = 0;
	for (ib = 0; ib < 11; ib++) {
		for (ic = ib + 1; ic < 11; ic++) {
			Area_ratio += 1. - sqrt((1. - (Area_uniq[ib] / Area_uniq[ic]))*(1. - (Area_uniq[ib] / Area_uniq[ic])));
			++jv;
		}
	}
	p.Hex_AR = Area_ratio / jv;

	// Perimeter Ratio Calculations
	// Two extra apothems are now useful                 
	double apoth4 = sqrt(3.)*p.P_hull / 12.;
	double apoth5 = sqrt(4. * p.A_hull / (4.5*sqrt(3.)));

	double Perim_uniq[14];
	Perim_uniq[0] = sqrt(24. * p.Area / sqrt(3.));
	Perim_uniq[1] = sqrt(24. * p.A_hull / sqrt(3.));
	Perim_uniq[2] = p.Peri;
	Perim_uniq[3] = p.P_hull;
	Perim_uniq[4] = 3. * p.Feret;
	Perim_uniq[5] = 6. * p.MinFeret / sqrt(3.);
	Perim_uniq[6] = 2. * p.Area / apoth1;
	Perim_uniq[7] = 2. * p.Area / apoth2;
	Perim_uniq[8] = 2. * p.Area / apoth3;
	Perim_uniq[9] = 2. * p.Area / apoth4;
	Perim_uniq[10] = 2. * p.Area / apoth5;
	Perim_uniq[11] = 2. * p.A_hull / apoth1;
	Perim_uniq[12] = 2. * p.A_hull / apoth2;
	Perim_uniq[13] = 2. * p.A_hull / apoth3;

	double Perim_ratio = 0.;
	jv = 0;
	for (ib = 0; ib < 14; ib++) {
		for (ic = ib + 1; ic < 14; ic++) {
			Perim_ratio += 1 - sqrt((1. - (Perim_uniq[ib] / Perim_uniq[ic]))*(1. - (Perim_uniq[ib] / Perim_uniq[ic])));
			++jv;
		}
	}
	p.Hex_SR = Perim_ratio / jv;

	p.Hex_Ave = 10. * (p.Hex_AR + p.Hex_SR) / 2.;
}


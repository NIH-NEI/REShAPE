#include <string>
#define NO_EXTERNS
#include "rsparticle.h"

// ,Area,Mean,StdDev,Mode,Min,Max,X,Y,XM,YM,Perim.,BX,BY,Width,Height,Major,Minor,Angle,Circ.,
//Feret,IntDen,Median,Skew,Kurt,%Area,RawIntDen,FeretX,FeretY,FeretAngle,MinFeret,AR,Round,Solidity,
//XStart,YStart,Area/Perim.,Ferets AR,Compactness,Extent,Neighbors,Area Convext Hull,
//Perim Convext Hull,PoN,PSR,PAR,Poly_Ave,HSR,HAR,Hex_Ave
rsDisplayPar visualParams[] = {
	{offsetof(rsParticlePar, Area), "Area", "Area", "Area"},
	{offsetof(rsParticlePar, Peri), "Peri", "Perim.", "Perimeter"},
	{offsetof(rsParticlePar, AoP), "AoP", "Area/Perim.", "Area/Perimeter"},
	{offsetof(rsParticlePar, PoN), "PoN", "PoN", "Perimeter/Neighbors"},
	{offsetof(rsParticlePar, EllipMaj), "EllipMaj", "Major", "Ellipse Major Axis"},
	{offsetof(rsParticlePar, ElipMin), "ElipMin", "Minor", "Ellipse Minor Axis"},
	{offsetof(rsParticlePar, AR), "AR", "AR", "Ellipse Aspect Ratio"},
	{offsetof(rsParticlePar, Angle), "Angle", "Angle", "Ellipse Angle"},
	{offsetof(rsParticlePar, Feret), "Feret", "Feret", "Feret Major"},
	{offsetof(rsParticlePar, MinFeret), "MinFeret", "MinFeret", "Feret Minor"},
	{offsetof(rsParticlePar, FeretAR), "FeretAR", "Ferets AR", "Feret Aspect Ratio"},
	{offsetof(rsParticlePar, FeretAngle), "FeretAngle", "FeretAngle", "Feret Angle"},
	{offsetof(rsParticlePar, Circ), "Circ", "Circ.", "Circularity"},
	{offsetof(rsParticlePar, Solidity), "Solidity", "Solidity", "Solidity"},
	{offsetof(rsParticlePar, Compactness), "Compactness", "Compactness", "Compactness"},
	{offsetof(rsParticlePar, Extent), "Extent", "Extent", "Extent"},
	{offsetof(rsParticlePar, Poly_SR), "Poly_SR", "PSR", "Polygon Side Ratio"},
	{offsetof(rsParticlePar, Poly_AR), "Poly_AR", "PAR", "Polygon Area Ratio"},
	{offsetof(rsParticlePar, Poly_Ave), "Poly_Ave", "Poly_Ave", "Polygonality Score"},
	{offsetof(rsParticlePar, Hex_SR), "Hex_SR", "HSR", "Hexagon Side Ratio"},
	{offsetof(rsParticlePar, Hex_AR), "Hex_AR", "HAR", "Hexagon Area Ratio"},
	{offsetof(rsParticlePar, Hex_Ave), "Hex_Ave", "Hex_Ave", "Hexagonality Score"}

};
size_t numVisualParams = sizeof visualParams / sizeof(rsDisplayPar);

std::ostream& operator << (std::ostream & out, rsRect &r) {
	out << "rsRect(" << std::to_string(r.x0) << "-" << std::to_string(r.x1) << ", " << std::to_string(r.y0) << "-" << std::to_string(r.y1) << ")";
	return out;
}

std::ostream& operator << (std::ostream & out, rsPoint &p)
{
	out << "rsPoint(" << std::to_string(p.x) << "," << std::to_string(p.y) << ")";
	return out;
}

// Translate from big image coordinates to tile coordinates
rsParticle::rsParticle(const rsParticle & other, const rsRect r)
{
	id = other.id;
	fill.resize(other.fill.size());
	for (size_t i = 0; i < other.fill.size(); i++) {
		rsHSeg s = other.fill[i];
		s.x0 -= r.x0;
		s.x1 -= r.x0;
		s.y -= r.y0;
		fill[i] = s;
	}
	ul.x = other.ul.x - r.x0;
	ul.y = other.ul.y - r.y0;
	lr.x = other.lr.x - r.x0;
	lr.y = other.lr.y - r.y0;
	xc = other.xc - r.x0;
	yc = other.yc - r.y0;
	p = other.p;
}

void rsParticle::validate()
{
	if (fill.size() == 0) return;
	uint64 xc = 0;
	uint64 yc = 0;
	int nx = 0, ny = 0;
	for (size_t i = 0; i < fill.size(); i++) {
		rsHSeg & s = fill[i];
		int dx = (int)(s.x1 - s.x0 + 1);
		xc += (s.x0 + s.x1) * dx / 2;
		nx += dx;
		yc += s.y;
		ny += 1;
		if (i == 0) {
			ul.y = lr.y = s.y;
			ul.x = s.x0;
			lr.x = s.x1;
			continue;
		}
		if (s.y < ul.y) ul.y = s.y;
		if (s.y > lr.y) lr.y = s.y;
		if (s.x0 < ul.x) ul.x = s.x0;
		if (s.x1 > lr.x) lr.x = s.x1;
	}
	this->xc = nx > 0 ? xc / nx : 0;
	this->yc = ny > 0 ? yc / ny : 0;
}

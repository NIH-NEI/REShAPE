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
	{offsetof(rsParticlePar, Hex_Ave), "Hex_Ave", "Hex_Ave", "Hexagonality Score"},
	{offsetof(rsParticlePar, selected), "selected", "Selected", "In Selection"}

};
size_t numVisualParams = sizeof visualParams / sizeof(rsDisplayPar);

std::vector<HSeg> get_particle_fill(int sz)
{
	std::vector<HSeg> fill;

	if (sz > 5 && sz < 25) {
		int ymax = sz - 1;
		for (int y = -ymax; y <= ymax; y++) {
			int xmax = int(sqrt(double(sz) * double(sz) - double(y) * double(y)));
			fill.push_back(HSeg(y, -xmax, xmax));
		}
		return fill;
	}

	switch (sz) {
	case 1:
		fill.push_back(HSeg(0, 0, 1));
		fill.push_back(HSeg(1, 0, 1));
		break;
	case 2:
		fill.push_back(HSeg(-1, -1, 1));
		fill.push_back(HSeg(0, -1, 1));
		fill.push_back(HSeg(1, -1, 1));
		break;
	case 3:
		fill.push_back(HSeg(-2, -1, 1));
		fill.push_back(HSeg(-1, -2, 2));
		fill.push_back(HSeg(0, -2, 2));
		fill.push_back(HSeg(1, -2, 2));
		fill.push_back(HSeg(2, -1, 1));
		break;
	case 4:
		fill.push_back(HSeg(-3, -1, 1));
		fill.push_back(HSeg(-2, -2, 2));
		fill.push_back(HSeg(-1, -3, 3));
		fill.push_back(HSeg(0, -3, 3));
		fill.push_back(HSeg(1, -3, 3));
		fill.push_back(HSeg(2, -2, 2));
		fill.push_back(HSeg(3, -1, 1));
		break;
	case 5:
		fill.push_back(HSeg(-4, -1, 1));
		fill.push_back(HSeg(-3, -3, 3));
		fill.push_back(HSeg(-2, -3, 3));
		fill.push_back(HSeg(-1, -4, 4));
		fill.push_back(HSeg(0, -4, 4));
		fill.push_back(HSeg(1, -4, 4));
		fill.push_back(HSeg(2, -3, 3));
		fill.push_back(HSeg(3, -3, 3));
		fill.push_back(HSeg(4, -1, 1));
		break;
	default:
		fill.push_back(HSeg(0, 0, 0));
		break;
	}

	return fill;
}
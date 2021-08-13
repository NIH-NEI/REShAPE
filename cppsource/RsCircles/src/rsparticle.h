#ifndef rsparticle_h
#define rsparticle_h

#include <memory.h>
#include <cstddef>
#include <iostream>
#include <vector>
#include <algorithm>

#include "rsdef.h"

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
	double xm;
	double ym;
	// internal
	double selected;
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

class rsParticle {
public:
	rsParticlePar p;
	size_t startpos;
	size_t endpos;

	double *parPtr(size_t poff) { return (double *)(((char *)(&p)) + poff); }
};

#endif

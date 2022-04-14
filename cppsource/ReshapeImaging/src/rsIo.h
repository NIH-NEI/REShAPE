#ifndef rsIo_h
#define rsIo_h

#include <QString>
#include <QVector>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QImage>
#include <QImageWriter>
#include <QPainter>

#include "rsGeom.h"
#include "rsImage.h"

struct rsImageTile
{
	size_t n_idx;
	std::string name;
	uint64 x, y;
	uint64 w, h;
	std::string outlines;
	std::string csvdata;

	rsImageTile() {
		n_idx = 0;
		x = y = w = h = 0;
		name = "";
		outlines = "";
		csvdata = "";
	}

	Boundary getBoundary() {
		return Boundary(x, y, x + w - 1, y + h - 1);
	}
};

struct rsTiledImage
{
	std::string error_msg;
	std::string fpath;
	uint64 width, height;
	int numtilerows, numtilecolumns;
	std::string source;
	std::vector<rsImageTile> tiles;
	int unit_conv;
	double unit_pix, unit_real;

	rsTiledImage() { init(); }

	void init() {
		error_msg = "";
		fpath = "";
		width = height = 0;
		numtilerows = numtilecolumns = -1;
		source = "";
		tiles.resize(0);
		unit_conv = 0;
		unit_pix = unit_real = 1.;
	}
	bool open(std::string jFileName);
	int numTiles() { return int(tiles.size()); }
	rsImageTile& getTile(int idx) { return tiles[idx]; }
};

class rsLUT
{
protected:
	QDir ldir;
	QVector<QRgb> palette;
	QString lutName;
	void setAppDir(const QString& appDir);

public:
	rsLUT() : ldir(".") { getDefaultPalette(); }
	rsLUT(const QString& appDir) {
		getDefaultPalette();
		setAppDir(appDir);
	}
	QDir getLutDir() { return ldir; }
	QString getLutName() { return lutName; }
	QVector<QRgb> getPalette() { return palette; }
	QStringList listLutNames();
	QVector<QRgb> getDefaultPalette();
	QVector<QRgb> getGlasbeyPalette();
	QVector<QRgb> loadPalette(QString lname);
	QVector<QRgb> loadPalette(const char* _lname) { return loadPalette(QString(_lname)); }
	void getBlackWhite(int* pb, int* pw);
	QImage generateNeighborLegend(int maxnn, int64 w, int64 h);
	QImage generateParamLegend(double vmin, double vmax, int64 w, int64 h);
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
	const char* label;
	const char* text;
};

#ifndef __rsDisplayPar_main__
extern rsDisplayPar visualParams[];
extern size_t numVisualParams;
#endif

void compute_particle_parameters(rsParticlePar& p);

Raster8* read_image8(std::string fname);
bool write_image8(std::string fname, Raster8* img, const char *fmt = "tif", const QVector<QRgb> *pal = NULL);

#endif
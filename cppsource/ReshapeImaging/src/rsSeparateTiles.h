#ifndef rsSeparateTiles_h
#define rsSeparateTiles_h

#include <iostream>
#include <unordered_map>
#include <time.h>

#include <itkTextOutput.h>
#include "QuickView.h"

#include "rsImageReader.h"
#include "rscsvio.h"
#include "rslutio.h"

#include "itkImageSeriesReader.h"

typedef std::unordered_map<std::string, std::string> Properties;

void write_byte_image(rsImage *img, const char *out_file);
void read_argv(int argc, char **argv, Properties &prop);

class rsSeparateTilesImageReader
{
private:
	std::string error_message;
	QDir imgdir;
	QJsonObject imgobj;
	QString source;
	int numtilerows, numtilecolumns;
	uint64 width, height;
	int unit_conv;
	double unit_pix;
	double unit_real;
	QJsonArray jtiles;
	std::vector<rsImageTile> tiles;

	bool ReadTile(int iTile, const char *out_file);
public:
	const char *GetError() { return error_message.c_str(); }
	bool OpenTiledImage(const char *json_name);
	int readTiles(const char *tiles_dir);
	QString GetSource() { return source; }
	int GetNumTileRows() { return numtilerows; }
	int GetNumTileColumns() { return numtilecolumns; }
	uint64 GetWidth() { return width; }
	uint64 GetHeight() { return height; }
	int GetUnitConv() { return unit_conv; }
	double GetUnitPix() { return unit_pix; }
	double GetUnitReal() { return unit_real; }
	int GetNumTiles() { return jtiles.size(); }
	rsImageTile & GetTileAt(size_t i) { return tiles[i]; }
	rsCsvIO::Pointer GetParticleData(std::vector<rsParticle> &particles);
};

int separate_tiles_main(std::string &app_dir, Properties & param);

#endif

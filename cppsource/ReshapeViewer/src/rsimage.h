#ifndef rsimage_h
#define rsimage_h

#include <QtGlobal>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

#include <itkImage.h>
#include <itkImageFileReader.h>

#include "rsparticle.h"

typedef itk::RGBPixel<byte> RGBPixelType;
typedef itk::Image<RGBPixelType, 2> RGBImageType;
typedef itk::ImageFileReader<RGBImageType> RGBReaderType;

typedef itk::Image<byte, 2>  ImageType2D;
typedef itk::ImageFileReader<ImageType2D> ReaderType2D;

class rsImageTile
{
public:
	bool areas_read;
	std::string name;
	uint64 x, y;
	uint64 width, height;
	inline rsRect GetRect() {
		rsRect r;
		r.x0 = x;
		r.y0 = y;
		r.x1 = x + width - 1;
		r.y1 = y + height - 1;
		return r;
	}
	void FindParticleAreas(ImageType2D *pimg, std::vector<rsParticle> &particles);
};

class rsTiledImageReader
{
private:
	bool areas_read;
	std::string file_name;
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

	bool FindDataFile(QString &dataPath);
public:
	const char *GetFileName() { return file_name.c_str(); }
	const char *GetError() { return error_message.c_str(); }
	bool OpenTiledImage(const char *json_name);
	bool ReadParticleAreas(std::vector<rsParticle> & particles);
	bool ReadParticleData(std::vector<rsParticle> & particles);
	QString GetSource() { return source; }
	int GetNumTileRows() { return numtilerows; }
	int GetNumTileColumns() { return numtilecolumns; }
	uint64 GetWidth() { return width; }
	uint64 GetHeight() { return height; }
	int GetUnitConv() { return unit_conv; }
	double GetUnitPix() { return unit_pix; }
	double GetUnitReal() { return unit_real; }
	int GetNumTiles() { return jtiles.size(); }
	rsImageTile &GetTileAt(size_t iTile) { return tiles[iTile]; }
	QString fullName(const std::string & name) { return imgdir.absoluteFilePath(name.c_str()); }
};

#endif

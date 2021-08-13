
#ifndef rsImageReader_h
#define rsImageReader_h

#include <string>
#include <iomanip>
#include <itkImage.h>
#include <itkImageFileReader.h>

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "rsImage.h"
#include "rscsvio.h"

typedef itk::Image<byte, 2>  ImageType2D;
typedef itk::ImageFileReader<ImageType2D> ReaderType2D;

class rsImageReader
{
protected:
	ImageType2D::Pointer pimg;
	ImageType2D::PixelContainer::Pointer container;
	rsImage img;

	std::string error_message;
public:
	const char *GetError() { return error_message.c_str(); }
	rsImage *ReadImage(const char *img_name);
	ImageType2D *GetItkImage() { return pimg.GetPointer(); }
	void commit() { pimg->SetPixelContainer(container.GetPointer()); }
};

void read_particle_params(rsCsvIO &csvdata, std::vector<rsParticleOrigin> & pOrigs);
std::string write_particle_data(rsCsvIO &csvdata, std::vector<rsParticle> & particles, clock_t t_start, std::string & base_name, std::string & orig_name, std::string & data_dir);

typedef std::pair<rsParticle *, rsCsvRow *> rsParticleDesc;

class rsImageTile
{
private:
	size_t n_idx;
public:
	std::string name;
	uint64 x, y;
	uint64 width, height;
	rsCsvIO::Pointer csvdata;
	std::vector<rsParticleOrigin> pOrigs;
	std::vector<rsParticle> particles;
	std::vector<rsParticleDesc> p_map;

	rsImageTile() { csvdata = rsCsvIO::New(); }
	void ResetRowIter() { n_idx = 0; }
	bool HasNextRow() { return n_idx < p_map.size(); }
	rsParticleDesc *PeekNextRow() { return &(p_map[n_idx]); }
	void PopNextRow() { if (n_idx < p_map.size()) n_idx++; }
};

class rsTiledImageReader : public rsImageReader
{
private:
	QDir imgdir;
	QJsonObject imgobj;
	uint64 width, height;
	int unit_conv;
	double unit_pix;
	double unit_real;
	QJsonArray jtiles;
	std::vector<rsImageTile> tiles;

	bool ReadTile(int iTile);
public:
	rsImage *ReadTiledImage(const char *json_name);
	uint64 GetWidth() { return width; }
	uint64 GetHeight() { return height; }
	int GetNumTiles() { return tiles.size(); }
	rsCsvIO::Pointer GetParticleData(std::vector<rsParticle> & particles);
};

#endif

#ifndef rsMain_h
#define rsMain_h

#include <time.h>
#include "rsGeom.h"
#include "rsIo.h"
#include "csv.h"

#include <QtGlobal>
#include <QString>
#include <QList>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>

const uint64 MAX_UINT64 = 0x7FFFFFFFFFFFFFFFll;

const int64 EXPAND = 6;
const int64 NBSQDIST = 41;

struct rsParticle : Particle
{
	int ptId;
	int tileId;
	double score;
};

struct rsParticleP : Particle
{
	int ptId;
	int nn;
	std::vector<Point> peri;
	CsvRow row;

	int bdist;

	void translate(int x, int y) {
		Particle::translate(x, y);
		for (Point& p : peri) {
			p.x += x;
			p.y += y;
		}
	}
	int64 sqdist(rsParticleP& other) {
		int64 rc = MAX_UINT64;
		Boundary b = bnd.intersection(other.bnd);
		b.expand(5);
		for (Point& op : other.peri) {
			if (!b.IsInside(op)) continue;
			for (Point& p : peri) {
				if (!b.IsInside(p)) continue;
				int64 dx = op.x - p.x;
				int64 dy = op.y - p.y;
				int64 d = dx * dx + dy * dy;
				if (d < rc) rc = d;
			}
		}
		return rc;
	}
};

struct rsTileData
{
	Boundary bnd;
	std::vector<rsParticleP> particles;
	bool valid = false;

	Boundary inner;

	void loadParticles(rsImageTile& tile, Raster8* img);
};

class rsReconstruct
{
protected:
	clock_t t_start;
	Properties param;
	QFileInfo appFile;
	QDir appPath;
	QString appDir;
	rsLUT lut;

	rsTiledImage timg;
	std::vector<std::string> names;
	Raster8* fullImg = NULL;
	std::vector<rsTileData> tds;
	std::vector<rsParticle> particles;
	std::vector<rsParticlePar> ppars;
	CsvRow headers;
	int maxnn;

	std::string orig_name;
	std::string base_name;
	std::string json_name;

	std::string tmp_dir;
	std::string seg_dir;
	std::string data_dir;
	std::string vis_dir;
	std::string lut_choice;
	std::string graphic_choice;
	std::string graphic_format;
	std::string feature_limits;
	std::string img_format;

	std::string dat_name;
	std::string nbr_name;
	uint64 fullsz;
	int64 pt_count;

	clock_t _t_tic;
	clock_t _tic_incr;


	void toc(clock_t __tic_incr = 5) {
		_tic_incr = __tic_incr * CLOCKS_PER_SEC;
		_t_tic = clock() + _tic_incr;
	}
	bool tic() {
		clock_t t_next = clock();
		if (t_next < _t_tic) return false;
		_t_tic += _tic_incr;
		if (_t_tic < t_next) _t_tic = t_next + _tic_incr;
		return true;
	}

	bool need_images() {
		return (!graphic_choice.empty()) && (graphic_choice != "None");
	}
	bool overlayImage(Raster8* img, int x0, int y0);
	void detectParticles(uint64 min_area, uint64 max_area, uint8 bkg=0, uint8 fgd=0xFF);
	int startIndex(int ytop);
	int64 dedupe();
	void find_neighbors(int64 maxsqd=NBSQDIST);
	void read_particle_data();
	int write_particle_data();
	void write_outlines_image();
	void write_neighbors_image();
	void write_parameter_images();

	void handleMultipleTiles();
	void handleSingleTile();
	int reconstructTiles();

	int64 dedupeTiled();

	int separateTiles();

public:
	rsReconstruct(int argc, char** argv);
	~rsReconstruct() { close(); }
	clock_t getStartTs() { return t_start; }
	int open();
	void close();
	int process();
	std::string format_elapsed(clock_t tstart);
};

#endif

#ifndef rslut_h
#define rslut_h

#include <string.h>
#include <memory.h>

#include <QtCore>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QColor>
#include "rsparticle.h"

struct rsRGB {
	byte r, g, b;
};

class rsPalette
{
private:
	std::vector<rsRGB> pal;
	std::string name;
public:
	rsPalette() {}
	bool load(std::string pal_name, std::string lut_dir);
	void loadJetPalette();
	void loadGlasbeyPalette();
	void copy(rsPalette & other) {
		this->name = other.name;
		this->pal = other.pal;
	}
	std::string & GetName() { return name; }
	int GetNumColors() { return (int)pal.size(); }
	void SetNumColors(int ncolors) { pal.resize(ncolors); }
	rsRGB & GetColorAt(int idx) { return pal[idx]; }
	void SetColorAt(int idx, byte c[]) {
		rsRGB rgb;
		rgb.r = c[0];
		rgb.g = c[1];
		rgb.b = c[2];
		if (idx < 0) {
			pal.push_back(rgb);
		}
		else if (idx < (int)pal.size()) {
			pal[idx] = rgb;
		}
	}
};

class rsLUT
{
private:
	std::string lut_dir;
	std::vector<rsPalette> palettes;
	rsPalette glasbey;

public:
	rsLUT();
	void loadPalettes();
	int GetNumPalettes() { return (int)palettes.size(); }
	rsPalette & GetPaletteAt(int idx) { return palettes[idx]; }
	rsPalette * GetGlasbey() { return &glasbey; }
	int FindPalette(std::string pal_name) {
		for (size_t i = 0; i < palettes.size(); i++) {
			if (palettes[i].GetName() == pal_name) return (int)i;
		}
		return -1;
	}
};

void FillParamLegend(QImage *qimg, byte r, byte g, byte b);
void DrawParamLegend(double vmin, double vmax, QImage *qimg, rsPalette *pal);
void DrawNeighborsLegend(int maxn, QImage *qimg, rsPalette *pal);

#endif

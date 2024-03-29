#ifndef rsimageview_h
#define rsimageview_h

#include <QWidget>
#include "rsimage.h"
#include "rslut.h"

class rsImageView
{
protected:
	std::vector<rsParticle> *p_particles = NULL;
	byte pr=0, pg=0x22, pb=0x22;
	double opacity = 1.;
	int vsz = 2;
	std::string par_name = "Outlines";
	int par_idx = 0;
	size_t par_off = 0;
	rsPalette *palette = NULL;
	rsPalette *glasbey = NULL;
	int legend_width = 360;
	double vmin, vmax;
	int maxn;
	QImage *legend = NULL;
	
	virtual void clearImage() = 0;
	void fillParticle(rsParticle &pt, byte r, byte g, byte b);

public:
	rsImageView() {}
	virtual ~rsImageView();

	void reFillParticles();
	void GenerateLegend();

	virtual void ResetView(bool camera_flag = true) = 0; //used for initialization
	virtual void InitializeView() = 0;
	virtual void ImageDataModified() = 0;
//	virtual void SetGrayImage(ImageType2D::Pointer) = 0;
	virtual byte *GetPixelDataPointer(int x, int y) = 0;
	virtual QWidget *GetImageWidget(QWidget *parent = NULL) = 0;

	void SetParticles(std::vector<rsParticle> *p_particles);
	void SetGlasbeyPalette(rsPalette *glasbey) { this->glasbey = glasbey; }
	void SetVisualParameter(int par_idx, double vmin, double vmax);
	void SetPalette(rsPalette *palette);
	void SetLegendWidth(int w);
	QImage *GetLegend() { return legend; }
	void SetOpacity(double v) { opacity = v; }
	void SetParticleSize(int v) { vsz = v; }
};

#endif

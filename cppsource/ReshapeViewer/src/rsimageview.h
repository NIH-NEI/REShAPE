#ifndef rsimageview_h
#define rsimageview_h

#include <QWidget>
#include "rsimage.h"
#include "rslut.h"

class rsImageView
{
protected:
	std::vector<rsParticle> particles;
	byte pr=0, pg=0x22, pb=0x22;
	std::string par_name = "Outlines";
	int par_idx = 0;
	size_t par_off = 0;
	rsPalette *palette = NULL;
	rsPalette *glasbey = NULL;
	int legend_width = 360;
	double vmin, vmax;
	int maxn;
	QImage *legend = NULL;
	
	void fillParticle(rsParticle &pt, byte r, byte g, byte b);
	void reFillParticles();
	void GenerateLegend();

public:
	rsImageView() {}
	virtual ~rsImageView();

	virtual void ResetView(bool camera_flag = true) = 0; //used for initialization
	virtual void InitializeView() = 0;
	virtual void ImageDataModified() = 0;
	virtual void SetGrayImage(ImageType2D::Pointer) = 0;
	virtual byte *GetPixelDataPointer(int x, int y) = 0;
	virtual QWidget *GetImageWidget(QWidget *parent = NULL) = 0;

	void SetParticles(std::vector<rsParticle> &srcp, rsRect br);
	void SetGlasbeyPalette(rsPalette *glasbey) { this->glasbey = glasbey; }
	void SetVisualParameter(int par_idx);
	void SetPalette(rsPalette *palette);
	void SetLegendWidth(int w);
	QImage *GetLegend() { return legend; }
};

#endif

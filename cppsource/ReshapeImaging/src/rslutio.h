#ifndef rslutio_h
#define rslutio_h

#include <algorithm>
#include <QtGlobal>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QImageWriter>

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkPNGImageIOFactory.h"
#include "itkPNGImageIO.h"
#include "itkScalarToRGBColormapImageFilter.h"
#include "itkCustomColormapFunction.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIterator.h"
#include "itkTIFFImageIO.h"

#include "rsImageReader.h"

typedef itk::RGBPixel<byte> RGBPixelType;
typedef itk::Image<RGBPixelType, 2> RGBImageType;
typedef std::vector<RGBPixelType> Palette;

typedef itk::Function::CustomColormapFunction<ImageType2D::PixelType, RGBImageType::PixelType> ColormapType;
typedef itk::ScalarToRGBColormapImageFilter<ImageType2D, RGBImageType> RGBFilterType;

typedef  itk::ImageFileWriter<RGBImageType> WriterTypeRGB;
typedef  itk::TIFFImageIO TIFFIOType;

class rsLUT
{
private:
	std::string lutDir;
	std::string lutName;
	Palette palette;
	uint64 width, height;
	void overlay(QImage &img, RGBImageType::Pointer itkImg);
public:
	rsLUT() { loadDefaultPalette(); }
	rsLUT(std::string appDir, std::string lutName) {
		findLutDir(appDir);
		if (!loadPalette(lutName)) {
			if (lutName == "glasbey")
				loadGlasbeyPalette();
			else
				loadDefaultPalette();
		}
	}
	void findLutDir(std::string appDir);
	bool loadPalette(std::string lutName);
	void loadDefaultPalette();
	void loadGlasbeyPalette();
	Palette & getPalette() { return palette; }
	QVector<QRgb> getColorPalette(int *p_black, int *p_white);
	void setSize(uint64 width, uint64 height) {
		this->width = width;
		this->height = height;
	}
	ColormapType::Pointer getColormap();
	void overlayLegend(double vmin, double vmax, QImage & qimg);
	void overlayNbLegend(int maxn, QImage & qimg);
	RGBImageType::Pointer generateLegend(double vmin, double vmax, RGBImageType::Pointer itkImg = NULL);
	RGBImageType::Pointer generateNbLegend(int maxn, RGBImageType::Pointer itkImg = NULL);
	RGBImageType::Pointer allocItk(rsImage *img);
	void fillItk(rsImage *img, RGBImageType::Pointer &pimg);
	RGBImageType::Pointer rsToItk(rsImage *img);
};

RGBImageType::Pointer binaryToItk(rsImage *img);
RGBImageType::Pointer toItkRgbImage(QImage & img);

#endif

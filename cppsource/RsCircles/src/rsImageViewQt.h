#ifndef rsimageviewqt_h
#define rsimageviewqt_h

#include <math.h>
#include <QApplication>
#include <QScreen>
#include <QImage>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include "rsimageview.h"
#include "rsFigure.h"

class rsImageViewQt;
class rsImageLabel;

class rsScrollArea : public QScrollArea
{
	Q_OBJECT
private:
	rsImageViewQt * v;
	rsImageLabel *imgLabel;
	int screen_height;
	bool drag = false;
	QPoint startPos;
	int xscrl, yscrl;

	QTimer *timer = NULL;
	bool in_progress = false;
	double o_sc;
	double sc;
	QPoint mpos;
private slots:
	void on_timer();
protected:
	virtual void wheelEvent(QWheelEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
public:
	rsScrollArea(rsImageViewQt * v);
	virtual ~rsScrollArea();
	void setWidget(rsImageLabel *lbl) {
		QScrollArea::setWidget((QWidget *)lbl);
		imgLabel = lbl;
	}
};

class rsImageLabel : public QLabel
{
private:
	rsImageViewQt * v;
	double sc = 1.;
	rsFigure * innerCir = NULL;
	rsFigure * outerCir = NULL;
protected:
	virtual void resizeEvent(QResizeEvent *e) override;
	virtual void paintEvent(QPaintEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
public:
	rsImageLabel(rsImageViewQt * v, double sc=1.) : QLabel() {
		this->v = v;
		this->sc = sc;
	}
	void SetInner(rsFigure *innerCir) { this->innerCir = innerCir; if (innerCir) innerCir->SetScale(sc, size().width(), size().height()); }
	void SetOuter(rsFigure *outerCir) { this->outerCir = outerCir; if (outerCir) outerCir->SetScale(sc, size().width(), size().height()); }
	rsFigure *GetInner() { return innerCir; }
	rsFigure *GetOuter() { return outerCir; }
	double GetScale() { return sc; }
	void SetScale(double sc) {
		this->sc = sc;
		QSize sz = size();
		if (innerCir) innerCir->SetScale(sc, sz.width(), sz.height());
		if (outerCir) outerCir->SetScale(sc, sz.width(), sz.height());
	}
	int isHot() {
		if (innerCir && innerCir->isHot()) return 1;
		if (outerCir && outerCir->isHot()) return 1;
		return 0;
	}
	void cancelHot() {
		if (innerCir) innerCir->cancelHot();
		if (outerCir) outerCir->cancelHot();
	}
};

class rsImageViewQt : public rsImageView
{
private:
	rsImageLabel *imgLabel = NULL;
	rsScrollArea *scrollArea = NULL;
	QImage *img = NULL;
	int width, height;
	double sc = 1.;
	bool wait_fl = false;

	void createImage(int width, int height);
public:
	rsImageViewQt() { width = height = 1; }
	rsImageViewQt(int width, int height) { createImage(width, height); }
	virtual ~rsImageViewQt() override;

	double GetFitToViewScale();
	double GetScale() { return sc; }
	void SetScale(double sc);
	void SetInner(rsFigure *innerCir) { imgLabel->SetInner(innerCir); }
	void SetOuter(rsFigure *outerCir) { imgLabel->SetOuter(outerCir); }
	rsFigure *GetInner() { return imgLabel->GetInner(); }
	rsFigure *GetOuter() { return imgLabel->GetOuter(); }
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	void beginWait();
	void endWait();
	void repaint() { imgLabel->repaint(); }

	virtual void ResetView(bool camera_flag = true) override; //used for initialization
	virtual void InitializeView() override;
	void SetParticles(std::vector<rsParticle> *p_particles, int w, int h, double opacity, int vsz);
	virtual void ImageDataModified() override;
	virtual byte *GetPixelDataPointer(int x, int y) override;
	virtual QWidget *GetImageWidget(QWidget *parent = NULL) override;
};

#endif

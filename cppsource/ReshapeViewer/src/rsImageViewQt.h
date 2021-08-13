#ifndef rsimageviewqt_h
#define rsimageviewqt_h

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

class rsImageViewQt;

class rsScrollArea : public QScrollArea
{
	Q_OBJECT
private:
	rsImageViewQt * v;
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
};

class rsImageLabel : public QLabel
{
private:
	rsImageViewQt * v;
protected:
	virtual void resizeEvent(QResizeEvent *e) override;
	virtual void paintEvent(QPaintEvent *e) override;
public:
	rsImageLabel(rsImageViewQt * v) : QLabel() {
		this->v = v;
	}
};

class rsImageViewQt : public rsImageView
{
private:
	QLabel *imgLabel = NULL;
	QScrollArea *scrollArea = NULL;
	QImage *img = NULL;
	int width, height;
	double sc = 1.;
	bool wait_fl = false;

	void createImage(int width, int height);
public:
	rsImageViewQt() { width = height = 1; }
	virtual ~rsImageViewQt() override;

	double GetFitToViewScale();
	double GetScale() { return sc; }
	void SetScale(double sc);
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	void beginWait();
	void endWait();

	virtual void ResetView(bool camera_flag = true) override; //used for initialization
	virtual void InitializeView() override;
	virtual void SetGrayImage(ImageType2D::Pointer) override;
	virtual void ImageDataModified() override;
	virtual byte *GetPixelDataPointer(int x, int y) override;
	virtual QWidget *GetImageWidget(QWidget *parent = NULL) override;
};

#endif

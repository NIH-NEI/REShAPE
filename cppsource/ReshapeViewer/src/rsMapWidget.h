#ifndef rsmapwidget_h
#define rsmapwidget_h

#include <QWidget>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>

#include "rsimage.h"

class rsMapWidget : public QWidget
{
	Q_OBJECT
private:
	int sz;
	QImage *p_img;
	uint64 tgtw;
	uint64 tgth;
	int sc_w, sc_h;
	int nrows, ncols;
	int tile;
	QColor bkg = QColor::fromRgb(0xDD, 0xDD, 0xDD);
	QColor fgd = QColor::fromRgb(0, 0, 0);
	QColor sel = QColor::fromRgb(0xFF, 0xFF, 0);
	QColor pcol = QColor::fromRgb(0x55, 0xAA, 0xAA);

	void UpdateImage();
	void paintEvent(QPaintEvent * event) override;
protected:
	void mouseReleaseEvent(QMouseEvent *event) override;
public:
	rsMapWidget(int sz, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~rsMapWidget();
	void SetScale(uint64 tgtw, uint64 tgth, int ncols, int nrows);
	void SetParticles(std::vector<rsParticle> & particles);
	void SetTile(int tile);
signals:
	void TileChanged(int newTile);
};

#endif

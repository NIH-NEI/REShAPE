#include "rsMapWidget.h"

rsMapWidget::rsMapWidget(int sz, QWidget *parent, Qt::WindowFlags f) :
	QWidget(parent, f)
{
	this->sz = sz;
	sc_w = sc_h = sz;
	nrows = ncols = 1;
	p_img = new QImage(sz, sz, QImage::Format_RGB888);
	setMinimumSize(sz, sz);

	QPainter paint;
	if (paint.begin(p_img)) {
		paint.fillRect(0, 0, sz, sz, fgd);
	}
}

rsMapWidget::~rsMapWidget()
{
	delete p_img;
}

void rsMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	int col = pos.x() * ncols / sc_w;
	int row = pos.y() * nrows / sc_h;
	if (col >= ncols || row >= nrows) return;
	int tileIdx = row * ncols + col;
	if (tileIdx != tile) {
		emit TileChanged(tileIdx);
	}
}

void rsMapWidget::UpdateImage()
{
	QPainter paint;
	if (paint.begin(p_img)) {
		paint.fillRect(0, 0, sz, sz, bkg);
		paint.fillRect(0, 0, sc_w, sc_h, fgd);
	}
}

void rsMapWidget::paintEvent(QPaintEvent *event) {
	QPainter paint(this);
	paint.drawImage(0, 0, *p_img);
	paint.setPen(bkg);
	int col, row;
	for (col = 1; col < ncols; col++) {
		int x = col * sc_w / ncols;
		paint.drawLine(x, 0, x, sc_h - 1);
	}
	for (row = 1; row < nrows; row++) {
		int y = row * sc_h / nrows;
		paint.drawLine(0, y, sc_w - 1, y);
	}
	paint.setPen(sel);
	row = tile / ncols;
	col = tile % ncols;
	int x0 = col * sc_w / ncols;
	int w = (col + 1) * sc_w / ncols - x0 - 1;
	int y0 = row * sc_h / nrows;
	int h = (row + 1) * sc_h / nrows - y0 - 1;
	paint.drawRect(x0, y0, w, h);
	paint.drawRect(x0+1, y0+1, w-2, h-2);
}

void rsMapWidget::SetScale(uint64 tgtw, uint64 tgth, int ncols, int nrows)
{
	this->tgtw = tgtw;
	this->tgth = tgth;
	if (tgtw > tgth) {
		sc_w = sz;
		sc_h = (int)(tgth * sz / tgtw);
	}
	else {
		sc_h = sz;
		sc_w = (int)(tgtw * sz / tgth);
	}
	this->ncols = ncols;
	if (this->ncols < 1) this->ncols = 1;
	this->nrows = nrows;
	if (this->nrows < 1) this->nrows = 1;
	tile = 0;
	UpdateImage();
	update();
	// emit TileChanged(tile);
}

void rsMapWidget::SetParticles(std::vector<rsParticle> & particles)
{
	for (size_t iPart = 0; iPart < particles.size(); iPart++) {
		rsParticle &pt = particles[iPart];
		int px = int(pt.GetXCenter() * sc_w / tgtw);
		if (px < 0 || px >= sc_w) continue;
		int py = int(pt.GetYCenter() * sc_h / tgth);
		if (py < 0 || py >= sc_h) continue;

		byte *p = p_img->scanLine(py) + (px * 3);
		int r = (pcol.red() + p[0]) / 2; if (r > 0xFF) r = 0xFF;
		p[0] = (byte)r;
		int g = (pcol.green() + p[1]) / 2; if (g > 0xFF) g = 0xFF;
		p[1] = (byte)g;
		int b = (pcol.blue() + p[2]) / 2; if (b > 0xFF) b = 0xFF;
		p[2] = (byte)b;
	}
	update();
}

void rsMapWidget::SetTile(int tile)
{
	int maxtile = ncols * nrows;
	if (tile >= 0 && tile < maxtile)
		this->tile = tile;
	else
		this->tile = 0;
	update();
}
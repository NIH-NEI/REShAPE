#include "rsImageViewQt.h"

rsScrollArea::rsScrollArea(rsImageViewQt * v) : QScrollArea()
{
	this->v = v;
	screen_height = QGuiApplication::primaryScreen()->geometry().height();
	timer = new QTimer(this);
	timer->setInterval(50);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(on_timer()));
}

rsScrollArea::~rsScrollArea()
{
	delete timer;
}

void rsScrollArea::mousePressEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton) {
		drag = true;
		startPos = e->globalPos();
		xscrl = horizontalScrollBar()->value();
		yscrl = verticalScrollBar()->value();
	}
	QScrollArea::mousePressEvent(e);
}

void rsScrollArea::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton) {
		drag = false;
	}
	QScrollArea::mouseReleaseEvent(e);
}

void rsScrollArea::mouseMoveEvent(QMouseEvent *e)
{
	if (drag) {
		QPoint here = e->globalPos();
		horizontalScrollBar()->setValue(xscrl - here.x() + startPos.x());
		verticalScrollBar()->setValue(yscrl - here.y() + startPos.y());
	}
	QScrollArea::mouseMoveEvent(e);
}

void rsScrollArea::wheelEvent(QWheelEvent *e)
{
	double scs;
	if (in_progress) {
		scs = sc;
	}
	else {
		o_sc = scs = v->GetScale();
		mpos = e->pos();
	}

	QPoint dg8 = e->angleDelta();
	sc = scs;
	if (dg8.y() > 0) {
		// Wheel forward - zoom in
		sc *= ((100. + dg8.y() / 12.)*0.01);
		if (sc > 5.) sc = 5.;
	}
	else if (dg8.y() < 0) {
		// Wheel backward - zoom out
		double min_sc = screen_height * 0.05 / v->GetHeight();
		sc /= ((100. - dg8.y() / 12.)*0.01);
		if (sc < min_sc) sc = min_sc;
	}
	else return;
	if (scs == sc) return;

//	std::cout << "Request rescale to " << sc << std::endl;
	in_progress = true;
	timer->start();
}

void rsScrollArea::on_timer()
{
//	std::cout << "Process rescale to " << sc << std::endl;
	in_progress = false;

	int xpos = horizontalScrollBar()->value();
	int ypos = verticalScrollBar()->value();
	QSize wsz = widget()->size();
	QSize vsz = viewport()->size();

	// Force scale to "fit-to-view" if it comes close
	double nsc = v->GetFitToViewScale();
	if (sc > nsc / 1.06 && sc < nsc*0.06) sc = nsc;
	// Force scale to 1:1 if close
	if (sc > 1. / 1.06 && sc < 1.06) sc = 1.;

	v->SetScale(sc);

	// Try to preserve image position under the mouse
	if (wsz.width() >= vsz.width()) {
		int xpose = int(xpos*sc/o_sc + mpos.x()*(sc/o_sc - 1.));
		horizontalScrollBar()->setValue(xpose);
	}
	if (wsz.height() >= vsz.height()) {
		int ypose = int(ypos*sc/o_sc + mpos.y()*(sc/o_sc - 1.));
		verticalScrollBar()->setValue(ypose);
	}
}

void rsImageLabel::resizeEvent(QResizeEvent *e)
{
	v->beginWait();
	QLabel::resizeEvent(e);
}

void rsImageLabel::paintEvent(QPaintEvent *e)
{
	QLabel::paintEvent(e);
	v->endWait();
}

rsImageViewQt::~rsImageViewQt()
{
	if (img) delete img;
}

void rsImageViewQt::createImage(int width, int height)
{
	if (img) delete img;
	this->width = width;
	this->height = height;
	img = new QImage(width, height, QImage::Format_RGB888);
}

void rsImageViewQt::SetScale(double sc)
{
	this->sc = sc;
	QSize pms = imgLabel->pixmap()->size();
	imgLabel->resize(int(sc*pms.width()), int(sc*pms.height()));
}

double rsImageViewQt::GetFitToViewScale()
{
	QSize pms = imgLabel->pixmap()->size();
	QSize vps = scrollArea->size();
	double scx = double(vps.width()-2) / double(pms.width());
	double scy = double(vps.height()-2) / double(pms.height());
	double ret = scy;
	if (scx < scy) ret = scx;
	if (ret > 1.) ret = 1.;
	return ret;
}

void rsImageViewQt::beginWait()
{
	if (wait_fl) return;
	wait_fl = true;
	QApplication::setOverrideCursor(Qt::WaitCursor);
}

void rsImageViewQt::endWait()
{
	if (!wait_fl) return;
	wait_fl = false;
	QApplication::restoreOverrideCursor();
}

void rsImageViewQt::ResetView(bool camera_flag)
{
	if (!imgLabel) return;
	if (!imgLabel->pixmap()) return;
	if (camera_flag) {
		QSize pms = imgLabel->pixmap()->size();
		sc = GetFitToViewScale();
		imgLabel->resize(int(sc*pms.width()), int(sc*pms.height()));
	}
}

void rsImageViewQt::InitializeView()
{

}

void rsImageViewQt::SetGrayImage(ImageType2D::Pointer simg)
{
	beginWait();
	ImageType2D::RegionType region = simg->GetBufferedRegion();
	ImageType2D::SizeType size = region.GetSize();
	if (!img || width != size[0] || height != size[1])
		createImage(size[0], size[1]);
	ImageType2D::PixelContainer::Pointer container = simg->GetPixelContainer();
	byte *sptr = container->GetImportPointer();
	for (int row = 0; row < height; row++) {
		byte *tptr = img->scanLine(row);
		for (int col = 0; col < width; col++) {
			byte c = *sptr++;
			*tptr++ = c;
			*tptr++ = c;
			*tptr++ = c;
		}
	}
	scrollArea->setVisible(true);
	ImageDataModified();
}

void rsImageViewQt::ImageDataModified()
{
	if (!img || !imgLabel) return;
	beginWait();
	imgLabel->setPixmap(QPixmap::fromImage(*img));
}

byte * rsImageViewQt::GetPixelDataPointer(int x, int y)
{
	return img->scanLine(y) + (x * 3);
}

QWidget * rsImageViewQt::GetImageWidget(QWidget *parent)
{
	if (!scrollArea) {
		imgLabel = new rsImageLabel(this);
		imgLabel->setBackgroundRole(QPalette::Base);
		imgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
		imgLabel->setScaledContents(true);

		scrollArea = new rsScrollArea(this);
		scrollArea->setBackgroundRole(QPalette::Dark);
		scrollArea->setWidget(imgLabel);
		scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		scrollArea->setVisible(false);
	}
	return (QWidget *)scrollArea;
}

#ifndef rsFigure_h
#define rsFigure_h

#include <iostream>
#include <vector>
#include <QColor>
#include <QPoint>
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include "rsdef.h"

struct GPoint {
	double x, y;
	GPoint() { x = y = 0.; }
	GPoint(double x, double y) { this->x = x; this->y = y; }
	QJsonArray toJson() {
		QJsonArray res;
		res.append(QJsonValue(x));
		res.append(QJsonValue(y));
		return res;
	}
	bool fromJson(QJsonArray jpt) {
		if (jpt.size() < 2) return false;
		double _x, _y;
		if (jpt[0].isDouble()) _x = jpt[0].toDouble();
		else return false;
		if (jpt[1].isDouble()) _y = jpt[1].toDouble();
		else return false;
		x = _x;
		y = _y;
		return true;
	}
};

struct GCell : public GPoint {
	double r, rsq;
	GCell() : GPoint() { r = rsq = 0.; }
	GCell(double x, double y, double ar) : GPoint(x, y) {
		rsq = ar / PI;
		r = sqrt(rsq);
	}
};

struct GRect {
	double x0, y0, x1, y1;
	bool IsInside(GCell &p) {
		return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
	}
};

class GFigure {
public:
	GFigure() {}
	virtual ~GFigure() {}
	virtual std::string GetShape() { return std::string("none"); }
	virtual bool IsInside(GPoint p) { return false; }
	virtual bool IsBorder(GCell c) { return false; }
	bool IsInside(double x, double y) { return IsInside(GPoint(x, y)); }
	virtual QJsonObject toJson() { return QJsonObject(); }
	virtual double area() { return 0.; }
	virtual GRect GetRect(double extend = 0.) {
		GRect r;
		r.x0 = r.x1 = r.y0 = r.y1 = 0.;
		return r;
	}
};

class GCircle : public GFigure {
protected:
	double xc, yc, r;
public:
	GCircle() : GFigure() {
		xc = yc = r = 0.;
	}
	GCircle(double xc, double yc, double r) {
		this->xc = xc;
		this->yc = yc;
		this->r = r;
	}
	virtual ~GCircle() override {}
	virtual std::string GetShape() override { return std::string("circle"); }
	double GetXC() { return xc; }
	double GetYC() { return yc; }
	double GetR() { return r; }
	GCircle & operator = (GCircle gc) {
		xc = gc.xc;
		yc = gc.yc;
		r = gc.r;
		return *this;
	}
	virtual bool IsInside(GPoint p) override {
		double dx = double(xc - p.x);
		double dy = double(yc - p.y);
		return sqrt(dx * dx + dy * dy) <= r;
	}
	virtual bool IsBorder(GCell c) override {
		double dx = double(xc - c.x);
		double dy = double(yc - c.y);
		return fabs(sqrt(dx * dx + dy * dy) - r) < c.r;
	}
	virtual QJsonObject toJson() override {
		QJsonObject res;
		res["shape"] = QString("circle");
		GPoint center(xc, yc);
		res["center"] = center.toJson();
		res["radius"] = QJsonValue(r);
		return res;
	}
	bool fromJson(QJsonObject jfig) {
		if (jfig["shape"] != QString("circle")) return false;
		GPoint center;
		if (!center.fromJson(jfig["center"].toArray())) return false;
		if (!jfig["radius"].isDouble()) return false;
		r = jfig["radius"].toDouble();
		xc = center.x;
		yc = center.y;
		return true;
	}
	virtual double area() override { return PI*r*r; }
	virtual GRect GetRect(double extend=0.) override {
		GRect rect;
		rect.x0 = xc - r - extend;
		rect.x1 = xc + r + extend;
		rect.y0 = yc - r - extend;
		rect.y1 = yc + r + extend;
		return rect;
	}
};

class GPolygon : public GFigure {
protected:
	std::vector<GPoint> vertices;
public:
	GPolygon() : GFigure() {}
	GPolygon(size_t sz) : GFigure(), vertices(sz) {}
	virtual ~GPolygon() override {}
	virtual std::string GetShape() override { return std::string("polygon"); }
	size_t size() { return vertices.size(); }
	void resize(size_t sz) { vertices.resize(sz); }
	GPoint & at(size_t idx) { return vertices[idx]; }
	GPoint & operator [] (size_t idx) { return vertices[idx]; }
	GPolygon & operator = (GPolygon gp) {
		vertices = gp.vertices;
		return *this;
	}
	virtual bool IsInside(GPoint p) override;
	virtual bool IsBorder(GCell c) override;
	virtual QJsonObject toJson() override {
		QJsonObject res;
		res["shape"] = QString("polygon");
		QJsonArray jvertices;
		for (size_t i = 0; i < vertices.size(); i++)
			jvertices.append(vertices[i].toJson());
		res["vertices"] = jvertices;
		return res;
	}
	bool fromJson(QJsonObject jfig) {
		if (jfig["shape"] != QString("polygon")) return false;
		if (!jfig["vertices"].isArray()) return false;
		QJsonArray jvertices = jfig["vertices"].toArray();
		std::vector<GPoint> _vertices(jvertices.size());
		for (int i = 0; i < jvertices.size(); i++) {
			GPoint pt;
			if (!pt.fromJson(jvertices[i].toArray())) return false;
			_vertices[i] = pt;
		}
		vertices = _vertices;
		return true;
	}
	virtual double area() override;
	virtual GRect GetRect(double extend = 0.) override;
};

class rsFigure {
protected:
	QColor nc, hc;
	double sc;
	int w, h;
	bool drag;
	double rat;
	int hot;
	int hotx, hoty;
	QPoint startLoc;
	int startXc, startYc;

	virtual int mouse_drag(QPoint loc) = 0;
	virtual void rescale() = 0;
public:
	const double EPSI = 12.;
	const double EPSIDE = 0.3;
	const int VR = 16;
	const int VSZ = VR + VR + 1;
	const int ER = 10;
	const int ESZ = ER + ER + 1;

	rsFigure(QColor _nc, QColor _hc) : nc(_nc), hc(_hc) {
		sc = 1.;
		hot = 0;
		drag = false;
		rat = 1.;
	}
	virtual ~rsFigure() {}
	virtual void moveto(int xc0, int yc0, int r0) = 0;
	void SetScale(double _sc, int w, int h) {
		this->w = w;
		this->h = h;
		sc = _sc;
		rescale();
	}
	double GetScale() { return sc; }
	virtual bool size_ok(int w, int h) = 0;
	virtual void paint(QPainter &g) = 0;
	virtual int mouse_move(QPoint loc) = 0;
	virtual int mouse_pressed(QPoint loc, Qt::MouseButton btn) = 0;
	virtual int mouse_released(QPoint loc, Qt::MouseButton btn) = 0;
	int isHot() { return hot; }
	bool isDrag() { return drag; }
	void cancelHot() { hot = 0; drag = false; }
	virtual GFigure & toGFigure(double scale) = 0;
	virtual bool fromGFigure(GFigure & gfig, double scale) = 0;
};

class rsCircle : public rsFigure {
private:
	int xc0, yc0, r0;
	int xc, yc, r;
	int startR;
	GCircle gcir;

protected:
	virtual void rescale() override {
		xc = int(xc0 * sc + 0.5);
		yc = int(yc0 * sc + 0.5);
		r = int(r0 * sc + 0.5);
	}
	virtual int mouse_drag(QPoint loc) override;
public:
	rsCircle(int xc, int yc, int r, QColor nc, QColor hc) : rsFigure(nc, hc) {
		moveto(xc, yc, r);
	}
	virtual void moveto(int xc0, int yc0, int r0) override {
		this->xc0 = xc0;
		this->yc0 = yc0;
		this->r0 = r0;
		rescale();
	}
	int GetXC() { return xc0; }
	int GetYC() { return yc0; }
	int GetR() { return r0; }

	virtual bool size_ok(int w, int h) override {
		int sz = h; if (sz > w) sz = w;
		sz = sz * 3 / 4;
		if (r0 < 10 || r0 > sz || xc0 < 5 || xc0 > w - 5 || yc0 < 5 || yc0 > h - 5) return false;
		return true;
	}
	virtual void paint(QPainter &g) override;
	virtual int mouse_move(QPoint loc) override;
	virtual int mouse_pressed(QPoint loc, Qt::MouseButton btn) override;
	virtual int mouse_released(QPoint loc, Qt::MouseButton btn) override;

	GCircle & toGCircle(double scale) {
		gcir = GCircle(xc0 / scale, yc0 / scale, r0 / scale);
		return gcir;
	}
	virtual GFigure & toGFigure(double scale) override {
		return (GFigure &) toGCircle(scale);
	}
	virtual bool fromGFigure(GFigure & gfig, double scale) override
	{
		if (gfig.GetShape() != std::string("circle")) return false;
		GCircle & gcir = (GCircle &)gfig;
		xc0 = int(gcir.GetXC()*scale + 0.5);
		yc0 = int(gcir.GetYC()*scale + 0.5);
		r0 = int(gcir.GetR()*scale + 0.5);
		rescale();
		return true;
	}
};

class rsPolygon : public rsFigure {
private:
	std::vector<QPoint> pts0;
	std::vector<QPoint> pts;
	int xc, yc;
	int startDx, startDy;
	size_t hotIdx;
	GPolygon gpoly;

	void calc_center();
protected:
	virtual void rescale() override;
	virtual int mouse_drag(QPoint loc) override;
public:
	rsPolygon(int xc, int yc, int r, QColor nc, QColor hc) : rsFigure(nc, hc) {
		moveto(xc, yc, r);
	}
	virtual void moveto(int xc0, int yc0, int r0) override;
	virtual bool size_ok(int w, int h) override;
	virtual void paint(QPainter &g) override;
	virtual int mouse_move(QPoint loc) override;
	virtual int mouse_pressed(QPoint loc, Qt::MouseButton btn) override;
	virtual int mouse_released(QPoint loc, Qt::MouseButton btn) override;

	GPolygon & toGPolygon(double scale);
	virtual GFigure & toGFigure(double scale) override { return (GFigure &)toGPolygon(scale); }
	virtual bool fromGFigure(GFigure & gfig, double scale) override;
};


#endif

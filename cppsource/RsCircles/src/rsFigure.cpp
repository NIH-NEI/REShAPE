
#include "rsFigure.h"

//-- "Winding Number" algorithm for the inclusion of a point in polygon, adapted from:
// http://geomalgorithms.com/a03-inclusion.html

// tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2  on the line
//            <0 for P2  right of the line
static inline int isLeft(GPoint P0, GPoint P1, GPoint P2)
{
	return ((P1.x - P0.x) * (P2.y - P0.y)
		- (P2.x - P0.x) * (P1.y - P0.y));
}

bool GPolygon::IsInside(GPoint P)
{
	int n = (int)vertices.size();
	if (n == 0) return false;

	// V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
	std::vector<GPoint> V(vertices);
	V.push_back(vertices[0]);

	int wn = 0;    // the  winding number counter (=0 only when P is outside)

	// loop through all edges of the polygon
	for (int i = 0; i < n; i++) {		// edge from V[i] to  V[i+1]
		if (V[i].y <= P.y) {			// start y <= P.y
			if (V[i + 1].y > P.y)		// an upward crossing
				if (isLeft(V[i], V[i + 1], P) > 0)	// P left of  edge
					++wn;				// have  a valid up intersect
		}
		else {							// start y > P.y (no test needed)
			if (V[i + 1].y <= P.y)		// a downward crossing
				if (isLeft(V[i], V[i + 1], P) < 0)	// P right of  edge
					--wn;				// have  a valid down intersect
		}
	}
	return wn != 0;
}

//-- End of "Winding Number" algorithm

double GPolygon::area()
{
	double area = 0.;
	size_t numvert = vertices.size();
	if (numvert < 3) return area;
	size_t j = numvert - 1;
	for (size_t i = 0; i < numvert; i++) {
		area = area + (vertices[j].x + vertices[i].x) * (vertices[j].y - vertices[i].y);
		j = i;
	}
	return fabs(area/2.);
}

GRect GPolygon::GetRect(double extend)
{
	GRect rect;
	rect.x0 = rect.y0 = 1e20;
	rect.x1 = rect.y1 = 0.;
	for (size_t i = 0; i < vertices.size(); i++) {
		GPoint &pt = vertices[i];
		if (rect.x0 > pt.x) rect.x0 = pt.x;
		if (rect.x1 < pt.x) rect.x1 = pt.x;
		if (rect.y0 > pt.y) rect.y0 = pt.y;
		if (rect.y1 < pt.y) rect.y1 = pt.y;
	}
	rect.x0 -= extend;
	rect.x1 += extend;
	rect.y0 -= extend;
	rect.y1 += extend;
	return rect;
}

static double pDistanceSq(double x, double y, double x1, double y1, double x2, double y2) {

	double A = x - x1;
	double B = y - y1;
	double C = x2 - x1;
	double D = y2 - y1;

	double dot = A * C + B * D;
	double len_sq = C * C + D * D;
	double param = -1;
	if (len_sq != 0.) //in case of 0 length line
		param = dot / len_sq;

	double xx, yy;

	if (param < 0) {
		xx = x1;
		yy = y1;
	}
	else if (param > 1) {
		xx = x2;
		yy = y2;
	}
	else {
		xx = x1 + param * C;
		yy = y1 + param * D;
	}

	double dx = x - xx;
	double dy = y - yy;
	return dx * dx + dy * dy;
}

bool GPolygon::IsBorder(GCell c)
{
	size_t numvert = vertices.size();
	if (numvert < 3) return false;
	size_t j = numvert - 1;
	for (size_t i = 0; i < numvert; i++) {
		double dsq = pDistanceSq(c.x, c.y, vertices[j].x, vertices[j].y, vertices[i].x, vertices[i].y);
		if (dsq < c.rsq) return true;
		j = i;
	}
	return false;
}

int rsCircle::mouse_drag(QPoint loc)
{
	if (!hot) return 0;
	if (hot == 1) {
		xc = startXc - startLoc.x() + loc.x();
		if (xc < 5) xc = 5; else if (xc > w - 5) xc = w - 5;
		hotx = xc;
		yc = startYc - startLoc.y() + loc.y();
		if (yc < 5) yc = 5; else if (yc > h - 5) yc = h - 5;
		hoty = yc;
		xc0 = int(xc / sc + 0.5);
		yc0 = int(yc / sc + 0.5);
		return 1;
	}
	else if (hot == 2) {
		hotx = loc.x();
		hoty = loc.y();
		double dx = loc.x() - startXc;
		double dy = loc.y() - startYc;
		r = int(sqrt(dx*dx + dy * dy)*rat + 0.5);
		if (r < 15) r = 20;
		r0 = int(r / sc + 0.5);
		return 1;
	}
	return 0;
}

int rsCircle::mouse_move(QPoint loc)
{
	if (drag) return mouse_drag(loc);
	// std::cout << "loc(" << loc.x() << "," << loc.y() << ") xc=" << xc << " yc=" << yc << std::endl;
	int ohot = hot;
	double dx = xc - loc.x();
	double dy = yc - loc.y();
	double dist = sqrt(dx * dx + dy * dy);
	if (dist < EPSI) {
		hotx = xc;
		hoty = yc;
		hot = 1;
	}
	else if (dist < r + EPSI && dist > r - EPSI) {
		// std::cout << " dist=" << dist << " r=" << r << std::endl;
		rat = r / dist;
		hotx = int(xc - dx * rat + 0.5);
		hoty = int(yc - dy * rat + 0.5);

		// hotx = loc.x();
		// hoty = loc.y();
		hot = 2;
		return 1;
	}
	else hot = 0;
	return hot != ohot;
}

int rsCircle::mouse_pressed(QPoint loc, Qt::MouseButton btn)
{
	if (btn != Qt::LeftButton) return mouse_move(loc);
	if (!hot) mouse_move(loc);
	if (hot) {
		startLoc = loc;
		startXc = xc;
		startYc = yc;
		startR = r;
	}
	drag = true;
	return hot;
}

int rsCircle::mouse_released(QPoint loc, Qt::MouseButton btn)
{
	if (btn != Qt::LeftButton) return mouse_move(loc);
	bool rc = drag;
	drag = false;
	mouse_move(loc);
	return rc;
}

void rsCircle::paint(QPainter &g)
{
	bool _hot = hot && drag;
	int x0 = xc - r;
	int y0 = yc - r;
	int sz = r + r;
	g.setPen(Qt::black);
	g.drawEllipse(x0 - 2, y0 - 2, sz + 4, sz + 4);
	g.drawEllipse(x0 + 1, y0 + 1, sz - 2, sz - 2);
	g.fillRect(xc - 6, yc - 6, 13, 13, Qt::black);
	g.setPen(_hot ? hc : nc);
	g.drawEllipse(x0 - 1, y0 - 1, sz + 2, sz + 2);
	g.drawEllipse(x0, y0, sz, sz);
	g.fillRect(xc - 5, yc - 5, 11, 11, _hot ? hc : nc);
	if (hot) {
		g.setPen(nc);
		x0 = hotx - VR;
		y0 = hoty - VR;
		sz = VSZ;
		g.drawEllipse(x0 - 1, y0 - 1, sz + 2, sz + 2);
		g.drawEllipse(x0, y0, sz, sz);
		g.drawLine(x0, hoty, x0 + sz, hoty);
		g.drawLine(hotx, y0, hotx, y0 + sz);
	}
}

void rsPolygon::rescale()
{
	pts.resize(pts0.size());
	for (size_t i = 0; i < pts0.size(); i++) {
		QPoint & pt = pts0[i];
		double x = pt.x()*sc;
		double y = pt.y()*sc;
		pts[i] = QPoint(int(x + 0.5), int(y + 0.5));
	}
	calc_center();
}

void rsPolygon::moveto(int xc0, int yc0, int r0)
{
	double cos60 = sqrt(3.) / 2.;
	pts0.resize(3);
	pts0[0] = QPoint(xc0, yc0 - r0);
	pts0[1] = QPoint(int(xc0 + r0 * cos60 + 0.5), int(yc0 + r0 * 0.5 + 0.5));
	pts0[2] = QPoint(int(xc0 - r0 * cos60 + 0.5), int(yc0 + r0 * 0.5 + 0.5));
	rescale();
}

static inline void lineShift(QPoint &p1, QPoint &p2, int *sx, int *sy) {
	int dx = p1.x() - p2.x(); if (dx < 0) dx = -dx;
	int dy = p1.y() - p2.y(); if (dy < 0) dy = -dy;
	if (dy > dx) {
		*sx = 1;
		*sy = 0;
	}
	else {
		*sx = 0;
		*sy = 1;
	}
}

bool rsPolygon::size_ok(int w, int h)
{
	if (pts0.size() < 3) return false;
	size_t i;
	double xc = 0.;
	double yc = 0.;
	for (i = 0; i < pts0.size(); i++) {
		QPoint &p = pts0[i];
		xc += p.x();
		yc += p.y();
	}
	xc /= pts0.size();
	yc /= pts0.size();
	double r = 0.;
	for (i = 0; i < pts0.size(); i++) {
		QPoint &p = pts0[i];
		double dx = p.x() - xc;
		double dy = p.y() - yc;
		double d = sqrt(dx * dx + dy * dy);
		if (r < d) r = d;
	}
	int sz = h; if (sz < w) sz = w;
	sz = sz * 1.25;
	if (r < 10. || r > sz || xc < 5. || xc > w - 5. || yc < 5. || yc > h - 5.) return false;
	return true;
}

void rsPolygon::paint(QPainter &g)
{
	bool _hot = hot && drag;
	size_t n = pts.size();
	if (n == 0) return;
	size_t i;
	int sx, sy;
	QPoint p0 = pts[n - 1];

	g.setPen(Qt::black);

	for (i = 0; i < n; i++) {
		QPoint p1 = pts[i];
		lineShift(p1, p0, &sx, &sy);
		g.drawLine(p0.x() - sx, p0.y() - sy, p1.x() - sx, p1.y() - sy);
		sx += sx; sy += sy;
		g.drawLine(p0.x() + sx, p0.y() + sy, p1.x() + sx, p1.y() + sy);
		g.fillRect(p1.x() - 4, p1.y() - 4, 9, 9, Qt::black);
		p0 = p1;
	}

	g.fillRect(xc - 6, yc - 6, 13, 13, Qt::black);

	g.setPen(_hot ? hc : nc);

	p0 = pts[n - 1];
	for (i = 0; i < n; i++) {
		QPoint p1 = pts[i];
		lineShift(p1, p0, &sx, &sy);
		g.drawLine(p0, p1);
		g.drawLine(p0.x() + sx, p0.y() + sy, p1.x() + sx, p1.y() + sy);
		g.fillRect(p1.x() - 3, p1.y() - 3, 7, 7, _hot ? hc : nc);
		p0 = p1;
	}

	g.fillRect(xc - 5, yc - 5, 11, 11, _hot ? hc : nc);

	if (hot == 2) {
		QPoint p = startLoc;
		g.setPen(nc);
		int x0 = p.x() - ER;
		int y0 = p.y() - ER;
		int sz = ESZ;
		g.drawRect(x0, y0, sz, sz);
		g.drawRect(x0+1, y0+1, sz-2, sz-2);
		g.drawLine(x0, p.y(), x0 + sz, p.y());
		g.drawLine(p.x(), y0, p.x(), y0 + sz);
	} else if (hot == 1 || hot == 3) {
		QPoint p(xc, yc);
		if (hot == 1) p = pts[hotIdx];
		g.setPen(nc);
		int x0 = p.x() - VR;
		int y0 = p.y() - VR;
		int sz = VSZ;
		g.drawEllipse(x0 - 1, y0 - 1, sz + 2, sz + 2);
		g.drawEllipse(x0, y0, sz, sz);
		g.drawLine(x0, p.y(), x0 + sz, p.y());
		g.drawLine(p.x(), y0, p.x(), y0 + sz);
	}
}

/*
void rsPolygon::calc_center()
{
	if (pts.size() == 0) return;
	double xx = 0.;
	double yy = 0.;
	for (size_t i = 0; i < pts.size(); i++) {
		QPoint &p = pts[i];
		xx += p.x();
		yy += p.y();
	}
	xc = int(xx / pts.size() + 0.5);
	yc = int(yy / pts.size() + 0.5);
}
*/

void rsPolygon::calc_center()
{
	if (pts.size() == 0) return;
	double xx = 0.;
	double yy = 0.;
	double peri = 0.;
	QPoint p0 = pts[pts.size() - 1];
	for (size_t i = 0; i < pts.size(); i++) {
		QPoint &p = pts[i];
		double dx = p0.x() - p.x();
		double dy = p0.y() - p.y();
		double side = sqrt(dx*dx + dy*dy);
		peri += side;
		xx += 0.5*side*(p0.x()+p.x());
		yy += 0.5*side*(p0.y()+p.y());
		p0 = p;
	}
	xc = int(xx / peri + 0.5);
	yc = int(yy / peri + 0.5);
}

int rsPolygon::mouse_drag(QPoint loc)
{
	if (!hot) return 0;
	if (hot == 1) {
		int x = startXc - startLoc.x() + loc.x();
		if (x < 5) x = 5; else if (x > w - 5) x = w - 5;
		int y = startYc - startLoc.y() + loc.y();
		if (y < 5) y = 5; else if (y > h - 5) y = h - 5;
		QPoint &p = pts[hotIdx];
		p = QPoint(x, y);
		pts0[hotIdx] = QPoint(int(p.x() / sc + 0.5), int(p.y() / sc + 0.5));
		calc_center();
		return 1;
	} else if (hot == 3) {
		xc = startXc - startLoc.x() + loc.x();
		if (xc < 5) xc = 5; else if (xc > w - 5) xc = w - 5;
		yc = startYc - startLoc.y() + loc.y();
		if (yc < 5) yc = 5; else if (yc > h - 5) yc = h - 5;
		int dx = xc - startXc;
		int dy = yc - startYc;
		for (size_t i = 0; i < pts.size(); i++) {
			QPoint &p = pts[i];
			p = QPoint(p.x() + dx - startDx, p.y() + dy - startDy);
			pts0[i] = QPoint(int(p.x() / sc + 0.5), int(p.y() / sc + 0.5));
		}
		startDx = dx;
		startDy = dy;
		return 1;
	}
	return 0;
}

int rsPolygon::mouse_move(QPoint loc)
{
	if (drag) return mouse_drag(loc);
	size_t n = pts.size();
	if (n == 0) return 0;
	int ohot = hot;
	size_t i;
	double dx, dy, dist;

	for (i = 0; i < pts.size(); i++) {
		QPoint & p = pts[i];
		dx = p.x() - loc.x();
		dy = p.y() - loc.y();
		dist = sqrt(dx * dx + dy * dy);
		if (dist < EPSI) {
			hot = 1;
			hotIdx = i;
			return hot != ohot;
		}
	}

	QPoint p0 = pts[pts.size() - 1];
	for (i = 0; i < pts.size(); i++) {
		QPoint & p1 = pts[i];
		dx = p1.x() - p0.x();
		dy = p1.y() - p0.y();
		double side = sqrt(dx*dx + dy*dy);
		dx = p1.x() - loc.x();
		dy = p1.y() - loc.y();
		double d1 = sqrt(dx*dx + dy*dy);
		dx = p0.x() - loc.x();
		dy = p0.y() - loc.y();
		double d0 = sqrt(dx*dx + dy*dy);
		if (d0 + d1 < side + EPSIDE) {
			hot = 2;
			hotIdx = i;
			startLoc = loc;
			return 1;
		}
		p0 = p1;
	}

	dx = xc - loc.x();
	dy = yc - loc.y();
	dist = sqrt(dx * dx + dy * dy);
	if (dist < EPSI) {
		hot = 3;
		return hot != ohot;
	}

	hot = 0;
	return hot != ohot;
}
int rsPolygon::mouse_pressed(QPoint loc, Qt::MouseButton btn)
{
	if (btn != Qt::LeftButton) return mouse_move(loc);
	if (!hot) mouse_move(loc);
	if (hot == 2) {
		QPoint p0(int(loc.x() / sc + 0.5), int(loc.y() / sc + 0.5));
		if (hotIdx == 0) {
			pts.push_back(loc);
			pts0.push_back(p0);
			hotIdx = pts.size() - 1;
		}
		else {
			pts.insert(pts.begin() + hotIdx, loc);
			pts0.insert(pts0.begin() + hotIdx, p0);
		}
		calc_center();
		hot = 1;
	}
	if (hot == 1) {
		startLoc = loc;
		QPoint &p = pts[hotIdx];
		startXc = p.x();
		startYc = p.y();
		drag = true;
	} else if (hot == 3) {
		startLoc = loc;
		startXc = xc;
		startYc = yc;
		startDx = startDy = 0;
		drag = true;
	}
	return hot;
}
int rsPolygon::mouse_released(QPoint loc, Qt::MouseButton btn)
{
	if (btn == Qt::RightButton && hot == 1 && pts.size() > 3) {
		pts.erase(pts.begin() + hotIdx);
		pts0.erase(pts0.begin() + hotIdx);
		calc_center();
		drag = false;
		mouse_move(loc);
		return 1;
	}
	if (btn != Qt::LeftButton) return mouse_move(loc);
	bool rc = drag;
	drag = false;
	mouse_move(loc);
	return rc;
}

GPolygon & rsPolygon::toGPolygon(double scale)
{
	gpoly.resize(pts0.size());
	for (size_t i = 0; i < pts0.size(); i++) {
		QPoint &p = pts0[i];
		gpoly[i] = GPoint(p.x() / scale, p.y() / scale);
	}
	return gpoly;
}

bool rsPolygon::fromGFigure(GFigure & gfig, double scale)
{
	if (gfig.GetShape() != std::string("polygon")) return false;
	GPolygon & gpoly = (GPolygon &)gfig;
	pts0.resize(gpoly.size());
	for (size_t i = 0; i < gpoly.size(); i++) {
		GPoint &gp = gpoly[i];
		pts0[i] = QPoint(int(gp.x*scale + 0.5), int(gp.y*scale + 0.5));
	}
	rescale();
	return true;
}
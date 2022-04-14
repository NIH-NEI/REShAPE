
#define __rsDisplayPar_main__ 1
#include "rsIo.h"

static std::string format_double(double v, const char* dformat = "%lg")
{
	std::string s;
	if (std::isnan(v))
		s = "NaN";
	else {
		s.resize(256);
		auto len = std::snprintf(&s[0], s.size(), dformat, v);
		s.resize(len);
	}
	return s;
}

rsDisplayPar visualParams[] = {
	{offsetof(rsParticlePar, Area), "Area", "Area"},
	{offsetof(rsParticlePar, Peri), "Peri", "Perimeter"},
	{offsetof(rsParticlePar, AoP), "AoP", "Area/Perimeter"},
	{offsetof(rsParticlePar, PoN), "PoN", "Perimeter/Neighbors"},
	{offsetof(rsParticlePar, EllipMaj), "EllipMaj", "Ellipse Major Axis"},
	{offsetof(rsParticlePar, ElipMin), "ElipMin", "Ellipse Minor Axis"},
	{offsetof(rsParticlePar, AR), "AR", "Ellipse Aspect Ratio"},
	{offsetof(rsParticlePar, Angle), "Angle", "Ellipse Angle"},
	{offsetof(rsParticlePar, Feret), "Feret", "Feret Major"},
	{offsetof(rsParticlePar, MinFeret), "MinFeret", "Feret Minor"},
	{offsetof(rsParticlePar, FeretAR), "FeretAR", "Feret Aspect Ratio"},
	{offsetof(rsParticlePar, FeretAngle), "FeretAngle", "Feret Angle"},
	{offsetof(rsParticlePar, Circ), "Circ", "Circularity"},
	{offsetof(rsParticlePar, Solidity), "Solidity", "Solidity"},
	{offsetof(rsParticlePar, Compactness), "Compactness", "Compactness"},
	{offsetof(rsParticlePar, Extent), "Extent", "Extent"},
	{offsetof(rsParticlePar, Poly_SR), "Poly_SR", "Polygon Side Ratio"},
	{offsetof(rsParticlePar, Poly_AR), "Poly_AR", "Polygon Area Ratio"},
	{offsetof(rsParticlePar, Poly_Ave), "Poly_Ave", "Polygonality Score"},
	{offsetof(rsParticlePar, Hex_SR), "Hex_SR", "Hexagon Side Ratio"},
	{offsetof(rsParticlePar, Hex_AR), "Hex_AR", "Hexagon Area Ratio"},
	{offsetof(rsParticlePar, Hex_Ave), "Hex_Ave", "Hexagonality Score"}

};
size_t numVisualParams = sizeof visualParams / sizeof(rsDisplayPar);


bool rsTiledImage::open(std::string jFileName)
{
	init();

	QFileInfo finfo(jFileName.c_str());
	QString qFileName = finfo.canonicalFilePath();

	if (!finfo.exists()) {
		error_msg = std::string("File {") + qFileName.toStdString() + std::string("} does not exist.");
		return false;
	}
	QFile qf(qFileName);
	if (!qf.open(QIODevice::ReadOnly)) {
		error_msg = std::string("Failed to read {") + qf.fileName().toStdString() + std::string("}");
		return false;
	}

	QJsonDocument json = QJsonDocument::fromJson(qf.readAll());
	qf.close();

	QJsonObject imgobj = json.object();

	QJsonArray jtiles;
	if (imgobj["width"].isDouble())
		width = uint64(imgobj["width"].toDouble());
	if (imgobj["height"].isDouble())
		height = uint64(imgobj["height"].toDouble());
	if (imgobj["numtilerows"].isDouble())
		numtilerows = imgobj["numtilerows"].toInt();
	if (imgobj["numtilecolumns"].isDouble())
		numtilecolumns = imgobj["numtilecolumns"].toInt();
	if (imgobj["tiles"].isArray())
		jtiles = imgobj["tiles"].toArray();
	if (imgobj["source"].isString())
		source = imgobj["source"].toString().toStdString();
	if (width == 0 || height == 0 || jtiles.size() == 0 || numtilerows <= 0 || numtilecolumns <= 0) {
		goto inv_format;
	}

	fpath = qFileName.toStdString();

	unit_conv = imgobj["unit_conv"].toInt();
	if (unit_conv) {
		unit_pix = imgobj["unit_pix"].toDouble();
		unit_real = imgobj["unit_real"].toDouble();
	}

	for (int j=0; j<jtiles.size(); j++) {
		QJsonObject jtile = jtiles[j].toObject();
		if (!jtile["attr"].isObject()) goto inv_format;
		tiles.resize(tiles.size() + 1);
		rsImageTile& tile = tiles[tiles.size()-1];
		tile.x = int64(jtile["x"].toInt());
		tile.y = int64(jtile["y"].toInt());
		tile.w = int64(jtile["w"].toInt());
		tile.h = int64(jtile["h"].toInt());
		tile.name = jtile["name"].toString().toStdString();
		QJsonObject jattr = jtile["attr"].toObject();
		tile.csvdata = jattr["Results"].toString().toStdString();
		tile.outlines = jattr["Outlines"].toString().toStdString();
	}

	return true;
inv_format:
	error_msg = std::string("Invalid format: {") + qf.fileName().toStdString() + std::string("}");
	return false;
}


// ------- rsLUT ----------

void rsLUT::setAppDir(const QString& appDir)
{
	ldir = QDir(".");
	QDir pDir(appDir);
	for (int lvl = 0; lvl < 3; lvl++) {
		pDir.cdUp();
		QFileInfo lutf(pDir, QString("LUT"));
		if (lutf.isDir()) {
			ldir = QDir(lutf.absoluteFilePath());
			break;
		}
	}
}

QStringList rsLUT::listLutNames()
{
	QStringList res;
	res << "Jet" << "glasbey";

	QStringList filt;
	filt << "*.png";
	QFileInfoList luts = ldir.entryInfoList(filt);
	for (QFileInfo& qfi : luts) {
		if (!qfi.isFile()) continue;
		QString bn = qfi.baseName();
		if (res.indexOf(bn) < 0)
			res << bn;
	}

	return res;
}

QVector<QRgb> rsLUT::getDefaultPalette()
{
	int ncolors = 256;
	palette.resize(ncolors);
	for (int i = 0; i < ncolors; i++) {
		double i4 = 4. * i / 256.;
		int r = (int)(255. * std::min(std::max(std::min(i4 - 1.5, -i4 + 4.5), 0.), 1.));
		int g = (int)(255. * std::min(std::max(std::min(i4 - 0.5, -i4 + 3.5), 0.), 1.));
		int b = (int)(255. * std::min(std::max(std::min(i4 + 0.5, -i4 + 2.5), 0.), 1.));
		palette[i] = qRgb(r, g, b);
	}
	lutName = "Jet";
	return palette;
}

static uint8 glasbey_data[256][3] = {
	{ 255, 255, 255 }, { 0, 0, 255 }, { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 51 }, { 255, 0, 182 }, { 0, 83, 0 }, { 255, 211, 0 },
	{ 0, 159, 255 }, { 154, 77, 66 }, { 0, 255, 190 }, { 120, 63, 193 }, { 31, 150, 152 }, { 255, 172, 253 }, { 177, 204, 113 }, { 241, 8, 92 },
	{ 254, 143, 66 }, { 221, 0, 255 }, { 32, 26, 1 }, { 114, 0, 85 }, { 118, 108, 149 }, { 2, 173, 36 }, { 200, 255, 0 }, { 136, 108, 0 },
	{ 255, 183, 159 }, { 133, 133, 103 }, { 161, 3, 0 }, { 20, 249, 255 }, { 0, 71, 158 }, { 220, 94, 147 }, { 147, 212, 255 }, { 0, 76, 255 },
	{ 0, 66, 80 }, { 57, 167, 106 }, { 238, 112, 254 }, { 0, 0, 100 }, { 171, 245, 204 }, { 161, 146, 255 }, { 164, 255, 115 }, { 255, 206, 113 },
	{ 71, 0, 21 }, { 212, 173, 197 }, { 251, 118, 111 }, { 171, 188, 0 }, { 117, 0, 215 }, { 166, 0, 154 }, { 0, 115, 254 }, { 165, 93, 174 },
	{ 98, 132, 2 }, { 0, 121, 168 }, { 0, 255, 131 }, { 86, 53, 0 }, { 159, 0, 63 }, { 66, 45, 66 }, { 255, 242, 187 }, { 0, 93, 67 },
	{ 252, 255, 124 }, { 159, 191, 186 }, { 167, 84, 19 }, { 74, 39, 108 }, { 0, 16, 166 }, { 145, 78, 109 }, { 207, 149, 0 }, { 195, 187, 255 },
	{ 253, 68, 64 }, { 66, 78, 32 }, { 106, 1, 0 }, { 181, 131, 84 }, { 132, 233, 147 }, { 96, 217, 0 }, { 255, 111, 211 }, { 102, 75, 63 },
	{ 254, 100, 0 }, { 228, 3, 127 }, { 17, 199, 174 }, { 210, 129, 139 }, { 91, 118, 124 }, { 32, 59, 106 }, { 180, 84, 255 }, { 226, 8, 210 },
	{ 0, 1, 20 }, { 93, 132, 68 }, { 166, 250, 255 }, { 97, 123, 201 }, { 98, 0, 122 }, { 126, 190, 58 }, { 0, 60, 183 }, { 255, 253, 0 },
	{ 7, 197, 226 }, { 180, 167, 57 }, { 148, 186, 138 }, { 204, 187, 160 }, { 55, 0, 49 }, { 0, 40, 1 }, { 150, 122, 129 }, { 39, 136, 38 },
	{ 206, 130, 180 }, { 150, 164, 196 }, { 180, 32, 128 }, { 110, 86, 180 }, { 147, 0, 185 }, { 199, 48, 61 }, { 115, 102, 255 }, { 15, 187, 253 },
	{ 172, 164, 100 }, { 182, 117, 250 }, { 216, 220, 254 }, { 87, 141, 113 }, { 216, 85, 34 }, { 0, 196, 103 }, { 243, 165, 105 }, { 216, 255, 182 },
	{ 1, 24, 219 }, { 52, 66, 54 }, { 255, 154, 0 }, { 87, 95, 1 }, { 198, 241, 79 }, { 255, 95, 133 }, { 123, 172, 240 }, { 120, 100, 49 },
	{ 162, 133, 204 }, { 105, 255, 220 }, { 198, 82, 100 }, { 121, 26, 64 }, { 0, 238, 70 }, { 231, 207, 69 }, { 217, 128, 233 }, { 255, 211, 209 },
	{ 209, 255, 141 }, { 36, 0, 3 }, { 87, 163, 193 }, { 211, 231, 201 }, { 203, 111, 79 }, { 62, 24, 0 }, { 0, 117, 223 }, { 112, 176, 88 },
	{ 209, 24, 0 }, { 0, 30, 107 }, { 105, 200, 197 }, { 255, 203, 255 }, { 233, 194, 137 }, { 191, 129, 46 }, { 69, 42, 145 }, { 171, 76, 194 },
	{ 14, 117, 61 }, { 0, 30, 25 }, { 118, 73, 127 }, { 255, 169, 200 }, { 94, 55, 217 }, { 238, 230, 138 }, { 159, 54, 33 }, { 80, 0, 148 },
	{ 189, 144, 128 }, { 0, 109, 126 }, { 88, 223, 96 }, { 71, 80, 103 }, { 1, 93, 159 }, { 99, 48, 60 }, { 2, 206, 148 }, { 139, 83, 37 },
	{ 171, 0, 255 }, { 141, 42, 135 }, { 85, 83, 148 }, { 150, 255, 0 }, { 0, 152, 123 }, { 255, 138, 203 }, { 222, 69, 200 }, { 107, 109, 230 },
	{ 30, 0, 68 }, { 173, 76, 138 }, { 255, 134, 161 }, { 0, 35, 60 }, { 138, 205, 0 }, { 111, 202, 157 }, { 225, 75, 253 }, { 255, 176, 77 },
	{ 229, 232, 57 }, { 114, 16, 255 }, { 111, 82, 101 }, { 134, 137, 48 }, { 99, 38, 80 }, { 105, 38, 32 }, { 200, 110, 0 }, { 209, 164, 255 },
	{ 198, 210, 86 }, { 79, 103, 77 }, { 174, 165, 166 }, { 170, 45, 101 }, { 199, 81, 175 }, { 255, 89, 172 }, { 146, 102, 78 }, { 102, 134, 184 },
	{ 111, 152, 255 }, { 92, 255, 159 }, { 172, 137, 178 }, { 210, 34, 98 }, { 199, 207, 147 }, { 255, 185, 30 }, { 250, 148, 141 }, { 49, 34, 78 },
	{ 254, 81, 97 }, { 254, 141, 100 }, { 68, 54, 23 }, { 201, 162, 84 }, { 199, 232, 240 }, { 68, 152, 0 }, { 147, 172, 58 }, { 22, 75, 28 },
	{ 8, 84, 121 }, { 116, 45, 0 }, { 104, 60, 255 }, { 64, 41, 38 }, { 164, 113, 215 }, { 207, 0, 155 }, { 118, 1, 35 }, { 83, 0, 88 },
	{ 0, 82, 232 }, { 43, 92, 87 }, { 160, 217, 146 }, { 176, 26, 229 }, { 29, 3, 36 }, { 122, 58, 159 }, { 214, 209, 207 }, { 160, 100, 105 },
	{ 106, 157, 160 }, { 153, 219, 113 }, { 192, 56, 207 }, { 125, 255, 89 }, { 149, 0, 34 }, { 213, 162, 223 }, { 22, 131, 204 }, { 166, 249, 69 },
	{ 109, 105, 97 }, { 86, 188, 78 }, { 255, 109, 81 }, { 255, 3, 248 }, { 255, 0, 73 }, { 202, 0, 35 }, { 67, 109, 18 }, { 234, 170, 173 },
	{ 191, 165, 0 }, { 38, 44, 51 }, { 85, 185, 2 }, { 121, 182, 158 }, { 254, 236, 212 }, { 139, 165, 89 }, { 141, 254, 193 }, { 0, 60, 43 },
	{ 63, 17, 40 }, { 255, 221, 246 }, { 17, 26, 146 }, { 154, 66, 84 }, { 149, 157, 238 }, { 126, 130, 72 }, { 58, 6, 101 }, { 189, 117, 101 }
};

QVector<QRgb> rsLUT::getGlasbeyPalette()
{
	int ncolors = int(sizeof glasbey_data / sizeof glasbey_data[0]);
	palette.resize(ncolors);
	for (int i = 0; i < ncolors; i++) {
		int r = (int)glasbey_data[i][0];
		int g = (int)glasbey_data[i][1];
		int b = (int)glasbey_data[i][2];
		palette[i] = qRgb(r, g, b);
	}
	lutName = "glasbey";
	return palette;
}

QVector<QRgb> rsLUT::loadPalette(QString lname)
{
	QString fn = lname + ".png";
	QFileInfo qfi(ldir, fn);
	if (qfi.isFile()) {
		QImage qimg = QImage(qfi.absoluteFilePath());
		if (!qimg.isNull()) {
			QVector<QRgb> pal = qimg.colorTable();
			if (pal.size() >= 16 && pal.size() <= 256) {
				palette = pal;
				lutName = lname;
				return palette;
			}
		}
	}
	if (lname == "glasbey")
		return getGlasbeyPalette();
	return getDefaultPalette();
}

static double cSqDist(QRgb a, QRgb b)
{
	int dr = qRed(a) - qRed(b);
	int dg = qGreen(a) - qGreen(b);
	int db = qBlue(a) - qBlue(b);
	return double(dr * dr) + double(dg * dg) + double(db * db);
}

void rsLUT::getBlackWhite(int* pb, int* pw)
{
	static constexpr QRgb black = qRgb(0, 0, 0);
	static constexpr QRgb white = qRgb(0xFF, 0xFF, 0xFF);
	int bi = 0, wi = 0xFF;
	double bdist = 1e9, wdist = 1e9;
	for (int i = 0; i < palette.size(); i++) {
		QRgb c = palette[i];
		double dist = cSqDist(c, black);
		if (dist < bdist) {
			bdist = dist;
			bi = i;
		}
		dist = cSqDist(c, white);
		if (dist < wdist) {
			wdist = dist;
			wi = i;
		}
	}
	*pb = bi;
	*pw = wi;
}

QImage rsLUT::generateNeighborLegend(int maxnn, int64 w, int64 h)
{
	int64 col, row;
	int64 h75 = h * 2 / 3;
	int64 stepw = w / 16 + 2;
	if (stepw < 5) stepw = 5;
	if (maxnn < 4) maxnn = 4;
	int64 nwidth = stepw * maxnn;

	QImage qimg(nwidth, h, QImage::Format_Grayscale8);

	uint8* onerow = new uint8[nwidth];
	uint8* p = onerow;
	for (col = 0; col < nwidth; col++) {
		uint64 c = col / stepw + 1;
		*p++ = (uint8)c;
	}
	for (row = 0; row < h75; row++) {
		uint8* p = qimg.scanLine(row);
		memcpy(p, onerow, nwidth * sizeof(uint8));
	}
	delete[] onerow;
	for (row = h75; row < h; row++) {
		uint8* p = qimg.scanLine(row);
		memset(p, 0, nwidth * sizeof(uint8));
	}

	QPainter paint;
	if (paint.begin(&qimg)) {
		int64 txh = h - h75 - 1;
		int64 fntsz = txh * 2 / 3;
		if (fntsz < 10) fntsz = 10;

		QFont font("Arial", -1, QFont::Normal);
		font.setPixelSize(fntsz);
		paint.setPen(Qt::white);
		paint.setFont(font);

		for (int step = 0; step < maxnn; step++) {
			int64 dy = 0;
			int64 dh = h75 / 12;
			if (step & 1) {
				dy = dh;
				dh = 0;
			}
			int64 nn = step + 1;
			QRect rect(step * stepw, h75 + 2 + dy, stepw, txh - 2 - dh);
			std::string label = std::to_string(nn);
			paint.drawText(rect, Qt::AlignCenter | Qt::AlignTop, label.c_str());
		}
		paint.end();
	}

	int black = 0;
	int white = 255;
	getBlackWhite(&black, &white);
	for (row = h75; row < h; row++) {
		uint8* p = qimg.scanLine(row);
		for (int64 i = 0; i < nwidth; i++) {
			if (*p >= 0xAA) *p++ = (uint8)white;
			else *p++ = (uint8)black;
		}
	}

	qimg.reinterpretAsFormat(QImage::Format_Indexed8);
	qimg.setColorTable(palette);
	return qimg;
}

QImage rsLUT::generateParamLegend(double vmin, double vmax, int64 w, int64 h)
{
	int64 col, row;
	int64 h75 = h * 2 / 3;

	QImage qimg(w, h, QImage::Format_Grayscale8);

	uint8* onerow = new uint8[w];
	uint8* p = onerow;
	for (col = 0; col < w; col++) {
		int64 c = (col * palette.size()) / w;
		*p++ = (uint8)c;
	}
	for (row = 0; row < h75; row++) {
		uint8* p = qimg.scanLine(row);
		memcpy(p, onerow, w * sizeof(uint8));
	}
	delete[] onerow;
	for (row = h75; row < h; row++) {
		uint8* p = qimg.scanLine(row);
		memset(p, 0, w * sizeof(uint8));
	}

	const char* dformat = "%1.3lf";
	if (vmax >= 100.) dformat = "%1.0lf";
	else if (vmax > 10.) dformat = "%1.1lf";
	else if (vmax > 1.) dformat = "%1.2lf";
	std::string left_label = format_double(vmin, dformat);
	std::string mid_label = format_double((vmin + vmax)/2., dformat);
	std::string right_label = format_double(vmax, dformat);

	QPainter paint;
	if (paint.begin(&qimg)) {
		uint64 txh = h - h75 - 1;

		QFont font("Arial", -1, QFont::Bold);
		font.setPixelSize(txh - 3);
		paint.setPen(Qt::white);
		paint.setFont(font);
		QRect rect(2, h75, w - 4, txh);
		paint.drawText(rect, Qt::AlignLeft | Qt::AlignTop, left_label.c_str());
		paint.drawText(rect, Qt::AlignCenter | Qt::AlignTop, mid_label.c_str());
		paint.drawText(rect, Qt::AlignRight | Qt::AlignTop, right_label.c_str());
		paint.end();
	}

	int black = 0;
	int white = 255;
	getBlackWhite(&black, &white);
	for (row = h75; row < h; row++) {
		uint8* p = qimg.scanLine(row);
		for (int64 i = 0; i < w; i++) {
			if (*p >= 0xAA) *p++ = (uint8)white;
			else *p++ = (uint8)black;
		}
	}

	qimg.reinterpretAsFormat(QImage::Format_Indexed8);
	qimg.setColorTable(palette);
	return qimg;
}

// ----------- Global utilities ------------

Raster8* read_image8(std::string fname)
{
	QImage qimg(QString(fname.c_str()));
	if (qimg.isNull()) return NULL;
	if (qimg.depth() != 8) return NULL;
	Raster8* img = new Raster8(qimg.width(), qimg.height(), NULL);
	for (int y = 0; y < img->h; y++) {
		uint8* src = qimg.scanLine(y);
		uint8* tgt = img->scanLine(y);
		memcpy(tgt, src, img->w * sizeof(uint8));
	}
	return img;
}

bool write_image8(std::string fname, Raster8* img, const char* fmt, const QVector<QRgb>* pal)
{
	QImage qimg(img->buf, img->w, img->h, img->w, QImage::Format_Grayscale8);
	if (pal) {
		qimg.reinterpretAsFormat(QImage::Format_Indexed8);
		qimg.setColorTable(*pal);
	}
	QString fn(fname.c_str());

	if (!strcmp(fmt, "png"))
		return qimg.save(fn, fmt, 6);

	QImageWriter wr(QString(fname.c_str()), QByteArray(fmt));
	wr.setCompression(1);
	return wr.write(qimg);
}

void compute_particle_parameters(rsParticlePar& p)
{
	p.AoP = p.Area / p.Peri;
	p.FeretAR = p.Feret / p.MinFeret;
	p.Compactness = sqrt((4. / PI) * p.Area) / p.EllipMaj;
	p.Extent = p.Area / ((p.Width) * (p.Height));
	p.A_hull = p.Area / p.Solidity;
	p.P_hull = 6. * sqrt(p.A_hull / (1.5 * sqrt(3.)));
	if (p.neighbors > 0)
		p.PoN = p.Peri / p.neighbors;
	else
		p.PoN = NaN;
	p.Poly_SR = p.Poly_AR = p.Poly_Ave = p.Hex_SR = p.Hex_AR = p.Hex_Ave = NaN;
	if (p.neighbors < 3) return;

	// Polygonality metrics calculated based on the number of sides of the polygon
	p.Poly_SR = 1. - sqrt((1. - (p.PoN / (sqrt((4. * p.Area) / (p.neighbors * (1. / (tan(PI / p.neighbors)))))))) * (1. - (p.PoN / (sqrt((4. * p.Area) / (p.neighbors * (1. / (tan(PI / p.neighbors)))))))));
	p.Poly_AR = 1. - sqrt((1. - (p.Area / (0.25 * p.neighbors * p.PoN * p.PoN * (1. / (tan(PI / p.neighbors)))))) * (1. - (p.Area / (0.25 * p.neighbors * p.PoN * p.PoN * (1. / (tan(PI / p.neighbors)))))));
	p.Poly_Ave = 10. * (p.Poly_SR + p.Poly_AR) / 2.;

	//Hexagonality metrics calculated based on a convex, regular, hexagon
	int ib, ic, jv;

	// Unique area calculations from the derived and primary measures above
	double apoth1 = sqrt(3.) * p.Peri / 12.;
	double apoth2 = sqrt(3.) * p.Feret / 4.;
	double apoth3 = p.MinFeret / 2.;
	double side1 = p.Peri / 6.;
	double side2 = p.Feret / 2.;
	double side3 = p.MinFeret / sqrt(3.);
	double side4 = p.P_hull / 6.;

	double Area_uniq[11];
	Area_uniq[0] = 0.5 * (3. * sqrt(3.)) * side1 * side1;
	Area_uniq[1] = 0.5 * (3. * sqrt(3.)) * side2 * side2;
	Area_uniq[2] = 0.5 * (3. * sqrt(3.)) * side3 * side3;
	Area_uniq[3] = 3. * side1 * apoth2;
	Area_uniq[4] = 3. * side1 * apoth3;
	Area_uniq[5] = 3. * side2 * apoth3;
	Area_uniq[6] = 3. * side4 * apoth1;
	Area_uniq[7] = 3. * side4 * apoth2;
	Area_uniq[8] = 3. * side4 * apoth3;
	Area_uniq[9] = p.A_hull;
	Area_uniq[10] = p.Area;

	double Area_ratio = 0.;
	jv = 0;
	for (ib = 0; ib < 11; ib++) {
		for (ic = ib + 1; ic < 11; ic++) {
			Area_ratio += 1. - sqrt((1. - (Area_uniq[ib] / Area_uniq[ic])) * (1. - (Area_uniq[ib] / Area_uniq[ic])));
			++jv;
		}
	}
	p.Hex_AR = Area_ratio / jv;

	// Perimeter Ratio Calculations
	// Two extra apothems are now useful                 
	double apoth4 = sqrt(3.) * p.P_hull / 12.;
	double apoth5 = sqrt(4. * p.A_hull / (4.5 * sqrt(3.)));

	double Perim_uniq[14];
	Perim_uniq[0] = sqrt(24. * p.Area / sqrt(3.));
	Perim_uniq[1] = sqrt(24. * p.A_hull / sqrt(3.));
	Perim_uniq[2] = p.Peri;
	Perim_uniq[3] = p.P_hull;
	Perim_uniq[4] = 3. * p.Feret;
	Perim_uniq[5] = 6. * p.MinFeret / sqrt(3.);
	Perim_uniq[6] = 2. * p.Area / apoth1;
	Perim_uniq[7] = 2. * p.Area / apoth2;
	Perim_uniq[8] = 2. * p.Area / apoth3;
	Perim_uniq[9] = 2. * p.Area / apoth4;
	Perim_uniq[10] = 2. * p.Area / apoth5;
	Perim_uniq[11] = 2. * p.A_hull / apoth1;
	Perim_uniq[12] = 2. * p.A_hull / apoth2;
	Perim_uniq[13] = 2. * p.A_hull / apoth3;

	double Perim_ratio = 0.;
	jv = 0;
	for (ib = 0; ib < 14; ib++) {
		for (ic = ib + 1; ic < 14; ic++) {
			Perim_ratio += 1 - sqrt((1. - (Perim_uniq[ib] / Perim_uniq[ic])) * (1. - (Perim_uniq[ib] / Perim_uniq[ic])));
			++jv;
		}
	}
	p.Hex_SR = Perim_ratio / jv;

	p.Hex_Ave = 10. * (p.Hex_AR + p.Hex_SR) / 2.;
}

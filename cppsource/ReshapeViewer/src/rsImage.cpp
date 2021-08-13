#include "rsimage.h"
#include "rscsvio.h"

static inline byte * replace_color(byte *p, byte bkg, byte fgd) {
	if (bkg == *p) *p = fgd;
	++p;
	return p;
}

void FillBorder(byte *buffer, uint64 width, uint64 height, byte bkg, byte fgd)
{
	uint64 row, col, off;
	byte *p;
	for (col = 0, p = buffer; col < width; col++)
		p = replace_color(p, bkg, fgd);
	for (row = 1; row < height - 1; row++) {
		off = row * width + (width - 1);
		p = replace_color(buffer + off, bkg, fgd);
		replace_color(p, bkg, fgd);
	}
	off = (height - 1)*width;
	for (col = 0, p = buffer + off; col < width; col++)
		p = replace_color(p, bkg, fgd);
}

static void sanitize(byte *buffer, uint64 width, uint64 height, byte oc, byte nc)
{
	if (!buffer) return;
	byte *p = buffer;
	uint64 size = width * height;
	for (uint64 i = 0; i < size; i++, p++) {
		if (*p != oc) *p = nc;
	}
}


static void FindParticleSegment(byte *buffer, uint64 width, uint64 height, rsParticle &pt, uint64 x, uint64 y, byte c)
{
	if (pt.GetFillSize() >= 500) {
		pt.clear();
		return;
	}

	byte *p = buffer + (y * width);
	if (p[x] != 0) return;

	uint64 x0, x1;
	for (x0 = x; x0 >= 0; x0--)
		if (p[x0] != 0) {
			++x0;
			break;
		}
	for (x1 = x + 1; x1 < width; x1++)
		if (p[x1] != 0) {
			--x1;
			break;
		}

	for (x = x0; x <= x1; x++)
		p[x] = c;
	pt.AddFill(rsHSeg(y, x0, x1));

	if (x0 <= 1 || x1 >= width - 2) return;

	if (y > 0) {
		p = buffer + ((y - 1) * width);
		for (x = x0; x <= x1; x++) {
			if (p[x] == 0)
				FindParticleSegment(buffer, width, height, pt, x, y - 1, c);
		}
	}

	if (y + 1 < height) {
		p = buffer + ((y + 1) * width);
		for (x = x0; x <= x1; x++) {
			if (p[x] == 0)
				FindParticleSegment(buffer, width, height, pt, x, y + 1, c);
		}
	}
}

void rsImageTile::FindParticleAreas(ImageType2D *pimg, std::vector<rsParticle> &particles)
{
	if (areas_read) return;

	ImageType2D::RegionType region = pimg->GetBufferedRegion();
	ImageType2D::SizeType size = region.GetSize();

	ImageType2D::PixelContainer::Pointer container = pimg->GetPixelContainer();
	byte *buffer = container->GetImportPointer();
	width = size[0];
	height = size[1];
	rsRect bnd = GetRect();

	FillBorder(buffer, width, height, 0, 0xFE);

	for (size_t iPart = 0; iPart < particles.size(); iPart++) {
		rsParticle &pt = particles[iPart];
		rsPoint orig(uint64(pt.p.xStart), uint64(pt.p.yStart));
		if (orig.x < bnd.x0 + 3 || orig.x + 3 > bnd.x1 || orig.y < bnd.y0 + 3 || orig.y + 3 > bnd.y1)
			continue;
		if (pt.GetFillSize() > 0) continue;
		orig.x -= x;
		orig.y -= y;
		FindParticleSegment(buffer, width, height, pt, orig.x, orig.y, 0x88);
		pt.TranslateFill(x, y);
		pt.validate();
		if (pt.GetUL().x < bnd.x0 + 2 || pt.GetLR().x + 2 > bnd.x1 ||
			pt.GetUL().y < bnd.y0 + 2 || pt.GetLR().y + 2 > bnd.y1) {
			pt.clear();
		}
	}

	sanitize(buffer, width, height, 0xFF, 0);
	areas_read = 1;
}

bool rsTiledImageReader::OpenTiledImage(const char *json_name)
{
	error_message.clear();
	QFile fi(json_name);
	QFileInfo fii(fi);
	imgdir = fii.dir();
	if (fi.open(QIODevice::ReadOnly)) {
		QJsonDocument json = QJsonDocument::fromJson(fi.readAll());
		fi.close();
		imgobj = json.object();
	}
	else {
		error_message = std::string("Reading JSON {") + fi.fileName().toStdString() + std::string("}");
		return false;
	}

	file_name = fii.absoluteFilePath().toStdString();
	width = height = 0;
	jtiles = QJsonArray();
	if (imgobj["width"].isDouble())
		width = (uint64)imgobj["width"].toInt();
	if (imgobj["height"].isDouble())
		height = (uint64)imgobj["height"].toInt();
	if (imgobj["numtilerows"].isDouble())
		numtilerows = imgobj["numtilerows"].toInt();
	if (imgobj["numtilecolumns"].isDouble())
		numtilecolumns = imgobj["numtilecolumns"].toInt();
	if (imgobj["tiles"].isArray())
		jtiles = imgobj["tiles"].toArray();
	if (imgobj["source"].isString())
		source = imgobj["source"].toString();
	if (width == 0 || height == 0 || jtiles.size() == 0) {
		error_message = std::string("Invalid format {") + fi.fileName().toStdString() + std::string("}");
		return false;
	}

	unit_conv = imgobj["unit_conv"].toInt();
	if (unit_conv) {
		unit_pix = imgobj["unit_pix"].toDouble();
		unit_real = imgobj["unit_real"].toDouble();
	}
	else {
		unit_pix = unit_real = 1.;
	}

	tiles.resize(jtiles.size());
	for (int iTile = 0; iTile < jtiles.size(); iTile++) {
		QJsonObject jtile = jtiles[iTile].toObject();
		rsImageTile &tile = tiles[iTile];
		tile.areas_read = false;
		tile.x = jtile["x"].toInt();
		tile.y = jtile["y"].toInt();
		tile.width = jtile["w"].toInt();
		tile.height = jtile["h"].toInt();
		tile.name = jtile["name"].toString().toStdString();
		if (jtile["attr"].isObject()) {
			QJsonObject attr = jtile["attr"].toObject();
			if (attr["Outlines"].isString())
				tile.name = attr["Outlines"].toString().toStdString();
		}
	}

	return true;
}

bool rsTiledImageReader::ReadParticleAreas(std::vector<rsParticle> & particles)
{
	if (!imgobj["areas"].isString()) return false;
	QString areaPath = imgdir.absoluteFilePath(imgobj["areas"].toString());
	QFile areaFile(areaPath);
	if (!areaFile.open(QIODevice::ReadOnly)) return false;
	QTextStream afi(&areaFile);
	while (!afi.atEnd()) {
		QString line = afi.readLine();
		QStringList parts = line.split(QChar(','));
		if (parts.size() < 1) continue;
		uint64 id = uint64(parts[0].toLongLong());
		if (!id) continue;
		rsParticle _p(id);
		particles.push_back(_p);
		rsParticle & p = particles[particles.size() - 1];
		p.p.xStart = -1;
		int nseg = (parts.size() - 1) / 3;
		for (int iseg = 0; iseg < nseg; iseg++) {
			int idx = iseg * 3 + 1;
			rsHSeg s(uint64(parts[idx].toLongLong()), uint64(parts[idx+1].toLongLong()), uint64(parts[idx+2].toLongLong()));
			p.AddFill(s);
		}
		p.validate();
	}
//	std::cout << std::to_string(particles.size()) << " particles from " << areaPath.toStdString() << std::endl;
	areaFile.close();
	for (size_t i = 0; i < tiles.size(); i++)
		tiles[i].areas_read = true;
	return true;
}

bool rsTiledImageReader::FindDataFile(QString &dataPath)
{
	if (imgobj["datafile"].isString()) {
		dataPath = imgdir.absoluteFilePath(imgobj["datafile"].toString());
		return true;
	}
	else {
		QFileInfo srcf(source);
		QString fn = srcf.baseName();
		int idx = fn.lastIndexOf('.');
		if (idx > 0) fn.resize(idx);
		fn = fn + "_Data.csv";
		
		QDir baseDir(imgdir);
		for (int lvl = 0; lvl < 2; lvl++) {
			QDir aDir(baseDir.absoluteFilePath("Analysis"));
			QFile csvf(aDir.absoluteFilePath(fn));
			if (csvf.exists()) {
				dataPath = csvf.fileName();
				return true;
			}
			baseDir.cdUp();
		}
	}
	return false;
}

bool rsTiledImageReader::ReadParticleData(std::vector<rsParticle> & particles)
{
	areas_read = ReadParticleAreas(particles);
	QString dataPath;
	if (!FindDataFile(dataPath)) return areas_read;
	rsCsvIO::Pointer csvdata = rsCsvIO::New();
	size_t i;
	csvdata->Read(dataPath.toStdString().c_str());
	if (!areas_read) {
		particles.resize(csvdata->GetRowCount());
		for (i = 0; i < particles.size(); i++)
			particles[i].SetId(uint64(i + 1));
	}

	int iXStart = csvdata->GetHeaderIndex("XStart");
	int iYStart = csvdata->GetHeaderIndex("YStart");
	int iNeighbors = csvdata->GetHeaderIndex("Neighbors");
	
	std::vector<int> dataParIdx(numVisualParams);
	for (i = 0; i < numVisualParams; i++) {
		dataParIdx[i] = csvdata->GetHeaderIndex(visualParams[i].header);
	}

	size_t iPart, iPar;
	for (iPart = iPar = 0; iPar < csvdata->GetRowCount(); iPar++) {
		csvdata->GetRow(iPar);
		uint64 id = (uint64)csvdata->GetIntAt(0);
		for (; iPart < particles.size(); iPart++)
			if (particles[iPart].GetId() >= id) break;
		if (iPart >= particles.size()) break;
		rsParticle &pt = particles[iPart];
		if (pt.GetId() != id) continue;
		pt.p.xStart = csvdata->GetIntAt(iXStart);
		pt.p.yStart = csvdata->GetIntAt(iYStart);
// std::cout << "Pt ID: " << std::to_string(id) << " x=" << std::to_string(pt.p.xStart) << " y=" << std::to_string(pt.p.yStart) << std::endl;
		pt.p.neighbors = csvdata->GetIntAt(iNeighbors);
		for (i = 0; i < numVisualParams; i++) {
			*pt.parPtr(visualParams[i].off) = csvdata->GetDoubleAt(dataParIdx[i]);
		}
		pt.FixCenter();
	}
	return true;
}

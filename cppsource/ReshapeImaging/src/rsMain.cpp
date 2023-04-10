
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "rsMain.h"

static void detectPerimeter(Raster8* img, rsParticleP& ptc, uint8 bordc)
{
	ptc.peri.clear();
	for (HSeg& hs : ptc.fill) {
		int y0 = hs.y;
		for (int x0 = hs.xl; x0 <= hs.xr; x0++) {
			for (int j = 1; j < HOOD_SIZE_MOORE; j++) {
				if (img->value(x0 + hood_pts[j].dx, y0 + hood_pts[j].dy) != bordc) continue;
				ptc.peri.push_back(Point(x0, y0));
			}
		}
	}
}

void rsTileData::loadParticles(rsImageTile& tile, Raster8* img)
{
	valid = false;
	bnd = img->getBoundary();
	CsvReader rdr(tile.csvdata.c_str());
	if (rdr.next()) {
		CsvRow headers = rdr.GetRow();
		rdr.setHeaders(headers);

		int xs_idx = rdr.header_index("XStart");
		int ys_idx = rdr.header_index("YStart");
		while (rdr.next()) {
			int x0 = rdr.GetInt(xs_idx, -1);
			int y0 = rdr.GetInt(ys_idx, -1);
			particles.resize(particles.size() + 1);
			rsParticleP& particle = particles[particles.size() - 1];
			particle.clear();
			particle.ptId = -1;
			particle.nn = 0;
			if (!bnd.IsInside(x0, y0) || img->value(x0, y0) != 0) continue;
			img->detectParticle(particle, x0, y0, 0x80);
			detectPerimeter(img, particle, 0xFF);
			particle.translate(tile.x, tile.y);
			valid = true;
		}
	}
	bnd.translate(tile.x, tile.y);
}

rsReconstruct::rsReconstruct(int argc, char** argv)
{
	t_start = clock();
	for (int i = 1; i < argc; i++) {
		std::string kv = argv[i];
		size_t pos = kv.find('=');
		if (pos != std::string::npos) {
			std::string key = kv.substr(0, pos);
			param[key] = kv.substr(pos + 1);
		}
	}
	appFile = QFileInfo(argv[0]);
	appPath = QDir(appFile.absolutePath());
	appDir = appFile.absolutePath();

	std::cout << "Parameters:" << std::endl;
	for (auto it = param.begin(); it != param.end(); ++it) {
		std::cout << it->first << " = " << it->second << std::endl;
	}
	std::cout << std::endl;
}

int rsReconstruct::open()
{
	static char* required[] = { "Basename", "Origname", "Jsonname", "AnalysisDir" };
	static int n_required = sizeof required / sizeof required[0];

	int notfound = 0;
	for (int i = 0; i < n_required; i++) {
		auto val = param.find(required[i]);
		if (val == param.end()) {
			std::cout << "ERROR: Required parameter missing <" << required[i] << ">" << std::endl;
			++notfound;
		}
	}
	if (notfound > 0) return 2;

	orig_name = param["Origname"];
	base_name = param["Basename"];
	json_name = param["Jsonname"];

	tmp_dir = param["TempDir"];
	seg_dir = param["SegmentedDir"];
	data_dir = param["AnalysisDir"];
	vis_dir = param["VisualizationDir"];
	lut_choice = param["LUT_choice"];
	graphic_choice = param["graphic_choice"];
	graphic_format = param["graphic_format"];
	feature_limits = param["feature_limits"];

	if (!timg.open(json_name)) {
		std::cout << "ERROR: " << timg.error_msg << std::endl;
		return 2;
	}

	fullsz = timg.width * timg.height;
	std::cout << "Reading: " << timg.fpath << std::endl;
	std::cout << "Image size: " << timg.width << "x" << timg.height << std::endl;
	std::cout << "Number of tiles: " << timg.numTiles() << std::endl;

	return 0;
}

void rsReconstruct::close()
{
	if (fullImg) delete fullImg;
	fullImg = NULL;
	timg.init();
	names.clear();
	tds.clear();
	particles.clear();
	ppars.clear();
	headers.clear();
}

bool rsReconstruct::overlayImage(Raster8* img, int x0, int y0)
{
	Boundary fbnd = fullImg->getBoundary();
	Boundary ibnd = img->getBoundary();
	ibnd.translate(x0, y0);
	if (fbnd.IsInside(ibnd.xmin, ibnd.ymin) && fbnd.IsInside(ibnd.xmax, ibnd.ymax)) {
		for (int y = 0; y < img->h; y++) {
			uint8* sb = img->scanLine(y);
			uint8* tb = fullImg->scanLine(y + y0) + x0;
			for (int x = 0; x < img->w; x++) {
				if (tb[x] == 0) tb[x] = sb[x];
			}
			//memcpy(tb + uint64(x0), sb, uint64(img->w) * sizeof(uint8));
		}
		return true;
	}
	return false;
}

void rsReconstruct::detectParticles(uint64 min_area, uint64 max_area, uint8 bkg, uint8 fgd)
{
	toc();
	fullImg->fillBorder(0x10);
	for (int y0 = 1; y0 < fullImg->h - 1; y0++) {
		if (y0 % 1000 == 0 && tic()) {
			int64 pct10 = int64(y0) * 1000 / int64(fullImg->h - 1);
			int64 pct = pct10 / 10; pct10 %= 10;
			std::cout << "  ...Detecting, " << pct << "." << pct10 << "% done." << std::endl << std::flush;
		}
		uint8* buf = fullImg->scanLine(y0);
		for (int x0 = 1; x0 < fullImg->w - 1; x0++) {
			if (buf[x0] != bkg) continue;
			particles.resize(particles.size() + 1);
			rsParticle& ptc = particles[particles.size() - 1];
			uint64 a = fullImg->detectParticle(ptc, x0, y0, 0x50);
			if (a >= min_area && a <= max_area) {
				fullImg->paintParticle(ptc, 0xA0);
				ptc.ptId = ptc.tileId = -1;
			}
			else {
				fullImg->paintParticle(ptc, 0x40);
				particles.resize(particles.size() - 1);
			}
		}
	}
}

int rsReconstruct::startIndex(int ytop)
{
	int iLow = 0;
	int iHigh = int(particles.size()) - 1;
	int iMid = iLow;
	while (iLow < iHigh) {
		iMid = (iLow + iHigh) / 2;
		rsParticle& ptc = particles[iMid];
		if (ptc.y0 > ytop) {
			iHigh = iMid;
		}
		else {
			iLow = iMid + 1;
		}
	}
	return iMid;
}

int64 rsReconstruct::dedupe()
{
	toc();
	for (int iTile = 0; size_t(iTile) < tds.size(); iTile++) {
		rsTileData& td = tds[iTile];
		if (!td.valid) continue;
		if (tic()) {
			std::cout << "  ...Deduping particles, tile " << (iTile + 1) << " of " << tds.size() << std::endl << std::flush;
		}
		for (int tidx = 0; size_t(tidx) < td.particles.size(); tidx++) {
			rsParticleP& tptc = td.particles[tidx];
			if (tptc.empty()) continue;
			int sPtId = -1;
			double score = 0.;
			int sy0 = tptc.y0 - 200;
			if (sy0 < 0) sy0 = 0;
			int sidx0 = startIndex(sy0);
			for (int sidx = sidx0; size_t(sidx) < particles.size(); sidx++) {
				rsParticle& sptc = particles[sidx];
				if (sptc.empty()) continue;
				if (sptc.bnd.ymin - 20 >= tptc.bnd.ymax) break;
				//if (sptc.bnd.ymin >= tptc.bnd.ymax || sptc.bnd.ymax <= tptc.bnd.ymin ||
				//	sptc.bnd.xmin >= tptc.bnd.xmax || sptc.bnd.xmax <= tptc.bnd.xmin) continue;
				uint64 ovl = tptc.overlay_area(sptc);
				if (ovl == 0) continue;
				if (sPtId >= 0) {
					sPtId = -1;
					break;
				}
				double sc = double(ovl) / (double(sptc.area) + double(tptc.area) - ovl);
				if (sc < 0.2) continue;
				score = sc;
				sPtId = sidx;
			}
			if (sPtId >= 0) {
				rsParticle& sptc = particles[sPtId];
				if ((sptc.ptId < 0 || sptc.score < score) && score > 0.8) {
					sptc.ptId = tidx;
					sptc.tileId = iTile;
					sptc.score = score;
				}
				else {
					tptc.clear();
				}
			}
			else {
				tptc.clear();
			}
		}
	}
	pt_count = 0ll;
	for (rsParticle& pt : particles) {
		if (pt.ptId >= 0) ++pt_count;
	}
	return pt_count;
}

void rsReconstruct::find_neighbors(int64 maxsqd)
{
	toc();
	for (int idx = 0; size_t(idx) < particles.size(); idx++) {
		rsParticle& pt1 = particles[idx];
		if (idx % 5000 == 0 && tic()) {
			int64 pct10 = int64(idx) * 1000ll / int64(particles.size());
			int64 pct = pct10 / 10; pct10 %= 10;
			std::cout << "  ...Counting neighbors, " << pct << "." << pct10 << "% done." << std::endl << std::flush;
		}
		if (pt1.ptId < 0) continue;
		rsParticleP& ptc1 = tds[pt1.tileId].particles[pt1.ptId];
		Boundary b1 = ptc1.bnd;
		b1.expand(10);
		ptc1.ptId = idx;
		int ylim = pt1.bnd.ymax + 10;
		for (int idx2 = idx + 1; size_t(idx2) < particles.size(); idx2++) {
			rsParticle& pt2 = particles[idx2];
			if (pt2.bnd.ymin > ylim) break;
			if (pt2.ptId < 0) continue;
			rsParticleP& ptc2 = tds[pt2.tileId].particles[pt2.ptId];
			if (!b1.intersects(ptc2.bnd)) continue;
			if (ptc1.sqdist(ptc2) < maxsqd) {
				++ptc1.nn;
				++ptc2.nn;
			}
		}
	}
}

// extra headers
static const char* extra_headers[] = {
	"Area/Perim.", "Ferets AR", "Compactness", "Extent", "Neighbors", "Area Convext Hull",
	"Perim Convext Hull", "PoN", "PSR", "PAR", "Poly_Ave", "HSR", "HAR", "Hex_Ave"
};
static const int len_extra_headers = sizeof extra_headers / sizeof extra_headers[0];
static const char* dfmt = "%1.3lf";

void rsReconstruct::read_particle_data()
{
	bool headers_set = false;

	double sc = 1.;
	if (timg.unit_conv) sc = timg.unit_real / timg.unit_pix;

	int iX, iY, iXM, iYM, iBX, iBY, iXStart, iYStart, idxNN;
	int iArea, iPeri, iEllipMaj, iElipMin, iAR, iAngle, iFeret, iMinFeret, iFeretAngle, iCirc, iSolidity;
	int iWidth, iHeight;

	for (int iTile = 0; iTile < timg.numTiles(); iTile++) {
		rsTileData& td = tds[iTile];
		if (!td.valid) continue;
		rsImageTile& tile = timg.getTile(iTile);
		CsvReader rdr(tile.csvdata.c_str());
		if (!rdr.next()) continue;
		std::cout << "  ...Reading particle data from: " << tile.csvdata << std::endl << std::flush;
		if (!headers_set) {
			headers = rdr.GetRow();
			headers_set = true;
			for (int j = 0; j < len_extra_headers; j++)
				headers.push_back(std::string(extra_headers[j]));
			rdr.setHeaders(headers);

			iX = rdr.header_index("X");
			iY = rdr.header_index("Y");
			iXM = rdr.header_index("XM");
			iYM = rdr.header_index("YM");
			iBX = rdr.header_index("BX");
			iBY = rdr.header_index("BY");
			iXStart = rdr.header_index("XStart");
			iYStart = rdr.header_index("YStart");
			idxNN = rdr.header_index("Neighbors");

			iArea = rdr.header_index("Area");
			iPeri = rdr.header_index("Perim.");
			iEllipMaj = rdr.header_index("Major");
			iElipMin = rdr.header_index("Minor");
			iAR = rdr.header_index("AR");
			iAngle = rdr.header_index("Angle");
			iFeret = rdr.header_index("Feret");
			iMinFeret = rdr.header_index("MinFeret");
			iFeretAngle = rdr.header_index("FeretAngle");
			iCirc = rdr.header_index("Circ.");
			iSolidity = rdr.header_index("Solidity");
			iWidth = rdr.header_index("Width");
			iHeight = rdr.header_index("Height");
		}
		else {
			CsvRow hdr = rdr.GetRow();
			rdr.setHeaders(hdr);
		}

		double ox = double(tile.x) * sc;
		double oy = double(tile.y) * sc;

		for (int idx = 0; size_t(idx) < td.particles.size(); idx++) {
			if (!rdr.next()) break;
			rsParticleP& ptc = td.particles[idx];
			if (ptc.ptId < 0) continue;
			rsParticle& pt = particles[ptc.ptId];
			rsParticlePar& pp = ppars[ptc.ptId];

			ptc.row = rdr.GetRow();

			// Translatable coordinates
			ptc.row[iX] = csv_double(rdr.GetDouble(iX) + ox, dfmt);
			ptc.row[iY] = csv_double(rdr.GetDouble(iY) + oy, dfmt);
			ptc.row[iXM] = csv_double(rdr.GetDouble(iXM) + ox, dfmt);
			ptc.row[iYM] = csv_double(rdr.GetDouble(iYM) + oy, dfmt);
			ptc.row[iBX] = csv_double(rdr.GetDouble(iBX) + ox, dfmt);
			ptc.row[iBY] = csv_double(rdr.GetDouble(iBY) + oy, dfmt);
			// Origin on _Outlines image (may be 1pix off from the tile image)
			ptc.row[iXStart] = std::to_string(pt.x0);
			ptc.row[iYStart] = std::to_string(pt.y0);

			// Base parameters
			pp.Area = rdr.GetDouble(iArea);
			pp.Peri = rdr.GetDouble(iPeri);
			pp.EllipMaj = rdr.GetDouble(iEllipMaj);
			pp.ElipMin = rdr.GetDouble(iElipMin);
			pp.AR = rdr.GetDouble(iAR);
			pp.Angle = rdr.GetDouble(iAngle);
			pp.Feret = rdr.GetDouble(iFeret);
			pp.MinFeret = rdr.GetDouble(iMinFeret);
			pp.FeretAngle = rdr.GetDouble(iFeretAngle);
			pp.Circ = rdr.GetDouble(iCirc);
			pp.Solidity = rdr.GetDouble(iSolidity);
			pp.Width = rdr.GetDouble(iWidth);
			pp.Height = rdr.GetDouble(iHeight);

			// Parameters derived from #neighbors + base parameters
			pp.neighbors = ptc.nn;
			compute_particle_parameters(pp);

			// Append derived parameters to the CSV row
			ptc.row.push_back(csv_double(pp.AoP, dfmt));
			ptc.row.push_back(csv_double(pp.FeretAR, dfmt));
			ptc.row.push_back(csv_double(pp.Compactness, dfmt));
			ptc.row.push_back(csv_double(pp.Extent, dfmt));
			ptc.row.push_back(std::to_string(pp.neighbors));
			ptc.row.push_back(csv_double(pp.A_hull, dfmt));
			ptc.row.push_back(csv_double(pp.P_hull, dfmt));
			ptc.row.push_back(csv_double(pp.PoN, dfmt));
			ptc.row.push_back(csv_double(pp.Poly_SR, dfmt));
			ptc.row.push_back(csv_double(pp.Poly_AR, dfmt));
			ptc.row.push_back(csv_double(pp.Poly_Ave, dfmt));
			ptc.row.push_back(csv_double(pp.Hex_SR, dfmt));
			ptc.row.push_back(csv_double(pp.Hex_AR, dfmt));
			ptc.row.push_back(csv_double(pp.Hex_Ave, dfmt));

			// Limit precision to 3 digits past "." for a few base parameters
			ptc.row[iSolidity] = csv_double(pp.Solidity, dfmt);
			ptc.row[iPeri] = csv_double(pp.Peri, dfmt);
			ptc.row[iFeret] = csv_double(pp.Feret, dfmt);
			ptc.row[iMinFeret] = csv_double(pp.MinFeret, dfmt);
			ptc.row[iEllipMaj] = csv_double(pp.EllipMaj, dfmt);
		}
	}

}

int rsReconstruct::write_particle_data()
{
	dat_name = data_dir + base_name + "_Data.csv";
	nbr_name = data_dir + base_name + "_Neighbors.csv";

	int nb_counts[32];
	maxnn = 1;
	memset(nb_counts, 0, sizeof nb_counts);

	CsvWriter wr(dat_name.c_str());
	if (!wr.is_open()) {
		std::cout << "Error writing <" << dat_name << ">" << std::endl;
		return 0;
	}
	wr.SetRow(headers);
	wr.next();
	int id = 1;
	for (rsParticle& ptc : particles) {
		if (ptc.ptId < 0) continue;
		rsParticleP& pt = tds[ptc.tileId].particles[ptc.ptId];
		pt.row[0] = std::to_string(id);
		++id;
		wr.WriteRow(pt.row);

		if (pt.nn > 31) continue;
		++nb_counts[pt.nn];
		if (maxnn < pt.nn) maxnn = pt.nn;
	}
	wr.close();

	double elapsed = (double)(clock() - t_start) / CLOCKS_PER_SEC;
	std::cout << "[" << elapsed << "] ParticleData: {" << dat_name << "}" << std::endl;
	names.push_back(nbr_name);

	CsvWriter nwr(nbr_name.c_str());
	if (!nwr.is_open()) {
		std::cout << "Error writing <" << nbr_name << ">" << std::endl;
		return 0;
	}

	nwr.append(" ");
	nwr.append("Neighbors");
	nwr.append(orig_name);
	nwr.next();

	for (int nn = 0; nn <= maxnn; nn++) {
		nwr.append(int(nn + 1));
		nwr.append(nn);
		nwr.append(nb_counts[nn]);
		nwr.next();
	}

	nwr.close();

	elapsed = (double)(clock() - t_start) / CLOCKS_PER_SEC;
	std::cout << "[" << elapsed << "] NeighborStats: {" << nbr_name << "}" << std::endl;
	names.push_back(nbr_name);

	return maxnn;
}

void rsReconstruct::write_outlines_image()
{
	std::string out_name = vis_dir + base_name + "_Outlines." + img_format;
	if (write_image8(out_name, fullImg, img_format.c_str())) {
		double elapsed = (double)(clock() - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] Outlines: {" << out_name << "}" << std::endl;
		names.push_back(out_name);
	}
	else {
		std::cout << "Error writing <" << out_name << ">" << std::endl;
	}
}

void rsReconstruct::write_neighbors_image()
{
	for (int idx = 0; size_t(idx) < particles.size(); idx++) {
		rsParticle& pt = particles[idx];
		if (pt.ptId < 0) continue;
		rsParticlePar& pp = ppars[idx];
		fullImg->paintParticle(pt, uint8(pp.neighbors));
	}
	int cb = 0, cw = 0xFF;
	lut.getBlackWhite(&cb, &cw);
	if (cb != 0) fullImg->replaceColor(0xFF, cb);

	std::string nbr_name = vis_dir + base_name + "_Neighbors." + img_format;
	int64 w = (int64(fullImg->w) + 4) / 5;
	int64 h = (int64(fullImg->h) + 19) / 20;
	QImage qimg = lut.generateNeighborLegend(maxnn, w, h);
	for (int y = 0; y < qimg.height(); y++) {
		uint8* s = qimg.scanLine(y);
		uint8* t = fullImg->scanLine(y);
		memcpy(t, s, qimg.width() * sizeof(uint8));
	}
	QVector<QRgb> pal = lut.getPalette();
	if (write_image8(nbr_name, fullImg, img_format.c_str(), &pal)) {
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] Neighbors: {" << nbr_name << "}" << std::endl;
		names.push_back(nbr_name);
	}
	else {
		std::cout << "Error writing <" << nbr_name << ">" << std::endl;
	}
}

void rsReconstruct::write_parameter_images()
{
	QJsonObject jlim;
	bool use_limits = false;
	if (!feature_limits.empty()) {
		QFile fi(feature_limits.c_str());
		if (fi.open(QIODevice::ReadOnly)) {
			std::cout << "Using Feature Limits from: " << feature_limits.c_str() << std::endl;
			QJsonDocument json = QJsonDocument::fromJson(fi.readAll());
			fi.close();
			jlim = json.object();
			use_limits = true;
		}
	}

	int64 w = (int64(fullImg->w) + 4) / 5;
	int64 h = (int64(fullImg->h) + 19) / 20;

	for (size_t iPar = 0; iPar < numVisualParams; iPar++) {
		rsDisplayPar* dPar = visualParams + iPar;
		std::string out_name = vis_dir + base_name + "_" + std::string(dPar->label) + "." + img_format;

		fullImg->fill(0);
		double vmin = NaN;
		double vmax = NaN;
		for (int64 i = 0; size_t(i) < particles.size(); i++) {
			rsParticle& pt = particles[i];
			if (pt.ptId < 0) continue;
			rsParticlePar* pp = &(ppars[i]);
			double v = *(double*)(((char*)pp) + dPar->off);
			if (std::isnan(v)) continue;
			if (std::isnan(vmin)) {
				vmin = vmax = v;
			}
			else {
				if (vmin > v) vmin = v;
				if (vmax < v) vmax = v;
			}
		}
		if (vmin == vmax) vmax = vmax + 1.;

		if (use_limits) {
			QString minl = QString("Min.") + QString(dPar->label);
			if (jlim[minl].isDouble()) vmin = jlim[minl].toDouble();
			QString maxl = QString("Max.") + QString(dPar->label);
			if (jlim[maxl].isDouble()) vmax = jlim[maxl].toDouble();
		}
		if (vmin >= vmax) {
			vmin = -1.;
			vmax = 0.;
		}

		for (int64 i = 0; size_t(i) < particles.size(); i++) {
			rsParticle& pt = particles[i];
			if (pt.ptId < 0) continue;
			rsParticlePar* pp = &(ppars[i]);
			double v = *(double*)(((char*)pp) + dPar->off);
			if (std::isnan(v)) continue;

			double cc = 5. + (250. * (v - vmin) / (vmax - vmin));
			if (cc < 2. || cc >= 255.5) continue;
			uint8 c = (uint8)(cc + 0.5);
			fullImg->paintParticle(pt, c);
		}

		QImage qimg = lut.generateParamLegend(vmin, vmax, w, h);
		for (int y = 0; y < qimg.height(); y++) {
			uint8* s = qimg.scanLine(y);
			uint8* t = fullImg->scanLine(y);
			memcpy(t, s, qimg.width() * sizeof(uint8));
		}
		QVector<QRgb> pal = lut.getPalette();
		if (write_image8(out_name, fullImg, img_format.c_str(), &pal)) {
			clock_t t_end = clock();
			double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
			std::cout << "[" << elapsed << "] " << dPar->label << ": {" << out_name << "}" << std::endl;
			names.push_back(out_name);
		}
		else {
			std::cout << "Error writing <" << out_name << ">" << std::endl;
		}
	}
}

void rsReconstruct::handleMultipleTiles()
{
	fullImg = new Raster8(timg.width, timg.height, NULL);
	fullImg->fill(0);

	std::cout << "Reconstructing Outlines..." << std::endl << std::flush;
	int64 cnt = 0;
	clock_t tstart = clock();
	toc();
	for (int iTile = 0; iTile < timg.numTiles(); iTile++) {
		rsImageTile& tile = timg.getTile(iTile);
		if (tic()) {
			std::cout << "...Tile " << tile.name << " of " << timg.numTiles() << std::endl << std::flush;
		}
		Raster8* img = read_image8(tile.outlines);
		if (img) {
			overlayImage(img, tile.x, tile.y);

			img->fillBorder(0x10);
			tds[iTile].loadParticles(tile, img);

			cnt += int64(tds[iTile].particles.size());

			delete img;
		}
	}
	std::cout << "Writing reconstructed Outlines..." << std::endl << std::flush;
	std::string out_name = seg_dir + base_name + "_Outlines.tif";
	write_image8(out_name, fullImg, "tif");
	std::cout << "Reconstructing Outlines done in " << format_elapsed(tstart) <<
		"; Loaded " << cnt << " particles." << std::endl << std::flush;

	tstart = clock();
	std::cout << "Detecting particles on reconstructed Outlines image..." << std::endl << std::flush;
	detectParticles(10ll, 500000ll);
	std::cout << "Detecting done in " << format_elapsed(tstart) <<
		"; Found " << particles.size() << " particles." << std::endl << std::flush;

	tstart = clock();
	std::cout << "Removing particle duplicates (due to tile overlaps)..." << std::endl << std::flush;
	int64 valid_cnt = dedupe();
	std::cout << "De-duping done in " << format_elapsed(tstart) <<
		"; Final particle count: " << valid_cnt << std::endl << std::flush;
}

void rsReconstruct::handleSingleTile()
{
	clock_t tstart = clock();
	std::cout << "Loading Outlines..." << std::endl << std::flush;
	rsImageTile& tile = timg.getTile(0);
	rsTileData& td = tds[0];
	fullImg = read_image8(tile.outlines);
	pt_count = 0;
	if (fullImg) {
		fullImg->fillBorder(0x10);
		td.loadParticles(tile, fullImg);
		pt_count += int64(td.particles.size());
		fullImg->sanitize(0, 0xFF);
	}
	else {
		fullImg = new Raster8(timg.width, timg.height, NULL);
		fullImg->fill(0);
	}
	std::cout << "Writing Outlines..." << std::endl << std::flush;
	std::string out_name = seg_dir + base_name + "_Outlines.tif";
	write_image8(out_name, fullImg, "tif");

	particles.resize(td.particles.size());
	for (int idx = 0; size_t(idx) < particles.size(); idx++) {
		rsParticle& pt = particles[idx];
		rsParticleP& ptc = td.particles[idx];
		pt.copyfrom(ptc);
		pt.ptId = idx;
		pt.tileId = 0;
		ptc.ptId = idx;
	}

	std::cout << "Loading Outlines done in " << format_elapsed(tstart) <<
		"; Final particle count: " << pt_count << std::endl << std::flush;
}

int rsReconstruct::reconstructTiles()
{
	img_format = "tif";
	if (graphic_format.find("PNG") != std::string::npos && fullsz < 0x7FFFFFFFll)
		img_format = "png";
	std::cout << "Image file format: " << img_format << std::endl;

	tds.resize(timg.numTiles());

	try {
		if (timg.numTiles() > 1)
			handleMultipleTiles();
		else
			handleSingleTile();
	}
	catch (...) {
		std::cout << "ERROR: Out of memory." << std::endl;
		return 2;
	}

	clock_t tstart = clock();
	std::cout << "Counting particle neighbors..." << std::endl << std::flush;
	find_neighbors();
	for (rsTileData& td : tds) {
		for (rsParticleP& pt : td.particles) {
			pt.fill.clear();
			pt.peri.clear();
		}
	}
	std::cout << "Counting neighbors done in " << format_elapsed(tstart) << std::endl << std::flush;

	ppars.resize(particles.size());

	tstart = clock();
	std::cout << "Reading particle data..." << std::endl << std::flush;
	read_particle_data();
	std::cout << "Reading data done in " << format_elapsed(tstart) << std::endl << std::flush;

	tstart = clock();
	std::cout << "Writing particle data..." << std::endl << std::flush;
	write_particle_data();
	for (rsTileData& td : tds) {
		for (rsParticleP& pt : td.particles) {
			pt.row.clear();
		}
	}
	std::cout << "Writing data done in " << format_elapsed(tstart) << std::endl << std::flush;

	if (need_images()) {
		tstart = clock();
		std::cout << "About to generate color-coded images." << std::endl;
		lut = rsLUT(appDir);
		std::cout << "LUT directory: " << lut.getLutDir().absolutePath().toStdString() << std::endl << std::flush;
		
		// Outlines
		fullImg->sanitize(0, 0xFF);
		write_outlines_image();
		// Neighbors
		lut.loadPalette("glasbey");
		std::cout << "LUT palette for Neighbors image: " << lut.getLutName().toStdString() << std::endl << std::flush;
		write_neighbors_image();
		// Parameters
		lut.loadPalette(lut_choice.c_str());
		std::cout << "LUT palette for Parameter images: " << lut.getLutName().toStdString() << std::endl << std::flush;
		write_parameter_images();
		std::cout << "Writing color-coded images done in " << format_elapsed(tstart) << std::endl << std::flush;
	}

	return 0;
}

struct ptDup {
	int ptId1, tileId1, ptId2, tileId2;
	ptDup() {}
	ptDup(int _ptId1, int _tileId1, int _ptId2, int _tileId2) :
		ptId1(_ptId1), tileId1(_tileId1), ptId2(_ptId2), tileId2(_tileId2) {}
};

int64 rsReconstruct::dedupeTiled()
{
	std::vector<ptDup> dups;

	toc();
	for (int it = 0; size_t(it) < tds.size(); it++) {
		if (tic()) {
			int64 pct10 = int64(it) * 1000ll / int64(tds.size());
			int64 pct = pct10 / 10; pct10 %= 10;
			std::cout << "  De-duping particles, " << pct << "." << pct10 << "% done" << std::endl << std::flush;
		}
		rsTileData& tdi = tds[it];
		for (int jt = 0; size_t(jt) < tds.size(); jt++) {
			if (jt == it) continue;
			rsTileData& tdj = tds[jt];
			if (!tdj.bnd.intersects(tdi.bnd)) continue;

			for (int i = 0; size_t(i) < tdi.particles.size(); i++) {
				rsParticleP& pti = tdi.particles[i];
				if (pti.bdist < 0 || pti.fill.empty()) continue;

				int nm = 0;
				int mj = -1;

				for (int j = 0; size_t(j) < tdj.particles.size(); j++) {
					rsParticleP& ptj = tdj.particles[j];
					if (ptj.bdist < 0 || ptj.fill.empty()) continue;
					uint64 ovl = pti.overlay_area(ptj);
					if (ovl > 0) {
						++nm;
						if (nm > 1) break;
						mj = j;
					}
				}

				if (nm > 1) {
					pti.fill.clear();
				}
				else if (nm == 1) {
					dups.push_back(ptDup(i, it, mj, jt));
				}
			}
		}
	}

	for (ptDup& d : dups) {
		rsParticleP& pti = tds[d.tileId1].particles[d.ptId1];
		rsParticleP& ptj = tds[d.tileId2].particles[d.ptId2];
		if (pti.fill.empty() || ptj.fill.empty()) continue;
		if (pti.bdist < ptj.bdist) {
			pti.fill.clear();
		}
		else {
			ptj.fill.clear();
		}
	}

	// Merge particles into single list, sorted by (y0, x0)
	std::cout << "Merging particles from separate tiles into single sorted list..." << std::endl << std::flush;
	pt_count = 0;
	std::vector<size_t> ptids(tds.size());
	int64 it=-1ll, y0=0, x0=0;
	while (true) {
		it = -1ll;
		y0 = timg.height + 10;
		x0 = timg.width + 10;
		for (int iTile = 0; size_t(iTile) < ptids.size(); iTile++) {
			size_t idx = ptids[iTile];
			if (idx >= tds[iTile].particles.size()) continue;
			rsParticleP& pt = tds[iTile].particles[idx];
			if (it < 0 || pt.y0 < y0 || (pt.y0 == y0 && pt.x0 < x0)) {
				it = iTile;
				y0 = pt.y0;
				x0 = pt.x0;
			}
		}
		if (it < 0) break;
		int ptId = int(ptids[it]);
		++ptids[it];
		rsParticleP& pt = tds[it].particles[ptId];
		if (pt.fill.empty()) continue;

		rsParticle& ptc = particles[pt_count];
		pt.ptId = pt_count;
		++pt_count;

		ptc.ptId = ptId;
		ptc.tileId = it;
		ptc.bnd = pt.bnd;
	};

	particles.resize(pt_count);
	return pt_count;
}

int rsReconstruct::separateTiles()
{
	if (timg.numtilecolumns < 1 || timg.numtilerows < 1) {
		std::cout << "ERROR: Invalid numtilerows/numtilecolumns: {" << timg.fpath << "}" << std::endl;
		return 2;
	}

	std::cout << "Segmented dir: " << seg_dir << std::endl;
	QDir segDir(seg_dir.c_str());
	QString subDir(base_name.c_str());
	subDir = subDir + "_tiles";
	QDir tilesDir(segDir.absolutePath() + "/" + subDir);
	std::cout << "Tiles dir: " << tilesDir.absolutePath().toStdString() << std::endl;
	if (!tilesDir.exists())
		if (!segDir.mkpath(subDir)) {
			std::cout << "ERROR: " << "Failed to create directory " << tilesDir.absolutePath().toStdString() << std::endl;
			return 2;
		}

	tds.resize(timg.numTiles());

	std::cout << "Loading particles..." << std::endl << std::flush;
	int64 cnt = 0;
	clock_t tstart = clock();
	toc();
	for (int iTile = 0; iTile < timg.numTiles(); iTile++) {
		rsImageTile& tile = timg.getTile(iTile);
		if (tic()) {
			std::cout << "...Tile " << tile.name << " of " << timg.numTiles() << std::endl << std::flush;
		}
		Raster8* img = read_image8(tile.outlines);
		if (img) {
			tile.w = img->w;
			tile.h = img->h;

			tile.name = std::string("tile_") + std::to_string(iTile + 1) + ".tif";
			QString tName(tile.name.c_str());
			QString tPath = tilesDir.absoluteFilePath(tName);
			write_image8(tPath.toStdString(), img);

			img->fillBorder(0x10);
			tds[iTile].loadParticles(tile, img);
			cnt += int64(tds[iTile].particles.size());
			delete img;
		}

		tds[iTile].bnd = tile.getBoundary();
	}
	particles.resize(cnt);

	// Tile inner boundaries
	int ntiles = timg.numTiles();
	for (int iTile = 0; iTile < ntiles; iTile++) {
		rsImageTile& tile = timg.getTile(iTile);
		rsTileData& td = tds[iTile];
		td.inner = td.bnd;
		int iRow = iTile / timg.numtilecolumns;
		int iCol = iTile % timg.numtilecolumns;

		if (iRow > 0) {
			int iTop = (iRow - 1) * timg.numtilecolumns + iCol;
			td.inner.ymin = tds[iTop].bnd.ymax + 1;
		}
		if (iCol < timg.numtilecolumns - 1) {
			int iRight = iRow * timg.numtilecolumns + iCol + 1;
			td.inner.xmax = tds[iRight].bnd.xmin - 1;
		}
		if (iRow < timg.numtilerows - 1) {
			int iBottom = (iRow + 1) * timg.numtilecolumns + iCol;
			td.inner.ymax = tds[iBottom].bnd.ymin - 1;
		}
		if (iCol > 0) {
			int iLeft = iRow * timg.numtilecolumns + iCol - 1;
			td.inner.xmin = tds[iLeft].bnd.xmax + 1;
		}

		for (rsParticleP& pt : td.particles) {
			if (td.inner.contains(pt.bnd)) pt.bdist = -1;
			else pt.bdist = td.bnd.minbdist(pt.bnd);
		}
	}

	std::cout << "Loading particles done in " << format_elapsed(tstart) <<
		"; Loaded " << cnt << " particles." << std::endl << std::flush;

	std::cout << "Removing particle duplicates..." << std::endl << std::flush;
	tstart = clock();
	cnt = dedupeTiled();
	std::cout << "De-duping done in " << format_elapsed(tstart) <<
		"; Final particle count: " << cnt << std::endl << std::flush;

	Raster8* img = new Raster8(timg.width, timg.height, NULL);
	img->fill(0);
	for (rsParticle& ptc : particles) {
		rsParticleP& pt = tds[ptc.tileId].particles[ptc.ptId];
		if (pt.bdist < 0) {
			img->paintParticle(pt, 0xC0);
		}
		else {
			img->paintParticleADD(pt, 0x3F);
		}
	}
	std::string fn("combo.tif");
	write_image8(fn, img);
	delete img;

	tstart = clock();
	std::cout << "Finding particle neighbors..." << std::endl << std::flush;
	find_neighbors();
	std::cout << "Finding neighbors done in " << format_elapsed(tstart) << std::endl << std::flush;

	ppars.resize(particles.size());

	tstart = clock();
	std::cout << "Reading particle data..." << std::endl << std::flush;
	read_particle_data();
	std::cout << "Reading data done in " << format_elapsed(tstart) << std::endl << std::flush;

	tstart = clock();
	std::cout << "Writing particle data..." << std::endl << std::flush;
	write_particle_data();
	std::cout << "Writing data done in " << format_elapsed(tstart) << std::endl << std::flush;

	// Particle areas
	QString arName("areas.csv");
	{
		QString arPath = tilesDir.absoluteFilePath(arName);
		CsvWriter wr(arPath.toStdString().c_str());
		toc();
		if (wr.is_open()) {
			std::cout << "Writing particle areas..." << std::endl << std::flush;
			tstart = clock();
			int id = 1;
			for (rsParticle& ptc : particles) {
				if (id % 5000 == 0 && tic()) {
					int64 pct10 = int64(id) * 1000ll / int64(particles.size());
					int64 pct = pct10 / 10; pct10 %= 10;
					std::cout << "  Writing areas, " << pct << "." << pct10 << "% done" << std::endl << std::flush;
				}
				rsParticleP& pt = tds[ptc.tileId].particles[ptc.ptId];
				wr.append(id);
				for (HSeg& hs : pt.fill) {
					wr.append(hs.y);
					wr.append(hs.xl);
					wr.append(hs.xr);
				}
				++id;
				wr.next();
			}
			wr.close();
			double elapsed = (double)(clock() - t_start) / CLOCKS_PER_SEC;
			std::cout << "[" << elapsed << "] ParticleAreas: {" << arPath.toStdString() << "}" << std::endl;
			std::cout << "Writing areas done in " << format_elapsed(tstart) << std::endl << std::flush;
		}
	}

	// Tiled image metadata
	std::cout << "Writing tiled image metadata..." << std::endl;
	QJsonObject jout;
	jout["source"] = timg.source.c_str();
	jout["width"] = QJsonValue(int64(timg.width));
	jout["height"] = QJsonValue(int64(timg.height));
	jout["numtilerows"] = QJsonValue(timg.numtilerows);
	jout["numtilecolumns"] = QJsonValue(timg.numtilecolumns);
	jout["unit_conv"] = QJsonValue(timg.unit_conv);
	jout["unit_pix"] = QJsonValue(timg.unit_pix);
	jout["unit_real"] = QJsonValue(timg.unit_real);
	jout["datafile"] = segDir.relativeFilePath(QString(dat_name.c_str()));
	jout["areas"] = subDir + "/" + arName;
	QJsonArray joutTiles;
	for (int iTile = 0; iTile < timg.numTiles(); iTile++) {
		rsImageTile& tile = timg.getTile(iTile);
		QJsonObject jtile;
		jtile["x"] = QJsonValue((int64)tile.x);
		jtile["y"] = QJsonValue((int64)tile.y);
		jtile["w"] = QJsonValue((int64)tile.w);
		jtile["h"] = QJsonValue((int64)tile.h);
		jtile["name"] = subDir + "/" + QString(tile.name.c_str());
		joutTiles.append(jtile);
	}
	jout["tiles"] = joutTiles;

	QString toutFileName = segDir.absolutePath() + "/" + base_name.c_str() + ".tiled";
	QFile fo(toutFileName);
	QJsonDocument json = QJsonDocument(jout);
	if (fo.open(QIODevice::WriteOnly)) {
		fo.write(json.toJson(QJsonDocument::Indented));
		fo.close();
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] Outlines: {" << toutFileName.toStdString() << "}" << std::endl;
	}

	return 0;
}

int rsReconstruct::process()
{
	if (param["reconstruct_tiled"] == "No" && timg.numTiles() > 1)
		return separateTiles();
	return reconstructTiles();
}

std::string rsReconstruct::format_elapsed(clock_t tstart)
{
	char buf[64];
	uint64 tsec = uint64(10.*(clock() - tstart) / CLOCKS_PER_SEC);
	int dec = tsec % 10; tsec /= 10;
	int sec = tsec % 60; tsec /= 60;
	int min = tsec % 60;
	int hrs = int(tsec / 60);
	sprintf(buf, "%d:%02d:%02d.%01d", hrs, min, sec, dec);
	return std::string(buf);
}

int main(int argc, char* argv[])
{
	rsReconstruct proc(argc, argv);
	QGuiApplication app(argc, argv);
	int rc = proc.open();
	if (!rc) rc = proc.process();

	if (!rc) {
		std::cout << "Elapsed: " << proc.format_elapsed(proc.getStartTs()) << std::endl;
	}
	std::cout << "Exiting(" << rc << ")" << std::endl;
	return rc;
}


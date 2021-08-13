#include "rsSeparateTiles.h"

void write_byte_image(rsImage *img, const char *out_file)
{
	QImage qimg(img->GetImportPointer(), img->GetWidth(), img->GetHeight(), img->GetWidth(), QImage::Format_Grayscale8);
	QImageWriter wr;
	wr.setFormat("tif");
	wr.setCompression(1);
	wr.setFileName(out_file);
	wr.write(qimg);
}

void read_argv(int argc, char **argv, Properties &prop)
{
	for (int i = 1; i < argc; i++) {
		std::string kv = argv[i];
		size_t pos = kv.find('=');
		if (pos != std::string::npos) {
			std::string key = kv.substr(0, pos);
			prop[key] = kv.substr(pos + 1);
		}
	}
}

bool rsSeparateTilesImageReader::OpenTiledImage(const char *json_name)
{
	error_message.clear();
	QFile fi(json_name);
	imgdir = QFileInfo(fi).dir();
	if (fi.open(QIODevice::ReadOnly)) {
		QJsonDocument json = QJsonDocument::fromJson(fi.readAll());
		fi.close();

		imgobj = json.object();
	}
	else {
		error_message = std::string("Reading JSON {") + fi.fileName().toStdString() + std::string("}");
		return false;
	}
	width = height = 0;
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

	tiles.resize(jtiles.size());

	unit_conv = imgobj["unit_conv"].toInt();
	if (unit_conv) {
		unit_pix = imgobj["unit_pix"].toDouble();
		unit_real = imgobj["unit_real"].toDouble();
	}
	else {
		unit_pix = unit_real = 1.;
	}

	return true;
}

bool rsSeparateTilesImageReader::ReadTile(int iTile, const char *out_file)
{
	QJsonObject jtile = jtiles[iTile].toObject();
	if (!jtile["attr"].isObject()) {
		error_message = "Invalid format";
		return false;
	}
	QJsonObject attr = jtile["attr"].toObject();
	QFileInfo tpath(attr["Outlines"].toString());

	std::cout << tpath.absoluteFilePath().toStdString() << std::endl;

	uint64 x = (uint64)jtile["x"].toInt();
	uint64 y = (uint64)jtile["y"].toInt();

	double dx = (double)x;
	double dy = (double)y;
	if (unit_conv) {
		dx = x * unit_real / unit_pix;
		dy = y * unit_real / unit_pix;
	}

	rsImageReader rd;
	rsImage *timg = rd.ReadImage(tpath.absoluteFilePath().toStdString().c_str());
	if (!timg) {
		error_message = rd.GetError();
		return false;
	}

	rsImageTile &tile = tiles[iTile];
	tile.x = x;
	tile.y = y;
	tile.width = timg->GetWidth();
	tile.height = timg->GetHeight();
	tile.csvdata->SetDoubleFormat("%1.3lf");

	QFileInfo rpath(attr["Results"].toString());
	std::cout << rpath.absoluteFilePath().toStdString() << std::endl;

	tile.csvdata->Read(rpath.absoluteFilePath().toStdString().c_str());
	read_particle_params(*tile.csvdata, tile.pOrigs);

	timg->FillBorder(0, 0xFE);

	//	, Area, Mean, StdDev, Mode, Min, Max, X, Y, XM, YM, Perim., BX, BY, Width, Height, Major, Minor, Angle, Circ., Feret, IntDen, Median, Skew, Kurt, %Area, RawIntDen, FeretX, FeretY, FeretAngle, MinFeret, AR, Round, Solidity, XStart, YStart
	int iX = tile.csvdata->GetHeaderIndex("X");
	int iY = tile.csvdata->GetHeaderIndex("Y");
	int iXM = tile.csvdata->GetHeaderIndex("XM");
	int iYM = tile.csvdata->GetHeaderIndex("YM");
	int iBX = tile.csvdata->GetHeaderIndex("BX");
	int iBY = tile.csvdata->GetHeaderIndex("BY");
	int iXStart = tile.csvdata->GetHeaderIndex("XStart");
	int iYStart = tile.csvdata->GetHeaderIndex("YStart");

	tile.particles.resize(tile.pOrigs.size());
	tile.p_map.resize(tile.pOrigs.size());
	size_t i, j = 0;
	for (i = 0; i < tile.pOrigs.size(); i++) {
		rsParticleOrigin & po = tile.pOrigs[i];
		rsParticle srcpt = timg->FindParticle(po);
		rsParticle &pt = tile.particles[j];
		pt.translate(srcpt, x, y);

		rsCsvRow & cRow = tile.csvdata->GetRow(i);
		tile.csvdata->SetDoubleAt(iX, tile.csvdata->GetDoubleAt(iX) + dx);
		tile.csvdata->SetDoubleAt(iY, tile.csvdata->GetDoubleAt(iY) + dy);
		tile.csvdata->SetDoubleAt(iXM, tile.csvdata->GetDoubleAt(iXM) + dx);
		tile.csvdata->SetDoubleAt(iYM, tile.csvdata->GetDoubleAt(iYM) + dy);
		tile.csvdata->SetDoubleAt(iBX, tile.csvdata->GetDoubleAt(iBX) + dx);
		tile.csvdata->SetDoubleAt(iBY, tile.csvdata->GetDoubleAt(iBY) + dy);

		tile.csvdata->SetIntAt(iXStart, po.GetX());
		tile.csvdata->SetIntAt(iYStart, po.GetY());

		tile.p_map[j].first = &(tile.particles[j]);
		tile.p_map[j].second = &cRow;

		++j;
	}

	timg->sanitize(0xFF, 0);
	if (out_file) {
		write_byte_image(timg, out_file);
		std::cout << "Writing Tile " << std::to_string(iTile + 1) << " to " << out_file << std::endl;
	}

	return true;
}

int rsSeparateTilesImageReader::readTiles(const char *tiles_dir)
{
	QDir tDir(tiles_dir);
	int iTile;
	for (iTile = 0; iTile < GetNumTiles(); iTile++) {
		std::string t_name = "tile_";
		t_name = t_name + std::to_string(iTile + 1) + ".tif";
		QString tName(t_name.c_str());
		QString tPath = tDir.absoluteFilePath(tName);
		if (!ReadTile(iTile, tPath.toStdString().c_str())) break;
		tiles[iTile].name = t_name;
	}
	return iTile;
}

rsCsvIO::Pointer rsSeparateTilesImageReader::GetParticleData(std::vector<rsParticle> &particles)
{
	rsCsvIO::Pointer csvdata = rsCsvIO::New();
	csvdata->SetDoubleFormat("%1.3lf");

	std::vector<rsParticleDesc> p_map;

	size_t n_particles = 0;

	bool headers_read = false;
	int iTile;
	for (iTile = 0; iTile < tiles.size(); iTile++) {
		rsImageTile &tile = tiles[iTile];
		tile.ResetRowIter();
		n_particles += tile.p_map.size();
		if (!headers_read) {
			rsCsvRow & headers = tile.csvdata->GetHeaders();
			if (headers.size() > 0) {
				for (std::string & hdr : headers)
					csvdata->AppendHeader(hdr);
				headers_read = true;
			}
		}
	}

	p_map.resize(n_particles);
	size_t iPart = 0;

	rsParticleDesc *next_pd = NULL;
	rsImageTile *p_tile = NULL;
	rsPoint cpt;
	do {
		next_pd = NULL;
		p_tile = NULL;
		size_t cur_tidx;
		for (iTile = 0; iTile < tiles.size(); iTile++) {
			rsImageTile &tile = tiles[iTile];
			if (!tile.HasNextRow()) continue;
			rsParticleDesc *pd = tile.PeekNextRow();
			if (next_pd != NULL)
				if (pd->first->GetPoint().compare(cpt) < 0) {
					next_pd = NULL;
				}
			if (next_pd == NULL) {
				next_pd = pd;
				cpt = next_pd->first->GetPoint();
				p_tile = &tile;
				cur_tidx = iTile;
			}
		}
		if (!next_pd) break;
		p_map[iPart] = *next_pd;
		++iPart;
		p_tile->PopNextRow();
	} while (next_pd);

	for (iPart = 0; iPart < p_map.size()-1; iPart++) {
		rsParticleDesc &pd = p_map[iPart];
		if (!pd.first) continue;
		rsPoint lr = pd.first->GetLR();
		rsPoint ul = pd.first->GetUL();
		uint64 pa = pd.first->GetArea();
		bool marked = false;
		for (size_t j = iPart + 1; j < p_map.size(); j++) {
			rsParticleDesc &opd = p_map[j];
			if (!opd.first) continue;
			rsPoint oul = opd.first->GetUL();
			if (oul.y > lr.y) break;
			if (oul.x > lr.x || opd.first->GetLR().x < ul.x) continue;
			uint64 area = pd.first->intersect(*(opd.first)) * 5 / 4;
			if (area > pa) {
				opd.first = NULL;
				break;
			}
			else if (area > opd.first->GetArea()) {
				marked = true;
			}
		}
		if (marked)
			pd.first = NULL;
	}

	particles.resize(n_particles);

	size_t nPart = 0;
	for (iPart = 0; iPart < p_map.size(); iPart++) {
		rsParticleDesc &pd = p_map[iPart];
		if (!pd.first) continue;
		particles[nPart] = *(pd.first);
		rsParticle &ptn = particles[nPart];
		csvdata->AppendRow(*(pd.second));
		++nPart;
		ptn.GetOrigin()->SetId((uint64)nPart);
		csvdata->SetIntAt(0, (int)nPart);
	}
	particles.resize(nPart);

	return csvdata;
}


int separate_tiles_main(std::string &app_dir, Properties & param)
{
	std::cout << "Processing tiles without reconstructing the original size image." << std::endl;

	std::string orig_name = param["Origname"];
	std::string base_name = param["Basename"];
	std::string json_name = param["Jsonname"];

	std::string tmp_dir = param["TempDir"];
	std::string seg_dir = param["SegmentedDir"];
	std::string data_dir = param["AnalysisDir"];

	if (orig_name.empty() || base_name.empty() || json_name.empty()) {
		std::cout << "ERROR: Required parameter missing" << std::endl;
		return EXIT_FAILURE;
	}

	clock_t t_start, t_end;
	double elapsed;
	t_start = clock();

	std::cout << "Segmented dir: " << seg_dir << std::endl;
	QDir segDir(seg_dir.c_str());
	QString subDir(base_name.c_str());
	subDir = subDir + "_tiles";
	QDir tilesDir(segDir.absolutePath() + "/" + subDir);
	std::cout << "Tiles dir: " << tilesDir.absolutePath().toStdString() << std::endl;
	if (!tilesDir.exists())
	if (!segDir.mkpath(subDir)) {
		std::cout << "ERROR: " << "Failed to create directory " << tilesDir.absolutePath().toStdString() << std::endl;
		return EXIT_FAILURE;
	}

	// Open "tiled" image medtadata
	rsSeparateTilesImageReader treader;
	if (!treader.OpenTiledImage(json_name.c_str())) {
		std::cout << "ERROR: " << treader.GetError() << std::endl;
		return EXIT_FAILURE;
	}
	else {
		std::cout << "Image size: " << std::to_string(treader.GetWidth()) << "x" << std::to_string(treader.GetHeight()) << std::endl;
		std::cout << "Number of tiles: " << std::to_string(treader.GetNumTiles()) << std::endl;
	}

	// Read tiles one at a time (tiff + csv)
	int nread = treader.readTiles(tilesDir.absolutePath().toStdString().c_str());
	if (nread != treader.GetNumTiles()) {
		std::cout << "ERROR: " << treader.GetError() << std::endl;
		return EXIT_FAILURE;
	}

	// Combine/dedupe particle CSV data
	std::vector<rsParticle> particles;
	rsCsvIO::Pointer csvdata = treader.GetParticleData(particles);

	std::cout << "Final number of particles: " << std::to_string(particles.size()) << std::endl;

	t_end = clock();
	elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;

	// Find particle neighbors
	std::cout << "Finding Particle Neighbors for " << orig_name << std::endl;
	std::cout << "<" << elapsed << "> 0 of " << particles.size() << " particles" << std::endl;
	find_neighbors(particles, EXPANDSQ, t_start);
	t_end = clock();
	elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
	std::cout << "<" << elapsed << "> " << particles.size() << " of " << particles.size() << " particles" << std::endl;

	// Write CSV (particle data, neighbors)
	std::string data_name = write_particle_data(*csvdata, particles, t_start, base_name, orig_name, data_dir);

	// Write particle areas
	rsCsvIO::Pointer csvareas = rsCsvIO::New();
	csvareas->SetHasColumnHeaders(false);
	for (size_t iPart = 0; iPart < particles.size(); iPart++) {
		rsParticle &pt = particles[iPart];
		rsCsvRow csvr;
		csvareas->AppendRow(csvr);
		csvareas->AppendInt(pt.GetId());
		for (size_t iFill = 0; iFill < pt.GetFillSize(); iFill++) {
			rsHSeg seg = pt.GetFillAt(iFill);
			csvareas->AppendInt(seg.y);
			csvareas->AppendInt(seg.x0);
			csvareas->AppendInt(seg.x1);
		}
	}

	QString arName("areas.csv");
	QString arPath = tilesDir.absoluteFilePath(arName);
	if (!csvareas->Write(arPath.toStdString())) {
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] ParticleAreas: {" << arPath.toStdString() << "}" << std::endl;
	}

	// Write JSON metadata
	QJsonObject jout;
	jout["source"] = treader.GetSource();
	jout["width"] = QJsonValue((long long)treader.GetWidth());
	jout["height"] = QJsonValue((long long)treader.GetHeight());
	jout["numtilerows"] = QJsonValue(treader.GetNumTileRows());
	jout["numtilecolumns"] = QJsonValue(treader.GetNumTileColumns());
	jout["unit_conv"] = QJsonValue(treader.GetUnitConv());
	jout["unit_pix"] = QJsonValue(treader.GetUnitPix());
	jout["unit_real"] = QJsonValue(treader.GetUnitReal());
	jout["datafile"] = segDir.relativeFilePath(QString(data_name.c_str()));
	jout["areas"] = subDir + "/" + arName;
	QJsonArray joutTiles;
	for (size_t iTile = 0; iTile < treader.GetNumTiles(); iTile++) {
		rsImageTile & tile = treader.GetTileAt(iTile);
		QJsonObject jtile;
		jtile["x"] = QJsonValue((long long)tile.x);
		jtile["y"] = QJsonValue((long long)tile.y);
		jtile["w"] = QJsonValue((long long)tile.width);
		jtile["h"] = QJsonValue((long long)tile.height);
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

	return EXIT_SUCCESS;
}
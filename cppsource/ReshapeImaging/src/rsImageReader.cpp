
#include "rsImageReader.h"

rsImage * rsImageReader::ReadImage(const char *img_name)
{
	ReaderType2D::Pointer reader = ReaderType2D::New();
	reader->SetFileName(img_name);
	pimg = reader->GetOutput();

	try {
		pimg->Update();
		error_message.clear();
	}
	catch (itk::ExceptionObject & err) {
		error_message = err.GetDescription();
		return NULL;
	}

	ImageType2D::RegionType region = pimg->GetBufferedRegion();
	ImageType2D::SizeType size = region.GetSize();

	container = pimg->GetPixelContainer();
	// container->SetContainerManageMemory(false);
	byte *buffer = container->GetImportPointer();

	img.set_data(buffer, size[0], size[1]);

	return &img;
}

// Read Particle parameters from the Results file
void read_particle_params(rsCsvIO &csvdata, std::vector<rsParticleOrigin> & pOrigs)
{
	uint64 nparticles = csvdata.GetRowCount();
	pOrigs.resize(nparticles);
	int idxX = csvdata.GetHeaderIndex("XStart");
	int idxY = csvdata.GetHeaderIndex("YStart");
	int idxNN = csvdata.GetHeaderIndex("Neighbors");

	int iArea = csvdata.GetHeaderIndex("Area");
	int iPeri = csvdata.GetHeaderIndex("Perim.");
	int iEllipMaj = csvdata.GetHeaderIndex("Major");
	int iElipMin = csvdata.GetHeaderIndex("Minor");
	int iAR = csvdata.GetHeaderIndex("AR");
	int iAngle = csvdata.GetHeaderIndex("Angle");
	int iFeret = csvdata.GetHeaderIndex("Feret");
	int iMinFeret = csvdata.GetHeaderIndex("MinFeret");
	int iFeretAngle = csvdata.GetHeaderIndex("FeretAngle");
	int iCirc = csvdata.GetHeaderIndex("Circ.");
	int iSolidity = csvdata.GetHeaderIndex("Solidity");
	int iWidth = csvdata.GetHeaderIndex("Width");
	int iHeight = csvdata.GetHeaderIndex("Height");

	for (uint64 iRow = 0; iRow < nparticles; iRow++) {
		csvdata.GetRow(iRow);
		rsParticleOrigin &po = pOrigs[iRow];
		po.SetData(iRow + 1, csvdata.GetIntAt(idxX), csvdata.GetIntAt(idxY), csvdata.GetIntAt(idxNN));

		po.p.Area = csvdata.GetDoubleAt(iArea);
		po.p.Peri = csvdata.GetDoubleAt(iPeri);
		po.p.EllipMaj = csvdata.GetDoubleAt(iEllipMaj);
		po.p.ElipMin = csvdata.GetDoubleAt(iElipMin);
		po.p.AR = csvdata.GetDoubleAt(iAR);
		po.p.Angle = csvdata.GetDoubleAt(iAngle);
		po.p.Feret = csvdata.GetDoubleAt(iFeret);
		po.p.MinFeret = csvdata.GetDoubleAt(iMinFeret);
		po.p.FeretAngle = csvdata.GetDoubleAt(iFeretAngle);
		po.p.Circ = csvdata.GetDoubleAt(iCirc);
		po.p.Solidity = csvdata.GetDoubleAt(iSolidity);
		po.p.Width = csvdata.GetDoubleAt(iWidth);
		po.p.Height = csvdata.GetDoubleAt(iHeight);
	}
}

std::string write_particle_data(rsCsvIO &csvdata, std::vector<rsParticle> & particles, clock_t t_start, std::string & base_name, std::string & orig_name, std::string & data_dir)
{
	std::string nbr_name = data_dir + base_name + "_Neighbors.csv";
	std::string data_name = data_dir + base_name + "_Data.csv";

	csvdata.AppendHeader("Area/Perim.");
	csvdata.AppendHeader("Ferets AR");
	csvdata.AppendHeader("Compactness");
	csvdata.AppendHeader("Extent");
	csvdata.AppendHeader("Neighbors");
	csvdata.AppendHeader("Area Convext Hull");
	csvdata.AppendHeader("Perim Convext Hull");
	csvdata.AppendHeader("PoN");
	csvdata.AppendHeader("PSR");
	csvdata.AppendHeader("PAR");
	csvdata.AppendHeader("Poly_Ave");
	csvdata.AppendHeader("HSR");
	csvdata.AppendHeader("HAR");
	csvdata.AppendHeader("Hex_Ave");

	int iSolidity = csvdata.GetHeaderIndex("Solidity");
	int iPeri = csvdata.GetHeaderIndex("Perim.");
	int iFeret = csvdata.GetHeaderIndex("Feret");
	int iMinFeret = csvdata.GetHeaderIndex("MinFeret");
	int iEllipMaj = csvdata.GetHeaderIndex("Major");

	int nn_counts[32];
	int maxnn = 0;
	memset(nn_counts, 0, sizeof(nn_counts));

	for (size_t i = 0; i < particles.size(); i++) {
		rsParticle & pt = particles[i];
		rsParticleOrigin &po = *pt.GetOrigin();
		size_t nn = pt.GetNeighborCount();
		po.p.neighbors = (int)nn;
		compute_particle_parameters(po.p);

		csvdata.GetRow(pt.GetId() - 1);
		csvdata.SetDoubleAt(iSolidity, po.p.Solidity);
		csvdata.SetDoubleAt(iPeri, po.p.Peri);
		csvdata.SetDoubleAt(iFeret, po.p.Feret);
		csvdata.SetDoubleAt(iMinFeret, po.p.MinFeret);
		csvdata.SetDoubleAt(iEllipMaj, po.p.EllipMaj);

		csvdata.AppendDouble(po.p.AoP);
		csvdata.AppendDouble(po.p.FeretAR);
		csvdata.AppendDouble(po.p.Compactness);
		csvdata.AppendDouble(po.p.Extent);
		csvdata.AppendInt(po.p.neighbors);
		csvdata.AppendDouble(po.p.A_hull);
		csvdata.AppendDouble(po.p.P_hull);
		csvdata.AppendDouble(po.p.PoN);
		csvdata.AppendDouble(po.p.Poly_SR);
		csvdata.AppendDouble(po.p.Poly_AR);
		csvdata.AppendDouble(po.p.Poly_Ave);
		csvdata.AppendDouble(po.p.Hex_SR);
		csvdata.AppendDouble(po.p.Hex_AR);
		csvdata.AppendDouble(po.p.Hex_Ave);

		if (nn > 31) nn = 31;
		nn_counts[nn]++;
		if (maxnn < nn) maxnn = nn;
	}

	if (!csvdata.Write(data_name)) {
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] ParticleData: {" << data_name << "}" << std::endl;
	}

	std::ofstream fout(nbr_name);
	if (fout.is_open())
	{
		fout << " ,Neighbors," << csv_quote(orig_name) << std::endl;
		for (size_t i = 0; i <= maxnn; i++) {
			fout << (i + 1) << "," << i << "," << nn_counts[i] << std::endl;
		}
		fout.close();
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] NeighborStats: {" << nbr_name << "}" << std::endl;
	}

	return data_name;
}

rsImage * rsTiledImageReader::ReadTiledImage(const char *json_name)
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
		return NULL;
	}
	width = height = 0;
	while (tiles.size()) tiles.pop_back();
	if (imgobj["width"].isDouble())
		width = (uint64)imgobj["width"].toInt();
	if (imgobj["height"].isDouble())
		height = (uint64)imgobj["height"].toInt();
	if (imgobj["tiles"].isArray())
		jtiles = imgobj["tiles"].toArray();
	if (width == 0 || height == 0 || jtiles.size()==0) {
		error_message = std::string("Invalid format {") + fi.fileName().toStdString() + std::string("}");
		return NULL;
	}

	tiles.resize(jtiles.size());

	ImageType2D::RegionType region;
	ImageType2D::IndexType start;
	start[0] = 0;
	start[1] = 0;
	ImageType2D::SizeType size;
	size[0] = width;
	size[1] = height;
	region.SetSize(size);
	region.SetIndex(start);

	pimg = ImageType2D::New();
	pimg->SetRegions(region);
	pimg->Allocate();

	container = pimg->GetPixelContainer();
	byte *buffer = container->GetImportPointer();
	img.set_data(buffer, size[0], size[1]);
	memset(buffer, 0, width*height);

	unit_conv = imgobj["unit_conv"].toInt();
	if (unit_conv) {
		unit_pix = imgobj["unit_pix"].toDouble();
		unit_real = imgobj["unit_real"].toDouble();
	}
	else {
		unit_pix = unit_real = 1.;
	}

	for (int iTile = 0; iTile < GetNumTiles(); iTile++) {
		if (!ReadTile(iTile)) return NULL;
	}

	return &img;
}

bool rsTiledImageReader::ReadTile(int iTile)
{
	QJsonObject jtile = jtiles[iTile].toObject();
	if (!jtile["attr"].isObject()) {
		error_message = "Invalid format";
		return false;
	}
	QJsonObject attr = jtile["attr"].toObject();
	QFileInfo tpath(attr["Outlines"].toString());

	std::cout << tpath.absoluteFilePath().toStdString() << std::endl;

	uint64 x = (uint64) jtile["x"].toInt();
	uint64 y = (uint64) jtile["y"].toInt();

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
	read_particle_params(* tile.csvdata, tile.pOrigs);

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
	size_t i, j=0;
	for (i = 0; i < tile.pOrigs.size(); i++) {
		rsParticleOrigin & po = tile.pOrigs[i];
		rsParticle srcpt = timg->FindParticle(po);
		rsParticle &pt = tile.particles[j];
		pt.translate(srcpt, x, y);

		uint64 a = img.ParticleArea(pt);
		if (a*2 < pt.GetArea()) {
			timg->PaintParticle(srcpt, 0);
			pt.clear();
		}
		else {
			img.PaintParticle(pt, 0);
			timg->PaintParticle(srcpt, 0xAA);

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
	}
	tile.particles.resize(j);
	tile.p_map.resize(j);

	timg->FillBorder(0xFE, 0);

	std::cout << "About to overlay tile " << std::to_string(iTile) << std::endl;
	byte *src, *tgt;
	for (uint64 row = 0; row < timg->GetHeight(); row++)
	{
		src = timg->GetImportPointer() + (row * timg->GetWidth());
		tgt = img.GetImportPointer() + ((row + y)*img.GetWidth() + x);
		for (uint64 col = 0; col < timg->GetWidth(); col++) {
			if (*tgt == 0 || *src==0xFF) { *tgt++ = *src++; }
			else { tgt++; src++; }
		}
	}

	return true;
}

rsCsvIO::Pointer rsTiledImageReader::GetParticleData(std::vector<rsParticle> & particles)
{
	rsCsvIO::Pointer csvdata = rsCsvIO::New();
	csvdata->SetDoubleFormat("%1.3lf");
	
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

	int iXStart = csvdata->GetHeaderIndex("XStart");
	int iYStart = csvdata->GetHeaderIndex("YStart");

	particles.resize(n_particles);
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
		particles[iPart] = *(next_pd->first);
		rsParticle &ptn = particles[iPart];
		csvdata->AppendRow(*(next_pd->second));
		++iPart;
		ptn.GetOrigin()->SetId((uint64)iPart);
		csvdata->SetIntAt(0, (int)iPart);
		p_tile->PopNextRow();
	} while (next_pd);

	return csvdata;
}

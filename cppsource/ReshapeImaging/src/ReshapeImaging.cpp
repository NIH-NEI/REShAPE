#include "rsSeparateTiles.h"

typedef std::vector<std::string> NameList;

/*
typedef  itk::ImageFileWriter<ImageType2D> WriterType2D;
void write_byte_image(ImageType2D *img, const char *out_file)
{
	WriterType2D::Pointer writer = WriterType2D::New();
	TIFFIOType::Pointer tiffIO = TIFFIOType::New();
	tiffIO->SetPixelType(itk::ImageIOBase::SCALAR);
	// tiffIO->SetCompressionToLZW();
	tiffIO->SetCompressionToPackBits();
	writer->SetFileName(out_file);
	writer->SetInput(img);
	writer->SetImageIO(tiffIO);
	writer->SetUseCompression(true);
	writer->Update();
}
*/

void write_outlines_image(rsImage *img, clock_t t_start, std::string & base_name, std::string & vis_dir, NameList & names)
{
	std::string out_name = vis_dir + base_name + "_Outlines." + img->GetFormat();
	QImage qimg(img->GetImportPointer(), img->GetWidth(), img->GetHeight(), img->GetWidth(), QImage::Format_Grayscale8);
	QImageWriter wr;
	wr.setFormat(img->GetFormat().c_str());
	wr.setCompression(1);
	wr.setFileName(out_name.c_str());
	if (wr.write(qimg))
	{
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] Outlines: {" << out_name << "}" << std::endl;
		names.push_back(out_name);
	}
	else {
		std::cout << "Error writing <" << out_name << ">" << std::endl;
	}
}

/*
void write_outlines_image(rsImage *img, clock_t t_start, std::string & base_name, std::string & vis_dir, NameList & names)
{
	RGBImageType::Pointer image = binaryToItk(img);
	std::string out_name = vis_dir + base_name + "_Outlines.tif";

	WriterTypeRGB::Pointer writer = WriterTypeRGB::New();
	TIFFIOType::Pointer tiffIO = TIFFIOType::New();
	tiffIO->SetPixelType(itk::ImageIOBase::RGB);
//	tiffIO->SetCompressionToDeflate();
//	tiffIO->SetCompressionToLZW();
	writer->SetFileName(out_name);
	writer->SetInput(image);
//	writer->SetUseCompression(true);
	writer->SetImageIO(tiffIO);
	try
	{
		writer->Update();
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] Outlines: {" << out_name << "}" << std::endl;
		names.push_back(out_name);
	}
	catch (itk::ExceptionObject & err)
	{
		std::cout << "ExceptionObject caught !" << std::endl;
		std::cout << err << std::endl;
	}
}
*/

void write_neighbor_image(rsImage *img, std::vector<rsParticle> & particles, clock_t t_start, std::string & base_name, std::string & vis_dir, std::string & app_dir, NameList & names)
{
	int maxnn = 1;
	img->fill(0);
	for (size_t i = 0; i < particles.size(); i++) {
		rsParticle & pt = particles[i];
		rsParticlePar *pp = &(pt.GetOrigin()->p);
		int nn = pp->neighbors;

		if (maxnn < nn) maxnn = nn;
		img->PaintParticle(pt, (byte)nn);
	}

	rsLUT lutio(app_dir, "glasbey");
	lutio.setSize((img->GetWidth() + 4) / 5, (img->GetHeight() + 19) / 20);

	QImage qimg(img->GetWidth(), img->GetHeight(), QImage::Format_Grayscale8);
	byte *src = img->GetImportPointer();
	for (uint64 i = 0; i < img->GetHeight(); i++) {
		byte *p = qimg.scanLine((int)i);
		memcpy(p, src, img->GetWidth() * sizeof(byte));
		src += img->GetWidth();
	}

	lutio.overlayNbLegend(maxnn, qimg);

	QImageWriter wr;
	wr.setFormat(img->GetFormat().c_str());
	wr.setCompression(1);

	std::string out_name = vis_dir + base_name + "_Neighbors." + img->GetFormat();
	wr.setFileName(out_name.c_str());
	if (wr.write(qimg))
	{
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] Neighbors: {" << out_name << "}" << std::endl;
		names.push_back(out_name);
	}
	else {
		std::cout << "Error writing <" << out_name << ">" << std::endl;
	}
}

/*
void write_neighbor_image(rsImage *img, std::vector<rsParticle> & particles, clock_t t_start, std::string & base_name, std::string & vis_dir, std::string & app_dir, NameList & names)
{
	int maxnn = 1;
	img->fill(0);
	for (size_t i = 0; i < particles.size(); i++) {
		rsParticle & pt = particles[i];
		int nn = pt.GetOrigin()->p.neighbors;
		if (maxnn < nn) maxnn = nn;
		img->PaintParticle(pt, (byte)nn);
	}

	rsLUT lutio(app_dir, "glasbey");
	lutio.setSize((img->GetWidth()+4)/5, (img->GetHeight()+19)/20);
	RGBImageType::Pointer itkImg = lutio.rsToItk(img);

	lutio.generateNbLegend(maxnn, itkImg);

	std::string out_name = vis_dir + base_name + "_Neighbors.tif";
	WriterTypeRGB::Pointer writer = WriterTypeRGB::New();
	TIFFIOType::Pointer tiffIO = TIFFIOType::New();
	tiffIO->SetPixelType(itk::ImageIOBase::RGB);
//	tiffIO->SetCompressionToDeflate();
//	tiffIO->SetCompressionToLZW();
	writer->SetFileName(out_name);
	writer->SetInput(itkImg);
//	writer->SetUseCompression(true);
	writer->SetImageIO(tiffIO);
	try
	{
		writer->Update();
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] Neighbors: {" << out_name << "}" << std::endl;
		names.push_back(out_name);
	}
	catch (itk::ExceptionObject & err)
	{
		std::cout << "ExceptionObject caught !" << std::endl;
		std::cout << err << std::endl;
	}

}
*/

void write_parameter_images(rsImage *img, std::string & lut_choice, std::vector<rsParticle> & particles, clock_t t_start, std::string & base_name, std::string & vis_dir, std::string & app_dir, std::string &feature_limits, NameList & names)
{
	QJsonObject jlim;
	QFile fi(feature_limits.c_str());
	if (fi.open(QIODevice::ReadOnly)) {
		std::cout << "Reading " << feature_limits.c_str() << std::endl;
		QJsonDocument json = QJsonDocument::fromJson(fi.readAll());
		fi.close();
		jlim = json.object();
	}

	rsLUT lutio(app_dir, lut_choice);
	lutio.setSize((img->GetWidth() + 4) / 5, (img->GetHeight() + 19) / 20);

	QImage qimg(img->GetWidth(), img->GetHeight(), QImage::Format_Grayscale8);

	QImageWriter wr;
	wr.setFormat(img->GetFormat().c_str());
	wr.setCompression(1);

	for (size_t iPar = 0; iPar < numVisualParams; iPar++) {
		rsDisplayPar *dPar = visualParams + iPar;

		size_t i;
		img->fill(0);
		double vmin = NaN;
		double vmax = NaN;
		for (i = 0; i < particles.size(); i++) {
			rsParticle & pt = particles[i];
			rsParticlePar *pp = &(pt.GetOrigin()->p);
			double v = *(double *)(((char *)pp) + dPar->off);
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

		QString minl = QString("Min.") + QString(dPar->label);
		if (jlim[minl].isDouble()) vmin = jlim[minl].toDouble();
		QString maxl = QString("Max.") + QString(dPar->label);
		if (jlim[maxl].isDouble()) vmax = jlim[maxl].toDouble();
		if (vmin >= vmax) {
			vmin = -1.;
			vmax = 0.;
		}

		for (i = 0; i < particles.size(); i++) {
			rsParticle & pt = particles[i];
			rsParticlePar *pp = &(pt.GetOrigin()->p);
			double v = *(double *)(((char *)pp) + dPar->off);
			if (std::isnan(v)) continue;

			double cc = 5. + (250. * (v - vmin) / (vmax - vmin));
			if (cc < 2. || cc >= 255.5) continue;
			byte c = (byte)(cc+0.5);
			img->PaintParticle(pt, c);
		}

		byte *src = img->GetImportPointer();
		for (i = 0; i < img->GetHeight(); i++) {
			byte *p = qimg.scanLine((int)i);
			memcpy(p, src, img->GetWidth() * sizeof(byte));
			src += img->GetWidth();
		}

		lutio.overlayLegend(vmin, vmax, qimg);

		std::string out_name = vis_dir + base_name + "_" + std::string(dPar->label) + "." + img->GetFormat();
		wr.setFileName(out_name.c_str());
		if (wr.write(qimg))
		{
			clock_t t_end = clock();
			double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
			std::cout << "[" << elapsed << "] " << dPar->label << ": {" << out_name << "}" << std::endl;
			names.push_back(out_name);
		}
		else {
			std::cout << "Error writing <" << out_name << ">" << std::endl;
		}

		qimg.reinterpretAsFormat(QImage::Format_Grayscale8);
	}
}

/*
void write_parameter_images(rsImage *img, std::string & lut_choice, std::vector<rsParticle> & particles, clock_t t_start, std::string & base_name, std::string & vis_dir, std::string & app_dir, NameList & names)
{
	rsLUT lutio(app_dir, lut_choice);
	lutio.setSize((img->GetWidth() + 4) / 5, (img->GetHeight() + 19) / 20);

	RGBImageType::Pointer itkImg = lutio.allocItk(img);

	WriterTypeRGB::Pointer writer = WriterTypeRGB::New();
	TIFFIOType::Pointer tiffIO = TIFFIOType::New();
	tiffIO->SetPixelType(itk::ImageIOBase::RGB);
	//	tiffIO->SetCompressionToDeflate();
	//	tiffIO->SetCompressionToLZW();
	//	writer->SetUseCompression(true);
	writer->SetImageIO(tiffIO);

	for (size_t iPar = 0; iPar < numVisualParams; iPar++) {
		rsDisplayPar *dPar = visualParams + iPar;

		size_t i;
		img->fill(0);
		double vmin = NaN;
		double vmax = NaN;
		for (i = 0; i < particles.size(); i++) {
			rsParticle & pt = particles[i];
			rsParticlePar *pp = &(pt.GetOrigin()->p);
			double v = *(double *)(((char *)pp) + dPar->off);
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

		for (i = 0; i < particles.size(); i++) {
			rsParticle & pt = particles[i];
			rsParticlePar *pp = &(pt.GetOrigin()->p);
			double v = *(double *)(((char *)pp) + dPar->off);
			if (std::isnan(v)) continue;

			byte c = (byte)(5.5 + (250. * (v - vmin) / (vmax - vmin)));
			img->PaintParticle(pt, c);
		}

		std::string out_name = vis_dir + base_name + "_" + std::string(dPar->label) + ".tif";
		lutio.fillItk(img, itkImg);
		lutio.generateLegend(vmin, vmax, itkImg);

		writer->SetFileName(out_name);
		writer->SetInput(itkImg);

		try
		{
			writer->Update();
			clock_t t_end = clock();
			double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
			std::cout << "[" << elapsed << "] " << dPar->label << ": {" << out_name << "}" << std::endl;
			names.push_back(out_name);
		}
		catch (itk::ExceptionObject & err)
		{
			std::cout << "ExceptionObject caught !" << std::endl;
			std::cout << err << std::endl;
			return;
		}
	}
}
*/

void write_stack(clock_t t_start, std::string & base_name, std::string & vis_dir, NameList & names)
{
	typedef itk::Image<byte, 3>  SeriesImageType;	// RGBPixelType
	typedef itk::ImageSeriesReader<SeriesImageType> SeriesReaderType;
	typedef itk::ImageFileWriter<SeriesImageType> SeriesWriterType;
	SeriesWriterType::Pointer writer = SeriesWriterType::New();
	TIFFIOType::Pointer tiffIO = TIFFIOType::New();
	tiffIO->SetPixelType(itk::ImageIOBase::SCALAR);	// itk::ImageIOBase::RGB
//	tiffIO->SetCompressionToDeflate();
//	tiffIO->SetCompressionToLZW();
	writer->SetUseCompression(true);
	writer->SetImageIO(tiffIO);

	std::string out_name = vis_dir + base_name + "_Visualized.tif";
	SeriesReaderType::Pointer reader = SeriesReaderType::New();

	reader->SetFileNames(names);
	writer->SetFileName(out_name);
	writer->SetInput(reader->GetOutput());
	try
	{
		writer->Update();
		clock_t t_end = clock();
		double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
		std::cout << "[" << elapsed << "] TIFFStack: {" << out_name << "}" << std::endl;
	}
	catch (itk::ExceptionObject & err)
	{
		std::cout << "ExceptionObject caught !" << std::endl;
		std::cout << err << std::endl;
	}

}

int main(int argc, char *argv[])
{
	itk::OutputWindow::SetInstance(itk::TextOutput::New());
	QGuiApplication app(argc, argv);

	QFileInfo appFile(argv[0]);
	QDir appPath(appFile.absolutePath());
	std::string app_dir = appFile.absolutePath().toStdString();

	// std::cout << "sizeof(uint) = " << std::to_string(sizeof(uint)) << std::endl;
	// std::cout << "sizeof(uint64) = " << std::to_string(sizeof(uint64)) << std::endl;

/*
	rsLUT jetp(app_dir, std::string("thermal"));
	jetp.setSize(400, 120);
	jetp.generateLegendOverlay(1., 10.);
	return 0;
*/

	Properties param;
	read_argv(argc, argv, param);

	std::string reconstruct_tiled = param["reconstruct_tiled"];
	if (reconstruct_tiled == "No")
		return separate_tiles_main(app_dir, param);

	NameList names;

	std::string orig_name = param["Origname"];
	std::string base_name = param["Basename"];
	std::string json_name = param["Jsonname"];

	std::string tmp_dir = param["TempDir"];
	std::string seg_dir = param["SegmentedDir"];
	std::string data_dir = param["AnalysisDir"];
	std::string vis_dir = param["VisualizationDir"];
	std::string lut_choice = param["LUT_choice"];
	std::string graphic_choice = param["graphic_choice"];
	std::string graphic_format = param["graphic_format"];
	std::string feature_limits = param["feature_limits"];

	if (orig_name.empty() || base_name.empty() || json_name.empty()) {
		std::cout << "ERROR: Required parameter missing" << std::endl;
		return EXIT_FAILURE;
	}

	bool need_images = (!graphic_choice.empty()) && (graphic_choice != "None");
	bool need_stack = need_images && (graphic_format.find("Stack") != std::string::npos);
	std::string img_format = "tif";
	if (graphic_format.find("PNG") != std::string::npos)
		img_format = "png";

	clock_t t_start, t_end;
	double elapsed;
	t_start = clock();

	rsTiledImageReader treader;
	rsImage *img = treader.ReadTiledImage(json_name.c_str());
	if (!img) {
		std::cout << "ERROR: " << treader.GetError() << std::endl;
		return EXIT_FAILURE;
	}
	else {
		std::cout << "Image size: " << std::to_string(treader.GetWidth()) << "x" << std::to_string(treader.GetHeight()) << std::endl;
		std::cout << "Number of tiles: " << std::to_string(treader.GetNumTiles()) << std::endl;
	}
	img->SetFormat(img_format);

	std::vector<rsParticle> particles;

	rsCsvIO::Pointer csvdata = treader.GetParticleData(particles);

	img->sanitize(0xFF, 0);
	if (treader.GetNumTiles() > 1) {
		std::cout << "Rescanning combined particles" << std::endl;
		img->FillBorder(0, 1);
		for (size_t ii = 0; ii < particles.size(); ii++) {
			rsParticle & pt = particles[ii];
			img->RescanParticle(pt);
		}
		img->sanitize(0xFF, 0);
	}

	std::string out_name = seg_dir + base_name + "_Outlines.tif";
	// write_byte_image(treader.GetItkImage(), out_name.c_str());
	std::cout << "Writing Outlines: " << out_name << std::endl;
	write_byte_image(img, out_name.c_str());

	if (need_images) {
		write_outlines_image(img, t_start, base_name, vis_dir, names);
	}

/*
	rsImageReader reader;
	rsImage *img = reader.ReadImage(tiff_name.c_str());
	if (!img) {
		std::cout << "ERROR: reading TIFF {" << tiff_name << "} " << reader.GetError() << std::endl;
		return EXIT_FAILURE;
	}

	img->FillBorder(0, 0xFE);

	rsCsvIO::Pointer csvdata = rsCsvIO::New();
	csvdata->SetDoubleFormat("%1.3lf");
	if (csvdata->Read(res_name)) {
		std::cout << "ERROR: reading CSV {" << res_name << "} " << csvdata->GetErrorMessage() << std::endl;
		return EXIT_FAILURE;
	}

	std::vector<rsParticleOrigin> pOrigs;
	read_particle_params(*csvdata, pOrigs);

	std::cout << "Read " << pOrigs.size() << " particle coords from " << res_name << std::endl;

	// Find particle areas
	std::cout << "Loading Particles for " << tiff_name << std::endl;
	std::vector<rsParticle> particles(pOrigs.size());
	size_t i;
	for (i = 0; i < pOrigs.size(); i++) {
		particles[i] = img->FindParticle(pOrigs[i]);
	}
*/

	t_end = clock();
	elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;

	// Find particle neighbors
	std::cout << "Finding Particle Neighbors for " << orig_name << std::endl;
	std::cout << "<" << elapsed << "> 0 of " << particles.size() << " particles" << std::endl;
	find_neighbors(particles, EXPANDSQ, t_start);
	t_end = clock();
	elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
	std::cout << "<" << elapsed << "> " << particles.size() << " of " << particles.size() << " particles" << std::endl;

	write_particle_data(*csvdata, particles, t_start, base_name, orig_name, data_dir);

	if (need_images) {
		write_neighbor_image(img, particles, t_start, base_name, vis_dir, app_dir, names);
		write_parameter_images(img, lut_choice, particles, t_start, base_name, vis_dir, app_dir, feature_limits, names);
		if (need_stack) {
			std::cout << "About to write TIFF stack <<" << base_name << "_Vizualized.tif>> (" << names.size() << " pages)" << std::endl;
			try {
				write_stack(t_start, base_name, vis_dir, names);
				for (auto &fn : names) {
					QFile file(fn.c_str());
					file.remove();
				}
			}
			catch (const std::exception& ex) {
				std::cout << "Exception " << ex.what() << std::endl;
				return EXIT_FAILURE;
			}
			catch (const std::string& ex) {
				std::cout << "Exception " << ex << std::endl;
				return EXIT_FAILURE;
			}
			catch (...) {
				std::cout << "Unknown error" << std::endl;
				return EXIT_FAILURE;
			}
		}
	}

/* DEBUG
	int nmism = 0;
	for (i = 0; i < particles.size(); i++) {
		rsParticle & pt = particles[i];
		byte c = 0;
		if (pt.GetNeighborCount() > pt.GetNN()) c = 0xAA;
		else if (pt.GetNeighborCount() < pt.GetNN()) c = 0x55;
		img->PaintParticle(pt, c);
		if (c) {
			++nmism;
			std::cout << pt.GetId() << ": " << pt.GetUL().x << "-" << pt.GetLR().x << " : " <<
				pt.GetUL().y << "-" << pt.GetLR().y << " p=" << pt.GetPerimSize() << " a=" <<
				pt.GetArea() << " NN=" << pt.GetNeighborCount() << "/" << pt.GetNN() << std::endl;
		}
	}
	std::cout << "Total NN mismatches: " << nmism << std::endl;
*/

/* DEBUG
	for (auto& x : param) {
		std::cout << x.first << " = " << x.second << std::endl;
	}
*/

	return EXIT_SUCCESS;
}

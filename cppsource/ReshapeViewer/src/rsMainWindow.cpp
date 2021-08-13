#include "rsMainWindow.h"

std::string rsImageView_VERSION = "1.0.0 (2019-02-16)";

rsMainWindow* rsMainWindow::TheApp = NULL;

// Constructor
rsMainWindow::rsMainWindow() 
{
	homeDir = QDir(QDir::home().filePath(".reshape"));
	stateFile = QFileInfo(homeDir, QString("ImageViewState.json"));

	QScreen *screen = QGuiApplication::primaryScreen();
	QRect  screenGeometry = screen->geometry();
	screen_height = screenGeometry.height();
	screen_width = screenGeometry.width();

	lutMgr = new rsLUT();
	lutMgr->loadPalettes();

	setWindowTitle(tr("REShAPE Image Viewer ver. ") + QString(rsImageView_VERSION.c_str()));
	setMinimumSize(screen_width / 3, screen_height / 3);
	resize(screen_width * 2 / 3, screen_height * 2 / 3);
	move(screen_width / 6, screen_height / 8);

#ifdef __linux__
	imgView = new rsImageViewQt();
#else
	imgView = new rsImageViewVTK();
#endif

	imgView->SetGlasbeyPalette(lutMgr->GetGlasbey());
	imgView->SetLegendWidth(screen_height / 4);
	mapw = new rsMapWidget(screen_height / 4, this);

	createActions();
	createMenus();
	createView();

	TheApp = this;
	loadState();

	setAcceptDrops(true);
}

rsMainWindow::~rsMainWindow()
{
	delete mapw;
	mapw = NULL;
	delete imgView;
	imgView = NULL;
	delete lutMgr;
	lutMgr = NULL;
}

void rsMainWindow::createActions()
{
	openImageAct = new QAction(tr("&Open"), this);
	openImageAct->setIcon(QIcon(":open.png"));
	openImageAct->setShortcuts(QKeySequence::Open);
	openImageAct->setToolTip(tr("Open Image (Ctrl+O)"));
	connect(openImageAct, SIGNAL(triggered()), this, SLOT(openImage()));

	quitAct = new QAction(tr("&Close"), this);
	quitAct->setShortcuts(QKeySequence::Quit);
	quitAct->setToolTip(tr("Close the program (Alt+F4)"));
	connect(quitAct, SIGNAL(triggered()), this, SLOT(quit()));
}

void rsMainWindow::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openImageAct);
	fileMenu->addAction(quitAct);
}

void rsMainWindow::createView()
{
	size_t i;
	QWidget *centralwidget = new QWidget();
	centralwidget->setAttribute(Qt::WA_DeleteOnClose, true);
	centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
	setCentralWidget(centralwidget);
	
	imageWidget = imgView->GetImageWidget(centralwidget);

	ctrlLabel = new QLabel(tr("Select A Tile:"));

	ctrlLayout = new QVBoxLayout();
	ctrlLayout->setMargin(5);
	ctrlLayout->setSpacing(15);
	ctrlLayout->addWidget(ctrlLabel);
	ctrlLayout->addWidget(mapw);
	ctrlLayout->setStretch(0, 0);
	ctrlLayout->setStretch(1, 0);

	cbParam = new QComboBox();
	cbParam->setEditable(false);
	cbParam->addItem("Outlines");
	cbParam->addItem("Neighbors");
	for (i = 0; i < numVisualParams; i++) {
		cbParam->addItem(visualParams[i].text);
	}
	ctrlLayout->addWidget(cbParam);
	ctrlLayout->setStretch(2, 0);

	legendLabel = new QLabel();
	legendLabel->setPixmap(QPixmap::fromImage(*imgView->GetLegend()));
	ctrlLayout->addWidget(legendLabel);
	ctrlLayout->setStretch(3, 0);

	cbLut = new QComboBox();
	cbLut->setEditable(false);
	for (i = 0; i < lutMgr->GetNumPalettes(); i++) {
		cbLut->addItem(lutMgr->GetPaletteAt(i).GetName().c_str());
	}
	ctrlLayout->addWidget(cbLut);
	ctrlLayout->setStretch(4, 0);

	QWidget *bottomWidget = new QWidget();
	bottomWidget->setAttribute(Qt::WA_DeleteOnClose, true);
	ctrlLayout->addWidget(bottomWidget);
	ctrlLayout->setStretch(5, 100);

	viewLayout = new QGridLayout(centralwidget);
	// viewLayout->setGeometry(QRect(10, 40, 40, 300));
	viewLayout->setObjectName(QString::fromUtf8("viewLayout"));
	viewLayout->addLayout(ctrlLayout, 0, 0);
	viewLayout->addWidget(imageWidget, 0, 1);
	viewLayout->setColumnStretch(0, 0);
	viewLayout->setColumnStretch(1, 100);

	connect(mapw, SIGNAL(TileChanged(int)), this, SLOT(setImageTile(int)));

	connect(cbParam, SIGNAL(currentIndexChanged(int)), this, SLOT(visualParamChanged(int)));
	connect(cbLut, SIGNAL(currentIndexChanged(int)), this, SLOT(lutChanged(int)));
	cbLut->setCurrentIndex(lutMgr->FindPalette("Thermal"));

	connect(this, SIGNAL(triggerTileChange(int)), this, SLOT(setImageTile(int)));
	connect(this, SIGNAL(triggerOpenFile(const char *)), this, SLOT(handleOpenFile(const char *)));
}

void rsMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void rsMainWindow::dropEvent(QDropEvent *e)
{
	for (int i = 0; i < e->mimeData()->urls().size(); i++) {
		QString fn = e->mimeData()->urls()[i].toLocalFile();
		if (fn.endsWith(tr(".tiled")) || fn.endsWith(tr(".json"))) {
			emit triggerOpenFile(fn.toStdString().c_str());
			break;
		}
	}
}

void rsMainWindow::openImage()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	//dialog.setNameFilter(tr("Image files(*.tiled *.png *.tif)"));
	dialog.setNameFilter(tr("Tiled Image files(*.tiled *.json)"));
	dialog.setWindowTitle("Open Image");
	dialog.restoreState(imgDialogState);
	dialog.setDirectory(imgDir);

	QStringList fileNames;
	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();
	}
	if (!fileNames.isEmpty()) {
		imgDir = dialog.directory();
		imgDialogState = dialog.saveState();
		saveState();

		QString fn = fileNames[0];
		if (fn.endsWith(tr(".tiled")) || fn.endsWith(tr(".json"))) {
			// std::cout << fn.toStdString() << std::endl;
			readTiledImage(fn.toStdString().c_str());
		}
	}
}

void rsMainWindow::readTiledImage(const char *fname)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	reader.OpenTiledImage(fname);
	mapw->SetScale(reader.GetWidth(), reader.GetHeight(), reader.GetNumTileColumns(), reader.GetNumTileRows());
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	particles.resize(0);
	reader.ReadParticleData(particles);
	mapw->SetParticles(particles);
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	QApplication::restoreOverrideCursor();
	// setImageTile(0);
	emit triggerTileChange(0);
}

void rsMainWindow::handleOpenFile(const char *fn)
{
	readTiledImage(fn);
}

void rsMainWindow::setImageTile(int tileIdx)
{
	if (tileIdx < 0 || tileIdx >= reader.GetNumTiles()) return;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	rsImageTile &tile = reader.GetTileAt(tileIdx);
	QString tpath = reader.fullName(tile.name);

	ReaderType2D::Pointer ireader = ReaderType2D::New();
	ireader->SetFileName(tpath.toStdString());
	ImageType2D::Pointer pimg = ireader->GetOutput();
	try {
		pimg->Update();
		tile.FindParticleAreas(pimg.GetPointer(), particles);
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		imgView->SetGrayImage(pimg);
//		imgView->ResetView();
		imgView->SetParticles(particles, tile.GetRect());
		imgView->ResetView();
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		mapw->SetTile(tileIdx);
		legendLabel->setPixmap(QPixmap::fromImage(*imgView->GetLegend()));
	}
	catch (itk::ExceptionObject & err) {
		std::cout << err.GetDescription() << std::endl;
	}
	QApplication::restoreOverrideCursor();
}

void rsMainWindow::visualParamChanged(int idx_par)
{
	imgView->SetVisualParameter(idx_par);
	imgView->ResetView(false);
	legendLabel->setPixmap(QPixmap::fromImage(*imgView->GetLegend()));
}

void rsMainWindow::lutChanged(int idx_lut)
{
	imgView->SetPalette(& lutMgr->GetPaletteAt(idx_lut));
	imgView->ResetView(false);
	legendLabel->setPixmap(QPixmap::fromImage(*imgView->GetLegend()));
}

void rsMainWindow::quit()
{
	close();
}

void rsMainWindow::loadState()
{
	QFile fi(stateFile.filePath());
	if (fi.open(QIODevice::ReadOnly)) {
		QJsonDocument json = QJsonDocument::fromJson(fi.readAll());
		fi.close();

		QJsonObject jobj = json.object();
		// cout << json.toJson(QJsonDocument::Indented).toStdString().c_str() << std::endl;
		if (jobj["imgDir"].isString())
			imgDir.setPath(jobj["imgDir"].toString());
		if (jobj["imgDialogState"].isString())
			imgDialogState = QByteArray::fromBase64(QByteArray(jobj["imgDialogState"].toString().toStdString().c_str()));
	}
}

void rsMainWindow::saveState()
{
	QJsonObject jobj;
	jobj["imgDir"] = imgDir.path();
	jobj["imgDialogState"] = QString(imgDialogState.toBase64());
	QJsonDocument json = QJsonDocument(jobj);

	// cout << json.toJson(QJsonDocument::Indented).toStdString().c_str() << std::endl;
	QFile fo(stateFile.filePath());
	if (fo.open(QIODevice::WriteOnly)) {
		fo.write(json.toJson(QJsonDocument::Indented));
		fo.close();
	}
}

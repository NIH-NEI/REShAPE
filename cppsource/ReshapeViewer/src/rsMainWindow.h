#ifndef rsmainwindow_h
#define rsmainwindow_h

#include <QtGlobal>
#include <QApplication>
#include <QtGui>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QVTKWidget.h>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>

#include "rsimageviewvtk.h"
#include "rsImageViewQt.h"
#include "rsMapWidget.h"
#include "rslut.h"

// #include <QuickView.h>

class rsMainWindow : public QMainWindow
{
	Q_OBJECT
public:

	// Constructor/Destructor
	rsMainWindow(); 
	~rsMainWindow();

	void readTiledImage(const char *fname);

signals:
	void triggerTileChange(int newTile);
	void triggerOpenFile(const char *fn);

protected:
	void dragEnterEvent(QDragEnterEvent * event);
	void dropEvent(QDropEvent *e);

private slots:
	void openImage();
	void quit();
	void setImageTile(int tile);
	void visualParamChanged(int idx_par);
	void lutChanged(int idx_lut);
	void handleOpenFile(const char *fn);

private:
	// QuickView qviewer;

	QDir homeDir;
	QFileInfo stateFile;
	QByteArray imgDialogState;
	QDir imgDir;

	static rsMainWindow *TheApp;
	int screen_width, screen_height;
	rsLUT * lutMgr;

	rsTiledImageReader reader;
	std::vector<rsParticle> particles;

	rsImageView *imgView;
	rsMapWidget *mapw;

	QMenu *fileMenu;
	QAction *openImageAct;
	QAction *quitAct;

	QWidget *centralwidget;
	QGridLayout *viewLayout;
	QWidget *imageWidget;
	QLabel *ctrlLabel;
	QVBoxLayout *ctrlLayout;
	QComboBox *cbParam;
	QLabel *legendLabel;
	QComboBox *cbLut;

	void createActions();
	void createMenus();
	void createView();
	void loadState();
	void saveState();

};

#endif

#ifndef rsmainwindow_h
#define rsmainwindow_h

#include <iostream>
#include <fstream>
#include <math.h>

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
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>

#include "rsimage.h"
#include "rsImageViewQt.h"

class rsMainWindow : public QMainWindow
{
	Q_OBJECT
public:

	// Constructor/Destructor
	rsMainWindow(); 
	~rsMainWindow();

	void readCsvFile(const char *fname);
	void writeCsvFile(const char *fname);

signals:
	void triggerOpenFile(const char *fn);

protected:
	void dragEnterEvent(QDragEnterEvent * event);
	void dropEvent(QDropEvent *e);
	void closeEvent(QCloseEvent *event) override;

private slots:
	void quit();
	void handleOpenFile(const char *fn);
	void openImageData();
	void visualParamChanged(int idx_par);
	void lutChanged(int idx_lut);
	void innerChanged(int idx);
	void outerChanged(int idx);
	void resetSelectors();
	void resetInner(rsFigure &inner);
	void resetOuter(rsFigure &outer);
	void saveImageData();
	void arcButtonClicked();

private:
	// QuickView qviewer;

	QDir homeDir;
	QFileInfo stateFile;
	QByteArray srcDialogState;
	QDir srcDir;
	QByteArray tgtDialogState;
	QDir tgtDir;
	QString fileName;

	static rsMainWindow *TheApp;
	int screen_width, screen_height;
	rsLUT * lutMgr;

	int img_width, img_height;
	double scale_to_img;
	double max_rad;
	rsCircle innerCir;
	GCircle inGCir;
	rsCircle outerCir;
	GCircle outGCir;
	rsPolygon innerPoly;
	GPolygon inGPoly;
	rsPolygon outerPoly;
	GPolygon outGPoly;
	bool circles_ok = false;

	rsImageViewQt *imgView = NULL;

	QMenu *fileMenu;
	QAction *openDataAct;
	QAction *saveDataAct;
	QAction *quitAct;
	QMenu *actionMenu;
	QAction *resetAct;

	QWidget *centralwidget;
	QGridLayout *viewLayout;
	QWidget *imageWidget;
	QLabel *ctrlLabel;
	QGridLayout *ctrlLayout;
	QComboBox *cbParam;
	QLabel *legendLabel;
	QLabel *lutLabel;
	QComboBox *cbLut;
	QLabel *inLabel;
	QComboBox *cbInner;
	QLabel *outLabel;
	QComboBox *cbOuter;

	QGroupBox *arcBox;
	QGridLayout *arcLayout;
	QTextEdit *arcText;
	QPushButton *arcButton;

	ParticleReader rdr;
	std::vector<rsParticle> particles;

	void createActions();
	void createMenus();
	void createView();
	void loadSelectors();
	void setSelectors();
	void loadState();
	void saveState();

};

#endif

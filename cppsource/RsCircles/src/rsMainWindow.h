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
#include <QLineEdit>
#include <QTextEdit>
#include <QSlider>
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
	void opacitySliderReleased();
	void cellSizeChanged(int idx);
	void lutChanged(int idx_lut);
	void innerChanged(int idx);
	void outerChanged(int idx);
	void resetSelectors();
	void resetInner(rsFigure &inner);
	void resetOuter(rsFigure &outer);
	void saveImageData();
	void arcButtonClicked();
	void updButtonClicked();

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

	std::vector<rsParamLimits> dfltLimits;
	std::vector<rsParamLimits> curLimits;

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

	QGroupBox* paramBox;
	QGridLayout* paramLayout;
	QLabel* cellSizeLabel;
	QComboBox* cbCellSize;
	QLabel* lbMin;
	QLabel* lbMax;
	QLineEdit* txMin;
	QLineEdit* txMax;
	QLabel* lbOpacity;
	QSlider* slOpacity;
	QPushButton* updButton;


	QGroupBox *arcBox;
	QGridLayout *arcLayout;
	QTextEdit *arcText;
	QPushButton *arcButton;

	ParticleReader rdr;
	std::vector<rsParticle> particles;
	bool _busy = false;

	void createActions();
	void createMenus();
	void createView();
	void loadSelectors();
	void setSelectors();
	void loadState();
	void saveState();
	void updateParticleLimits();
	void validateLimits();
	void loadLimits();
};

#endif

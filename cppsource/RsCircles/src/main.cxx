
#include <QApplication>

#include "rsMainWindow.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	rsMainWindow window;
	window.setWindowIcon(QIcon(":rscircles64.png"));
	window.show();

	for (int i = 1; i < argc; i++) {
		QString fn(argv[i]);
		if (fn.endsWith(".csv")) {
			window.readCsvFile(fn.toStdString().c_str());
			break;
		}
	}

	app.exec();
  
	return EXIT_SUCCESS;
}

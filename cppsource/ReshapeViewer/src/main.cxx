
#include <QApplication>
#include <itkTextOutput.h>

#include "rsMainWindow.h"

int main(int argc, char** argv)
{
	itk::OutputWindow::SetInstance(itk::TextOutput::New());

	QApplication app(argc, argv);

	rsMainWindow window;
	window.show();

	for (int i = 1; i < argc; i++) {
		QString fn(argv[i]);
		if (fn.endsWith(".tiled")) {
			window.readTiledImage(fn.toStdString().c_str());
			break;
		}
	}

	app.exec();
  
	return EXIT_SUCCESS;
}

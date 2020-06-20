#include <QApplication>
#include <QWidget>

#include "App.hpp"

int
main(int argc, char *argv[])
{
	QApplication qapp(argc, argv);
//	QApplication::addLibraryPath(QApplication::applicationDirPath()
//		+ QLatin1String("/lib"));
	
	quince::App app(argc, argv);
	app.setWindowTitle("Quince");
	app.show();
	
	return qapp.exec();
}

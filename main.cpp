#include <QApplication>
#include <QWidget>

#include "App.hpp"

int
main(int argc, char *argv[])
{
	QApplication qapp(argc, argv);
	quince::App app(argc, argv);
	app.setWindowTitle("Quince");
	app.show();
	
	return qapp.exec();
}

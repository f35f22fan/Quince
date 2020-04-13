#include <QApplication>
#include <QWidget>

#include "App.hpp"

int main(int argc, char *argv[])
{
	QApplication qapp(argc, argv);
	
	quince::App app(argc, argv);
	app.setWindowTitle("Quince");
	app.resize(800, 600);
	app.show();
	
	return qapp.exec();
}

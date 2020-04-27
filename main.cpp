#include <QApplication>
#include <QWidget>

#include "App.hpp"

#include "audio/flac.hh"

int
main(int argc, char *argv[])
{
	QApplication qapp(argc, argv);
	
//	const char *s = "/media/data/Audio/0 Chillout/Psydub/Phaeleh - (2010) Fallen Light [Afterglo, AFTRCD1001]/03 - Phaeleh - Lament.flac";
	
//	i32 duration;
//	if (quince::audio::flac::ReadFileDuration(s, duration)) {
//		mtl_info("Duration: %d, %d:%d", duration,
//			duration / 60, duration % 60);
//	} else {
//		mtl_info("Failed");
//	}
	
//	return 0;
	
	quince::App app(argc, argv);
	app.setWindowTitle("Quince");
	app.resize(800, 600);
	app.show();
	
	return qapp.exec();
}

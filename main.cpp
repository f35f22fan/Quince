#include <QApplication>
#include <QWidget>

#include "App.hpp"

#include "audio/ogg.hh"

int
main(int argc, char *argv[])
{
	QApplication qapp(argc, argv);
	
	/*
	const char *s = "/media/data/Audio/0 Chillout/Psy Chillout/Solar Fields - Movements (Remastered) 2018 FLAC-WEB/01. Sol (Remastered).opus";
	i64 duration;
	
	if (quince::audio::ogg::ReadFileDuration(s, duration)) {
		const i64 ns_per_sec = 1000'000'000L;
		auto to_m = 60L * ns_per_sec;
		auto m = duration / to_m;
		auto s = (duration / ns_per_sec) % 60;
		auto nano = duration % ns_per_sec;
		mtl_info("Duration: %ld, time: %ld:%ld.%ld", duration, m, s, nano);
	} else {
		mtl_trace();
	}
	
	return 0;
	*/
	quince::App app(argc, argv);
	app.setWindowTitle("Quince");
	app.resize(800, 600);
	app.show();
	
	return qapp.exec();
}

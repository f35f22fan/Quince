#pragma once

#include "io/io.hh"
#include "audio.hxx"
#include "audio/Meta.hpp"
#include "types.hxx"

#include <QByteArray>
#include <QString>

namespace quince {

class Song {
public:
	
	const QString& display_name() const { return display_name_; }
	void dispay_name(const QString &s) { display_name_ = s; }
	
	audio::Meta&
	meta() { return meta_; }
	
	const QString&
	uri() const { return uri_; }
	
	void
	uri(const QString &s) { uri_ = s; }
	
	static Song*
	FromFile(const io::File &file, const QString &dir_path);
	
	bool
	is_playing() const { return playing_at_ != -1; }
	
	i64
	playing_at() const { return playing_at_; }
	
	void
	playing_at(const i64 t) { playing_at_ = t; }
	
private:
	i64 playing_at_ = -1;
	QString display_name_;
	QString uri_;
	bool playing_ = false;
	audio::Meta meta_ = {};
};

}

#pragma once

#include "../types.hxx"

#include <QString>

namespace quince::gui {

class SongItem {
public:
	
	const QString& display_name() const { return display_name_; }
	void dispay_name(const QString &s) { display_name_ = s; }
	
	i64 duration_ns() { return duration_ns_; }
	void duration_ns(i64 n) { duration_ns_ = n; }
	
	const QString&
	full_path() const { return full_path_; }
	
	void
	full_path(const QString &s) { full_path_ = s; }
	
	static SongItem*
	New(const QString &name);
	
	bool playing() const { return playing_; }
	void playing(const bool b) { playing_ = b; }
	
private:
	i64 duration_ns_ = -1;
	QString display_name_;
	QString full_path_;
	bool playing_ = false;
};

} // quince::gui

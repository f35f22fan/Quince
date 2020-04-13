#pragma once

#include <QtGlobal>
#include <QtCore/QString>

#include "types.hxx"

namespace quince {

class Duration
{
public:
	Duration();
	Duration(const i32 h, const i32 m, const i32 s);
	virtual ~Duration();
	
	Duration*
	Clone() const;
	
	void
	Decode(const QString &str);
	
	static Duration
	FromNs(i64 duration_ns);
	
	static Duration*
	New(const i32 h, const i32 m, const i32 s);
	
	bool
	operator==(const Duration &rhs) const;
	
	operator QString();
	
	i32
	days() const { return days_; }
	
	void
	days(const i32 n) { days_ = n; }
	
	i32
	hours() const { return hours_; }
	
	void
	hours(const i32 n) { hours_ = n; }
	
	bool
	IsValid() const;
	
	i32
	minutes() const { return minutes_; }
	
	void
	minutes(const i32 n) { minutes_ = n; }
	
	i32
	months() const { return months_; }
	
	void
	months(const i32 n) { months_ = n; }
	
	i32
	seconds() const { return seconds_; }
	
	void
	seconds(const i32 n) { seconds_ = n; }
	
	QString
	toDurationString() const;
	
	QString
	toString() const;
	
	i32
	years() const { return years_; }
	
	void
	years(const i32 n) { years_ = n; }
	
private:
	
	i32 years_ = 0;
	i32 months_ = 0;
	i32 days_ = 0;
	i32 hours_ = 0;
	i32 minutes_ = 0;
	i32 seconds_ = 0;
};

}


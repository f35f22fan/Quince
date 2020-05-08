/*
 * Copyright © 2014 f35f22fan@gmail.com
 *
 * Read the LICENSE file that comes with this project for license details.
*/

#include "Duration.hpp"
#include "err.hpp"

#include <QtCore/QDebug>
#include <QtCore/QStringRef>

namespace quince {

Duration::Duration()
{}

Duration::Duration(const i32 h, const i32 m, const i32 s)
: hours_(h), minutes_(m), seconds_(s)
{}

Duration::~Duration()
{}

Duration*
Duration::Clone() const
{
	auto *p = new Duration();
	p->years_ = years_;
	p->months_ = months_;
	p->days_ = days_;
	p->hours_ = hours_;
	p->minutes_ = minutes_;
	p->seconds_ = seconds_;
	
	return p;
}

void
Duration::Decode(const QString &str)
{
	int last_offset = 1;
	int i = last_offset + 1;
	bool is_month = true;
	bool ok;

	for (; i <= str.size(); i++)
	{
		QStringRef s(&str, last_offset, i - last_offset);
		int n = s.toInt(&ok);
		
		if (ok)
			continue;
		
		s = QStringRef(&str, last_offset, i - last_offset - 1);
		n = s.toInt(&ok);
		last_offset = i;
		const QChar c = str.at(i - 1);

		if (c == 'Y') {
			years_ = n;
		} else if (c == 'M') {
			if (is_month) {
				months_ = n;
			} else {
				minutes_ = n;
			}
		} else if (c == 'D') {
			days_ = n;
		} else if (c == 'H') {
			hours_ = n;
		} else if (c == 'S') {
			seconds_ = n;
		} else if (c == 'T') {
			is_month = false;
		} else {
			mtl_warn("unknown char");
			return;
		}
	}
}

Duration
Duration::FromNs(i64 duration_ns)
{
	const i64 second = 1000'000'000L;
	const i64 minute= 60 * second;
	const i64 hour = 60 * minute;
	const i64 day = 24 * hour;
	
	i32 days = duration_ns / day;
	duration_ns -= days * day;
	
	i32 hours = duration_ns / hour;
	duration_ns -= hours * hour;
	
	i32 minutes = duration_ns / minute;
	duration_ns -= minutes * minute;
	
	i32 seconds = duration_ns / second;
	
	Duration d(hours, minutes, seconds);
	d.days(days);
	
	return d;
}

bool
Duration::IsValid() const
{
	return hours_ != -1 || minutes_ != -1 || seconds_ != -1 ||
		years_ != -1 || months_ != -1 || days_ != -1;
}

Duration*
Duration::New(const i32 h, const i32 m, const i32 s)
{
	auto *p = new Duration();
	p->hours_ = h;
	p->minutes_ = m;
	p->seconds_ = s;
	
	return p;
}

bool
Duration::operator==(const Duration &rhs) const
{
	return years_ == rhs.years_ &&
		months_ == rhs.months_ &&
		days_ == rhs.days_ &&
		hours_ == rhs.hours_ &&
		minutes_ == rhs.minutes_ &&
		seconds_ == rhs.seconds_;
}

Duration::operator QString()
{
	return toString();
}

QString
Duration::toDurationString() const
{
	QString str;
	
	if (days_ > 0)
		str.append(QString::number(days_) + QLatin1String("d "));
	
	if (hours_ > 0)
	{
		if (hours_ < 10)
			str.append('0');
		
		str.append(QString::number(hours_) + ':');
	}
	
	if (minutes_ < 10)
		str.append('0');
	
	str.append(QString::number(minutes_) + ':');
	
	if (seconds_ < 10)
		str.append('0');
	
	str.append(QString::number(seconds_));
	
	return str;
}

QString
Duration::toString() const
{
	QString str = QLatin1String("PT");
	str += QString::number(hours_);
	str += 'H';
	str += QString::number(minutes_);
	str += 'M';
	str += QString::number(seconds_);
	str += 'S';
	return str;
}

} // quince::

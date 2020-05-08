#include "TableModel.hpp"

#include "../App.hpp"
#include "../audio.hh"
#include "../audio/Meta.hpp"
#include "../Duration.hpp"
#include "../Song.hpp"

#include <QFont>
#include <QTime>
#include <gst/gst.h>

namespace quince::gui {

TableModel::TableModel(App *app, QObject *parent) :
app_(app),
QAbstractTableModel(parent)
{
	timer_ = new QTimer(this);
	timer_->setInterval(1000);
	connect(timer_, &QTimer::timeout, this, &TableModel::TimerHit);
	timer_->start();
}

TableModel::~TableModel()
{
	for (auto *song: songs_)
		delete song;
	
	songs_.clear();
}

int
TableModel::rowCount(const QModelIndex &parent) const
{
	return songs_.size();
}

int
TableModel::columnCount(const QModelIndex &parent) const
{
	return Column::Count;
}

QVariant
TableModel::data(const QModelIndex &index, int role) const
{
	const int row = index.row();
	const int col = index.column();
	
	if (row >= songs_.size())
	{
		return QVariant();
	}
	
	auto *song = songs_[row];
	audio::Meta &meta = song->meta();
	
	if (role == Qt::DisplayRole)
	{
		if (col == Column::Name) {
			return song->display_name();
		} else if (col == Column::Duration) {
			if (!meta.is_duration_set())
				return "--";
			
			auto d = Duration::FromNs(song->meta().duration());
			return d.toDurationString();
		} else if (col == Column::PlayingAt) {
			if (song->is_playing()) {
				playing_row_ = row;
				auto d = Duration::FromNs(song->playing_at());
				return d.toDurationString();
			}
		} else if (col == Column::Bitrate) {
			const i32 bitrate = meta.bitrate();
			
			if (bitrate != -1) {
				return QString::number(bitrate / 1000)
					+ QLatin1String(" kbps");
			}
		} else if (col == Column::Channels) {
			if (meta.channels() != -1)
				return QString::number(meta.channels());
		} else if (col == Column::BitsPerSample) {
			if (meta.bits_per_sample() != -1)
				return QString::number(meta.bits_per_sample());
		} else if (col == Column::SampleRate) {
			if (meta.sample_rate() != -1) {
				return QString::number(meta.sample_rate());
			}
		} else if (col == Column::Genre) {
			return audio::GenreToString(meta.genre());
		}
		
		return QVariant();
	} else if (role == Qt::TextAlignmentRole) {
		
		if (col == Column::Name)
			return Qt::AlignLeft + Qt::AlignVCenter;
		
		return Qt::AlignLeft /*Qt::AlignHCenter*/ + Qt::AlignVCenter;
	} else if (role == Qt::FontRole) {
		QFont font;
		
		if (song->is_playing())
			font.setBold(true);
		
		return font;
	}
	
	return QVariant();
}

QVariant
TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		
		if (orientation == Qt::Horizontal)
		{
			switch (section) {
			case Column::Name:
				return QLatin1String("Name");
			case Column::Duration:
				return QLatin1String("Duration");
			case Column::PlayingAt:
				return QLatin1String("Playing At");
			case Column::Bitrate:
				return QLatin1String("Bitrate");
			case Column::Channels:
				return QLatin1String("Channels");
			case Column::BitsPerSample:
				return QLatin1String("Bits Per Sample");
			case Column::SampleRate:
				return QLatin1String("Sample Rate");
			case Column::Genre:
				return QLatin1String("Genre");
			}
		} else {
			return QString::number(section + 1);
		}
	}
	return QVariant();
}

void
TableModel::TimerHit()
{
	if (playing_row_ >= songs_.size())
		return;
	
	bool update_one_column = UpdatePlayingSongPosition();
	
	Column c = update_one_column ? Column::PlayingAt : Column::Duration;
	QModelIndex top_left = createIndex(playing_row_, c);
	QModelIndex bottom_right = createIndex(playing_row_, Column::PlayingAt);
	
	emit dataChanged(top_left, bottom_right, {Qt::DisplayRole});
}

bool
TableModel::UpdatePlayingSongPosition()
{
	Song *song = app_->GetPlayingSong();
	
	if (song == nullptr)
		return true;
	
	if (!song->is_playing())
	{
		mtl_trace();
		return true;
	}
	
	GstElement *play_elem = app_->play_elem();
	i64 duration = -1;
	gboolean ok = gst_element_query_position (play_elem,
		GST_FORMAT_TIME, &duration);
		
	if (!ok) {
		mtl_trace();
		return true;
	}
	
	song->playing_at(duration);
	
	if (!song->meta().is_duration_set())
	{
		duration = -1;
		gboolean ok = gst_element_query_duration (play_elem,
			GST_FORMAT_TIME, &duration);
			
		if (!ok) {
			mtl_trace();
			return false;
		}
		song->meta().duration(duration);
		
		return false;
	}
	
	return true;
}

void
TableModel::UpdateRange(int row1, Column c1, int row2, Column c2)
{
	int first, last;
	
	if (row1 > row2) {
		first = row2;
		last = row1;
	} else {
		first = row1;
		last = row2;
	}
	
	QModelIndex top_left = createIndex(first, c1);
	QModelIndex bottom_right = createIndex(last, c2);
	emit dataChanged(top_left, bottom_right, {Qt::DisplayRole});
}

}

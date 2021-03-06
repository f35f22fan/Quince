#include "TableModel.hpp"

#include "../App.hpp"
#include "../audio.hh"
#include "../audio/Meta.hpp"
#include "../Duration.hpp"
#include "Playlist.hpp"
#include "../Song.hpp"
#include "SeekPane.hpp"
#include "Table.hpp"

#include <QFont>
#include <QTime>
#include <gst/gst.h>

namespace quince::gui {

TableModel::TableModel(App *app, Playlist *parent) :
app_(app),
QAbstractTableModel(parent),
playlist_(parent)
{
	timer_ = new QTimer(this);
	timer_->setInterval(1000);
	connect(timer_, &QTimer::timeout, this, &TableModel::TimerHit);
	timer_->start();
}

TableModel::~TableModel()
{
	delete timer_;
	timer_ = nullptr;
	
	for (auto *song: songs_)
		delete song;
	
	songs_.clear();
}

QModelIndex
TableModel::index(int row, int column, const QModelIndex &parent) const
{
	return createIndex(row, column);
}

int
TableModel::rowCount(const QModelIndex &parent) const
{
	return songs_.size();
}

int
TableModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	
	return i8(Column::Count);
}

QVariant
TableModel::data(const QModelIndex &index, int role) const
{
	const Column col = static_cast<Column>(index.column());
	
	if (role == Qt::TextAlignmentRole) {
		if (col == Column::Name)
			return Qt::AlignLeft + Qt::AlignVCenter;
		
		return Qt::AlignLeft /*Qt::AlignHCenter*/ + Qt::AlignVCenter;
	}
	
	const int row = index.row();
	
	if (row >= songs_.size())
		return {};
	
	auto *song = songs_[row];
	audio::Meta &meta = song->meta();
	
	if (role == Qt::DisplayRole)
	{
		if (col == Column::Name) {
			return song->display_name();
		} else if (col == Column::Duration) {
			QString s;
			if (meta.is_duration_set()) {
				auto d = Duration::FromNs(song->meta().duration());
				s = d.toDurationString();
			}
			
			if (song->is_playing_or_paused()) {
				s.append(' ');
				playing_row_ = row;
				auto d = Duration::FromNs(song->position());
				const QString dstr = d.toDurationString();
				
				if (song->is_paused())
					s.append('|').append(dstr).append('|');
				else
					s.append('[').append(dstr).append(']');
			}
			
			return s;
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
				return QString::number(meta.sample_rate()) +
					QLatin1String(" Hz");
			}
		} else if (col == Column::Genre) {
			return audio::GenresToString(meta.genres());
		}
		
		return QVariant();
	} else if (role == Qt::FontRole) {
		QFont font;
		
		if (song->is_playing_or_paused())
			font.setBold(true);
		
		return font;
	}
	
	return {};
}

QVariant
TableModel::headerData(int section_i, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
		{
			const Column section = static_cast<Column>(section_i);
			
			switch (section) {
			case Column::Name:
				return QLatin1String("Name");
			case Column::Duration:
				return QLatin1String("Duration");
			case Column::Bitrate:
				return QLatin1String("Bitrate");
			case Column::Channels:
				return QLatin1String("Channels");
			case Column::BitsPerSample:
				return QLatin1String("BPS");//Bits Per Sample
			case Column::SampleRate:
				return QLatin1String("Sample Rate");
			case Column::Genre:
				return QLatin1String("Genre");
			default: {
				mtl_trace();
				return {};
			}
			}
		}
		return QString::number(section_i + 1);
	}
	return {};
}

bool
TableModel::InsertRows(const i32 at, const QVector<Song*> &songs_to_add)
{
	if (songs_to_add.isEmpty())
		return false;
	
	const int first = at;
	const int last = at + songs_to_add.size() - 1;
	
	beginInsertRows(QModelIndex(), first, last);
	
	for (i32 i = 0; i < songs_to_add.size(); i++)
	{
		auto *song = songs_to_add[i];
		songs_.insert(at + i, song);
	}
	
	endInsertRows();
	
	return true;
}

bool
TableModel::removeRows(int row, int count, const QModelIndex &parent)
{
	if (count <= 0)
		return false;
	
	if (count != 1)
	{
		mtl_warn("Count != 1");
		return false;
	}
	
	const int first = row;
	const int last = row + count - 1;
	beginRemoveRows(QModelIndex(), first, last);
	
	for (int i = count - 1; i >= 0; i--) {
		const i32 index = first + i;
		auto *item = songs_[index];
		songs_.erase(songs_.begin() + index);
		delete item;
	}
	
	endRemoveRows();
	return true;
}

void
TableModel::StartOrStopTimer()
{
	if (playlist_->visible()) {
		TimerHit();
		timer_->start();
	} else {
		timer_->stop();
	}
}

void
TableModel::TimerHit()
{
	const bool is_visible = playlist_->table()->isVisible();
	app_->UpdatePlayingSongPosition(-1, is_visible);
	
	if (!is_visible)
		return;

	if (app_->seek_pane()->slider_dragged_by_user())
		return;
	
	if (playing_row_ >= songs_.size())
		return;

//	static int n = 0;
//	mtl_info("%d", n++);
	
	UpdateRange(playing_row_, Column::Duration, playing_row_, Column::Duration);
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
	
	const QModelIndex top_left = createIndex(first, int(c1));
	const QModelIndex bottom_right = createIndex(last, int(c2));
	emit dataChanged(top_left, bottom_right, {Qt::DisplayRole});
}

}

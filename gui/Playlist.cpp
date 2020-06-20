#include "Playlist.hpp"

#include "../App.hpp"
#include "../GstPlayer.hpp"
#include "../Song.hpp"
#include "Table.hpp"
#include "TableModel.hpp"

#include <QBoxLayout>

namespace quince::gui {

Playlist::Playlist(App *app, const QString &name)
: app_(app), name_(name)
{
	CreateGui();
}

Playlist::~Playlist()
{
	delete table_;
}

void
Playlist::CreateGui()
{
	table_model_ = new gui::TableModel(app_, this);
	table_ = new gui::Table();
	table_->setModel(table_model_);
	// column widths to be set after setting the model
	table_->setColumnWidth(i8(Column::Name), 500);
	table_->setColumnWidth(i8(Column::Duration), 200);
	table_->setColumnWidth(i8(Column::Bitrate), 120);
	table_->setColumnWidth(i8(Column::BitsPerSample), 120);
	table_->setColumnWidth(i8(Column::Channels), 120);
	table_->setColumnWidth(i8(Column::SampleRate), 120);
	
	connect(table_, &QTableView::doubleClicked, this,
		&Playlist::PlaylistDoubleClicked);
	
	QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom);
	setLayout(layout);
	layout->addWidget(table_);
}

Song*
Playlist::GetCurrentSong(int *index)
{
	QVector<Song*> &songs = this->songs();
	const i32 count = songs.size();
	
	for (int i = 0; i < count; i++)
	{
		auto *song = songs[i];
		
		if (song->is_playing_or_paused())
		{
			if (index != nullptr)
				*index = i;
			
			return song;
		}
	}
	
	return nullptr;
}

bool
Playlist::GetFullPath(QString &full_path) const
{
	QString dir;
	CHECK_TRUE(QuerySaveFolder(dir));
	full_path = dir + QChar('/') + QString::number(id_);
	
	return true;
}

void
Playlist::PlaylistDoubleClicked(QModelIndex index)
{
	int row = index.row();
	
	auto &songs = table_model_->songs();
	
	if (row >= songs.size()) {
		mtl_trace();
		return;
	}
	
	int last_playing = -1;
	Song *playing = GetCurrentSong(&last_playing);
	
	if (playing != nullptr) {
		playing->playing_at(-1);
		playing->state(GST_STATE_NULL);
	}

	Song *song = songs[row];
	app_->player()->PlayPause(song);
	song->playing_at(0);
	table_model_->UpdateRange(row, gui::Column::Name,
		last_playing, gui::Column::Duration);
}

bool
Playlist::QuerySaveFolder(QString &ret_val)
{
	static QString dir_path = QString();
	
	if (!dir_path.isEmpty())
	{
		ret_val = dir_path;
		return true;
	}
	
	QString app_config_path;
	CHECK_TRUE(App::QueryAppConfigPath(app_config_path));
	const QString subdir_name = QLatin1String("/Playlists");
	CHECK_TRUE(io::EnsureDir(app_config_path, subdir_name));
	dir_path = app_config_path + subdir_name;
	ret_val = dir_path;
	
	return true;
}

i32
Playlist::RemoveSelectedSongs()
{
	auto list = table_->selectionModel()->selectedRows();
	
	if (list.isEmpty())
		return 0;
	
	QVector<Song*> &songs = table_model_->songs();
	
	for (auto &next: list)
	{
		int row = next.row();
		
		if (row < 0 || row >= songs.size()) {
			mtl_trace("%d", row);
			continue;
		}
		
		songs[row]->mark_for_deletion();
	}
	
	i32 count = 0;
	for (i32 i = songs.size() - 1; i >= 0; i--)
	{
		quince::Song *song = songs[i];
		
		if (song->marked_for_deletion())
		{
			mtl_info("Deleting index %d", i);
			table_model_->removeRows(i, 1, QModelIndex());
			count++;
		}
	}

	return count;
}

QVector<Song*>&
Playlist::songs() { return table_model_->songs(); }

}

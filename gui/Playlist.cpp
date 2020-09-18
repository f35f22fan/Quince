#include "Playlist.hpp"

#include "../App.hpp"
#include "../io/File.hpp"
#include "../GstPlayer.hpp"
#include "../Song.hpp"
#include "Table.hpp"
#include "TableModel.hpp"

#include <QBoxLayout>
#include <QDragEnterEvent>
#include <QMimeData>

namespace quince::gui {

Playlist::Playlist(App *app, const QString &name)
: app_(app), name_(name)
{
	CreateGui();
	setAcceptDrops(true);
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
	table_->setColumnWidth(i8(Column::Bitrate), 90);
	table_->setColumnWidth(i8(Column::BitsPerSample), 90);
	table_->setColumnWidth(i8(Column::Channels), 90);
	table_->setColumnWidth(i8(Column::SampleRate), 110);
	table_->setColumnWidth(i8(Column::Genre), 200);
	
	connect(table_, &QTableView::doubleClicked, this, &Playlist::MouseDoubleClick);
	
	QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom);
	setLayout(layout);
	layout->addWidget(table_);
}

void
Playlist::dragEnterEvent(QDragEnterEvent *event)
{
	const QMimeData *mimedata = event->mimeData();
	
	if (mimedata->hasUrls())
		event->acceptProposedAction();
}

void
Playlist::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		gui::Playlist *playlist = app_->GetComboCurrentPlaylist();
		
		if (playlist == nullptr) {
			mtl_warn("Should never happen");
			return;
		} else {
//			auto ba = playlist->name().toLocal8Bit();
//			mtl_info("Playlist name: %s", ba.data());
		}
		
		QVector<io::File> files;
		
		for (const QUrl &url: event->mimeData()->urls())
		{
			QString path = url.path();
			io::File file;
			
			if (io::FileFromPath(file, path) == io::Err::Ok) {
//				auto ba = file.build_full_path().toLocal8Bit();
//				mtl_info("Adding file: \"%s\"", ba.data());
				files.append(file);
			}
		}
		
		app_->AddFilesToPlaylist(files, playlist);
	}
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

bool
Playlist::HasSong(Song *song) const
{
	for (Song *p: songs())
	{
		if (p == song)
			return true;
	}
	
	return false;
}

void
Playlist::MouseDoubleClick(QModelIndex index)
{
	int row = index.row();
	auto &songs = table_model_->songs();
	
	if (row >= songs.size()) {
		mtl_trace();
		return;
	}
	
	int last_playing = -1;
	Song *playing_song = GetCurrentSong(&last_playing);
	
	if (playing_song != nullptr) {
		playing_song->position(-1);
		playing_song->state(GST_STATE_NULL);
	}
	
	Song *song = songs[row];
	app_->player()->Play(song);
	song->position(0);
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
Playlist::RemoveAllSongs()
{
	QVector<Song*> &songs = table_model_->songs();
	
	i32 count = 0;
	for (i32 i = songs.size() - 1; i >= 0; i--)
	{
		quince::Song *song = songs[i];
		table_model_->removeRows(i, 1, QModelIndex());
		count++;
	}

	return count;
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
			//mtl_info("Deleting index %d", i);
			table_model_->removeRows(i, 1, QModelIndex());
			count++;
		}
	}

	return count;
}

QVector<Song*>&
Playlist::songs() const { return table_model_->songs(); }

}

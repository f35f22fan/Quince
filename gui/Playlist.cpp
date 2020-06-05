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
	auto &songs = table_model_->songs();
	
	for (int i = 0; i < songs.size(); i++)
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

i32
Playlist::RemoveSelectedSong()
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
		
		u8 &bits = songs[row]->bits();
		bits |= u8(SongBits::MarkForDeletion);
	}
	
	i32 count = 0;
	for (i32 i = songs.size() - 1; i >= 0; i--)
	{
		auto *song = songs[i];
		
		if (song->bits() & u8(SongBits::MarkForDeletion))
		{
			//songs.erase(songs.begin() + i);
			table_model_->BeginRemoveRows(i, i + 1);
			table_model_->removeRows(i, 1, QModelIndex());
			table_model_->EndRemoveRows();
			count++;
		}
	}

	return count;
}

}

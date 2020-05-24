#include "Playlist.hpp"

#include "../App.hpp"
#include "../GstPlayer.hpp"
#include "../Song.hpp"
#include "Table.hpp"
#include "TableModel.hpp"

#include <QBoxLayout>

namespace quince::gui {

Playlist::Playlist(App *app, const QString &name)
: app_(app)
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
	app_->LoadSavedSongData(table_model_);
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

}

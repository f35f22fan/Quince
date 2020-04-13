#include "App.hpp"

#include "actions.hxx"
#include "GstPlayer.hpp"
#include "gui/SongItem.hpp"
#include "gui/Table.hpp"
#include "gui/TableModel.hpp"
#include "io/io.hh"

#include <QBoxLayout>
#include <QScrollArea>
#include <QToolBar>

namespace quince {

App::App(int argc, char *argv[])
{
	CHECK_TRUE_RET_VOID(LoadSavedSongData());
	CHECK_TRUE_RET_VOID(CreateGui());
	
	setWindowIcon(QIcon(":/resources/Quince.png"));
	
	player_ = new GstPlayer(argc, argv);
}

App::~App() {
	delete player_;
}

bool
App::CreateGui()
{
	addToolBar(Qt::TopToolBarArea, CreateMediaActionsToolBar());
	
	if (table_model_ == nullptr)
		table_model_ = new gui::TableModel();
	
	table_ = gui::Table::Create(table_model_);
	connect(table_, &QTableView::doubleClicked, this, &App::PlaylistDoubleClicked);
//	QScrollArea *a = new QScrollArea();
//	a->setWidget(table_);
	
	QWidget *central_widget = new QWidget(this);
	
	auto *layout = new QBoxLayout(QBoxLayout::TopToBottom);
	layout->addWidget(table_);
	central_widget->setLayout(layout);
	
	setCentralWidget(central_widget);
	
	addToolBar(Qt::BottomToolBarArea, CreatePlaylistActionsToolBar());
	
	return true;
}

void
App::AddAction(QToolBar *tb, const QString &icon_name, const QString &action_name)
{
	QAction *action = tb->addAction(QIcon::fromTheme(icon_name), QString());
	connect(action, &QAction::triggered,
		[=] {ProcessAction(action_name);});
}

QToolBar*
App::CreateMediaActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	AddAction(tb, "go-previous", actions::MediaGoPrev);
	AddAction(tb, "media-playback-start", actions::MediaPlayPause);
	//AddAction(tb, "media-playback-pause", actions::MediaPause);
	AddAction(tb, "media-playback-stop", actions::MediaStop);
	AddAction(tb, "go-next", actions::MediaGoNext);
	
	return tb;
}

QToolBar*
App::CreatePlaylistActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	AddAction(tb, "list-add", actions::PlaylistNew);
	AddAction(tb, "list-remove", actions::PlaylistDelete);
	AddAction(tb, "edit-redo", actions::PlaylistRename);
	
	return tb;
}

bool
App::LoadSavedSongData()
{
	if (table_model_ == nullptr)
		table_model_ = new gui::TableModel();
	
	QVector<gui::SongItem*> &songs = table_model_->songs();
	
	const i64 sec = 1000'000'000L;
	const i64 min = sec * 60L;
	const i64 hour = min * 60L;
	const i64 day = hour * 24L;
	
	QVector<io::File> files;
	QString dir_path = "/media/data/Audio/0 Best Hits/";
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	
	if (io::ListFiles(dir_path, files, 0, io::IsSongExtension) != io::Err::Ok) {
		mtl_trace();
		return false;
	}
	
	for (io::File &file: files)
	{
		auto *song = gui::SongItem::New(file.name);
		song->full_path(dir_path + file.name);
		songs.append(song);
	}
	
//	songs.append(gui::SongItem::New("Staind  -  It's Been Awhile", 3 * min + 45 * sec));
//	auto s = gui::SongItem::New("Blade Runner - Ambient for sleeping", 8 * min + 2 * sec);
//	s->playing(true);
//	songs.append(s);
//	songs.append(gui::SongItem::New("4am Study _ Chill Vibes", 3 * day + 5 * min + 8 * sec));
	
	
	return true;
}

void
App::PlaylistDoubleClicked(QModelIndex index)
{
	int row = index.row();
	
	auto &songs = table_model_->songs();
	
	if (row >= songs.size()) {
		mtl_trace();
		return;
	}
	
	int last_playing = -1;
	
	for (int i = 0; i < songs.size(); i++)
	{
		auto *song = songs[i];
		
		if (song->playing()) {
			song->playing(false);
			last_playing = i;
			break;
		}
	}
	
	gui::SongItem *song = songs[row];
	player_->Play(song->full_path());
	song->playing(true);
	table_model_->UpdateRange(row, last_playing, gui::Column::PlayingAt);
}

void
App::ProcessAction(const QString &action_name)
{
	auto ba = action_name.toLocal8Bit();
	mtl_trace("Action: \"%s\"", ba.data());
}

} // quince::

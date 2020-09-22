#include "PlaylistStackWidget.hpp"

#include "../App.hpp"
#include "../io/File.hpp"

#include <QDragEnterEvent>
#include <QMimeData>

namespace quince::gui {

PlaylistStackWidget::PlaylistStackWidget(QWidget *parent, quince::App *app) :
QWidget(parent), app_(app)
{
	setAcceptDrops(true);
}

PlaylistStackWidget::~PlaylistStackWidget() {
}

void
PlaylistStackWidget::dragEnterEvent(QDragEnterEvent *event)
{
	const QMimeData *mimedata = event->mimeData();
	
	if (mimedata->hasUrls()) {
		event->acceptProposedAction();
	}
}

void
PlaylistStackWidget::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		
		gui::Playlist *playlist = app_->GetComboCurrentPlaylist();
		
		if (playlist == nullptr) {
			auto *new_one = app_->CreatePlaylist(QLatin1String("New Playlist"), true,
				PlaylistActivationOption::None, nullptr,
				playlist::Ctor::AssignNewId);
			
			CHECK_PTR_VOID(new_one);
			playlist = app_->active_playlist();
			CHECK_PTR_VOID(playlist);
		}
		
		QVector<io::File> files;
		
		for (const QUrl &url: event->mimeData()->urls())
		{
			QString path = url.path();
			io::File file;
			
			if (io::FileFromPath(file, path) != io::Err::Ok)
				continue;
			
			files.append(file);
		}
		
		app_->AddFilesToPlaylist(files, playlist, event->pos());
	}
}

}

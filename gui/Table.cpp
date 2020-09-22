#include "Table.hpp"

#include "../actions.hxx"
#include "../App.hpp"
#include "../io/File.hpp"
#include "../Song.hpp"
#include "TableModel.hpp"

#include <map>

#include <QAbstractItemView>
#include <QAction>
#include <QClipboard>
#include <QDialog>
#include <QDragEnterEvent>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QUrl>

namespace quince::gui {

Table::Table(TableModel *tm) :
table_model_(tm)
{
	setModel(table_model_);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	horizontalHeader()->setSectionsMovable(true);
	verticalHeader()->setSectionsMovable(true);
	setAcceptDrops(true);
}

Table::~Table() {}

void
Table::dragEnterEvent(QDragEnterEvent *event)
{
	const QMimeData *mimedata = event->mimeData();
	
	if (mimedata->hasUrls())
		event->acceptProposedAction();
}

void
Table::dragMoveEvent(QDragMoveEvent *event)
{
	const auto &pos = event->pos();
	mtl_info("x: %d, y: %d", pos.x(), pos.y());
}

void
Table::dropEvent(QDropEvent *event)
{
	App *app = table_model_->app();
	
	if (event->mimeData()->hasUrls()) {
		gui::Playlist *playlist = app->GetComboCurrentPlaylist();
		CHECK_PTR_VOID(playlist);
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
		
		app->AddFilesToPlaylist(files, playlist, event->pos());
	}
}
void
Table::keyPressEvent(QKeyEvent *event)
{
	const int key = event->key();
	
	if (key == Qt::Key_Delete) {
		table_model_->app()->RemoveSongsFromPlaylist(Which::Selected);
	}
}

void
Table::mousePressEvent(QMouseEvent *event)
{
	QTableView::mousePressEvent(event);
	
	if (event->button() == Qt::RightButton) {
		ShowRightClickMenu(event->globalPos());
	}
	
}

void
Table::ProcessAction(const QString &action)
{
	CHECK_PTR_VOID(table_model_);
	QItemSelectionModel *select = selectionModel();
	
	if (select->hasSelection()) {
		QModelIndexList rows = select->selectedRows();
		
		if (action == quince::actions::RemoveSongsAndDeleteFiles) {
			RemoveSongsAndDeleteFiles(rows);
		} else if (action == actions::ShowSongFolderPath) {
			if (rows.isEmpty())
				return;
			
			const int row_index = rows[0].row();
			QVector<Song*> &songs = table_model_->songs();
			
			if (row_index >= songs.size())
				return;
			
			ShowSongLocation(songs[row_index]);
		}
	}
}

void
Table::RemoveSongsAndDeleteFiles(const QModelIndexList &indices)
{
	const Qt::KeyboardModifiers mods = QGuiApplication::queryKeyboardModifiers();
	bool confirm_delete = (mods & Qt::ShiftModifier) == 0;
	
	const QString url_prefix = QLatin1String("file://");
	std::map<int, Song*> song_map;
	QVector<Song*> &songs = table_model_->songs();
	
	for (QModelIndex row: indices) {
		const int row_index = row.row();
		song_map[row_index] = songs[row_index];
	}
	
	for (auto iter = song_map.rbegin(); iter != song_map.rend(); ++iter)
	{
		Song *song = iter->second;
		int row = iter->first;
		QUrl url(song->uri());
		
		if (!url.isLocalFile())
			continue;
		
		if (confirm_delete) {
			QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm",
				"Delete file(s)?", QMessageBox::Yes | QMessageBox::No);
			
			if (reply != QMessageBox::Yes) {
				return;
			}
			
			confirm_delete = false;
		}
		
		QString full_path = url.toLocalFile();
		
		if (full_path.startsWith(url_prefix))
			full_path = full_path.mid(url_prefix.size());
		
		auto path_ba = full_path.toLocal8Bit();
		
		if (remove(path_ba.data()) != 0)
			mtl_status(errno);
		
		table_model_->removeRows(row, 1, QModelIndex());
	}
}

void
Table::ShowRightClickMenu(const QPoint &pos)
{
	QMenu *menu = new QMenu();
	{
		auto action_str = quince::actions::RemoveSongsAndDeleteFiles;
		QAction *action = menu->addAction(action_str);
		connect(action, &QAction::triggered, [=] {ProcessAction(action_str);});
	}
	{
		auto action_str = quince::actions::ShowSongFolderPath;
		QAction *action = menu->addAction(action_str);
		connect(action, &QAction::triggered, [=] {ProcessAction(action_str);});
	}
	
	menu->popup(pos);
}

void
Table::ShowSongLocation(Song *song)
{
	CHECK_PTR_VOID(song);
	
	QDialog *dialog = new QDialog(this);
	dialog->setWindowTitle("Song location");
	dialog->resize(600, 200);
	
	auto *layout = new QFormLayout();
	dialog->setLayout(layout);
	
	QIcon copy_icon = QIcon::fromTheme("edit-copy");
	QPushButton *copy_btn = new QPushButton(copy_icon, "Copy");
	QLineEdit *path_field = new QLineEdit();
	path_field->setMinimumSize(500, 0);
	path_field->setText(song->dir_path());
	path_field->setReadOnly(true);
	layout->addRow(path_field, copy_btn);
	
	connect(copy_btn, &QPushButton::clicked, [=] {
		QClipboard *clipboard = QGuiApplication::clipboard();
		clipboard->setText(path_field->text());
		//dialog->done(QDialog::Accepted);
	});
	
	dialog->show();
	dialog->raise();
	dialog->exec();
}

} // quince::gui::


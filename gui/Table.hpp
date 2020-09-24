#pragma once

#include "../decl.hxx"
#include "decl.hxx"
#include "../err.hpp"

#include <QAbstractTableModel>
#include <QMouseEvent>
#include <QPoint>
#include <QTableView>

namespace quince::gui {

class Table : public QTableView {
public:
	Table(TableModel *tm);
	virtual ~Table();
	
	virtual void dropEvent(QDropEvent *event) override;
	
	void
	ProcessAction(const QString &action);
	
protected:
	virtual void dragEnterEvent(QDragEnterEvent *event) override;
	virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
	virtual void dragMoveEvent(QDragMoveEvent *event) override;
	
	
	virtual void keyPressEvent(QKeyEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	
	virtual void paintEvent(QPaintEvent *event) override;
private:
	NO_ASSIGN_COPY_MOVE(Table);
	
	void RemoveSongsAndDeleteFiles(const QModelIndexList &indices);
	void ShowRightClickMenu(const QPoint &pos);
	void ShowSongLocation(Song *song);
	
	TableModel *table_model_ = nullptr;
	
	int drop_y_coord_ = -1;
};

} // quince::gui::

#pragma once

#include "decl.hxx"
#include "../err.hpp"

#include <QAbstractTableModel>
#include <QTableView>

namespace quince::gui {

class Table : public QTableView {
public:
	Table();
	virtual ~Table();
	
private:
	NO_ASSIGN_COPY_MOVE(Table);
	
	TableModel *table_model_ = nullptr;
};

} // quince::gui::

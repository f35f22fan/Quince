#include "Table.hpp"

#include "TableModel.hpp"

namespace quince::gui {

Table::Table()
{}

Table::~Table() {}

Table*
Table::Create(QAbstractTableModel *tm)
{
	auto *p = new Table();
	
	auto *h = p->horizontalHeader();
	auto *v = p->verticalHeader();
	
	p->setModel(tm);
	
	// column widths to be set after setting the model
	p->setColumnWidth(Column::Name, 500);
	p->setColumnWidth(Column::Duration, 120);
	p->setColumnWidth(Column::PlayingAt, 120);
	
	return p;
}

} // quince::gui::


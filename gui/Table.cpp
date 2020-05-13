#include "Table.hpp"

#include "TableModel.hpp"

#include <QHeaderView>

namespace quince::gui {

Table::Table()
{
	horizontalHeader()->setSectionsMovable(true);
	verticalHeader()->setSectionsMovable(true);
}

Table::~Table() {}

} // quince::gui::


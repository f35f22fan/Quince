#pragma once

#include "../types.hxx"

namespace quince::gui {

enum class UpdateTableRange : u8 {
	OneColumn,
	WholeRow,
};

class Playlist;
class SliderPane;
class Table;
class TableModel;
}

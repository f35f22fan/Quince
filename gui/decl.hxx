#pragma once

#include "../types.hxx"

namespace quince::gui {

enum class UpdateTableRange : u8 {
	OneColumn,
	WholeRow,
};

class Playlist;
class PlaylistStackWidget;
class SeekPane;
class Table;
class TableModel;
}

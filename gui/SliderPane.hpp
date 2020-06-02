#pragma once

#include <QLabel>
#include <QSlider>

#include "decl.hxx"
#include "../decl.hxx"
#include "../err.hpp"
#include "../types.hxx"

namespace quince::gui {

class SliderPane : public QWidget {
public:
	
	SliderPane(App *app);
	virtual ~SliderPane();
	
	void
	DisplayDuration(Playlist *playlist);
	
	bool slider_dragged_by_user() const { return slider_dragged_by_user_; }
	
	void SetCurrentSong(Song *song);
	void SliderValueChanged(int value);
	void UpdatePosition(const i64 new_pos);
	
private:
	NO_ASSIGN_COPY_MOVE(SliderPane);
	
	void CreateGui();
	void SetLabelValue(QLabel *label, i64 time);
	void SliderPressed();
	void SliderReleased();
	
	Song *current_song_ = nullptr;
	App *app_ = nullptr;
	bool slider_dragged_by_user_ = false;
	timespec last_seeked_ = {0, 0};
	QSlider *slider_ = nullptr;
	QLabel *position_label_ = nullptr, *duration_label_ = nullptr,
		*playlist_duration_ = nullptr;
};

}

#include "SeekPane.hpp"

#include "../App.hpp"
#include "../audio.hh"
#include "../Duration.hpp"
#include "../GstPlayer.hpp"
#include "Playlist.hpp"
#include "../Song.hpp"
#include "TableModel.hpp"

#include <QBoxLayout>
#include <time.h>

const i64 NS_MS_RATIO = 1000000L;

namespace quince::gui {

SeekPane::SeekPane(App *app) : app_(app),
	QWidget(app)
{
	CreateGui();
}

SeekPane::~SeekPane() {
	
}

void
SeekPane::ActivePlaylistChanged(gui::Playlist *playlist)
{
	CHECK_PTR_VOID(playlist);
	app_->UpdatePlaylistDuration(playlist);
	SetCurrentOrUpdate(playlist->GetCurrentSong());
}

void
SeekPane::CreateGui()
{
	QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
	setLayout(layout);
	
	position_label_ = new QLabel(this);
	layout->addWidget(position_label_);
	
	slider_ = new QSlider(Qt::Horizontal, this);
	slider_->setMinimum(0);
	connect(slider_, &QSlider::valueChanged, this, &SeekPane::SliderValueChanged);
	connect(slider_, &QSlider::sliderPressed, this, &SeekPane::SliderPressed);
	connect(slider_, &QSlider::sliderReleased, this, &SeekPane::SliderReleased);
	layout->addWidget(slider_);
	
	duration_label_ = new QLabel(this);
	layout->addWidget(duration_label_);
}

bool
SeekPane::IsActive(Song *song)
{
	return song == temp_song_info_.song;
}

void
SeekPane::SetCurrentOrUpdate(Song *song)
{
	i64 duration = -1;
	i64 position = -1;
	if (song != nullptr)
	{
		song->FillIn(temp_song_info_);
		duration = song->meta().duration();
		position = song->position();
		slider_->setMaximum(duration / NS_MS_RATIO);
		SetLabelValue(duration_label_, duration);
	} else {
		temp_song_info_ = {};
		slider_->setMaximum(0);
	}
	
	SetLabelValue(duration_label_, duration);
	SetLabelValue(position_label_, position);
	slider_->setValue(position / NS_MS_RATIO);
}

void
SeekPane::SetLabelValue(QLabel *label, i64 t)
{
	CHECK_PTR_VOID(label);
	
	if (t <= 0) {
		label->setText(QLatin1String("0:00"));
		return;
	}
	
	Duration d = Duration::FromNs(t);
	label->setText(d.toDurationString());
}

void
SeekPane::SliderPressed()
{
	last_seeked_ = {0, 0};
	slider_dragged_by_user_ = true;
}

void
SeekPane::SliderReleased()
{
	i64 pos = i64(slider_->value()) * NS_MS_RATIO;
	app_->player()->SeekTo(pos);
	slider_dragged_by_user_ = false;
}

void
SeekPane::SliderValueChanged(int value)
{
	if (slider_dragged_by_user_ && temp_song_info_.song != nullptr)
	{
		timespec now;
		
		if (clock_gettime(CLOCK_MONOTONIC_RAW, &now) != 0)
		{
			mtl_trace("%s", strerror(errno));
			return;
		}
		
		timespec diff;
		audio::timespec_diff(&last_seeked_, &now, &diff);
		i64 ms = diff.tv_sec * 1000L + diff.tv_nsec / 1000000L;
		const i64 nano = i64(value) * NS_MS_RATIO;
		
		if (ms > 300)
		{
			last_seeked_ = now;
			app_->player()->SeekTo(nano); // UpdatePosition() called implicitly
		} else {
			UpdatePosition(nano);
		}
	}
}

void
SeekPane::UpdatePosition(const i64 new_pos)
{
	SetLabelValue(position_label_, new_pos);
	
	if (!slider_dragged_by_user_)
		slider_->setValue(new_pos / NS_MS_RATIO);
}

}

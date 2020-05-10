#include "SliderPane.hpp"

#include "../App.hpp"
#include "../audio.hh"
#include "../Duration.hpp"
#include "../GstPlayer.hpp"
#include "../Song.hpp"

#include <QBoxLayout>
#include <time.h>

const i64 NS_MS_GAP = 1000000L;

namespace quince::gui {

SliderPane::SliderPane(App *app) : app_(app),
	QWidget(app)
{
	CreateGui();
}

SliderPane::~SliderPane() {
	
}

void
SliderPane::CreateGui()
{
	QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
	setLayout(layout);
	
	position_label_ = new QLabel(this);
	layout->addWidget(position_label_);
	
	slider_ = new QSlider(Qt::Horizontal, this);
	slider_->setMinimum(0);
	connect(slider_, &QSlider::valueChanged, this, &SliderPane::SliderValueChanged);
	connect(slider_, &QSlider::sliderPressed, this, &SliderPane::SliderPressed);
	connect(slider_, &QSlider::sliderReleased, this, &SliderPane::SliderReleased);
	layout->addWidget(slider_);
	
	duration_label_ = new QLabel(this);
	layout->addWidget(duration_label_);
}

void
SliderPane::SetCurrentSong(Song *song)
{
	current_song_ = song;
	const i64 max = song->meta().duration();
	slider_->setMaximum(max / NS_MS_GAP);
	SetLabelValue(duration_label_, max);
}

void
SliderPane::SetLabelValue(QLabel *label, i64 t)
{
	CHECK_PTR_RET_VOID(label);
	
	if (t <= 0) {
		label->setText(QLatin1String("0:00"));
		return;
	}
	
	Duration d = Duration::FromNs(t);
	label->setText(d.toDurationString());
}

void
SliderPane::SliderPressed()
{
	last_seeked_ = {0, 0};
	slider_dragged_by_user_ = true;
}

void
SliderPane::SliderReleased()
{
	i64 pos = i64(slider_->value()) * NS_MS_GAP;
	app_->player()->SeekTo(pos);
	slider_dragged_by_user_ = false;
}

void
SliderPane::SliderValueChanged(int value)
{
	if (slider_dragged_by_user_ && current_song_ != nullptr)
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
		
		if (ms > 300)
		{
			//mtl_info("Seek! %ld", ms);
			last_seeked_ = now;
			app_->player()->SeekTo(i64(value) * NS_MS_GAP);
		}
	}
}

void
SliderPane::UpdatePosition(const i64 new_pos)
{
	SetLabelValue(position_label_, new_pos);
	
	if (!slider_dragged_by_user_)
	{
		slider_->setValue(new_pos / NS_MS_GAP);
	}
}

}

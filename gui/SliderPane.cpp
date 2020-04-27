#include "SliderPane.hpp"

#include <QBoxLayout>

namespace quince::gui {

SliderPane::SliderPane(QWidget *parent) :
	QWidget(parent)
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
	
	slider_ = new QSlider(Qt::Horizontal, this);
	connect(slider_, &QSlider::valueChanged, this, &SliderPane::SliderValueChanged);
	layout->addWidget(slider_);
}

void
SliderPane::SliderValueChanged(int value)
{
	//mtl_info("%d", value);
}

}

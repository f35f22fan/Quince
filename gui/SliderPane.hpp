#pragma once

#include <QSlider>

#include "../err.hpp"
#include "../types.hxx"

namespace quince::gui {

class SliderPane : public QWidget {
public:
	
	SliderPane(QWidget *parent);
	virtual ~SliderPane();
	
	void SliderValueChanged(int value);
	
private:
	NO_ASSIGN_COPY_MOVE(SliderPane);
	
	void CreateGui();
	
	QSlider *slider_ = nullptr;
};

}

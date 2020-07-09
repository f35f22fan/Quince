#pragma once

#include "../decl.hxx"

#include <QWidget>

namespace quince::gui {

class PlaylistStackWidget : public QWidget {
public:
	PlaylistStackWidget(QWidget *parent, quince::App *app);
	virtual ~PlaylistStackWidget();
	
protected:
	virtual void dragEnterEvent(QDragEnterEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
	
private:
	quince::App *app_ = nullptr;
};

}

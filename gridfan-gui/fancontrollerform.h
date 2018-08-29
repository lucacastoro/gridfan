#ifndef FANCONTROLLERFORM_H
#define FANCONTROLLERFORM_H

#include "ui_fancontrollerform.h"
#include "libgridfan.h"

#include <QTimer>

class FanControllerForm : public QWidget
{
	Q_OBJECT

public:

	typedef uint32_t rpm;

	explicit FanControllerForm( grid::fan& fan, QWidget* parent = nullptr );
	void setName( const QString& name );

private slots:
	void on_verticalSlider_valueChanged(int value);
	void performChange();

private:

	Ui::FanControllerForm ui;
	QTimer delay;
	grid::fan& fan;
	int lastValue;
};

#endif // FANCONTROLLERFORM_H

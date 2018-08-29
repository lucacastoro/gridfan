#include "fancontrollerform.h"

FanControllerForm::FanControllerForm( grid::fan& f, QWidget* parent )
	: QWidget( parent )
	, fan( f )
{
	ui.setupUi( this );
	ui.groupBox->setTitle( tr("Fan %1").arg( fan.id() ) );
	ui.label->setText( tr( "%1 RPM" ).arg( fan.getSpeed() ) );
	ui.verticalSlider->setValue( fan.getPercent() );
	delay.setInterval( 250 );
	delay.setSingleShot( true );
	connect( &delay, &QTimer::timeout, this, &FanControllerForm::performChange );
}

void FanControllerForm::setName( const QString& name )
{
	ui.groupBox->setTitle( name );
}

void FanControllerForm::on_verticalSlider_valueChanged( int value )
{
	lastValue = value;
	delay.stop();
	delay.start();
}

void FanControllerForm::performChange()
{
	fan.setPercent( lastValue );
	ui.label->setText( tr( "%1 RPM" ).arg( fan.getSpeed() ) );
}

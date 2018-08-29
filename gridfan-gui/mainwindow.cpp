#include "mainwindow.h"
#include <QSpacerItem>

#define HorizontalSpacer QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum )

static constexpr const char* css = R"(
* {
	color: #AAA
}
QSlider::groove {
}
QSlider::handle {
}
QWidget {
	background: #333;
}
)";

MainWindow::MainWindow( QWidget* parent )
	: QMainWindow( parent )
{
	ui.setupUi( this );
	setStyleSheet( css );
	ui.statusBar->addPermanentWidget( &statusLabel );
	ui.centralWidget->setLayout( new QHBoxLayout() );

	setStatus("Connecting to the fan controller...");

	ui.centralWidget->layout()->addItem( new HorizontalSpacer );

	if( controller.empty() )
	{
		auto label = new QLabel( "No fan detected" );
		label->setFont( QFont( label->font().family(), 18 ) );
		ui.centralWidget->layout()->addWidget( label );
		setStatus("Fan controller unreachable");
	}
	else
	{
		setStatus("Fan controller connected");

		for( grid::fan& fan : controller )
		{
			auto x = new FanControllerForm( fan );
			fanControllers.append( x );
			ui.centralWidget->layout()->addWidget( x );
		}
	}

	ui.centralWidget->layout()->addItem( new HorizontalSpacer );
}

void MainWindow::setStatus( const QString& status )
{
	statusLabel.setText( status );
}

void MainWindow::on_action_Exit_triggered()
{
	close();
}

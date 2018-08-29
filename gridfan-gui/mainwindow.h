#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "fancontrollerform.h"
#include "libgridfan.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow( QWidget *parent = nullptr );

private slots:
	void on_action_Exit_triggered();

private:

	void setStatus( const QString& status );

	Ui::MainWindow ui;
	grid::controller controller;
	QList<FanControllerForm*> fanControllers;
	QLabel statusLabel;
};

#endif // MAINWINDOW_H

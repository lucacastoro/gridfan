#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include "mainwindow.h"

class Application : public QApplication
{
public:
	Application( int &argc, char **argv, int flags = ApplicationFlags );
private:
	MainWindow mainWindow;
};

#endif // APPLICATION_H

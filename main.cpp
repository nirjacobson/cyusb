/************************************************************************************************
 * Program Name		:	main.cpp							*
 * Author		:	V. Radhakrishnan ( rk@atr-labs.com )				*
 * License		:	LGPL Ver 2.1							*
 * Copyright		:	Cypress Semiconductors Inc. / ATR-LABS				*
 * Date written		:	July 7, 2012							*
 * Modification Notes	:									*
 * 												*
 * This program is the main GUI program for cyusb_suite for linux				*
\***********************************************************************************************/
#include <QtCore>
#include <QtGui>
#include <QtNetwork/QtNetwork>
#include <QProgressBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QMenuBar>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#include "controlcenter.h"
#include "include/cyusb.h"

ControlCenter *mainwin = NULL;
QProgressBar  *mbar = NULL;
QStatusBar *sb = NULL;

static QLocalServer server(0);

static int multiple_instances()
{

	if ( server.listen("/dev/shm/cyusb_linux") ) {
		return 0;   /* Only one instance of this application is running  */
	}

	/* If I am here, then EITHER the application is already runnning ( most likely )
	   OR the socket file already exists because of an earlier crash ! */
	return 1;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
    app.setStyle("fusion");
	if ( multiple_instances() ) {
		printf("Application already running ? If NOT, manually delete socket file /dev/shm/cyusb_linux and restart\n");
		return -1;
	}



	mainwin = new ControlCenter;
	QMainWindow *mw = new QMainWindow(0);
	mw->setCentralWidget(mainwin);
	QIcon *qic = new QIcon("cypress.png");
	app.setWindowIcon(*qic);
    mainwin->set_tool_tips();
	mw->setFixedSize(880, 660);

    mainwin->update_devlist();

	sb = mw->statusBar();

	QAction *exitAct  = new QAction("e&Xit", mw);
	QAction *aboutAct = new QAction("&About", mw);

	QMenu *fileMenu = new QMenu("&File");
	QMenu *helpMenu = new QMenu("&Help");
	fileMenu->addAction(exitAct);
	helpMenu->addAction(aboutAct);
	QMenuBar *menuBar = new QMenuBar(mw);
	menuBar->addMenu(fileMenu);
	menuBar->addMenu(helpMenu);
	QObject::connect(exitAct, SIGNAL(triggered()), mainwin, SLOT(appExit()));
	QObject::connect(aboutAct,SIGNAL(triggered()), mainwin, SLOT(about()));

	mw->show();

	sb->showMessage("Starting Application...",2000);

	return app.exec();
}

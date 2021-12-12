/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/



#include <QtWidgets>
#include "QSettings"

#include "TabDialog.h"

extern "C" void SaveSettings();

QLineEdit *hostEdit;
QLineEdit *portEdit;
QLineEdit *userEdit;
QLineEdit *passEdit;

extern QAction *actHost[16];
extern QAction *actSetup[16];

extern int ConfigHost;

#define MAXHOSTS 16

extern "C" char Host[MAXHOSTS + 1][100];
extern "C" int Port[MAXHOSTS + 1];
extern "C" char UserName[MAXHOSTS + 1][80];
extern "C" char Password[MAXHOSTS + 1][80];


//ARDOP_GUI::ARDOP_GUI(QWidget *parent) : QMainWindow(parent), ui(new Ui::ARDOP_GUI)

TabDialog::TabDialog(QWidget *parent) : QDialog(parent)
{
	tabWidget = new QTabWidget;
	tabWidget->addTab(new GeneralTab, tr("General"));

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);

	//   connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(myaccept()));
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(tabWidget);
	mainLayout->addWidget(buttonBox);
	setLayout(mainLayout);

	setWindowTitle(tr("TermTCP Host Configuration"));
}

GeneralTab::GeneralTab(QWidget *parent)
    : QWidget(parent)
{
	char portnum[16];

	QLabel *hostLabel = new QLabel(tr("Host Name:"));
	hostEdit = new QLineEdit(Host[ConfigHost]);

	QLabel *portLabel = new QLabel(tr("Port:"));
	sprintf(portnum, "%d", Port[ConfigHost]);
	portEdit = new QLineEdit(portnum);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	QLabel *userLabel = new QLabel(tr("User:"));
	userEdit = new QLineEdit(UserName[ConfigHost]);

	QLabel *passLabel = new QLabel(tr("Password:"));
	passEdit = new QLineEdit(Password[ConfigHost]);

	mainLayout->addWidget(hostLabel);
	mainLayout->addWidget(hostEdit);
	mainLayout->addWidget(portLabel);
	mainLayout->addWidget(portEdit);
	mainLayout->addWidget(userLabel);
	mainLayout->addWidget(userEdit);
	mainLayout->addWidget(passLabel);
	mainLayout->addWidget(passEdit);

	mainLayout->addStretch(1);
	setLayout(mainLayout);
}

void TabDialog::myaccept()
{
	QString val = hostEdit->text();
	QByteArray qb = val.toLatin1();
	char * ptr = qb.data();
	strcpy(Host[ConfigHost], ptr);
		
	val = portEdit->text();
	qb = val.toLatin1();
	ptr = qb.data();
	Port[ConfigHost] = atoi(ptr);

	val = userEdit->text();
	qb = val.toLatin1();
	ptr = qb.data();
	strcpy(UserName[ConfigHost], ptr);

	val = passEdit->text();
	qb = val.toLatin1();
	ptr = qb.data();
	strcpy(Password[ConfigHost], ptr);

	actHost[ConfigHost]->setText(Host[ConfigHost]);
	actSetup[ConfigHost]->setText(Host[ConfigHost]);

	SaveSettings();


	TabDialog::accept();

}

TabDialog::~TabDialog()
{
}

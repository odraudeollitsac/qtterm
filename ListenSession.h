/********************************************************************************
** Form generated from reading UI file 'ListenSession.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LISTENSESSION_H
#define UI_LISTENSESSION_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include "QTcpSocket"

QT_BEGIN_NAMESPACE

class myTcpSocket : public QTcpSocket
{
public:
	QWidget * LUI = NULL;
};

class Ui_ListenSession : public QMainWindow
{
//	Q_OBJECT

public:
	explicit Ui_ListenSession(QWidget *Parent = 0) : QMainWindow(Parent) {}
	~Ui_ListenSession() {}

    QWidget *centralwidget;
    QTextEdit *textEdit;
    QLineEdit *lineEdit;
    QMenuBar *menubar;
    QStatusBar *statusbar;
	QTcpSocket *clientSocket;

	char * KbdStack[50] = { NULL };
	int StackIndex = 0;

	QMainWindow * UI;
	int OutputSaveLen = 0;
	char OutputSave[16384];

    void setupUi(QMainWindow *ListenSession)
    {
		if (ListenSession->objectName().isEmpty())
			ListenSession->setObjectName(QString::fromUtf8("ListenSession"));
		ListenSession->resize(800, 602);
		centralwidget = new QWidget(ListenSession);
		centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
		textEdit = new QTextEdit(centralwidget);
		textEdit->setObjectName(QString::fromUtf8("textEdit"));
		textEdit->setGeometry(QRect(10, 16, 631, 511));
		lineEdit = new QLineEdit(centralwidget);
		lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
		lineEdit->setGeometry(QRect(10, 540, 631, 31));
		ListenSession->setCentralWidget(centralwidget);
		menubar = new QMenuBar(ListenSession);
		menubar->setObjectName(QString::fromUtf8("menubar"));
		menubar->setGeometry(QRect(0, 0, 800, 26));
		ListenSession->setMenuBar(menubar);

		retranslateUi(ListenSession);

		QMetaObject::connectSlotsByName(ListenSession);
	} // setupUi

    void retranslateUi(QMainWindow *ListenSession)
    {
        ListenSession->setWindowTitle(QApplication::translate("ListenSession", "MainWindow", nullptr));
    } // retranslateUi

protected:
	void resizeEvent(QResizeEvent *event) override;

private slots:

	void returnPressed();

};

namespace Ui {
    class ListenSession: public Ui_ListenSession {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LISTENSESSION_H



#include "QtTermTCP.h"
#include "TabDialog.h"
#include "QMessageBox"
#include "QTimer"
#include "QSettings"
#include "QThread"
#include <QFontDialog>
#include <QScrollBar>
#include <QFileDialog>

#define VersionString "0.0.0.15"

// .12 Save font weight
// .13 Display incomplete lines (ie without CR)
// .14 Add YAPP and Listen Mode
// .15 Reuse windows in Listen Mode 

#define MAXHOSTS 16
#define MAXPORTS 32

#define UNREFERENCED_PARAMETER(P)          (P)

extern "C" char Host[MAXHOSTS + 1][100];
extern "C" int Port[MAXHOSTS + 1];
extern "C" char UserName[MAXHOSTS + 1][80];
extern "C" char Password[MAXHOSTS + 1][80];
extern "C" char MonParams[MAXHOSTS + 1][80];
 
// There is something odd about this. It doesn't match BPQTERMTCP though it looks the same

// Chat uses these (+ 10)
//{ 0, 4, 9, 11, 13, 16, 17, 42, 45, 50, 61, 64, 66, 72, 81, 84, 85, 86, 87, 89 };

// As we have a white background we need dark colours

QRgb Colours[256] = { 0,
		qRgb(0,0,0), qRgb(0,0,128), qRgb(0,0,192), qRgb(0,0,255),				// 1 - 4
		qRgb(0,64,0), qRgb(0,64,128), qRgb(0,64,192), qRgb(0,64,255),			// 5 - 8
		qRgb(0,128,0), qRgb(0,128,128), qRgb(0,128,192), qRgb(0,128,255),		// 9 - 12
		qRgb(0,192,0), qRgb(0,192,128), qRgb(0,192,192), qRgb(0,192,255),		// 13 - 16
		qRgb(0,255,0), qRgb(0,255,128), qRgb(0,255,192), qRgb(0,255,255),		// 17 - 20

		qRgb(64,0,0), qRgb(64,0,128), qRgb(64,0,192), qRgb(0,0,255),				// 21 
		qRgb(64,64,0), qRgb(64,64,128), qRgb(64,64,192), qRgb(64,64,255),
		qRgb(64,128,0), qRgb(64,128,128), qRgb(64,128,192), qRgb(64,128,255),
		qRgb(64,192,0), qRgb(64,192,128), qRgb(64,192,192), qRgb(64,192,255),
		qRgb(64,255,0), qRgb(64,255,128), qRgb(64,255,192), qRgb(64,255,255),

		qRgb(128,0,0), qRgb(128,0,128), qRgb(128,0,192), qRgb(128,0,255),				// 41
		qRgb(128,64,0), qRgb(128,64,128), qRgb(128,64,192), qRgb(128,64,255),
		qRgb(128,128,0), qRgb(128,128,128), qRgb(128,128,192), qRgb(128,128,255),
		qRgb(128,192,0), qRgb(128,192,128), qRgb(128,192,192), qRgb(128,192,255),
		qRgb(128,255,0), qRgb(128,255,128), qRgb(128,255,192), qRgb(128,255,255),

		qRgb(192,0,0), qRgb(192,0,128), qRgb(192,0,192), qRgb(192,0,255),				// 61
		qRgb(192,64,0), qRgb(192,64,128), qRgb(192,64,192), qRgb(192,64,255),
		qRgb(192,128,0), qRgb(192,128,128), qRgb(192,128,192), qRgb(192,128,255),
		qRgb(192,192,0), qRgb(192,192,128), qRgb(192,192,192), qRgb(192,192,255),
		qRgb(192,255,0), qRgb(192,255,128), qRgb(192,255,192), qRgb(192,255,255),

		qRgb(255,0,0), qRgb(255,0,128), qRgb(255,0,192), qRgb(255,0,255),				// 81
		qRgb(255,64,0), qRgb(255,64,128), qRgb(255,64,192), qRgb(255,64,255),
		qRgb(255,128,0), qRgb(255,128,128), qRgb(255,128,192), qRgb(255,128,255),
		qRgb(255,192,0), qRgb(255,192,128), qRgb(255,192,192), qRgb(255,192,255),
		qRgb(255,255,0), qRgb(255,255,128), qRgb(255,255,192), qRgb(255,255,255)
};


extern "C"
{
	void EncodeSettingsLine(int n, char * String);
	void DecodeSettingsLine(int n, char * String);
	void WritetoOutputWindow(unsigned char * Buffer, int Len);
	void WritetoOutputWindowEx(unsigned char * Buffer, int len, QTextEdit * termWindow, int *OutputSaveLen, char * OutputSave);
	void WritetoMonWindow(char * Buffer, int Len);
	void ProcessReceivedData(unsigned char * Buffer, int len);
	void SendTraceOptions();
	void SetPortMonLine(int i, char * Text, int visible, int enabled);
	void SaveSettings();
	void Beep();
	void YAPPSendFile(char * FN);
	int SocketSend(char * Buffer, int len);
}

extern "C" int portmask;
extern "C" int mtxparam;
extern "C" int mcomparam;
extern "C" int monUI;
extern "C" int MonitorNODES;
extern "C" int MonitorColour;
extern "C" int ChatMode;
extern "C" int Bells;

extern "C" int InputMode;
extern "C" int SlowTimer;
extern "C" int MonData;
extern "C"

extern "C"	time_t LastWrite;
extern "C"	int AlertInterval;
extern "C"	int AlertBeep;
extern "C"	int AlertFreq;
extern "C"	int AlertDuration;

extern "C" char YAPPPath[256];

int OutputSaveLen;

void menuChecked(QAction * Act);

void GetSettings();

// These widgets defined here as they are accessed from outside the framework

QTextEdit *monWindow;
QTextEdit *termWindow;
QLineEdit *inputWindow;
QTcpSocket *tcpSocket = nullptr;

QAction *actHost[16];
QAction *actSetup[16];

QMenu *monitorMenu; 
QMenu * YAPPMenu;
QMenu * ListenMenu;

QAction *MonTX;
QAction *MonSup;
QAction *MonNodes;
QAction *MonUI;
QAction *MonColour;
QAction *MonPort[32];
QAction *actChatMode;
QAction *actBells;
QAction *actFonts;
QAction *actSplit;
QAction *YAPPSend;
QAction *YAPPSetRX;


extern "C" int CurrentHost;

int listenPort = 8015;
bool listenEnable = false;

int ConfigHost = 0;

char * KbdStack[50] = { NULL };
int StackIndex = 0;

int Split = 50;				// Mon/Term size split

int termX;

void resizeWindow(QRect r)
{
	int H, Mid, monHeight, termHeight, Width;

	H = r.height() - 20;
	Width = r.width() - 10;

	// Calc Positions of window

	Mid = (H * Split) / 100;

	monHeight = Mid - 38;
	termX = Mid;
	termHeight = H - Mid - 23;

	Mid += 7;

	monWindow->setGeometry(QRect(5, 38, Width, monHeight));
	termWindow->setGeometry(QRect(5, Mid, Width, termHeight));
	inputWindow->setGeometry(QRect(5, H - 9, Width, 25));

	//	splitter->setGeometry(QRect(10, 10, A * 2	 - 20, W));

}

void QtTermTCP::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	resizeWindow(geometry());

}

void Ui_ListenSession::resizeEvent(QResizeEvent* )
{
}

bool QtTermTCP::eventFilter(QObject* obj, QEvent *event)
{
	// See if from a Listening Session

	for (myTcpSocket* socket : _sockets)
	{
		Ui_ListenSession * LUI = (Ui_ListenSession *)socket->LUI;

		if (LUI->UI == obj)
		{
			if (event->type() == QEvent::Close)
			{
				LUI->clientSocket->disconnectFromHost();
			}

			if (event->type() == QEvent::Resize)
			{
				QRect r = LUI->UI->rect();

				int H, termHeight, Width;

				H = r.height() - 45;
				Width = r.width() - 10;

				// Calc Positions of window

				termHeight = H - 23;

				LUI->textEdit->setGeometry(QRect(5, 10, Width, termHeight));
				LUI->lineEdit->setGeometry(QRect(5, H - 9, Width, 25));
			}
		}

		if (LUI->lineEdit == obj)
		{
			if (event->type() == QEvent::KeyPress)
			{
				QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

				int key = keyEvent->key();

				if (key == Qt::Key_Up)
				{
					// Scroll up

					if (LUI->KbdStack[LUI->StackIndex] == NULL)
						return true;

					LUI->lineEdit->setText(LUI->KbdStack[LUI->StackIndex]);
					LUI->lineEdit->cursorForward(strlen(LUI->KbdStack[LUI->StackIndex]));

					LUI->StackIndex++;
					if (LUI->StackIndex == 50)
						LUI->StackIndex = 49;

					return true;
				}
				else if (key == Qt::Key_Down)
				{
					// Scroll down

					if (LUI->StackIndex == 0)
					{
						LUI->lineEdit->setText("");
						return true;
					}

					LUI->StackIndex--;

					if (LUI->StackIndex && LUI->KbdStack[LUI->StackIndex - 1])
					{
						LUI->lineEdit->setText(LUI->KbdStack[LUI->StackIndex - 1]);
						LUI->lineEdit->cursorForward(strlen(LUI->KbdStack[LUI->StackIndex - 1]));
					}
					else
						LUI->lineEdit->setText("");

					return true;
				}
				else if (key == Qt::Key_Return || key == Qt::Key_Enter)
				{
					LreturnPressed(LUI);
					return true;
				}

				return false;
			}

			if (event->type() == QEvent::MouseButtonPress)
			{
				QMouseEvent *k = static_cast<QMouseEvent *> (event);

				// Paste on Right Click

				if (k->button() == Qt::RightButton)
				{
					LUI->lineEdit->paste();
					return true;
				}
				return QMainWindow::eventFilter(obj, event);
			}
		}
	}
	
	if (obj == inputWindow)
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *k = static_cast<QMouseEvent *> (event);
			
			// Paste on Right Click

			if (k->button() == Qt::RightButton)
			{
				inputWindow->paste();
				return true;
			}
		}

		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
	
			if (keyEvent->key() == Qt::Key_Up)
			{
				// Scroll up

				if (KbdStack[StackIndex] == NULL)
					return true;

				inputWindow->setText(KbdStack[StackIndex]);
				inputWindow->cursorForward(strlen(KbdStack[StackIndex]));
	
				StackIndex++;
				if (StackIndex == 50)
					StackIndex = 49;

				return true;
			}
			else if (keyEvent->key() == Qt::Key_Down)
			{
				// Scroll down

				if (StackIndex == 0)
				{
					inputWindow->setText("");
					return true;
				}

				StackIndex--;

				if (StackIndex && KbdStack[StackIndex - 1])
				{
					inputWindow->setText(KbdStack[StackIndex - 1]);
					inputWindow->cursorForward(strlen(KbdStack[StackIndex - 1]));
				}
				else
					inputWindow->setText("");

				return true;
			}
			return false;
		}
		return false;
	}

//	if (event->type() == QEvent::ActionChanged)
//	{
//		menuChecked((QAction *)obj);
//	}
	return QMainWindow::eventFilter(obj, event);
}
	
QAction * setupMenuLine(QMenu * Menu, char * Label, QObject * parent, int State)
{
	QAction * Mon = new QAction(Label, parent);
	Menu->addAction(Mon);

	Mon->setCheckable(true);
	if (State)
		Mon->setChecked(true);

	// This isn't ideal for QT5 but should be Qt4 compatible

	//Mon->installEventFilter(parent);

	//	parent->connect(Mon, SIGNAL(triggered()), parent, SLOT(menuChecked()));

	parent->connect(Mon, &QAction::triggered, parent, [=] { menuChecked(Mon); });

	return Mon;
}


QtTermTCP::QtTermTCP(QWidget *parent)
	: QMainWindow(parent)
{
	int i;
	char Title[80];

	ui.setupUi(this);

	sprintf(Title, "QtTermTCP Version %s", VersionString);

	this->setWindowTitle(Title);

	tcpSocket = new QTcpSocket(this);

	GetSettings();

	QFont menufont = QFont("Aerial", 10);
	
//	ui.menuBar->setGeometry(QRect(0, 0, 951, 50));
	ui.menuBar->setFont(menufont);
	
	connectMenu = ui.menuBar->addMenu(tr("&Connect"));

	for (i = 0; i < MAXHOSTS; i++)
	{
		actHost[i] = new QAction(Host[i], this);
		actHost[i]->setFont(menufont);
		connectMenu->addAction(actHost[i]);
		connect(actHost[i], &QAction::triggered, this, [=] { Connect(i); });
	}

	discAction = ui.menuBar->addAction("&Disconnect");
	//	discAction->setShortcut(Qt::Key_D | Qt::ALT);

	connect(discAction, SIGNAL(triggered()), this, SLOT(Disconnect()));

	discAction->setEnabled(false);

	setupMenu = ui.menuBar->addMenu(tr("&Setup"));

	hostsubMenu = setupMenu->addMenu("Setup Hosts");

	for (i = 0; i < MAXHOSTS; i++)
	{
		if (Host[i][0])
			actSetup[i] = new QAction(Host[i], this);
		else
			actSetup[i] = new QAction("New Host", this);

		hostsubMenu->addAction(actSetup[i]);
		connect(actSetup[i], &QAction::triggered, this, [=] {SetupHosts(i); });
	}

	actFonts = new QAction("Setup Fonts", this);
	setupMenu->addAction(actFonts);


	connect(actFonts, SIGNAL(triggered()), this, SLOT(selFont()));

	actChatMode = setupMenuLine(setupMenu, (char *)"Chat Terminal Mode", this, ChatMode);
	actBells = setupMenuLine(setupMenu, (char *)"Enable Bells", this, Bells);


	monitorMenu = ui.menuBar->addMenu(tr("&Monitor"));

	MonTX = setupMenuLine(monitorMenu, (char *)"Monitor TX", this, mtxparam);
	MonSup = setupMenuLine(monitorMenu, (char *)"Monitor Supervisory", this, mcomparam);
	MonUI = setupMenuLine(monitorMenu, (char *)"Only Monitor UI Frames", this, monUI);
	MonNodes = setupMenuLine(monitorMenu, (char *)"Monitor NODES Broadcasts", this, MonitorNODES);
	MonColour = setupMenuLine(monitorMenu, (char *)"Enable Colour", this, MonitorColour);

	for (i = 0; i < MAXPORTS; i++)
	{
		MonPort[i] = setupMenuLine(monitorMenu, (char *)"Port", this, 0);
		MonPort[i]->setVisible(false);
	}

	//	ListenMenu = ui.menuBar->addMenu(tr("&Listen"));
	ListenAction = ui.menuBar->addAction("&Listen");
	connect(ListenAction, SIGNAL(triggered()), this, SLOT(ListenSlot()));

	YAPPMenu = ui.menuBar->addMenu(tr("&YAPP"));

	YAPPSend = new QAction("Send File", this);
	YAPPSetRX = new QAction("Set Receive Directory", this);

	YAPPMenu->addAction(YAPPSend);
	YAPPMenu->addAction(YAPPSetRX);
	YAPPSend->setEnabled(false);


	connect(YAPPSend, SIGNAL(triggered()), this, SLOT(doYAPPSend()));
	connect(YAPPSetRX, SIGNAL(triggered()), this, SLOT(doYAPPSetRX()));

	//	QSplitter *splitter = new QSplitter(this);
	//	splitter->setOrientation(Qt::Vertical);

	//	splitter->setGeometry(QRect(10, 10, 770, 700));
	monWindow = new QTextEdit(this);
	monWindow->setReadOnly(1);

	monWindow->setGeometry(QRect(0, 0, 770, 300));

	termWindow = new QTextEdit(this);
	termWindow->setReadOnly(1);

	termWindow->setGeometry(QRect(0, 0, 770, 300));

	inputWindow = new QLineEdit(this);
	inputWindow->setGeometry(QRect(0, 0, 770, 650));
	inputWindow->installEventFilter(this);
	inputWindow->setContextMenuPolicy(Qt::PreventContextMenu);

	// Add Custom Menu to set Mon/Term Split with Right Click

	monWindow->setContextMenuPolicy(Qt::CustomContextMenu);
	termWindow->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(monWindow, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(showContextMenuM(const QPoint &)));

	connect(termWindow, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(showContextMenuT(const QPoint &)));

	connect(termWindow, SIGNAL(selectionChanged()),
		this, SLOT(ontermselectionChanged()));

	connect(monWindow, SIGNAL(selectionChanged()),
		this, SLOT(onmonselectionChanged()));

	connect(inputWindow, SIGNAL(selectionChanged()),
		this, SLOT(oninputselectionChanged()));

	actSplit = new QAction("Set Monitor/Output Split", this);

	QSettings settings("G8BPQ", "QT_TERM_TCP");

	QFont font = QFont(settings.value("FontFamily", "Courier New").value<QString>(),
		settings.value("PointSize", 10).toInt(),
		settings.value("Weight", 50).toInt());

	monWindow->setFont(font);
	termWindow->setFont(font);
	inputWindow->setFont(font);

	//splitter->addWidget(monWindow);
	//splitter->addWidget(termWindow);

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(10000);

	connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &QtTermTCP::displayError);
	connect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
	connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(tcpSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
	connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));

//	connect(inputWindow, SIGNAL(textChanged(const QString &)), this, SLOT(QtTermTCP::inputChanged(const QString &)));
	connect(inputWindow, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

	if (listenEnable)
		_server.listen(QHostAddress::Any, listenPort);
	
	connect(&_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

}

// "Copy on select" Code

void QtTermTCP::ontermselectionChanged()
{
	termWindow->copy();
}

void QtTermTCP::onmonselectionChanged()
{
	monWindow->copy();
}

void QtTermTCP::oninputselectionChanged()
{
	inputWindow->copy();
}

int splitY;					// Value when menu added

void QtTermTCP::setSplit()
{
	QRect Size = geometry();
	int y = Size.height() - 50;

	// y is height of whole windom. splitX is new split position
	// So split = 100 * splitx/x

	Split = (splitY * 100) / y;

	if (Split < 10)
		Split = 10;
	else if (Split > 90)
		Split = 90;

	resizeWindow(geometry());
}

void QtTermTCP::showContextMenuT(const QPoint &pt)				// Term Window
{
	QMenu *menu = termWindow->createStandardContextMenu();
	menu->addAction(actSplit);

	splitY = pt.y() + termX;

	connect(actSplit, SIGNAL(triggered()), this, SLOT(setSplit()));

	menu->exec(termWindow->mapToGlobal(pt));
	delete menu;
}

void QtTermTCP::showContextMenuM(const QPoint &pt)				// Mon Window
{
	QMenu *menu = monWindow->createStandardContextMenu();
	menu->addAction(actSplit);

	splitY = pt.y();

	connect(actSplit, SIGNAL(triggered()), this, SLOT(setSplit()));

	menu->exec(monWindow->mapToGlobal(pt));
	delete menu;
}

void QtTermTCP::selFont()
{
	bool ok;
	QFont font= QFontDialog::getFont(&ok, monWindow->font(), this, "Title", QFontDialog::MonospacedFonts);

	if (ok)
	{
		monWindow->setFont(font);
		termWindow->setFont(font);
		inputWindow->setFont(font);
	
		QSettings settings("G8BPQ", "QT_TERM_TCP");

		settings.setValue("FontFamily", font.family());
		settings.setValue("PointSize", font.pointSize());
		settings.setValue("Weight", font.weight());
	}
}

void QtTermTCP::SetupHosts(int i)
{
	ConfigHost = i;

	TabDialog tabdialog(0);
	tabdialog.exec();
}

void QtTermTCP::Connect(int i)
{
	CurrentHost = i;

	// Set Monitor Params for this host

	sscanf(MonParams[CurrentHost], "%x %x %x %x %x %x",
		&portmask, &mtxparam, &mcomparam, &MonitorNODES, &MonitorColour, &monUI);

	MonTX->setChecked(mtxparam);
	MonSup->setChecked(mcomparam);
	MonUI->setChecked(monUI);
	MonNodes->setChecked(MonitorNODES);
	MonColour->setChecked(MonitorColour);

	// Remove old Monitor menu

	for (i = 0; i < 32; i++)
	{
		SetPortMonLine(i, (char *)"", 0, 0);			// Set all hidden
	}

	tcpSocket->abort();
	tcpSocket->connectToHost(Host[CurrentHost], Port[CurrentHost]);
}

void QtTermTCP::Disconnect()
{
	tcpSocket->disconnectFromHost();
}

void QtTermTCP::doYAPPSend()
{
	QFileDialog dialog(this);
	QStringList fileNames;
	dialog.setFileMode(QFileDialog::AnyFile);
	int len;
	char Mess[64];

	// Turn off monitoring

	len = sprintf(Mess, "\\\\\\\\%x %x %x %x %x %x %x %x\r", 0, 0, 0, 0, 0, 0, 0, 0);
	SocketSend(Mess, len);

	if (dialog.exec())
	{
		char FN[256];

		fileNames = dialog.selectedFiles();
		if (fileNames[0].length() < 256)
		{
			strcpy(FN, fileNames[0].toUtf8());
			YAPPSendFile(FN);
		}
	}
}

void QtTermTCP::doYAPPSetRX()
{
	QFileDialog dialog(this);
	QStringList fileNames;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setDirectory(YAPPPath);

	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();
		if (fileNames[0].length() < 256)
			strcpy(YAPPPath, fileNames[0].toUtf8());

		SaveSettings();
	}
}

void menuChecked(QAction * Act)
{
	int state = Act->isChecked();
	
	if (Act == MonTX)
		mtxparam = state;
	else if (Act == MonSup)
		mcomparam = state;
	else if (Act == MonUI)
		monUI = state;
	else if (Act == MonNodes)
		MonitorNODES = state;
	else if (Act == MonColour)
		MonitorColour = state;
	else if (Act == actChatMode)
		ChatMode = state;
	else if (Act == actBells)
		Bells = state;
	else
	{
		// look for port entry

		for (int i = 0; i < MAXPORTS; i++)
		{
			if (Act == MonPort[i])
			{
				unsigned int mask = 0x1 << (i - 1);

				if (state)
					portmask |= mask;
				else
					portmask &= ~mask;
				break;
			}
		}
	}
	SendTraceOptions();
}

//void QtTermTCP::inputChanged(const QString &)
//{
//}


void QtTermTCP::LDisconnect(Ui_ListenSession * LUI)
{
	if (LUI->clientSocket)
		LUI->clientSocket->disconnectFromHost();
}

void QtTermTCP::LreturnPressed(Ui_ListenSession * LUI)
{

	QByteArray stringData = LUI->lineEdit->text().toUtf8();


	// if multiline input (eg from copy/paste) replace LF with CR

	char * ptr;
	char * Msgptr;
	char * Msg;

	QScrollBar *scrollbar = LUI->textEdit->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

	if (scrollbarAtBottom)
		LUI->textEdit->moveCursor(QTextCursor::End);					// So we don't get blank lines

	// Stack it


	LUI->StackIndex = 0;

	if (LUI->KbdStack[49])
		free(LUI->KbdStack[49]);

	for (int i = 48; i >= 0; i--)
	{
		LUI->KbdStack[i + 1] = LUI->KbdStack[i];
	}

	LUI->KbdStack[0] = qstrdup(stringData.data());

	stringData.append('\n');

	Msgptr = stringData.data();

	LastWrite = time(NULL);				// Stop initial beep

	if (LUI->clientSocket->state() == QAbstractSocket::ConnectedState) 
	{
		while ((ptr = strchr(Msgptr, '\n')))
		{
			*ptr++ = 0;
			Msg = (char *)malloc(strlen(Msgptr) + 5);
			strcpy(Msg, Msgptr);
			strcat(Msg, "\r");

			LUI->clientSocket->write(Msg);
			LUI->textEdit->setTextColor(QColor("black"));
			LUI->textEdit->insertPlainText(Msg);

			free(Msg);

			Msgptr = ptr;
		}
	}
	else
	{
		LUI->textEdit->setTextColor(QColor("red"));
		LUI->textEdit->insertPlainText("Not Connected\r");
	}

	if (scrollbarAtBottom)
		LUI->textEdit->moveCursor(QTextCursor::End);

	LUI->lineEdit->setText("");
}

void QtTermTCP::returnPressed()
{
	QByteArray stringData = inputWindow->text().toUtf8();

	// if multiline input (eg from copy/paste) replace LF with CR

	char * ptr;
	char * Msgptr;
	char * Msg;
	
	QScrollBar *scrollbar = monWindow->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

	if (scrollbarAtBottom)
		termWindow->moveCursor(QTextCursor::End);					// So we don't get blank lines
																	
	// Stack it

	StackIndex = 0;

	if (KbdStack[49])
		free(KbdStack[49]);

	for (int i = 48; i >= 0; i--)
	{
		KbdStack[i + 1] = KbdStack[i];
	}

	KbdStack[0] = qstrdup(stringData.data());

	stringData.append('\n');

	Msgptr = stringData.data();

	LastWrite = time(NULL);				// Stop initial beep

	if (tcpSocket->state() != QAbstractSocket::ConnectedState)
	{
		// Not Connected 

		termWindow->setTextColor(QColor("red"));
		termWindow->insertPlainText("Connecting....\r");

		tcpSocket->abort();
		tcpSocket->connectToHost(Host[CurrentHost], Port[CurrentHost]);
	}
	else
	{
		while ((ptr = strchr(Msgptr, '\n')))
		{
			*ptr++ = 0;
			Msg = (char *)malloc(strlen(Msgptr) + 5);
			strcpy(Msg, Msgptr);
			strcat(Msg, "\r");

			tcpSocket->write(Msg);
			termWindow->setTextColor(QColor("black"));
			termWindow->insertPlainText(Msg);
			
			free(Msg);

			Msgptr = ptr;
		}
	}

	if (scrollbarAtBottom)
		termWindow->moveCursor(QTextCursor::End);

	inputWindow->setText("");
}



void QtTermTCP::displayError(QAbstractSocket::SocketError socketError)
{
	switch (socketError)
	{
	case QAbstractSocket::RemoteHostClosedError:
		break;

	case QAbstractSocket::HostNotFoundError:
		QMessageBox::information(this, tr("QtTermTCP"),
			tr("The host was not found. Please check the "
				"host name and port settings."));
		break;

	case QAbstractSocket::ConnectionRefusedError:
		QMessageBox::information(this, tr("QtTermTCP"),
			tr("The connection was refused by the peer."));
		break;

	default:
		QMessageBox::information(this, tr("QtTermTCP"),
			tr("The following error occurred: %1.")
			.arg(tcpSocket->errorString()));
	}

	connectMenu->setEnabled(true);
}

void QtTermTCP::connected()
{
	char Signon[256];
	char Title[128];

	sprintf(Signon, "%s\r%s\rBPQTERMTCP\r", UserName[CurrentHost], Password[CurrentHost]);

	tcpSocket->write(Signon);

	discAction->setEnabled(true);
	YAPPSend->setEnabled(true);
	connectMenu->setEnabled(false);

	SendTraceOptions();

	InputMode = 0;
	SlowTimer = 0;
	MonData = 0;
	OutputSaveLen = 0;			// Clear any part line

	sprintf(Title, "QtTermTCP Version %s - Connected to %s", VersionString, Host[CurrentHost]);

	this->setWindowTitle(Title);
}

void QtTermTCP::disconnected()
{
	char Title[128];

	termWindow->setTextColor(QColor("red"));
	termWindow->append("*** Disconnected\r");
	termWindow->moveCursor(QTextCursor::End); 
	termWindow->moveCursor(QTextCursor::End);
	connectMenu->setEnabled(true);
	YAPPSend->setEnabled(false);

	sprintf(Title, "QtTermTCP Version %s - Disonnected", VersionString);

	this->setWindowTitle(Title);
}

void QtTermTCP::bytesWritten(qint64)
{
	//qDebug() << bytes << " bytes written...";
}

void QtTermTCP::readyRead()
{
	int Read;
	unsigned char Buffer[4096];

	// read the data from the socket

	Read = tcpSocket->read((char *)Buffer, 4095);

	if (Read > 0)
	{
		Buffer[Read] = 0;

//		if (InputMode == 'Y')			// Yapp
//		{
//			QString myString = QString::fromUtf8((char*)Buffer, Read);
//			QByteArray ptr = myString.toLocal8Bit();
//			memcpy(Buffer, ptr.data(), ptr.length());
//			Read = ptr.length();
//		}

		ProcessReceivedData(Buffer, Read);

		QString myString = QString::fromUtf8((char*)Buffer);
//		qDebug() << myString;
	}
}

extern "C" int SocketSend(char * Buffer, int len)
{
	if (tcpSocket->state() == QAbstractSocket::ConnectedState)
		return tcpSocket->write(Buffer, len);
	
	return 0;
}

extern "C" int SocketFlush()
{
	if (tcpSocket->state() == QAbstractSocket::ConnectedState)
		return tcpSocket->flush();

	return 0;
}

extern "C" void Sleep(int ms)
{
	QThread::msleep(ms);
}

extern "C" void SetPortMonLine(int i, char * Text, int visible, int enabled)
{
	MonPort[i]->setText(Text);
	MonPort[i]->setVisible(visible);
	MonPort[i]->setChecked(enabled);
}


char OutputSave[16384];		// Should never be more than 12 K (8K save, 4K new)

bool scrollbarAtBottom = 0;

extern "C" void WritetoOutputWindowEx(unsigned char * Buffer, int len, QTextEdit * termWindow, int * OutputSaveLen, char * OutputSave);

extern "C" void WritetoOutputWindow(unsigned char * Buffer, int len)
{
	WritetoOutputWindowEx(Buffer, len, termWindow, &OutputSaveLen, OutputSave);
}


extern "C" void WritetoOutputWindowEx(unsigned char * Buffer, int len, QTextEdit * termWindow, int * OutputSaveLen, char * OutputSave)
{
	char Copy[8192];
	char * ptr1, *ptr2;
	char Line[512];
	int num;



	time_t NOW = time(NULL);


	// Beep if no output for a while

	if (AlertInterval && (NOW - LastWrite) > AlertInterval)
	{
		if (AlertBeep)
			Beep();
	}

	LastWrite = NOW;

	// Mustn't mess with original buffer

	memcpy(Copy, Buffer, len);

	Copy[len] = 0;

	ptr1 = Copy;

	// Write a line at a time so we can action colour chars

//		Buffer[Len] = 0;

	if (*OutputSaveLen)
	{
		// Have part line - append to it
		memcpy(&OutputSave[*OutputSaveLen], Copy, len);
		*OutputSaveLen += len;
		ptr1 = OutputSave;
		len = *OutputSaveLen;
		*OutputSaveLen = 0;

		// part line was written to screen so remove it


		termWindow->setFocus();
		QTextCursor storeCursorPos = termWindow->textCursor();
		termWindow->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
		termWindow->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
		termWindow->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
		termWindow->textCursor().removeSelectedText();
//		termWindow->textCursor().deletePreviousChar();
		termWindow->setTextCursor(storeCursorPos);

	}
	else
	{
		QScrollBar *scrollbar = termWindow->verticalScrollBar();
		scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

		if (scrollbarAtBottom)
			termWindow->moveCursor(QTextCursor::End);
	}

lineloop:

	if (len <= 0)
	{
		if (scrollbarAtBottom)
			termWindow->moveCursor(QTextCursor::End);

		return;
	}

	//	copy text to control a line at a time	

	ptr2 = (char *)memchr(ptr1, 13, len);

	if (ptr2 == 0)	// No CR
	{
		if (len > 8000)
			len = 8000;			// Should never get lines this long

		memmove(OutputSave, ptr1, len);
		*OutputSaveLen = len;

		// Write part line to screen


		memcpy(Line, ptr1, len);
		Line[len] = 0;

		if (Line[0] == 0x1b)			// Colour Escape
		{
			if (MonitorColour)
				termWindow->setTextColor(Colours[Line[1] - 10]);

			termWindow->insertPlainText(QString::fromUtf8((char*)&Line[2]));
		}
		else
		{
			termWindow->setTextColor(Colours[23]);
			termWindow->insertPlainText(QString::fromUtf8((char*)Line));
		}

		return;
	}

	*(ptr2++) = 0;

	if (Bells)
	{
		char * ptr;

		do {
			ptr = (char *)memchr(ptr1, 7, len);

			if (ptr)
			{
				*(ptr) = 32;
				Beep();
			}
		} while (ptr);
	}

	num = ptr2 - ptr1 - 1;

	//		if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

	memcpy(Line, ptr1, num);
	Line[num++] = 13;
	Line[num] = 0;

	if (Line[0] == 0x1b)			// Colour Escape
	{
		if (MonitorColour)
			termWindow->setTextColor(Colours[Line[1] - 10]);

		termWindow->insertPlainText(QString::fromUtf8((char*)&Line[2]));
	}
	else
	{
		termWindow->setTextColor(Colours[23]);
		termWindow->insertPlainText(QString::fromUtf8((char*)Line));
	}

	len -= (ptr2 - ptr1);

	ptr1 = ptr2;

	goto lineloop;

	termWindow->setTextColor(Colours[23]);

}

char MonSave[2000];
int MonSaveLen;


extern "C" void WritetoMonWindow(char * Msg, int len)
{
	char * ptr1 = Msg, * ptr2;
	char Line[512];
	int num;

	QScrollBar *scrollbar = monWindow->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

	// Write a line at a time so we can action colour chars
		
//		Buffer[Len] = 0;

	if (scrollbarAtBottom)
		monWindow->moveCursor(QTextCursor::End);	

	if (MonSaveLen)
	{
		// Have part line - append to it
		memcpy(&MonSave[MonSaveLen], Msg, len);
		MonSaveLen += len;
		ptr1 = MonSave;
		len = MonSaveLen;
		MonSaveLen = 0;
	}

lineloop:

	if (len <= 0)
	{
		if (scrollbarAtBottom)
			monWindow->moveCursor(QTextCursor::End);

		return;
	}

	//	copy text to control a line at a time	

	ptr2 = (char *)memchr(ptr1, 13, len);

	if (ptr2 == 0)	// No CR
	{
		memmove(MonSave, ptr1, len);
		MonSaveLen = len;
		return;
	}

	*(ptr2++) = 0;

//		if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

	num = ptr2 - ptr1 - 1;

	memcpy(Line, ptr1, num);
	Line[num++] = 13;
	Line[num] = 0;

	if (Line[0] == 0x1b)			// Colour Escape
	{
		if (MonitorColour)
		{
			if (Line[1] == 17)
				monWindow->setTextColor(qRgb(0, 0, 192));
			else
				monWindow->setTextColor(QColor(qRgb(192, 0, 0)));
		}
		else
			monWindow->setTextColor(QColor("black"));


		monWindow->insertPlainText(QString::fromUtf8((char*)&Line[2]));
	}
	else
	{
		monWindow->insertPlainText(QString::fromUtf8((char*)Line));
	}
	len -= (ptr2 - ptr1);

	ptr1 = ptr2;

	goto lineloop;

}


void GetSettings()
{
	QByteArray qb;
	int i;
	char Key[16];

	QSettings settings("G8BPQ", "QT_TERM_TCP");

	for (i = 0; i < MAXHOSTS; i++)
	{
		sprintf(Key, "HostParams%d", i);

		qb = settings.value(Key).toByteArray();

		DecodeSettingsLine(i, qb.data());
	}

	Split = settings.value("Split", 50).toInt();
	ChatMode = settings.value("ChatMode", 1).toInt();
	Bells = settings.value("Bells", 1).toInt();
	CurrentHost = settings.value("CurrentHost", 0).toInt();
	strcpy(YAPPPath, settings.value("YAPPPath", "").toString().toUtf8());

	listenPort = settings.value("listenPort", 8015).toInt();
	listenEnable = settings.value("listenEnable", false).toBool();
}

extern "C" void SaveSettings()
{
	QSettings settings("G8BPQ", "QT_TERM_TCP");
	int i;
	char Param[512];
	char Key[16];


	for (i = 0; i < MAXHOSTS; i++)
	{
		sprintf(Key, "HostParams%d", i);
		EncodeSettingsLine(i, Param);
		settings.setValue(Key, Param);
	}

	settings.setValue("Split", Split);
	settings.setValue("ChatMode", ChatMode);
	settings.setValue("Bells", Bells);
	settings.setValue("CurrentHost", CurrentHost);

	settings.setValue("YAPPPath", YAPPPath);
	settings.setValue("listenPort", listenPort);
	settings.setValue("listenEnable", listenEnable);

	settings.sync();
}


QtTermTCP::~QtTermTCP()
{
	SaveSettings();
}


void QtTermTCP::MyTimerSlot()
{
	// Runs every 10 seconds

	if (tcpSocket->state() != QAbstractSocket::ConnectedState)
		return;

	if (!ChatMode)
		return;

	SlowTimer++;

	if (SlowTimer > 54)				// About 9 mins
	{
		SlowTimer = 0;
		tcpSocket->write("\0", 1);
	}
}

extern "C" void Beep()
{
	QApplication::beep();
}

Ui_ListenPort  * Sess;
QDialog * LUI;

void QtTermTCP::myaccept()
{
	QString val = Sess->portNo->text();
    QByteArray qb = val.toLatin1();
	char * ptr = qb.data();

	listenPort = atoi(ptr);
	listenEnable = Sess->Enabled->isChecked();

	if (_server.isListening())
		_server.close();

	if (listenEnable)
		_server.listen(QHostAddress::Any, listenPort);

	SaveSettings();
	LUI->close();
}



void QtTermTCP::ListenSlot()
{
	Sess = new(Ui_ListenPort);
	LUI = new(QDialog);
	char portnum[16];
	sprintf(portnum, "%d", listenPort);
	QString portname(portnum);

 	Sess->setupUi(LUI);

	Sess->portNo->setText(portname);
	Sess->Enabled->setChecked(listenEnable);
	
	connect(Sess->buttonBox, SIGNAL(accepted()), this, SLOT(myaccept()));

	LUI->show();
}


QAction *LdiscAction;

void QtTermTCP::onNewConnection()
{
	myTcpSocket *clientSocket = (myTcpSocket *)_server.nextPendingConnection();

	clientSocket->LUI = NULL;
	
	_sockets.push_back(clientSocket);

	Ui_ListenSession  * Sess = NULL;
	QMainWindow * LUI = NULL;

	// See if an old session can be reused

	for (Ui_ListenSession * S: _sessions)
	{
		if (S->clientSocket == NULL)
		{
			Sess = S;
			break;
		}	
	}

	// Create a window if none found, else reuse old

	if (Sess == NULL)
	{
		Sess = new(Ui_ListenSession);
		_sessions.push_back(Sess);
		LUI = new(QMainWindow);

		Sess->setupUi(LUI);
		Sess->UI = LUI;

		LdiscAction = Sess->menubar->addAction("&Disconnect");

		connect(LdiscAction, &QAction::triggered, this, [=] {  LDisconnect(Sess); });

//		connect(Sess->lineEdit, &QLineEdit::returnPressed, this, [=] {  LreturnPressed(Sess); });

		QRect r = LUI->rect();

		int H, termHeight, Width;

		H = r.height() - 45;
		Width = r.width() - 10;

		// Calc Positions of window

		termHeight = H - 23;

		Sess->textEdit->setGeometry(QRect(5, 10, Width, termHeight));
		Sess->lineEdit->setGeometry(QRect(5, H - 9, Width, 25));

		LUI->show();

		QSettings settings("G8BPQ", "QT_TERM_TCP");

		QFont font = QFont(settings.value("FontFamily", "Courier New").value<QString>(),
			settings.value("PointSize", 10).toInt(),
			settings.value("Weight", 50).toInt());

		Sess->lineEdit->setFont(font);
		Sess->textEdit->setFont(font);

		// Look for UP/Down or right click for paste and suppress normal context menu

		Sess->lineEdit->installEventFilter(this);
		Sess->lineEdit->setContextMenuPolicy(Qt::PreventContextMenu);
	}
	else
		LUI = Sess->UI;

	clientSocket->LUI = Sess;

	char Title[128];

	QByteArray Host = clientSocket->peerAddress().toString().toUtf8();

	// See if any data from host - first msg should be callsign

	clientSocket->waitForReadyRead(1000);

	QByteArray datas = clientSocket->readAll();

	datas.chop(2);
	datas.append('\0');

	sprintf( Title, "Inward Connect from %s:%d Call " + datas, 
		Host.data(), clientSocket->peerPort());

	LUI->setWindowTitle(Title);

	connect(clientSocket, &QTcpSocket::readyRead, this, [=] {  onReadyRead(Sess); });
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

	Sess->clientSocket = clientSocket;

	Sess->UI->installEventFilter(this);

	Beep();
}

void QtTermTCP::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		myTcpSocket* sender = static_cast<myTcpSocket*>(QObject::sender());
		Ui_ListenSession * LUI = (Ui_ListenSession *)sender->LUI;

		LUI->textEdit->setTextColor(QColor("red"));
		LUI->textEdit->insertPlainText("Disconnected\r");
		LUI->UI->setWindowTitle("Disconnected");
		LUI->clientSocket = NULL;

		_sockets.removeOne(sender);
	}
}

void QtTermTCP::onReadyRead(Ui_ListenSession * LUI)
{
	myTcpSocket* sender = static_cast<myTcpSocket*>(QObject::sender());
	QByteArray datas = sender->readAll();

//	LUI->textEdit->setTextColor(Colours[23]);
//	LUI->textEdit->insertPlainText(datas);

	WritetoOutputWindowEx((unsigned char *)datas.data(), datas.length(),
		LUI->textEdit, &LUI->OutputSaveLen, LUI->OutputSave);


	// This needs to feed through WriteToOutputWindow

	//	for (QTcpSocket* socket : _sockets) {
	//		if (socket != sender)
	//			socket->write(datas);
	//	}
}



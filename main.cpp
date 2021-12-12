#include "QtTermTCP.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QtTermTCP w;
	w.show();

	return a.exec();
}


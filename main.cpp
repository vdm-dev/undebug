#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("UnDebug");
    application.setOrganizationName("");
    application.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("UnDebug - a PDB parser");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(application);

    MainWindow mainWin;
    const QStringList posArgs = parser.positionalArguments();
    if (!posArgs.isEmpty())
        mainWin.openFile(posArgs.at(0));

    mainWin.show();
    return application.exec();
}

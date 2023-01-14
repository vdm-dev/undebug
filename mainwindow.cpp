#include "mainwindow.h"

#include <QtWidgets>
#include <QDebug>
#include <QMap>

#include "mdichild.h"
#include "path.h"


MainWindow::MainWindow()
    : mdiArea(new QMdiArea)
    , _library(NULL)
    , _diaDataSource(NULL)
    , _diaSession(NULL)
    , _diaSymbolGlobal(NULL)
{
    createDockedTree(&_treeModules, "Modules", QStringList({"Module", "Path"}));
    createDockedTree(&_treeObjects, "Objects", QStringList({"Object", "Description"}));
    createDockedTree(&_treeTest, "Test", QStringList({"Test"}));
    createDockedTree(&_treeTypedefs, "Typedefs", QStringList({"Base Type", "New Type"}));


    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(mdiArea);
    connect(mdiArea, &QMdiArea::subWindowActivated,
            this, &MainWindow::updateMenus);

    createActions();
    createStatusBar();
    updateMenus();

    readSettings();

    setWindowTitle("UnDebug");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        closeFile();
        writeSettings();
        event->accept();
    }
}

void MainWindow::open()
{
    const QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
        openFile(fileName);
}

bool MainWindow::openFile(const QString &fileName)
{
    closeFile();

    _library = LoadLibraryW(L"msdia140.dll");
    if (!_library)
        return false;

    HRESULT (WINAPI *GetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

    *((FARPROC*) &GetClassObject) = GetProcAddress(_library, "DllGetClassObject");
    if (!GetClassObject)
    {
        FreeLibrary(_library);
        _library = NULL;
        return false;
    }

    IClassFactory* factory = nullptr;
    HRESULT result = GetClassObject(__uuidof(DiaSource), IID_IClassFactory, (LPVOID*) &factory);
    if (FAILED(result))
    {
        FreeLibrary(_library);
        _library = NULL;
        return false;
    }

    result = factory->CreateInstance(NULL, __uuidof(IDiaDataSource), (void**) &_diaDataSource);
    factory->Release();

    QFileInfo fileInfo(fileName);

    if (SUCCEEDED(result))
    {
        if (fileInfo.suffix().compare("exe", Qt::CaseInsensitive) == 0)
        {
            result = _diaDataSource->loadDataForExe((wchar_t*) fileName.utf16(), NULL, NULL);
        }
        else
        {
            result = _diaDataSource->loadDataFromPdb((wchar_t*) fileName.utf16());
        }
    }

    if (SUCCEEDED(result))
        result = _diaDataSource->openSession(&_diaSession);

    if (SUCCEEDED(result))
        result = _diaSession->get_globalScope(&_diaSymbolGlobal);

    if (FAILED(result))
    {
        if (_diaSymbolGlobal)
        {
            _diaSymbolGlobal->Release();
            _diaSymbolGlobal = NULL;
        }
        if (_diaSession)
        {
            _diaSession->Release();
            _diaSession = NULL;
        }
        if (_diaDataSource)
        {
            _diaDataSource->Release();
            _diaDataSource = NULL;
        }
        FreeLibrary(_library);
        _library = NULL;
        return false;
    }

    readModules();
    //readSourceFiles();
    readTypedefs();

    prependToRecentFiles(fileName);

    setWindowTitle(fileInfo.fileName() + " - UnDebug");

    return true;
}

void MainWindow::closeFile()
{
    _treeModules->clear();
    QDockWidget* dock = qobject_cast<QDockWidget*>(_treeModules->parent());
    if (dock)
        dock->setWindowTitle("Modules");

    _treeObjects->clear();
    dock = qobject_cast<QDockWidget*>(_treeObjects->parent());
    if (dock)
        dock->setWindowTitle("Objects");

    if (_diaSymbolGlobal)
    {
        _diaSymbolGlobal->Release();
        _diaSymbolGlobal = NULL;
    }
    if (_diaSession)
    {
        _diaSession->Release();
        _diaSession = NULL;
    }
    if (_diaDataSource)
    {
        _diaDataSource->Release();
        _diaDataSource = NULL;
    }
    FreeLibrary(_library);
    _library = NULL;

    setWindowTitle("UnDebug");
}

static inline QString recentFilesKey() { return QStringLiteral("recentFileList"); }
static inline QString fileKey() { return QStringLiteral("file"); }

static QStringList readRecentFiles(QSettings &settings)
{
    QStringList result;
    const int count = settings.beginReadArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        result.append(settings.value(fileKey()).toString());
    }
    settings.endArray();
    return result;
}

static void writeRecentFiles(const QStringList &files, QSettings &settings)
{
    const int count = files.size();
    settings.beginWriteArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        settings.setValue(fileKey(), files.at(i));
    }
    settings.endArray();
}

bool MainWindow::hasRecentFiles()
{
    QSettings settings(qApp->applicationDirPath() + "/undebug.ini", QSettings::IniFormat);
    const int count = settings.beginReadArray(recentFilesKey());
    settings.endArray();
    return count > 0;
}

void MainWindow::prependToRecentFiles(const QString &fileName)
{
    QSettings settings(qApp->applicationDirPath() + "/undebug.ini", QSettings::IniFormat);

    const QStringList oldRecentFiles = readRecentFiles(settings);
    QStringList recentFiles = oldRecentFiles;
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    if (oldRecentFiles != recentFiles)
        writeRecentFiles(recentFiles, settings);

    setRecentFilesVisible(!recentFiles.isEmpty());
}

void MainWindow::setRecentFilesVisible(bool visible)
{
    recentFileSubMenuAct->setVisible(visible);
    recentFileSeparator->setVisible(visible);
}

void MainWindow::updateRecentFileActions()
{
    QSettings settings(qApp->applicationDirPath() + "/undebug.ini", QSettings::IniFormat);

    const QStringList recentFiles = readRecentFiles(settings);
    const int count = qMin(int(MaxRecentFiles), recentFiles.size());
    int i = 0;
    for ( ; i < count; ++i) {
        const QString fileName = QFileInfo(recentFiles.at(i)).fileName();
        recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
        recentFileActs[i]->setData(recentFiles.at(i));
        recentFileActs[i]->setVisible(true);
    }
    for ( ; i < MaxRecentFiles; ++i)
        recentFileActs[i]->setVisible(false);
}

void MainWindow::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction *>(sender()))
        openFile(action->data().toString());
}

void MainWindow::save()
{
    if (activeMdiChild() && activeMdiChild()->save())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveAs()
{
    MdiChild *child = activeMdiChild();
    if (child && child->saveAs()) {
        statusBar()->showMessage(tr("File saved"), 2000);
        MainWindow::prependToRecentFiles(child->currentFile());
    }
}

#ifndef QT_NO_CLIPBOARD
void MainWindow::cut()
{
    if (activeMdiChild())
        activeMdiChild()->cut();
}

void MainWindow::copy()
{
    if (activeMdiChild())
        activeMdiChild()->copy();
}

void MainWindow::paste()
{
    if (activeMdiChild())
        activeMdiChild()->paste();
}
#endif

void MainWindow::updateMenus()
{
    bool hasMdiChild = (activeMdiChild() != nullptr);
    saveAct->setEnabled(hasMdiChild);
    saveAsAct->setEnabled(hasMdiChild);
#ifndef QT_NO_CLIPBOARD
    pasteAct->setEnabled(hasMdiChild);
#endif
    closeAct->setEnabled(hasMdiChild);
    closeAllAct->setEnabled(hasMdiChild);
    tileAct->setEnabled(hasMdiChild);
    cascadeAct->setEnabled(hasMdiChild);
    nextAct->setEnabled(hasMdiChild);
    previousAct->setEnabled(hasMdiChild);
    windowMenuSeparatorAct->setVisible(hasMdiChild);

#ifndef QT_NO_CLIPBOARD
    bool hasSelection = (activeMdiChild() &&
                         activeMdiChild()->textCursor().hasSelection());
    cutAct->setEnabled(hasSelection);
    copyAct->setEnabled(hasSelection);
#endif
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(windowMenuSeparatorAct);

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    windowMenuSeparatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
        QMdiSubWindow *mdiSubWindow = windows.at(i);
        MdiChild *child = qobject_cast<MdiChild *>(mdiSubWindow->widget());

        QString text;
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1)
                               .arg(child->userFriendlyCurrentFile());
        } else {
            text = tr("%1 %2").arg(i + 1)
                              .arg(child->userFriendlyCurrentFile());
        }
        QAction *action = windowMenu->addAction(text, mdiSubWindow, [this, mdiSubWindow]() {
            mdiArea->setActiveSubWindow(mdiSubWindow);
        });
        action->setCheckable(true);
        action ->setChecked(child == activeMdiChild());
    }
}

MdiChild *MainWindow::createMdiChild()
{
    MdiChild *child = new MdiChild;
    mdiArea->addSubWindow(child);

#ifndef QT_NO_CLIPBOARD
    connect(child, &QTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
    connect(child, &QTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
#endif

    return child;
}

void MainWindow::createDockedTree(QTreeWidget** widget, const QString& name,
                                  const QStringList& header)
{
    (*widget) = new QTreeWidget();
    (*widget)->setMinimumWidth(250);

    if (!header.isEmpty())
    {
        QTreeWidgetItem* itemHeader = new QTreeWidgetItem();
        for (int i = 0; i < header.size(); ++i)
            itemHeader->setText(i, header.at(i));

        (*widget)->setHeaderItem(itemHeader);
    }

    QDockWidget* dockModules = new QDockWidget(name, this);
    dockModules->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockModules->setWidget(*widget);
    addDockWidget(Qt::LeftDockWidgetArea, dockModules);
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));

    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAct);
    fileToolBar->addAction(openAct);

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    fileToolBar->addAction(saveAct);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    saveAsAct = new QAction(saveAsIcon, tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);
    fileMenu->addAction(saveAsAct);

    fileMenu->addSeparator();

    QMenu *recentMenu = fileMenu->addMenu(tr("Recent..."));
    connect(recentMenu, &QMenu::aboutToShow, this, &MainWindow::updateRecentFileActions);
    recentFileSubMenuAct = recentMenu->menuAction();

    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = recentMenu->addAction(QString(), this, &MainWindow::openRecentFile);
        recentFileActs[i]->setVisible(false);
    }

    recentFileSeparator = fileMenu->addSeparator();

    setRecentFilesVisible(MainWindow::hasRecentFiles());

    fileMenu->addSeparator();

//! [0]
    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), qApp, &QApplication::closeAllWindows);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    fileMenu->addAction(exitAct);
//! [0]

#ifndef QT_NO_CLIPBOARD
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QToolBar *editToolBar = addToolBar(tr("Edit"));

    const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
    cutAct = new QAction(cutIcon, tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, &QAction::triggered, this, &MainWindow::cut);
    editMenu->addAction(cutAct);
    editToolBar->addAction(cutAct);

    const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
    copyAct = new QAction(copyIcon, tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, &QAction::triggered, this, &MainWindow::copy);
    editMenu->addAction(copyAct);
    editToolBar->addAction(copyAct);

    const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
    pasteAct = new QAction(pasteIcon, tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, &QAction::triggered, this, &MainWindow::paste);
    editMenu->addAction(pasteAct);
    editToolBar->addAction(pasteAct);
#endif

    windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, &QAction::triggered,
            mdiArea, &QMdiArea::closeActiveSubWindow);

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, &QAction::triggered, mdiArea, &QMdiArea::closeAllSubWindows);

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, &QAction::triggered, mdiArea, &QMdiArea::tileSubWindows);

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, &QAction::triggered, mdiArea, &QMdiArea::cascadeSubWindows);

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, &QAction::triggered, mdiArea, &QMdiArea::activateNextSubWindow);

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("Move the focus to the previous "
                                 "window"));
    connect(previousAct, &QAction::triggered, mdiArea, &QMdiArea::activatePreviousSubWindow);

    windowMenuSeparatorAct = new QAction(this);
    windowMenuSeparatorAct->setSeparator(true);

    updateWindowMenu();
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings(qApp->applicationDirPath() + "/undebug.ini", QSettings::IniFormat);
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = screen()->availableGeometry();
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings(qApp->applicationDirPath() + "/undebug.ini", QSettings::IniFormat);
    settings.setValue("geometry", saveGeometry());
}

MdiChild *MainWindow::activeMdiChild() const
{
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<MdiChild *>(activeSubWindow->widget());
    return nullptr;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName) const
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
    for (QMdiSubWindow *window : subWindows) {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
        if (mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return nullptr;
}

void MainWindow::readModules()
{
    /*
    QTreeWidgetItem* roots[SymTagMax] = { nullptr };
    DWORD tag;

    QVector<IDiaSymbol*> symbols = QDIA::findChildren(_diaSymbolGlobal, SymTagNull);
    for (int i = 0; i < symbols.size(); ++i)
    {
        IDiaSymbol* symbol = symbols.at(i);
        if (FAILED(symbol->get_symTag(&tag)))
            return;

        if (tag >= SymTagMax)
        {
            qDebug() << "Invalid tag value" << tag;
            continue;
        }

        if (!roots[tag])
        {
            roots[tag] = new QTreeWidgetItem(_treeTest);
            roots[tag]->setText(0, QDIA::getSymbolType(symbol));
            roots[tag]->setIcon(0, QIcon(":/images/folder.png"));
        }

        QString name = QDIA::getName(symbol);
        if (name.isEmpty())
            name = QStringLiteral("(empty)");

        QTreeWidgetItem* item = new QTreeWidgetItem(roots[tag]);
        item->setText(0, name);
    }
    */


    int objCounter = 0;

    QVector<IDiaSymbol*> compilands = QDIA::findChildren(_diaSymbolGlobal, SymTagCompiland);
    for (int i = 0; i < compilands.size(); ++i)
    {
        addModule(compilands.at(i));
        if (addObject(compilands.at(i)))
            objCounter++;
    }

    _treeModules->resizeColumnToContents(0);
    _treeObjects->resizeColumnToContents(0);
    QDockWidget* dock = qobject_cast<QDockWidget*>(_treeModules->parent());
    if (dock)
        dock->setWindowTitle(QStringLiteral("Modules (%1)").arg(_treeModules->topLevelItemCount()));

    dock = qobject_cast<QDockWidget*>(_treeObjects->parent());
    if (dock)
        dock->setWindowTitle(QStringLiteral("Objects (%1)").arg(objCounter));
}

void MainWindow::readSourceFiles()
{
    QMap<QString, QList<Path>> map;

    QVector<IDiaSymbol*> compilands = QDIA::findChildren(_diaSymbolGlobal, SymTagCompiland);
    for (int i = 0; i < compilands.size(); ++i)
    {
        QVector<IDiaSourceFile*> files = QDIA::findSourceFiles(_diaSession, compilands.at(i));
        for (int j = 0; j < files.size(); ++j)
        {
            IDiaSourceFile* file = files.at(j);
            QString fileName = QDIA::getFileName(file);
            Path path(fileName);
            QList<Path>& plist = map[path.fileName().toLower()];
            bool found = false;
            for (int k = 0; k < plist.size(); ++k)
            {
                if (plist.at(k).compare(path) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                plist << path;
        }
    }

    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        const QList<Path>& plist = it.value();

        if (plist.size() < 2)
            continue;

        QString str = it.key() + ": [";
        for (int i = 0; i < plist.size(); ++i)
            str += plist.at(i).path() + "; ";

        str += ']';
        qDebug() << str;
    }
}

void MainWindow::readTypedefs()
{
    QVector<IDiaSymbol*> typedefs = QDIA::findChildren(_diaSymbolGlobal, SymTagTypedef);
    for (int i = 0; i < typedefs.size(); ++i)
    {
        addTypedef(typedefs.at(i), nullptr);
    }
}

void MainWindow::addModule(IDiaSymbol* compiland)
{
    QString path = QDIA::getName(compiland);
    QString libraryPath = QDIA::getLibraryName(compiland);
    QString realPath = QDIA::getEnvPath(compiland);

    QString name;
    QString libraryName;

    int slash = qMax(path.lastIndexOf('\\'), path.lastIndexOf('/'));
    name = (slash >= 0) ? path.mid(slash + 1) : path;
    slash = qMax(libraryPath.lastIndexOf('\\'), libraryPath.lastIndexOf('/'));
    libraryName = (slash >= 0) ? libraryPath.mid(slash + 1) : libraryPath;

    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    bool isLibrary = libraryName.endsWith(".lib", cs);

    if (!isLibrary)
        libraryName = QLatin1String("Executable");

    QTreeWidgetItem* rootItem = nullptr;
    for (int i = 0; i < _treeModules->topLevelItemCount(); ++i)
    {
        if (_treeModules->topLevelItem(i)->text(0).compare(libraryName, cs) == 0)
        {
            rootItem = _treeModules->topLevelItem(i);
            break;
        }
    }

    if (!rootItem)
    {
        rootItem = new QTreeWidgetItem();
        rootItem->setText(0, libraryName);
        if (isLibrary)
        {
            rootItem->setIcon(0, QIcon(":/images/books_stack.png"));
            rootItem->setText(1, libraryPath);
            rootItem->setToolTip(1, rootItem->text(1));
            _treeModules->addTopLevelItem(rootItem);
        }
        else
        {
            rootItem->setIcon(0, QIcon(":/images/file_extension_exe.png"));
            _treeModules->insertTopLevelItem(0, rootItem);
        }
    }

    QTreeWidgetItem* item = new QTreeWidgetItem(rootItem);
    item->setText(0, name);
    item->setText(1, realPath.isEmpty() ? path : realPath);
    item->setToolTip(1, item->text(1));


    if (name.endsWith(".res", cs))
    {
        item->setIcon(0, QIcon(":/images/resources.png"));
    }
    else if (name.endsWith(".dll", cs))
    {
        item->setIcon(0, QIcon(":/images/file_extension_dll.png"));
    }
    else if (name.endsWith(".obj", cs))
    {
        item->setIcon(0, QIcon(":/images/module.png"));
    }
    else
    {
        item->setIcon(0, QIcon(":/images/page_error.png"));
    }
}

bool MainWindow::addObject(IDiaSymbol* compiland)
{
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    QString path = QDIA::getName(compiland);
    if (!path.endsWith(".obj", cs))
        return false;

    QString realPath = QDIA::getEnvPath(compiland);
    QString libraryPath = QDIA::getLibraryName(compiland);

    QString name;
    QString libraryName;

    int slash = qMax(path.lastIndexOf('\\'), path.lastIndexOf('/'));
    name = (slash >= 0) ? path.mid(slash + 1) : path;
    slash = qMax(libraryPath.lastIndexOf('\\'), libraryPath.lastIndexOf('/'));
    libraryName = (slash >= 0) ? libraryPath.mid(slash + 1) : libraryPath;

    //bool isLibrary = libraryName.endsWith(".lib", cs);

    Path pathTree = realPath.isEmpty() ? ("UNRESOLVED/" + path) : realPath;

    QTreeWidgetItem* root = nullptr;

    for (int i = 0; i < pathTree.size() - 1; ++i)
    {
        QString element = pathTree.at(i);

        if (i == 0)
        {
            int sorted = _treeObjects->topLevelItemCount();

            for (int j = 0; j < _treeObjects->topLevelItemCount(); ++j)
            {
                int check = element.compare(_treeObjects->topLevelItem(j)->text(0), cs);

                if (check > 0)
                {
                    sorted = j;
                }
                else if (check == 0)
                {
                    root = _treeObjects->topLevelItem(j);
                    break;
                }
            }
            if (!root)
            {
                root = new QTreeWidgetItem();
                root->setText(0, element);
                root->setIcon(0, QIcon(":/images/drive.png"));
                _treeObjects->insertTopLevelItem(sorted, root);
            }
            continue;
        }

        if (!root)
            return false;

        bool found = false;

        for (int j = 0; j < root->childCount(); ++j)
        {
            if (element.compare(root->child(j)->text(0), cs) == 0)
            {
                root = root->child(j);
                found = true;
                break;
            }
        }

        if (!found)
        {
            root = new QTreeWidgetItem(root);
            root->setText(0, element);
            root->setIcon(0, QIcon(":/images/folder.png"));
        }
    }

    QTreeWidgetItem* objectItem = new QTreeWidgetItem(root);
    objectItem->setText(0, name);
    objectItem->setText(1, realPath.isEmpty() ? path : realPath);
    objectItem->setToolTip(1, objectItem->text(1));
    objectItem->setIcon(0, QIcon(":/images/module.png"));

    addSymbols(compiland, objectItem);

    return true;
}

void MainWindow::addSymbols(IDiaSymbol* compiland, QTreeWidgetItem* parent)
{
    QTreeWidgetItem* typedefsItem = new QTreeWidgetItem();
    //addTypedef(compiland, typedefsItem);
    if (typedefsItem->childCount() > 0)
    {
        parent->addChild(typedefsItem);
        typedefsItem->setText(0, QStringLiteral("Typedefs (%1)").arg(typedefsItem->childCount()));
        typedefsItem->setIcon(0, QIcon(":/images/database_green.png"));
    }
    else
    {
        delete typedefsItem;
    }

    QTreeWidgetItem* functionsItem = new QTreeWidgetItem();
    addSymbolFunctions(compiland, functionsItem);
    if (functionsItem->childCount() > 0)
    {
        parent->addChild(functionsItem);
        functionsItem->setText(0, QStringLiteral("Functions (%1)").arg(functionsItem->childCount()));
        functionsItem->setIcon(0, QIcon(":/images/math_functions.png"));
    }
    else
    {
        delete functionsItem;
    }
}

void MainWindow::addTypedef(IDiaSymbol* symbol, QTreeWidgetItem* parent)
{
    QString name = QDIA::getName(symbol);
    QString type = QDIA::getTypeOfTypedef(symbol);

    QTreeWidgetItem* item = nullptr;
    if (parent)
    {
        parent->addChild(item);
    }
    else
    {
        int position = _treeTypedefs->topLevelItemCount();

        for (int i = 0; i < _treeTypedefs->topLevelItemCount(); ++i)
        {
            item = _treeTypedefs->topLevelItem(i);

            if (name > item->text(1))
                continue;

            if (name == item->text(1) && type == item->text(0))
                return;

            position = i;
            break;
        }

        item = new QTreeWidgetItem();
        _treeTypedefs->insertTopLevelItem(position, item);
    }
    item->setText(0, type);
    item->setText(1, name);
    item->setIcon(0, QIcon(":/images/token_lookaround.png"));

}

void MainWindow::addSymbolFunctions(IDiaSymbol* compiland, QTreeWidgetItem* parent)
{
    auto functions = QDIA::findChildren(compiland, SymTagFunction);
    for (int i = 0; i < functions.count(); ++i)
    {
        IDiaSymbol* function = functions.at(i);
        QTreeWidgetItem* functionItem = new QTreeWidgetItem(parent);
        functionItem->setText(0, QDIA::getUndName(function));
        functionItem->setIcon(0, QIcon(":/images/function.png"));
    }
}

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "qdia.h"

class MdiChild;
class Path;

class QAction;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QTreeWidget;
class QTreeWidgetItem;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    bool openFile(const QString &fileName);
    void closeFile();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void open();
    void save();
    void saveAs();
    void updateRecentFileActions();
    void openRecentFile();
    void cut();
    void copy();
    void paste();
    void updateMenus();
    void updateWindowMenu();
    MdiChild *createMdiChild();

private:
    enum { MaxRecentFiles = 5 };

    void createDockedTree(QTreeWidget** widget, const QString& name,
                          const QStringList& header = QStringList());
    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    static bool hasRecentFiles();
    void prependToRecentFiles(const QString &fileName);
    void setRecentFilesVisible(bool visible);
    MdiChild *activeMdiChild() const;
    QMdiSubWindow *findMdiChild(const QString &fileName) const;

    void readModules();
    void readSourceFiles();
    void readTypedefs();
    void readEnums();
    void readUserTypes();
    void addModule(IDiaSymbol* compiland);
    bool addObject(IDiaSymbol* compiland);
    void addSymbols(IDiaSymbol* compiland, QTreeWidgetItem* parent);
    void addTypedef(IDiaSymbol* symbol, QTreeWidgetItem* parent);
    void addEnum(IDiaSymbol* symbol, QTreeWidgetItem* parent);
    void addUserType(IDiaSymbol* symbol, QTreeWidgetItem* parent);
    void addSymbolFunctions(IDiaSymbol* compiland, QTreeWidgetItem* parent);

private:
    QMdiArea *mdiArea;

    QMenu *windowMenu;
    QAction *newAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *recentFileActs[MaxRecentFiles];
    QAction *recentFileSeparator;
    QAction *recentFileSubMenuAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *closeAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *windowMenuSeparatorAct;

    QTreeWidget* _treeModules;
    QTreeWidget* _treeObjects;
    QTreeWidget* _treeSources;
    QTreeWidget* _treeTest;
    QTreeWidget* _treeTypedefs;
    QTreeWidget* _treeEnums;
    QTreeWidget* _treeUserTypes;

private:
    HMODULE _library;
    IDiaDataSource* _diaDataSource;
    IDiaSession* _diaSession;
    IDiaSymbol* _diaSymbolGlobal;
};

#endif

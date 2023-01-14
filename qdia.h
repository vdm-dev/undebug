#ifndef QDIA_H
#define QDIA_H


#define WIN32_LEAN_AND_MEAN
#include <dia2.h>
#include <atlbase.h>
#include <atlcomcli.h>

#include <QString>
#include <QVector>
#include <QVariant>


class QDIA
{
public:
    static QVector<IDiaSymbol*> findChildren(IDiaSymbol* parent, enum SymTagEnum symtag, const QString& name = QString(), DWORD compareFlags = nsNone);
    static QVector<IDiaSourceFile*> findSourceFiles(IDiaSession* session, IDiaSymbol* parent);
    static QString getFileName(IDiaSourceFile* sourceFile);
    static QString getName(IDiaSymbol* symbol);
    static QString getLibraryName(IDiaSymbol* symbol);
    static QVariant getValue(IDiaSymbol* symbol);
    static QString getEnvPath(IDiaSymbol* symbol);
    static QString getUndName(IDiaSymbol* symbol);
    static QString getTypeString(IDiaSymbol* symbol);
    static QString getTypeOfTypedef(IDiaSymbol* symbol);
    static QString getNameOfBasicType(IDiaSymbol* baseType);
    static QString getNameOfPointerType(IDiaSymbol* pointerType);
    static QString getSymbolTag(IDiaSymbol* symbol);
};

#endif // QDIA_H

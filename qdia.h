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
    static QString getName(IDiaSymbol* symbol);
    static QString getLibraryName(IDiaSymbol* symbol);
    static QVariant getValue(IDiaSymbol* symbol);
    static QString getEnvPath(IDiaSymbol* symbol);
};

#endif // QDIA_H

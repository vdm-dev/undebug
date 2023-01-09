#include "qdia.h"

QVector<IDiaSymbol*> QDIA::findChildren(IDiaSymbol* parent, enum SymTagEnum symtag, const QString& name, DWORD compareFlags)
{
    QVector<IDiaSymbol*> result;

    if (!parent)
        return result;

    CComPtr<IDiaEnumSymbols> enumerator;

    if (FAILED(parent->findChildren(symtag, name.isEmpty() ? NULL : LPOLESTR(name.utf16()), compareFlags, &enumerator)))
        return result;

    LONG count = 0;
    if (FAILED(enumerator->get_Count(&count)))
        return result;

    result.resize(count);

    ULONG retrieved = 0;
    if (FAILED(enumerator->Next(count, result.data(), &retrieved)) || retrieved != count)
        result.clear();

    return result;
}

QString QDIA::getName(IDiaSymbol* symbol)
{
    QString result;

    if (!symbol)
        return result;

    CComBSTR string;
    if (SUCCEEDED(symbol->get_name(&string)))
        result = QString::fromWCharArray(BSTR(string), string.Length());

    return result;
}

QString QDIA::getLibraryName(IDiaSymbol* symbol)
{
    QString result;

    if (!symbol)
        return result;

    CComBSTR string;
    if (SUCCEEDED(symbol->get_libraryName(&string)))
        result = QString::fromWCharArray(BSTR(string), string.Length());

    return result;
}

QVariant QDIA::getValue(IDiaSymbol* symbol)
{
    QVariant result;
    if (!symbol)
        return result;

    VARIANT value;
    VariantInit(&value);

    if (FAILED(symbol->get_value(&value)))
    {
        VariantClear(&value);
        return result;
    }

    switch (value.vt)
    {
    case VT_BSTR:
        result = QString::fromWCharArray(value.bstrVal, ::SysStringLen(value.bstrVal));
        break;
    case VT_I2:
        result = value.iVal;
        break;
    case VT_I4:
        result = int(value.lVal);
        break;
    case VT_R4:
        result = value.fltVal;
        break;
    case VT_R8:
        result = value.dblVal;
        break;
    case VT_BOOL:
        result = value.boolVal ? true : false;
        break;
    case VT_I1:
        result = value.cVal;
        break;
    case VT_UI1:
        result = value.bVal;
        break;
    case VT_UI2:
        result = value.uiVal;
        break;
    case VT_UI4:
        result = uint(value.ulVal);
        break;
    case VT_I8:
        result = value.llVal;
        break;
    case VT_UI8:
        result = value.ullVal;
        break;
    default:
        break;
    }

    VariantClear(&value);
    return result;
}

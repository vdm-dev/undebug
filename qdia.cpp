#include "qdia.h"

#include <QDebug>

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

QVector<IDiaSourceFile*> QDIA::findSourceFiles(IDiaSession* session, IDiaSymbol* parent)
{
    QVector<IDiaSourceFile*> result;
    if (!session || !parent)
        return result;

    CComPtr<IDiaEnumSourceFiles> enumerator;

    if (FAILED(session->findFile(parent, NULL, nsNone, &enumerator)))
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

QString QDIA::getFileName(IDiaSourceFile* sourceFile)
{
    QString result;

    if (!sourceFile)
        return result;

    CComBSTR string;
    if (SUCCEEDED(sourceFile->get_fileName(&string)))
        result = QString::fromWCharArray(BSTR(string), string.Length());

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

QString QDIA::getEnvPath(IDiaSymbol* symbol)
{
    QVector<IDiaSymbol*> objects = QDIA::findChildren(symbol, SymTagCompilandEnv);

    for (int i = 0; i < objects.size(); ++i)
    {
        if (QDIA::getName(objects[i]) == QLatin1String("obj"))
            return QDIA::getValue(objects[i]).toString();
    }

    return QString();
}

QString QDIA::getUndName(IDiaSymbol* symbol)
{
    QString result;

    if (!symbol)
        return result;

    CComBSTR string;
    if (SUCCEEDED(symbol->get_undecoratedName(&string)))
    {
        result = QString::fromWCharArray(BSTR(string), string.Length());
    }
    else
    {
        result = getName(symbol);
    }

    return result;
}

QString QDIA::getTypeInformation(IDiaSymbol* symbol)
{
    CComPtr<IDiaSymbol> typeSymbol = nullptr;
    DWORD tag;

    if (!symbol || FAILED(symbol->get_type(&typeSymbol)) || !typeSymbol ||
        FAILED(typeSymbol->get_symTag(&tag)))
    {
        return QString();
    }

    QString result;
    QString name = getName(typeSymbol);

    if (tag != SymTagPointerType)
    {
        BOOL flag;
        if (SUCCEEDED(typeSymbol->get_constType(&flag)) && flag)
            result += QStringLiteral("const ");
        if (SUCCEEDED(typeSymbol->get_volatileType(&flag)) && flag)
            result += QStringLiteral("volatile ");
        if (SUCCEEDED(typeSymbol->get_unalignedType(&flag)) && flag)
            result += QStringLiteral("__unaligned ");
    }

    switch (tag)
    {
    case SymTagBaseType:
        result += getNameOfBasicType(typeSymbol);
        break;
    case SymTagPointerType:
        result += getNameOfPointerType(typeSymbol);
        break;
    case SymTagTypedef:
        result += name;
        break;
    case SymTagEnum:
        result += QStringLiteral("enum ") + (name.isEmpty() ? QStringLiteral("<unnamed>") : name);
        break;
    case SymTagFunctionType:
        result += QStringLiteral("<function>");
        break;
    case SymTagUDT:
        result += getNameOfUserType(typeSymbol);
        break;
    default:
        qDebug() << "Unhandled Type Tag:" << getSymbolTag(symbol);
        result.clear();
        break;
    }

    return result;
}

QString QDIA::getNameOfBasicType(IDiaSymbol* baseType)
{
    DWORD type;
    ULONGLONG length;
    if (FAILED(baseType->get_baseType(&type)) || FAILED(baseType->get_length(&length)))
        return QString();

    switch (type)
    {
    case btNoType:
        return QStringLiteral("<no type>");
    case btVoid:
        return QStringLiteral("void");
    case btChar:
        return QStringLiteral("char");
    case btWChar:
        return QStringLiteral("wchar_t");
    case btInt:
        switch (length)
        {
        case 1:
            return QStringLiteral("signed char");
        case 2:
            return QStringLiteral("short");
        case 4:
            return QStringLiteral("int");
        case 8:
            return QStringLiteral("__int64");
        }
        return QStringLiteral("<unknown int>");
    case btUInt:
        switch (length)
        {
        case 1:
            return QStringLiteral("unsigned char");
        case 2:
            return QStringLiteral("unsigned short");
        case 4:
            return QStringLiteral("unsigned int");
        case 8:
            return QStringLiteral("unsigned __int64");
        }
        return QStringLiteral("<unknown unsigned int>");
    case btFloat:
        switch (length)
        {
        case 4:
            return QStringLiteral("float");
        case 8:
            return QStringLiteral("double");
        }
        return QStringLiteral("<unknown float>");
    case btBCD:
        return QStringLiteral("<BCD>");
    case btBool:
        return QStringLiteral("bool");
    case btLong:
        return QStringLiteral("long");
    case btULong:
        return QStringLiteral("unsigned long");
    case btCurrency:
        return QStringLiteral("<currency>");
    case btDate:
        return QStringLiteral("<date>");
    case btVariant:
        return QStringLiteral("VARIANT");
    case btComplex:
        return QStringLiteral("<complex>");
    case btBit:
        return QStringLiteral("<bit>");
    case btBSTR:
        return QStringLiteral("BSTR");
    case btHresult:
        return QStringLiteral("HRESULT");
    case btChar16:
        return QStringLiteral("char16_t");
    case btChar32:
        return QStringLiteral("char32_t");
    case btChar8:
        return QStringLiteral("char8_t");
    default:
        return QString();
    }
}

QString QDIA::getNameOfPointerType(IDiaSymbol* pointerType)
{
    QString result = getTypeInformation(pointerType);
    if (result.isEmpty())
        return QString();

    BOOL flag;
    if (SUCCEEDED(pointerType->get_reference(&flag)) && flag)
    {
        result += " &";
    }
    else
    {
        result += " *";
    }

    if (SUCCEEDED(pointerType->get_constType(&flag)) && flag)
        result += " const";
    if (SUCCEEDED(pointerType->get_volatileType(&flag)) && flag)
        result += " volatile";
    if (SUCCEEDED(pointerType->get_unalignedType(&flag)) && flag)
        result += " __unaligned";

    return result;
}

QString QDIA::getNameOfFunctionType(IDiaSymbol* functionType)
{
    CComPtr<IDiaSymbol> classParent;
    functionType->get_classParent(&classParent);

    CComPtr<IDiaSymbol> lexParent;
    functionType->get_lexicalParent(&lexParent);

    qDebug() << "getNameOfFunctionType" << getName(classParent) << getName(lexParent);

    QString returnType = getTypeInformation(functionType);

    return returnType + " function " + getName(functionType);
}

QString QDIA::getNameOfUserType(IDiaSymbol* userType)
{
    DWORD kind = 0;
    if (FAILED(userType->get_udtKind(&kind)))
        return QString();

    QString name = getName(userType);

    switch (kind)
    {
    case UdtStruct:
        return QStringLiteral("struct ") + name;
    case UdtClass:
        return QStringLiteral("class ") + name;
    case UdtUnion:
        return QStringLiteral("union ") + name;
    case UdtInterface:
        return QStringLiteral("interface ") + name;
    default:
        return QStringLiteral("<no kind> ") + name;
    }
}

QString QDIA::getSymbolTag(IDiaSymbol* symbol)
{
    if (!symbol)
        return QString();

    DWORD tag;
    if (FAILED(symbol->get_symTag(&tag)))
        return QString();

    switch (tag)
    {
    case SymTagNull:
        return QStringLiteral("SymTagNull");

    case SymTagExe:
        return QStringLiteral("SymTagExe");

    case SymTagCompiland:
        return QStringLiteral("SymTagCompiland");

    case SymTagCompilandDetails:
        return QStringLiteral("SymTagCompilandDetails");

    case SymTagCompilandEnv:
        return QStringLiteral("SymTagCompilandEnv");

    case SymTagFunction:
        return QStringLiteral("SymTagFunction");

    case SymTagBlock:
        return QStringLiteral("SymTagBlock");

    case SymTagData:
        return QStringLiteral("SymTagData");

    case SymTagAnnotation:
        return QStringLiteral("SymTagAnnotation");

    case SymTagLabel:
        return QStringLiteral("SymTagLabel");

    case SymTagPublicSymbol:
        return QStringLiteral("SymTagPublicSymbol");

    case SymTagUDT:
        return QStringLiteral("SymTagUDT");

    case SymTagEnum:
        return QStringLiteral("SymTagEnum");

    case SymTagFunctionType:
        return QStringLiteral("SymTagFunctionType");

    case SymTagPointerType:
        return QStringLiteral("SymTagPointerType");

    case SymTagArrayType:
        return QStringLiteral("SymTagArrayType");

    case SymTagBaseType:
        return QStringLiteral("SymTagBaseType");

    case SymTagTypedef:
        return QStringLiteral("SymTagTypedef");

    case SymTagBaseClass:
        return QStringLiteral("SymTagBaseClass");

    case SymTagFriend:
        return QStringLiteral("SymTagFriend");

    case SymTagFunctionArgType:
        return QStringLiteral("SymTagFunctionArgType");

    case SymTagFuncDebugStart:
        return QStringLiteral("SymTagFuncDebugStart");

    case SymTagFuncDebugEnd:
        return QStringLiteral("SymTagFuncDebugEnd");

    case SymTagUsingNamespace:
        return QStringLiteral("SymTagUsingNamespace");

    case SymTagVTableShape:
        return QStringLiteral("SymTagVTableShape");

    case SymTagVTable:
        return QStringLiteral("SymTagVTable");

    case SymTagCustom:
        return QStringLiteral("SymTagVTable");

    case SymTagThunk:
        return QStringLiteral("SymTagThunk");

    case SymTagCustomType:
        return QStringLiteral("SymTagCustomType");

    case SymTagManagedType:
        return QStringLiteral("SymTagManagedType");

    case SymTagDimension:
        return QStringLiteral("SymTagDimension");

    case SymTagCallSite:
        return QStringLiteral("SymTagCallSite");

    case SymTagInlineSite:
        return QStringLiteral("SymTagInlineSite");

    case SymTagBaseInterface:
        return QStringLiteral("SymTagBaseInterface");

    case SymTagVectorType:
        return QStringLiteral("SymTagVectorType");

    case SymTagMatrixType:
        return QStringLiteral("SymTagMatrixType");

    case SymTagHLSLType:
        return QStringLiteral("SymTagHLSLType");

    case SymTagCaller:
        return QStringLiteral("SymTagCaller");

    case SymTagCallee:
        return QStringLiteral("SymTagCallee");

    case SymTagExport:
        return QStringLiteral("SymTagExport");

    case SymTagHeapAllocationSite:
        return QStringLiteral("SymTagHeapAllocationSite");

    case SymTagCoffGroup:
        return QStringLiteral("SymTagCoffGroup");

    case SymTagInlinee:
        return QStringLiteral("SymTagInlinee");

    case SymTagTaggedUnionCase:
        return QStringLiteral("SymTagTaggedUnionCase");

    case SymTagMax:
        return QStringLiteral("SymTagMax");

    default:
        return QString();
    }
}

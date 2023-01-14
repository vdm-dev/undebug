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

QString QDIA::getTypeString(IDiaSymbol* symbol)
{
    IDiaSymbol *pBaseType;
    IDiaEnumSymbols *pEnumSym;
    IDiaSymbol *pSym;
    DWORD dwTag;
    DWORD dwInfo;
    BOOL bSet;
    DWORD dwRank;
    LONG lCount = 0;
    ULONG celt = 1;

    QString result;

    if (FAILED(symbol->get_symTag(&dwTag)))
        return result;

    QString bstrName = getName(symbol);

    if (dwTag != SymTagPointerType)
    {
        if ((symbol->get_constType(&bSet) == S_OK) && bSet)
            result += "const ";

        if ((symbol->get_volatileType(&bSet) == S_OK) && bSet)
            result += "volatile ";

        if ((symbol->get_unalignedType(&bSet) == S_OK) && bSet)
            result += "__unaligned ";
    }

    ULONGLONG ulLen;

    symbol->get_length(&ulLen);

    switch (dwTag) {
      case SymTagUDT:
        //PrintUdtKind(symbol);
        //PrintName(symbol);
        break;

      case SymTagEnum:
        result += "enum ";
        result += getUndName(symbol);
        break;

      case SymTagFunctionType:
        result += "function ";
        break;

      case SymTagPointerType:
        if (FAILED(symbol->get_type(&pBaseType)))
          return result;

        result += getTypeString(pBaseType);
        pBaseType->Release();

        if ((symbol->get_reference(&bSet) == S_OK) && bSet) {
          result += " &";
        }

        else {
          result += " *";
        }

        if ((symbol->get_constType(&bSet) == S_OK) && bSet) {
          result += " const";
        }

        if ((symbol->get_volatileType(&bSet) == S_OK) && bSet) {
          result += " volatile";
        }

        if ((symbol->get_unalignedType(&bSet) == S_OK) && bSet) {
          result += " __unaligned";
        }
        break;

      case SymTagArrayType:
        break;

      case SymTagBaseType:
        if (symbol->get_baseType(&dwInfo) != S_OK) {
          return result;
        }

        switch (dwInfo) {
          case btUInt :
            result += "unsigned ";

          // Fall through

          case btInt :
            switch (ulLen) {
              case 1:
                if (dwInfo == btInt) {
                  result += "signed ";
                }

                result += "char";
                break;

              case 2:
                result += "short";
                break;

              case 4:
                result += "int";
                break;

              case 8:
                result += "__int64";
                break;
            }

            dwInfo = 0xFFFFFFFF;
            break;

          case btFloat :
            switch (ulLen) {
              case 4:
                result += "float";
                break;

              case 8:
                result += "double";
                break;
            }

            dwInfo = 0xFFFFFFFF;
            break;
        }

        if (dwInfo == 0xFFFFFFFF) {
           break;
        }

        //wprintf(L"%s", rgBaseType[dwInfo]);
        break;

      case SymTagTypedef:
        result += bstrName;
        break;

      case SymTagCustomType:
        {
          DWORD idOEM, idOEMSym;
          DWORD cbData = 0;
          DWORD count;

          if (symbol->get_oemId(&idOEM) == S_OK) {
            wprintf(L"OEMId = %X, ", idOEM);
          }

          if (symbol->get_oemSymbolId(&idOEMSym) == S_OK) {
            wprintf(L"SymbolId = %X, ", idOEMSym);
          }

          if (symbol->get_types(0, &count, NULL) == S_OK) {
            IDiaSymbol** rgpDiaSymbols = (IDiaSymbol**) _alloca(sizeof(IDiaSymbol *) * count);

            if (symbol->get_types(count, &count, rgpDiaSymbols) == S_OK) {
              for (ULONG i = 0; i < count; i++) {
                result += getTypeString(rgpDiaSymbols[i]);
                rgpDiaSymbols[i]->Release();
              }
            }
          }

          // print custom data

          if ((symbol->get_dataBytes(cbData, &cbData, NULL) == S_OK) && (cbData != 0)) {
            wprintf(L", Data: ");

            BYTE *pbData = new BYTE[cbData];

            symbol->get_dataBytes(cbData, &cbData, pbData);

            for (ULONG i = 0; i < cbData; i++) {
              wprintf(L"0x%02X ", pbData[i]);
            }

            delete [] pbData;
          }
        }
        break;

      case SymTagData: // This really is member data, just print its location
        break;
    }
    return result;
}

QString QDIA::getTypeOfTypedef(IDiaSymbol* symbol)
{
    CComPtr<IDiaSymbol> typeSymbol = nullptr;
    DWORD tag;

    if (!symbol || FAILED(symbol->get_type(&typeSymbol)) || !typeSymbol ||
        FAILED(typeSymbol->get_symTag(&tag)))
    {
        return QString();
    }

    QString result;

    if (tag != SymTagPointerType)
    {
        BOOL flag;
        if (SUCCEEDED(typeSymbol->get_constType(&flag)) && flag)
            result += "const ";
        if (SUCCEEDED(typeSymbol->get_volatileType(&flag)) && flag)
            result += "volatile ";
        if (SUCCEEDED(typeSymbol->get_unalignedType(&flag)) && flag)
            result += "__unaligned ";
    }

    switch (tag)
    {
    case SymTagBaseType:
        result += getNameOfBasicType(typeSymbol);
    case SymTagPointerType:
        result += getNameOfPointerType(typeSymbol);
    case SymTagTypedef:
    default:
        result += getName(typeSymbol);
        break;
    }

    return result;
}

QString QDIA::getNameOfBasicType(IDiaSymbol* baseType)
{
    DWORD type;
    if (FAILED(baseType->get_baseType(&type)))
        return QString();

    switch (type)
    {
    case btNoType:
        qWarning() << "type == btNoType";
        return "(no type)";
    case btVoid:
        return "void";
    case btChar:
        return "char";
    case btWChar:
        return "wchar_t";
    case btInt:
        return "int";
    case btUInt:
        return "unsigned int";
    case btFloat:
        return "flot";
    case btBCD:
        qWarning() << "type == btBCD";
        return "BCD";
    case btBool:
        qWarning() << "type == btBool";
        return "bool";
    case btLong:
        return "long";
    case btULong:
        return "unsigned long";
    case btCurrency:
        qWarning() << "type == btCurrency";
        return "CURRENCY";
    case btDate:
        qWarning() << "type == btDate";
        return "DATE";
    case btVariant:
        qWarning() << "type == btVariant";
        return "VARIANT";
    case btComplex:
        qWarning() << "type == btComplex";
        return "COMPLEX";
    case btBit:
        qWarning() << "type == btBit";
        return "BIT";
    case btBSTR:
        return "BSTR";
    case btHresult:
        return "HRESULT";
    case btChar16:
        qWarning() << "type == btChar16";
        return "char16_t";
    case btChar32:
        qWarning() << "type == btChar32";
        return "char32_t";
    case btChar8:
        qWarning() << "type == btChar8";
        return "char8_t";
    default:
        return QString();
    }
}

QString QDIA::getNameOfPointerType(IDiaSymbol* pointerType)
{
    QString result = getTypeOfTypedef(pointerType);
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

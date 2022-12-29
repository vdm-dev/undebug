#include "path.h"


QStringList Path::s_specialNames =
{
    QStringLiteral("CON"),  QStringLiteral("PRN"),
    QStringLiteral("AUX"),  QStringLiteral("NUL"),
    QStringLiteral("COM0"), QStringLiteral("COM1"),
    QStringLiteral("COM2"), QStringLiteral("COM3"),
    QStringLiteral("COM4"), QStringLiteral("COM5"),
    QStringLiteral("COM6"), QStringLiteral("COM7"),
    QStringLiteral("COM8"), QStringLiteral("COM9"),
    QStringLiteral("LPT0"), QStringLiteral("LPT1"),
    QStringLiteral("LPT2"), QStringLiteral("LPT3"),
    QStringLiteral("LPT4"), QStringLiteral("LPT5"),
    QStringLiteral("LPT6"), QStringLiteral("LPT7"),
    QStringLiteral("LPT8"), QStringLiteral("LPT9")
};

Path::Path(const Path& other)
    : _parts(other._parts)
    , _absolute(other._absolute)
{
}

Path::Path(const QString& path, bool resolve)
{
    setPath(path, resolve);
}

void Path::resolve()
{
    QStringList resolved;
    resolved.reserve(_parts.size());
    for (int i = 0; i < _parts.size(); ++i)
    {
        if (_parts.at(i) == QLatin1String(".."))
        {
            if (resolved.isEmpty())
            {
                _absolute = false;
            }
            else
            {
                resolved.removeLast();
            }
        }
        else if (_parts.at(i) != QLatin1Char('.'))
        {
            resolved.append(_parts.at(i));
        }
    }
    _parts = resolved;
}

inline int Path::validate(int options) const
{
    int result = Path::NoError;
    int level = 0;

    for (int i = 0; i < _parts.size(); ++i)
    {
        if (_parts.at(i) == QLatin1String(".."))
        {
            result |= Path::DotDots;

            if (level < 1)
            {
                if (_absolute)
                    result |= Path::AboveRoot;
            }
            else
            {
                level--;
            }
        }
        else if (_parts.at(i) == QLatin1Char('.'))
        {
            result |= Path::Dots;
        }
        else
        {
            result |= validatePart(_parts.at(i), options);
            level++;
        }
    }

    if ((options & Path::AllowEmptyFileName) == 0)
    {
        if (fileName().isEmpty())
        {
            result |= (EmptyFileName | EmptySuffix);
        }
        else if ((options & Path::AllowEmptySuffix) == 0)
        {
            if (suffix().isEmpty())
                result |= EmptySuffix;
        }
    }

    return result & ~options;
}

int Path::validate(const QString& path, int option)
{
    return Path(path).validate(option);
}

void Path::append(const Path& other, bool resolve)
{
    if (!_parts.isEmpty() && _parts.last().isEmpty())
        _parts.removeLast();

    _parts.append(other._parts);
    if (resolve)
        this->resolve();
}

int Path::compare(const Path& other, int flags) const
{
    Path::Type type = Path::Relative;
    if (flags & Path::CompareWithType)
        type = Path::Original;

    QString left;
    QString right;

    if (flags & Path::CompareResolved)
    {
        left = cleanPath(type);
        right = other.cleanPath(type);
    }
    else
    {
        left = path(type);
        right = other.path(type);
    }

    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    if (flags & Path::CompareSensitive)
        cs = Qt::CaseSensitive;

    return left.compare(right, cs);
}

bool Path::startsWith(const Path& other, int flags) const
{
    Path::Type type = Path::Relative;
    if (flags & Path::CompareWithType)
        type = Path::Original;

    QString left;
    QString right;

    if (flags & Path::CompareResolved)
    {
        left = cleanPath(type);
        right = other.cleanPath(type);
    }
    else
    {
        left = path(type);
        right = other.path(type);
    }

    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    if (flags & Path::CompareSensitive)
        cs = Qt::CaseSensitive;

    return left.startsWith(right, cs);
}

void Path::setPath(const QString& path, bool resolve)
{
    clear();
    if (path.isEmpty())
        return;

    int previousIndex = 0;
    for (int i = 0; i < path.size(); ++i)
    {
        if (path.at(i) != QLatin1Char('/') && path.at(i) != QLatin1Char('\\'))
            continue;

        // Special case: absolute path
        if (i == 0)
            _absolute = true;

        if (previousIndex < i)
        {
            QString part = path.mid(previousIndex, i - previousIndex).trimmed();
            if (!part.isEmpty())
            {
                if (part.size() == 2 && part[0].isLetter() && part[1] == ':')
                {
                    _absolute = true;
                    part.chop(1);
                    part[0] = part[0].toUpper();
                }

                if (resolve)
                {
                    if (part == QLatin1String("..") && !_parts.isEmpty())
                    {
                        _parts.removeLast();
                    }
                    else if (part != QLatin1Char('.'))
                    {
                        _parts.append(part);
                    }
                }
                else
                {
                    _parts.append(part);
                }
            }
        }

        previousIndex = i + 1;
    }

    // Filename
    QString part = path.mid(previousIndex);
    if (resolve)
    {
        if (part == QLatin1String(".."))
        {
            part.clear(); // Make empty filename
            if (!_parts.isEmpty())
                _parts.removeLast();
        }
        else if (part == QLatin1Char('.'))
        {
            part.clear(); // Make empty filename
        }
        _parts.append(part);
    }
    else
    {
        _parts.append(part);
        if (part == QLatin1String("..") || part == QLatin1Char('.'))
            _parts.append(QString()); // Insert empty filename after "." or ".."
    }
}

QString Path::path(Path::Type type) const
{
    if (_parts.isEmpty())
        return (type != Path::Relative && _absolute) ? QString(QLatin1Char('/')) : QString();

    QString result = _parts.join(QLatin1Char('/'));
    if (type == Path::Absolute || (type == Path::Original && _absolute))
    {
        return QLatin1Char('/') + result;
    }
    else
    {
        return result;
    }
}

QString Path::cleanPath(Path::Type type) const
{
    QStringList resolved;
    bool absolute = _absolute;
    resolved.reserve(_parts.size());
    for (int i = 0; i < _parts.size(); ++i)
    {
        if (_parts.at(i) == QLatin1String(".."))
        {
            if (resolved.isEmpty())
            {
                absolute = false;
            }
            else
            {
                resolved.removeLast();
            }
        }
        else if (_parts.at(i) != QLatin1Char('.'))
        {
            resolved.append(_parts.at(i));
        }
    }

    if (resolved.isEmpty())
        return (type != Path::Relative && absolute) ? QString(QLatin1Char('/')) : QString();

    QString result = resolved.join(QLatin1Char('/'));
    if (type == Path::Absolute || (type == Path::Original && absolute))
    {
        return QLatin1Char('/') + result;
    }
    else
    {
        return result;
    }
}

QString Path::parentPath(Path::Type type) const
{
    if (_parts.isEmpty())
        return QString();

    QString result;
    if (type == Path::Absolute || (type == Path::Original && _absolute))
        result = QLatin1Char('/');

    for (int i = 0; i < _parts.size() - 1; ++i)
    {
        if (_parts.at(i).isEmpty())
            result += QLatin1Char('/') + _parts.at(i);
    }
    return result;
}

QString Path::cleanParentPath(Path::Type type) const
{
    QStringList resolved;
    bool absolute = _absolute;
    resolved.reserve(_parts.size());
    for (int i = 0; i < _parts.size(); ++i)
    {
        if (_parts.at(i) == QLatin1String(".."))
        {
            if (resolved.isEmpty())
            {
                absolute = false;
            }
            else
            {
                resolved.removeLast();
            }
        }
        else if (_parts.at(i) != QLatin1Char('.'))
        {
            resolved.append(_parts.at(i));
        }
    }

    if (resolved.isEmpty())
        return QString();

    QString result;
    if (type == Path::Absolute || (type == Path::Original && absolute))
        result = QLatin1Char('/');

    for (int i = 0; i < resolved.size() - 1; ++i)
    {
        if (resolved.at(i).isEmpty())
            result += QLatin1Char('/') + resolved.at(i);
    }
    return result;

}

QString Path::fileName() const
{
    return _parts.isEmpty() ? QString() : _parts.last();
}

QString Path::baseName() const
{
    if (_parts.isEmpty())
        return QString();

    QString name = _parts.last();
    int index = name.indexOf(QLatin1Char('.'), 1);
    return name.left(index);
}

QString Path::suffix() const
{
    if (_parts.isEmpty())
        return QString();

    QString name = _parts.last();
    int index = name.lastIndexOf(QLatin1Char('.'));
    if (index <= 0)
        return QString();

    return name.mid(index + 1);
}

QString Path::completeBaseName() const
{
    if (_parts.isEmpty())
        return QString();

    QString name = _parts.last();
    int index = name.lastIndexOf(QLatin1Char('.'));
    if (index <= 0)
        return name;

    return name.left(index);
}

QString Path::completeSuffix() const
{
    if (_parts.isEmpty())
        return QString();

    QString name = _parts.last();
    int index = name.indexOf(QLatin1Char('.'), 1);
    if (index < 0)
        return QString();

    return name.mid(index + 1);
}

void Path::setFileName(const QString& replacement)
{
    QString name = replacement;
    while (name.endsWith(QLatin1Char('.')))
        name.chop(1);

    if (_parts.isEmpty())
    {
        if (!name.isEmpty())
            _parts.append(name);
    }
    else
    {
        _parts.last() = name;
    }
}

void Path::setBaseName(const QString& replacement)
{
    setFileName(replacement + completeSuffix());
}

void Path::setSuffix(const QString& replacement)
{
    setFileName(completeBaseName() + QLatin1Char('.') + replacement);
}

void Path::setCompleteBaseName(const QString& replacement)
{
    setFileName(replacement + QLatin1Char('.') + suffix());
}

void Path::setCompleteSuffix(const QString& replacement)
{
    setFileName(baseName() + QLatin1Char('.') + replacement);
}

void Path::removeFileName()
{
    if (_parts.isEmpty())
    {
        _absolute = false;
    }
    else
    {
        _parts.removeLast();
    }
}

int Path::validatePart(const QString& part, int options) const
{
    int result = Path::NoError;
    for (int i = 0; i < part.size(); ++i)
    {
        ushort ch = part.at(i).unicode();
        if (ch > 0x7F)
        {
            result |= Path::Unicode;
        }
        else if (ch < ' ' || ch == '"' || ch == '*' || ch == '/' || ch == ':' ||
                 ch == '<' || ch == '>' || ch == '?' || ch == '\\' || ch == '|' || ch == 0x7F)
        {
            result |= Path::ForbiddenCharacters;
        }
    }

    if (part.size() > 2 && part.endsWith('.'))
        result |= Path::SpecialNames;

    if ((result & (Path::Unicode | Path::ForbiddenCharacters)) == 0 &&
        (options & Path::AllowSpecialNames) == 0)
    {
        if (s_specialNames.contains(part, Qt::CaseInsensitive))
            result |= Path::SpecialNames;
    }

    return result;
}

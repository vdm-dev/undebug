#ifndef PATH_H
#define PATH_H


#include <QStringList>


class Path
{
public:
    enum Type
    {
        Original,
        Absolute,
        Relative
    };

    enum CompareFlags
    {
        CompareDefault      = 0,
        CompareSensitive    = 1,
        CompareWithType     = 2,
        CompareResolved     = 4,
        CompareExact        = CompareSensitive | CompareWithType
    };

    enum ValidationResult
    {
        NoError             = 0,
        Unicode             = 1,
        Dots                = 2,
        DotDots             = 4,
        AboveRoot           = 8,
        EmptySuffix         = 16,
        EmptyFileName       = 32,
        SpecialNames        = 64,
        ForbiddenCharacters = 128
    };

    enum ValidatorOptions
    {
        Strict              = 0,
        AllowUnicode        = Unicode,
        AllowDots           = Dots,
        AllowDotDots        = DotDots,
        AllowUnresolved     = AllowDots | AllowDotDots,
        AllowAboveRoot      = AboveRoot | AllowDotDots,
        AllowEmptySuffix    = EmptySuffix,
        AllowEmptyFileName  = EmptyFileName | AllowEmptySuffix,
        AllowSpecialNames   = SpecialNames,
        AllowAllCharacters  = ForbiddenCharacters | AllowUnicode,
        Tolerant            = AllowUnicode | AllowUnresolved | AllowAboveRoot | AllowEmptyFileName,
        Permissive          = Tolerant | AllowSpecialNames | AllowAllCharacters
    };

public:
    Path(const Path& other);
    Path(const QString& path = QString(), bool resolve = false);

    Path& operator=(const Path& other);
    Path& operator=(const QString& path);
    Path& operator=(Path&& other) noexcept;
    Path& operator+=(const Path& other);
    Path& operator+=(const QString& other);
    Path operator+(const Path& other);
    Path operator+(const QString& other);
    bool operator==(const Path& other) const;
    bool operator==(const QString& other) const;
    bool operator!=(const Path& other) const;
    bool operator!=(const QString& other) const;
    bool operator<(const Path& other) const;
    bool operator<(const QString& other) const;
    QString& operator[](int i);
    const QString& operator[](int i) const;

    void swap(Path& other) noexcept;

    void clear();
    void resolve();
    int validate(int options = Path::Tolerant) const;
    static int validate(const QString& path, int option = Path::Tolerant);

    void append(const Path& other, bool resolve = false);
    void append(const QString& other, bool resolve = false);

    const QString& at(int i) const;
    int count() const;
    int size() const;

    int compare(const Path& other, int flags = Path::CompareDefault) const;
    int compare(const QString& other, int flags = Path::CompareDefault) const;
    bool startsWith(const Path& other, int flags = Path::CompareDefault) const;
    bool startsWith(const QString& other, int flags = Path::CompareDefault) const;

    bool isEmpty() const;
    bool isRoot() const;
    bool isAbsolute() const;

    void makeAbsolute();
    void makeRelative();

    void setPath(const QString& path, bool resolve = false);
    QString path(Path::Type type = Path::Original) const;
    QString cleanPath(Path::Type type = Path::Original) const;

    QString parentPath(Path::Type type = Path::Original) const;
    QString cleanParentPath(Path::Type type = Path::Original) const;

    QString fileName() const;
    QString baseName() const;
    QString suffix() const;
    QString completeBaseName() const;
    QString completeSuffix() const;

    void setFileName(const QString& replacement);
    void setBaseName(const QString& replacement);
    void setSuffix(const QString& replacement);
    void setCompleteBaseName(const QString& replacement);
    void setCompleteSuffix(const QString& replacement);

    void removeFileName();

public:
    static QStringList s_specialNames;

private:
    int validatePart(const QString& part, int options) const;

private:
    QStringList _parts;
    bool _absolute;
};


inline Path& Path::operator=(const Path& other)
{
    _parts = other._parts;
    _absolute = other._absolute;
    return *this;
}

inline Path& Path::operator=(const QString& path)
{
    setPath(path);
    return *this;
}

inline Path& Path::operator=(Path&& other) noexcept
{
    swap(other);
    return *this;
}

inline Path& Path::operator+=(const Path& other)
{
    append(other, false);
    return *this;
}

inline Path& Path::operator+=(const QString& other)
{
    append(other, false);
    return *this;
}

inline Path Path::operator+(const Path& other)
{
    Path result(*this);
    result.append(other, false);
    return result;
}

inline Path Path::operator+(const QString& other)
{
    Path result(*this);
    result.append(other, false);
    return result;
}

inline bool Path::operator==(const Path& other) const
{
    return (compare(other, Path::CompareExact) == 0);
}

inline bool Path::operator==(const QString& other) const
{
    return (compare(other, Path::CompareExact) == 0);
}

inline bool Path::operator!=(const Path& other) const
{
    return (compare(other, Path::CompareExact) != 0);
}

inline bool Path::operator!=(const QString& other) const
{
    return (compare(other, Path::CompareExact) != 0);
}

inline bool Path::operator<(const Path& other) const
{
    return (compare(other, Path::CompareExact) < 0);
}

inline bool Path::operator<(const QString& other) const
{
    return (compare(other, Path::CompareExact) < 0);
}

inline QString& Path::operator[](int i)
{
    return _parts[i];
}

inline const QString& Path::operator[](int i) const
{
    return _parts[i];
}

inline void Path::swap(Path& other) noexcept
{
    qSwap(_parts, other._parts);
    qSwap(_absolute, other._absolute);
}

inline void Path::clear()
{
    _parts.clear();
    _absolute = false;
}

inline void Path::append(const QString& other, bool resolve)
{
    append(Path(other, false), resolve);
}

inline const QString& Path::at(int i) const
{
    return _parts.at(i);
}

inline int Path::count() const
{
    return _parts.count();
}

inline int Path::size() const
{
    return _parts.size();
}

inline int Path::compare(const QString& other, int flags) const
{
    return compare(Path(other, false), flags);
}

inline bool Path::startsWith(const QString& other, int flags) const
{
    return startsWith(Path(other, false), flags);
}

inline bool Path::isEmpty() const
{
    return _parts.isEmpty();
}

inline bool Path::isRoot() const
{
    return (isEmpty() && isAbsolute());
}

inline bool Path::isAbsolute() const
{
    return _absolute;
}

inline void Path::makeAbsolute()
{
    _absolute = true;
}

inline void Path::makeRelative()
{
    _absolute = false;
}


#endif // PATH_H

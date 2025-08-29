#include "AsciiTranslator.h"
#include <QRegularExpression>

static inline bool isSafeAsciiChar(unsigned char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == ' ' || c == '_' || c == '.' || c == '-';
}

static inline void replaceBoth(QString& s,
                               const QString& fromLower, const QString& toLower,
                               const QString& fromUpper, const QString& toUpper)
{
    s.replace(fromLower, toLower, Qt::CaseSensitive);
    s.replace(fromUpper, toUpper, Qt::CaseSensitive);
}

void AsciiTranslator::applyCommonReplacements(QString& s)
{
    // Norske
    replaceBoth(s, QStringLiteral("æ"), QStringLiteral("ae"),
                QStringLiteral("Æ"), QStringLiteral("AE"));
    replaceBoth(s, QStringLiteral("ø"), QStringLiteral("oe"),
                QStringLiteral("Ø"), QStringLiteral("OE"));
    replaceBoth(s, QStringLiteral("å"), QStringLiteral("aa"),
                QStringLiteral("Å"), QStringLiteral("AA"));

    // Vanlige europeiske
    replaceBoth(s, QStringLiteral("ß"), QStringLiteral("ss"),
                QStringLiteral("ẞ"), QStringLiteral("SS"));
    replaceBoth(s, QStringLiteral("œ"), QStringLiteral("oe"),
                QStringLiteral("Œ"), QStringLiteral("OE"));
    replaceBoth(s, QStringLiteral("ñ"), QStringLiteral("n"),
                QStringLiteral("Ñ"), QStringLiteral("N"));
    replaceBoth(s, QStringLiteral("ç"), QStringLiteral("c"),
                QStringLiteral("Ç"), QStringLiteral("C"));
    replaceBoth(s, QStringLiteral("ł"), QStringLiteral("l"),
                QStringLiteral("Ł"), QStringLiteral("L"));
    replaceBoth(s, QStringLiteral("đ"), QStringLiteral("d"),
                QStringLiteral("Đ"), QStringLiteral("D"));
    replaceBoth(s, QStringLiteral("þ"), QStringLiteral("th"),
                QStringLiteral("Þ"), QStringLiteral("TH"));
    replaceBoth(s, QStringLiteral("ð"), QStringLiteral("d"),
                QStringLiteral("Ð"), QStringLiteral("D"));
    replaceBoth(s, QStringLiteral("æ"), QStringLiteral("ae"),
                QStringLiteral("Æ"), QStringLiteral("AE")); // ok om duplikat

    // Typografiske streker / punkt
    s.replace(QStringLiteral("—"), QStringLiteral("-"));
    s.replace(QStringLiteral("–"), QStringLiteral("-"));
    s.replace(QStringLiteral("•"), QStringLiteral("-"));
    s.replace(QStringLiteral("·"), QStringLiteral("-"));
}

void AsciiTranslator::stripCombiningMarks(QString& s)
{
    // NFD: base + diakritikk som separate kodepunkter
    QString nfd = s.normalized(QString::NormalizationForm_D);

    // Fjern alle kombinerende markører (Unicode Mn)
    static const QRegularExpression reMn(QStringLiteral("\\p{Mn}+"));
    nfd.remove(reMn);

    s = nfd;
}

QString AsciiTranslator::filterToAscii(const QString& s, const Options& opt)
{
    QByteArray out;
    out.reserve(s.size());

    for (QChar qc : s) {
        ushort u = qc.unicode();

        if (u == '\n' || u == '\r' || u == '\t')
            u = ' ';

        if (u >= 0x20 && u <= 0x7E) {
            unsigned char c = static_cast<unsigned char>(u);
            if (opt.restrictToSafeAscii) {
                if (isSafeAsciiChar(c)) out.append(c);
                else out.append(opt.replaceUnknownWithQuestionMark ? '?' : ' ');
            } else {
                out.append(c);
            }
        } else {
            if (opt.replaceUnknownWithQuestionMark) out.append('?');
            // ellers droppes tegn utenfor ASCII
        }
    }

    QString ascii = QString::fromLatin1(out);

    if (opt.collapseWhitespace) {
        static const QRegularExpression reWhitespace(QStringLiteral("\\s+"));
        ascii.replace(reWhitespace, QStringLiteral(" "));
        ascii = ascii.trimmed();
    }

    return ascii;
}

QString AsciiTranslator::toAscii(const QString& input, const Options& opt)
{
    if (input.isEmpty())
        return QString();

    QString s = input;
    applyCommonReplacements(s);
    stripCombiningMarks(s);
    return filterToAscii(s, opt);
}

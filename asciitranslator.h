#ifndef ASCIITRANSLATOR_H
#define ASCIITRANSLATOR_H

#include <QString>

class AsciiTranslator
{
public:
    struct Options {
        Options() {};
        bool replaceUnknownWithQuestionMark = false;
        bool collapseWhitespace = true;
        bool restrictToSafeAscii = false; // kun [A-Za-z0-9 _.-]
    };

    static QString toAscii(const QString& input, const Options& opt = Options());

private:
    static void applyCommonReplacements(QString& s);
    static void stripCombiningMarks(QString& s);
    static QString filterToAscii(const QString& s, const Options& opt);
};

#endif

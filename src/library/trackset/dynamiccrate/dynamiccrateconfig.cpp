#include "library/trackset/dynamiccrate/dynamiccrateconfig.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlField>
#include <algorithm>

#include "library/queryutil.h"
#include "preferences/configobject.h"
#include "util/db/sqlite.h"

namespace {

constexpr QLatin1String kConfigFileName("dynamic_crates.cfg");

struct DynamicCrateFieldSpec {
    const char* fieldName;
    const char* sqlExpression;
    DynamicCrateFieldType fieldType;
};

struct ParsedDateTimeValue {
    QDateTime start;
    QDateTime endExclusive;
    bool dayPrecision = false;
};

QString stripGroupBrackets(QString group) {
    if (group.startsWith('[') && group.endsWith(']') && group.size() >= 2) {
        group.chop(1);
        group.remove(0, 1);
    }
    return group;
}

QString trimMatchingQuotes(QString value) {
    value = value.trimmed();
    if (value.size() >= 2 &&
            ((value.startsWith('"') && value.endsWith('"')) ||
                    (value.startsWith('\'') && value.endsWith('\'')))) {
        value.chop(1);
        value.remove(0, 1);
    }
    return value;
}

QString escapeLikeValue(QString value) {
    value.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    value.replace(QStringLiteral("%"), QStringLiteral("\\%"));
    value.replace(QStringLiteral("_"), QStringLiteral("\\_"));
    return value;
}

QString toSqlDateTimeString(const QDateTime& dateTime) {
    return mixxx::sqlite::writeGeneratedTimestamp(dateTime).toString();
}

QDateTime localDateStartToUtc(const QDate& date) {
    return QDateTime(date, QTime(0, 0)).toUTC();
}

const DynamicCrateFieldSpec* findFieldSpec(const QString& fieldName) {
    static const DynamicCrateFieldSpec kFieldSpecs[] = {
            {"artist", "library.artist", DynamicCrateFieldType::Text},
            {"title", "library.title", DynamicCrateFieldType::Text},
            {"album", "library.album", DynamicCrateFieldType::Text},
            {"album_artist", "library.album_artist", DynamicCrateFieldType::Text},
            {"albumartist", "library.album_artist", DynamicCrateFieldType::Text},
            {"year", "CAST(substr(library.year,1,4) AS INTEGER)", DynamicCrateFieldType::Number},
            {"genre", "library.genre", DynamicCrateFieldType::Text},
            {"composer", "library.composer", DynamicCrateFieldType::Text},
            {"grouping", "library.grouping", DynamicCrateFieldType::Text},
            {"tracknumber", "CAST(library.tracknumber AS INTEGER)", DynamicCrateFieldType::Number},
            {"track_number", "CAST(library.tracknumber AS INTEGER)", DynamicCrateFieldType::Number},
            {"track", "CAST(library.tracknumber AS INTEGER)", DynamicCrateFieldType::Number},
            {"filetype", "library.filetype", DynamicCrateFieldType::Text},
            {"file_type", "library.filetype", DynamicCrateFieldType::Text},
            {"type", "library.filetype", DynamicCrateFieldType::Text},
            {"location", "track_locations.location", DynamicCrateFieldType::Text},
            {"native_location", "track_locations.location", DynamicCrateFieldType::Text},
            {"filepath", "track_locations.location", DynamicCrateFieldType::Text},
            {"directory", "track_locations.directory", DynamicCrateFieldType::Text},
            {"folder", "track_locations.directory", DynamicCrateFieldType::Text},
            {"filename", "track_locations.filename", DynamicCrateFieldType::Text},
            {"filesize", "track_locations.filesize", DynamicCrateFieldType::Number},
            {"file_size", "track_locations.filesize", DynamicCrateFieldType::Number},
            {"comment", "library.comment", DynamicCrateFieldType::Text},
            {"url", "library.url", DynamicCrateFieldType::Text},
            {"duration", "library.duration", DynamicCrateFieldType::Number},
            {"bitrate", "library.bitrate", DynamicCrateFieldType::Number},
            {"bpm", "library.bpm", DynamicCrateFieldType::Number},
            {"bpm_lock", "library.bpm_lock", DynamicCrateFieldType::Number},
            {"replaygain", "library.replaygain", DynamicCrateFieldType::Number},
            {"replay_gain", "library.replaygain", DynamicCrateFieldType::Number},
            {"date_added", "library.datetime_added", DynamicCrateFieldType::DateTime},
            {"datetime_added", "library.datetime_added", DynamicCrateFieldType::DateTime},
            {"datetimeadded", "library.datetime_added", DynamicCrateFieldType::DateTime},
            {"played", "library.played", DynamicCrateFieldType::Number},
            {"timesplayed", "library.timesplayed", DynamicCrateFieldType::Number},
            {"times_played", "library.timesplayed", DynamicCrateFieldType::Number},
            {"rating", "library.rating", DynamicCrateFieldType::Number},
            {"key", "library.key", DynamicCrateFieldType::Text},
            {"key_id", "library.key_id", DynamicCrateFieldType::Number},
            {"color", "library.color", DynamicCrateFieldType::Text},
            {"last_played", "library.last_played_at", DynamicCrateFieldType::DateTime},
            {"last_played_at", "library.last_played_at", DynamicCrateFieldType::DateTime},
            {"lastplayed", "library.last_played_at", DynamicCrateFieldType::DateTime},
            {"samplerate", "library.samplerate", DynamicCrateFieldType::Number},
            {"sample_rate", "library.samplerate", DynamicCrateFieldType::Number},
            {"channels", "library.channels", DynamicCrateFieldType::Number},
    };

    const QString loweredFieldName = fieldName.trimmed().toLower();
    for (const auto& fieldSpec : kFieldSpecs) {
        if (loweredFieldName == QLatin1String(fieldSpec.fieldName)) {
            return &fieldSpec;
        }
    }
    return nullptr;
}

QString operatorToString(DynamicCrateOperator op) {
    switch (op) {
    case DynamicCrateOperator::Eq:
        return QStringLiteral("==");
    case DynamicCrateOperator::NotEq:
        return QStringLiteral("!=");
    case DynamicCrateOperator::Contains:
        return QStringLiteral("~=");
    case DynamicCrateOperator::NotContains:
        return QStringLiteral("!~=");
    case DynamicCrateOperator::StartsWith:
        return QStringLiteral("^=");
    case DynamicCrateOperator::NotStartsWith:
        return QStringLiteral("!^=");
    case DynamicCrateOperator::Lt:
        return QStringLiteral("<");
    case DynamicCrateOperator::Lte:
        return QStringLiteral("<=");
    case DynamicCrateOperator::Gt:
        return QStringLiteral(">");
    case DynamicCrateOperator::Gte:
        return QStringLiteral(">=");
    }
    return QString();
}

bool parseMatchMode(
        const QString& value,
        DynamicCrateMatchMode* pMatchMode) {
    const QString loweredValue = value.trimmed().toLower();
    if (loweredValue == QStringLiteral("any")) {
        *pMatchMode = DynamicCrateMatchMode::Any;
        return true;
    }
    if (loweredValue == QStringLiteral("all")) {
        *pMatchMode = DynamicCrateMatchMode::All;
        return true;
    }
    return false;
}

bool parseOperator(
        const QString& rawOperator,
        DynamicCrateOperator* pOperator) {
    static const QHash<QString, DynamicCrateOperator> kOperators = {
            {QStringLiteral("=="), DynamicCrateOperator::Eq},
            {QStringLiteral("!="), DynamicCrateOperator::NotEq},
            {QStringLiteral("~="), DynamicCrateOperator::Contains},
            {QStringLiteral("!~="), DynamicCrateOperator::NotContains},
            {QStringLiteral("^="), DynamicCrateOperator::StartsWith},
            {QStringLiteral("!^="), DynamicCrateOperator::NotStartsWith},
            {QStringLiteral("<"), DynamicCrateOperator::Lt},
            {QStringLiteral("<="), DynamicCrateOperator::Lte},
            {QStringLiteral(">"), DynamicCrateOperator::Gt},
            {QStringLiteral(">="), DynamicCrateOperator::Gte},
            {QStringLiteral("!<"), DynamicCrateOperator::Gte},
            {QStringLiteral("!<="), DynamicCrateOperator::Gt},
            {QStringLiteral("!>"), DynamicCrateOperator::Lte},
            {QStringLiteral("!>="), DynamicCrateOperator::Lt},
    };

    const auto it = kOperators.constFind(rawOperator);
    if (it == kOperators.constEnd()) {
        return false;
    }
    *pOperator = it.value();
    return true;
}

QString quoteRuleValue(QString value) {
    if (value.contains('"') && !value.contains('\'')) {
        return QStringLiteral("'%1'").arg(value);
    }
    if (!value.contains('"')) {
        return QStringLiteral("\"%1\"").arg(value);
    }
    return value;
}

QString cleanedAsciiToken(QString token) {
    QString cleanedToken;
    cleanedToken.reserve(token.size());
    const QString normalized = token.normalized(QString::NormalizationForm_KD);
    for (const QChar& ch : normalized) {
        switch (ch.category()) {
        case QChar::Mark_NonSpacing:
        case QChar::Mark_SpacingCombining:
        case QChar::Mark_Enclosing:
            continue;
        default:
            break;
        }
        if (ch.isLetterOrNumber() && ch.unicode() < 128) {
            cleanedToken.append(ch);
        }
    }
    return cleanedToken;
}

bool parseDateTimeValue(
        QString rawValue,
        ParsedDateTimeValue* pParsedValue) {
    rawValue = trimMatchingQuotes(std::move(rawValue));
    if (rawValue.isEmpty()) {
        return false;
    }

    if (rawValue.compare(QStringLiteral("now"), Qt::CaseInsensitive) == 0) {
        pParsedValue->start = QDateTime::currentDateTimeUtc();
        pParsedValue->endExclusive = pParsedValue->start;
        pParsedValue->dayPrecision = false;
        return true;
    }

    static const QRegularExpression kRelativeNowRegex(
            QStringLiteral("^now\\s*([+-])\\s*(\\d+)\\s*([hdwmy])$"),
            QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch relativeNowMatch = kRelativeNowRegex.match(rawValue);
    if (relativeNowMatch.hasMatch()) {
        QDateTime dateTime = QDateTime::currentDateTimeUtc();
        const int magnitude = relativeNowMatch.captured(2).toInt();
        const bool subtract = relativeNowMatch.captured(1) == QStringLiteral("-");
        const int direction = subtract ? -1 : 1;
        const QChar unit = relativeNowMatch.captured(3).at(0).toLower();
        switch (unit.unicode()) {
        case 'h':
            dateTime = dateTime.addSecs(direction * magnitude * 60 * 60);
            break;
        case 'd':
            dateTime = dateTime.addDays(direction * magnitude);
            break;
        case 'w':
            dateTime = dateTime.addDays(direction * magnitude * 7);
            break;
        case 'm':
            dateTime = dateTime.addMonths(direction * magnitude);
            break;
        case 'y':
            dateTime = dateTime.addYears(direction * magnitude);
            break;
        default:
            return false;
        }
        pParsedValue->start = dateTime;
        pParsedValue->endExclusive = dateTime;
        pParsedValue->dayPrecision = false;
        return true;
    }

    const QDate date = QDate::fromString(rawValue, Qt::ISODate);
    if (date.isValid() && rawValue.size() == 10) {
        pParsedValue->start = localDateStartToUtc(date);
        pParsedValue->endExclusive = pParsedValue->start.addDays(1);
        pParsedValue->dayPrecision = true;
        return true;
    }

    QDateTime dateTime = QDateTime::fromString(rawValue, Qt::ISODate);
    if (!dateTime.isValid()) {
        dateTime = QDateTime::fromString(rawValue, Qt::ISODateWithMs);
    }
    if (!dateTime.isValid()) {
        return false;
    }
    pParsedValue->start = dateTime.toUTC();
    pParsedValue->endExclusive = pParsedValue->start;
    pParsedValue->dayPrecision = false;
    return true;
}

bool validateOperatorForFieldType(
        DynamicCrateFieldType fieldType,
        DynamicCrateOperator op) {
    switch (fieldType) {
    case DynamicCrateFieldType::Text:
        return op == DynamicCrateOperator::Eq ||
                op == DynamicCrateOperator::NotEq ||
                op == DynamicCrateOperator::Contains ||
                op == DynamicCrateOperator::NotContains ||
                op == DynamicCrateOperator::StartsWith ||
                op == DynamicCrateOperator::NotStartsWith;
    case DynamicCrateFieldType::Number:
    case DynamicCrateFieldType::DateTime:
        return op == DynamicCrateOperator::Eq ||
                op == DynamicCrateOperator::NotEq ||
                op == DynamicCrateOperator::Lt ||
                op == DynamicCrateOperator::Lte ||
                op == DynamicCrateOperator::Gt ||
                op == DynamicCrateOperator::Gte;
    }
    return false;
}

bool parseRuleValue(
        QString rawRule,
        DynamicCrateRule* pRule,
        QString* pError) {
    static const QRegularExpression kRuleRegex(
            QStringLiteral(
                    "^([A-Za-z_][A-Za-z0-9_]*)\\s*(==|!=|!~=|~=|!\\^=|\\^=|!<=|!>=|!<|!>|<=|>=|<|>)\\s*(.+)$"));

    const QRegularExpressionMatch ruleMatch = kRuleRegex.match(rawRule.trimmed());
    if (!ruleMatch.hasMatch()) {
        if (pError) {
            *pError = QStringLiteral("Could not parse rule");
        }
        return false;
    }

    const QString fieldName = ruleMatch.captured(1).trimmed().toLower();
    const DynamicCrateFieldSpec* pFieldSpec = findFieldSpec(fieldName);
    if (pFieldSpec == nullptr) {
        if (pError) {
            *pError = QStringLiteral("Unknown field %1").arg(fieldName);
        }
        return false;
    }

    DynamicCrateOperator op;
    if (!parseOperator(ruleMatch.captured(2), &op)) {
        if (pError) {
            *pError = QStringLiteral("Unknown operator %1").arg(ruleMatch.captured(2));
        }
        return false;
    }
    if (!validateOperatorForFieldType(pFieldSpec->fieldType, op)) {
        if (pError) {
            *pError = QStringLiteral("Operator %1 is not supported for field %2")
                              .arg(ruleMatch.captured(2), fieldName);
        }
        return false;
    }

    QString rawValue = ruleMatch.captured(3).trimmed();
    if (rawValue.isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Missing rule value");
        }
        return false;
    }

    switch (pFieldSpec->fieldType) {
    case DynamicCrateFieldType::Text:
        rawValue = trimMatchingQuotes(rawValue);
        if (rawValue.isEmpty()) {
            if (pError) {
                *pError = QStringLiteral("Text rule value is empty");
            }
            return false;
        }
        break;
    case DynamicCrateFieldType::Number: {
        bool ok = false;
        trimMatchingQuotes(rawValue).toDouble(&ok);
        if (!ok) {
            if (pError) {
                *pError = QStringLiteral("Invalid numeric value %1").arg(rawValue);
            }
            return false;
        }
        rawValue = trimMatchingQuotes(rawValue);
        break;
    }
    case DynamicCrateFieldType::DateTime: {
        ParsedDateTimeValue parsedDateTimeValue;
        if (!parseDateTimeValue(rawValue, &parsedDateTimeValue)) {
            if (pError) {
                *pError = QStringLiteral("Invalid date/time value %1").arg(rawValue);
            }
            return false;
        }
        rawValue = trimMatchingQuotes(rawValue);
        break;
    }
    }

    pRule->fieldName = fieldName;
    pRule->fieldType = pFieldSpec->fieldType;
    pRule->op = op;
    pRule->rawValue = rawValue;
    return true;
}

QString escapedStringLiteral(
        const FieldEscaper& fieldEscaper,
        const QString& value) {
    return fieldEscaper.escapeString(value);
}

bool buildTextClause(
        const DynamicCrateFieldSpec& fieldSpec,
        DynamicCrateOperator op,
        const QString& rawValue,
        const FieldEscaper& fieldEscaper,
        QString* pClause,
        QString* pError) {
    const QString trimmedValue = trimMatchingQuotes(rawValue);
    const QString escapedValue = escapedStringLiteral(fieldEscaper, trimmedValue);
    const QString escapedContainsValue = escapedStringLiteral(
            fieldEscaper,
            QStringLiteral("%") +
                    escapeLikeValue(trimmedValue) +
                    QStringLiteral("%"));
    const QString escapedStartsWithValue = escapedStringLiteral(
            fieldEscaper,
            escapeLikeValue(trimmedValue) + QStringLiteral("%"));

    switch (op) {
    case DynamicCrateOperator::Eq:
        *pClause = QStringLiteral("lower(%1) = lower(%2)")
                           .arg(QLatin1String(fieldSpec.sqlExpression), escapedValue);
        return true;
    case DynamicCrateOperator::NotEq:
        *pClause = QStringLiteral("lower(%1) != lower(%2)")
                           .arg(QLatin1String(fieldSpec.sqlExpression), escapedValue);
        return true;
    case DynamicCrateOperator::Contains:
        *pClause = QStringLiteral("lower(%1) LIKE lower(%2) ESCAPE '\\'")
                           .arg(QLatin1String(fieldSpec.sqlExpression), escapedContainsValue);
        return true;
    case DynamicCrateOperator::NotContains:
        *pClause = QStringLiteral("lower(%1) NOT LIKE lower(%2) ESCAPE '\\'")
                           .arg(QLatin1String(fieldSpec.sqlExpression), escapedContainsValue);
        return true;
    case DynamicCrateOperator::StartsWith:
        *pClause = QStringLiteral("lower(%1) LIKE lower(%2) ESCAPE '\\'")
                           .arg(QLatin1String(fieldSpec.sqlExpression), escapedStartsWithValue);
        return true;
    case DynamicCrateOperator::NotStartsWith:
        *pClause = QStringLiteral("lower(%1) NOT LIKE lower(%2) ESCAPE '\\'")
                           .arg(QLatin1String(fieldSpec.sqlExpression), escapedStartsWithValue);
        return true;
    case DynamicCrateOperator::Lt:
    case DynamicCrateOperator::Lte:
    case DynamicCrateOperator::Gt:
    case DynamicCrateOperator::Gte:
        if (pError) {
            *pError = QStringLiteral("Comparison operator %1 is not supported for text field %2")
                              .arg(operatorToString(op), QString::fromLatin1(fieldSpec.fieldName));
        }
        return false;
    }
    return false;
}

bool buildNumericClause(
        const DynamicCrateFieldSpec& fieldSpec,
        DynamicCrateOperator op,
        const QString& rawValue,
        QString* pClause,
        QString* pError) {
    bool ok = false;
    const double numericValue = trimMatchingQuotes(rawValue).toDouble(&ok);
    if (!ok) {
        if (pError) {
            *pError = QStringLiteral("Invalid numeric value %1").arg(rawValue);
        }
        return false;
    }

    const QString numericLiteral = QString::number(numericValue, 'g', 16);
    switch (op) {
    case DynamicCrateOperator::Eq:
        *pClause = QStringLiteral("%1 = %2")
                           .arg(QLatin1String(fieldSpec.sqlExpression), numericLiteral);
        return true;
    case DynamicCrateOperator::NotEq:
        *pClause = QStringLiteral("%1 != %2")
                           .arg(QLatin1String(fieldSpec.sqlExpression), numericLiteral);
        return true;
    case DynamicCrateOperator::Lt:
        *pClause = QStringLiteral("%1 < %2")
                           .arg(QLatin1String(fieldSpec.sqlExpression), numericLiteral);
        return true;
    case DynamicCrateOperator::Lte:
        *pClause = QStringLiteral("%1 <= %2")
                           .arg(QLatin1String(fieldSpec.sqlExpression), numericLiteral);
        return true;
    case DynamicCrateOperator::Gt:
        *pClause = QStringLiteral("%1 > %2")
                           .arg(QLatin1String(fieldSpec.sqlExpression), numericLiteral);
        return true;
    case DynamicCrateOperator::Gte:
        *pClause = QStringLiteral("%1 >= %2")
                           .arg(QLatin1String(fieldSpec.sqlExpression), numericLiteral);
        return true;
    case DynamicCrateOperator::Contains:
    case DynamicCrateOperator::NotContains:
    case DynamicCrateOperator::StartsWith:
    case DynamicCrateOperator::NotStartsWith:
        if (pError) {
            *pError = QStringLiteral("Text operator %1 is not supported for numeric field %2")
                              .arg(operatorToString(op), QString::fromLatin1(fieldSpec.fieldName));
        }
        return false;
    }
    return false;
}

bool buildDateTimeClause(
        const DynamicCrateFieldSpec& fieldSpec,
        DynamicCrateOperator op,
        const QString& rawValue,
        const FieldEscaper& fieldEscaper,
        QString* pClause,
        QString* pError) {
    ParsedDateTimeValue parsedValue;
    if (!parseDateTimeValue(rawValue, &parsedValue)) {
        if (pError) {
            *pError = QStringLiteral("Invalid date/time value %1").arg(rawValue);
        }
        return false;
    }

    const QString fieldExpression = QLatin1String(fieldSpec.sqlExpression);
    const QString escapedStart = escapedStringLiteral(
            fieldEscaper,
            toSqlDateTimeString(parsedValue.start));
    const QString escapedEnd = escapedStringLiteral(
            fieldEscaper,
            toSqlDateTimeString(parsedValue.endExclusive));

    if (parsedValue.dayPrecision) {
        switch (op) {
        case DynamicCrateOperator::Eq:
            *pClause = QStringLiteral("(%1 >= %2 AND %1 < %3)")
                               .arg(fieldExpression, escapedStart, escapedEnd);
            return true;
        case DynamicCrateOperator::NotEq:
            *pClause = QStringLiteral("(%1 < %2 OR %1 >= %3)")
                               .arg(fieldExpression, escapedStart, escapedEnd);
            return true;
        case DynamicCrateOperator::Lt:
            *pClause = QStringLiteral("%1 < %2")
                               .arg(fieldExpression, escapedStart);
            return true;
        case DynamicCrateOperator::Lte:
            *pClause = QStringLiteral("%1 < %2")
                               .arg(fieldExpression, escapedEnd);
            return true;
        case DynamicCrateOperator::Gt:
            *pClause = QStringLiteral("%1 >= %2")
                               .arg(fieldExpression, escapedEnd);
            return true;
        case DynamicCrateOperator::Gte:
            *pClause = QStringLiteral("%1 >= %2")
                               .arg(fieldExpression, escapedStart);
            return true;
        case DynamicCrateOperator::Contains:
        case DynamicCrateOperator::NotContains:
        case DynamicCrateOperator::StartsWith:
        case DynamicCrateOperator::NotStartsWith:
            break;
        }
    }

    switch (op) {
    case DynamicCrateOperator::Eq:
        *pClause = QStringLiteral("%1 = %2")
                           .arg(fieldExpression, escapedStart);
        return true;
    case DynamicCrateOperator::NotEq:
        *pClause = QStringLiteral("%1 != %2")
                           .arg(fieldExpression, escapedStart);
        return true;
    case DynamicCrateOperator::Lt:
        *pClause = QStringLiteral("%1 < %2")
                           .arg(fieldExpression, escapedStart);
        return true;
    case DynamicCrateOperator::Lte:
        *pClause = QStringLiteral("%1 <= %2")
                           .arg(fieldExpression, escapedStart);
        return true;
    case DynamicCrateOperator::Gt:
        *pClause = QStringLiteral("%1 > %2")
                           .arg(fieldExpression, escapedStart);
        return true;
    case DynamicCrateOperator::Gte:
        *pClause = QStringLiteral("%1 >= %2")
                           .arg(fieldExpression, escapedStart);
        return true;
    case DynamicCrateOperator::Contains:
    case DynamicCrateOperator::NotContains:
    case DynamicCrateOperator::StartsWith:
    case DynamicCrateOperator::NotStartsWith:
        if (pError) {
            *pError = QStringLiteral("Text operator %1 is not supported for date field %2")
                              .arg(operatorToString(op), QString::fromLatin1(fieldSpec.fieldName));
        }
        return false;
    }
    return false;
}

bool buildRuleClause(
        const DynamicCrateRule& rule,
        const FieldEscaper& fieldEscaper,
        QString* pClause,
        QString* pError) {
    const DynamicCrateFieldSpec* pFieldSpec = findFieldSpec(rule.fieldName);
    if (pFieldSpec == nullptr) {
        if (pError) {
            *pError = QStringLiteral("Unknown field %1").arg(rule.fieldName);
        }
        return false;
    }

    switch (pFieldSpec->fieldType) {
    case DynamicCrateFieldType::Text:
        return buildTextClause(*pFieldSpec, rule.op, rule.rawValue, fieldEscaper, pClause, pError);
    case DynamicCrateFieldType::Number:
        return buildNumericClause(*pFieldSpec, rule.op, rule.rawValue, pClause, pError);
    case DynamicCrateFieldType::DateTime:
        return buildDateTimeClause(*pFieldSpec, rule.op, rule.rawValue, fieldEscaper, pClause, pError);
    }
    return false;
}

int ruleItemIndex(const QString& itemName) {
    static const QRegularExpression kRuleItemRegex(QStringLiteral("^rule_(\\d+)$"));
    const QRegularExpressionMatch match = kRuleItemRegex.match(itemName);
    if (!match.hasMatch()) {
        return -1;
    }
    return match.captured(1).toInt();
}

} // namespace

QString dynamicCrateMatchModeToConfigString(DynamicCrateMatchMode matchMode) {
    switch (matchMode) {
    case DynamicCrateMatchMode::Any:
        return QStringLiteral("any");
    case DynamicCrateMatchMode::All:
        return QStringLiteral("all");
    }
    return QString();
}

QString dynamicCrateOperatorToConfigString(DynamicCrateOperator op) {
    switch (op) {
    case DynamicCrateOperator::Eq:
        return QStringLiteral("==");
    case DynamicCrateOperator::NotEq:
        return QStringLiteral("!=");
    case DynamicCrateOperator::Contains:
        return QStringLiteral("~=");
    case DynamicCrateOperator::NotContains:
        return QStringLiteral("!~=");
    case DynamicCrateOperator::StartsWith:
        return QStringLiteral("^=");
    case DynamicCrateOperator::NotStartsWith:
        return QStringLiteral("!^=");
    case DynamicCrateOperator::Lt:
        return QStringLiteral("<");
    case DynamicCrateOperator::Lte:
        return QStringLiteral("<=");
    case DynamicCrateOperator::Gt:
        return QStringLiteral(">");
    case DynamicCrateOperator::Gte:
        return QStringLiteral(">=");
    }
    return QString();
}

QStringList dynamicCrateAvailableFields() {
    QStringList fields;
    static const DynamicCrateFieldSpec kFieldSpecs[] = {
            {"artist", "library.artist", DynamicCrateFieldType::Text},
            {"title", "library.title", DynamicCrateFieldType::Text},
            {"album", "library.album", DynamicCrateFieldType::Text},
            {"album_artist", "library.album_artist", DynamicCrateFieldType::Text},
            {"albumartist", "library.album_artist", DynamicCrateFieldType::Text},
            {"year", "CAST(substr(library.year,1,4) AS INTEGER)", DynamicCrateFieldType::Number},
            {"genre", "library.genre", DynamicCrateFieldType::Text},
            {"composer", "library.composer", DynamicCrateFieldType::Text},
            {"grouping", "library.grouping", DynamicCrateFieldType::Text},
            {"tracknumber", "CAST(library.tracknumber AS INTEGER)", DynamicCrateFieldType::Number},
            {"track_number", "CAST(library.tracknumber AS INTEGER)", DynamicCrateFieldType::Number},
            {"track", "CAST(library.tracknumber AS INTEGER)", DynamicCrateFieldType::Number},
            {"filetype", "library.filetype", DynamicCrateFieldType::Text},
            {"file_type", "library.filetype", DynamicCrateFieldType::Text},
            {"type", "library.filetype", DynamicCrateFieldType::Text},
            {"location", "track_locations.location", DynamicCrateFieldType::Text},
            {"native_location", "track_locations.location", DynamicCrateFieldType::Text},
            {"filepath", "track_locations.location", DynamicCrateFieldType::Text},
            {"directory", "track_locations.directory", DynamicCrateFieldType::Text},
            {"folder", "track_locations.directory", DynamicCrateFieldType::Text},
            {"filename", "track_locations.filename", DynamicCrateFieldType::Text},
            {"filesize", "track_locations.filesize", DynamicCrateFieldType::Number},
            {"file_size", "track_locations.filesize", DynamicCrateFieldType::Number},
            {"comment", "library.comment", DynamicCrateFieldType::Text},
            {"url", "library.url", DynamicCrateFieldType::Text},
            {"duration", "library.duration", DynamicCrateFieldType::Number},
            {"bitrate", "library.bitrate", DynamicCrateFieldType::Number},
            {"bpm", "library.bpm", DynamicCrateFieldType::Number},
            {"bpm_lock", "library.bpm_lock", DynamicCrateFieldType::Number},
            {"replaygain", "library.replaygain", DynamicCrateFieldType::Number},
            {"replay_gain", "library.replaygain", DynamicCrateFieldType::Number},
            {"date_added", "library.datetime_added", DynamicCrateFieldType::DateTime},
            {"datetime_added", "library.datetime_added", DynamicCrateFieldType::DateTime},
            {"datetimeadded", "library.datetime_added", DynamicCrateFieldType::DateTime},
            {"played", "library.played", DynamicCrateFieldType::Number},
            {"timesplayed", "library.timesplayed", DynamicCrateFieldType::Number},
            {"times_played", "library.timesplayed", DynamicCrateFieldType::Number},
            {"rating", "library.rating", DynamicCrateFieldType::Number},
            {"key", "library.key", DynamicCrateFieldType::Text},
            {"key_id", "library.key_id", DynamicCrateFieldType::Number},
            {"color", "library.color", DynamicCrateFieldType::Text},
            {"last_played", "library.last_played_at", DynamicCrateFieldType::DateTime},
            {"last_played_at", "library.last_played_at", DynamicCrateFieldType::DateTime},
            {"lastplayed", "library.last_played_at", DynamicCrateFieldType::DateTime},
            {"samplerate", "library.samplerate", DynamicCrateFieldType::Number},
            {"sample_rate", "library.samplerate", DynamicCrateFieldType::Number},
            {"channels", "library.channels", DynamicCrateFieldType::Number},
    };

    fields.reserve(static_cast<qsizetype>(std::size(kFieldSpecs)));
    for (const auto& fieldSpec : kFieldSpecs) {
        fields.append(QString::fromLatin1(fieldSpec.fieldName));
    }
    std::sort(fields.begin(), fields.end(), [](const QString& lhs, const QString& rhs) {
        return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
    });
    return fields;
}

bool dynamicCrateFieldTypeForField(
        const QString& fieldName,
        DynamicCrateFieldType* pFieldType) {
    const DynamicCrateFieldSpec* pFieldSpec = findFieldSpec(fieldName);
    if (pFieldSpec == nullptr || pFieldType == nullptr) {
        return false;
    }
    *pFieldType = pFieldSpec->fieldType;
    return true;
}

QList<DynamicCrateOperator> dynamicCrateSupportedOperators(
        DynamicCrateFieldType fieldType) {
    switch (fieldType) {
    case DynamicCrateFieldType::Text:
        return {
                DynamicCrateOperator::Eq,
                DynamicCrateOperator::NotEq,
                DynamicCrateOperator::Contains,
                DynamicCrateOperator::NotContains,
                DynamicCrateOperator::StartsWith,
                DynamicCrateOperator::NotStartsWith,
        };
    case DynamicCrateFieldType::Number:
    case DynamicCrateFieldType::DateTime:
        return {
                DynamicCrateOperator::Eq,
                DynamicCrateOperator::NotEq,
                DynamicCrateOperator::Lt,
                DynamicCrateOperator::Lte,
                DynamicCrateOperator::Gt,
                DynamicCrateOperator::Gte,
        };
    }
    return {};
}

QString serializeDynamicCrateRule(const DynamicCrateRule& rule) {
    const QString value = rule.fieldType == DynamicCrateFieldType::Text
            ? quoteRuleValue(rule.rawValue)
            : rule.rawValue;
    return QStringLiteral("%1 %2 %3")
            .arg(
                    rule.fieldName,
                    dynamicCrateOperatorToConfigString(rule.op),
                    value);
}

bool validateDynamicCrateRule(
        const DynamicCrateRule& rule,
        QString* pError) {
    const DynamicCrateFieldSpec* pFieldSpec = findFieldSpec(rule.fieldName);
    if (pFieldSpec == nullptr) {
        if (pError) {
            *pError = QStringLiteral("Unknown field %1").arg(rule.fieldName);
        }
        return false;
    }
    if (!validateOperatorForFieldType(pFieldSpec->fieldType, rule.op)) {
        if (pError) {
            *pError = QStringLiteral("Operator %1 is not supported for field %2")
                              .arg(
                                      dynamicCrateOperatorToConfigString(rule.op),
                                      QString::fromLatin1(pFieldSpec->fieldName));
        }
        return false;
    }

    const QString rawValue = rule.rawValue.trimmed();
    if (rawValue.isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Missing rule value");
        }
        return false;
    }

    switch (pFieldSpec->fieldType) {
    case DynamicCrateFieldType::Text:
        if (trimMatchingQuotes(rawValue).isEmpty()) {
            if (pError) {
                *pError = QStringLiteral("Text rule value is empty");
            }
            return false;
        }
        return true;
    case DynamicCrateFieldType::Number: {
        bool ok = false;
        trimMatchingQuotes(rawValue).toDouble(&ok);
        if (!ok) {
            if (pError) {
                *pError = QStringLiteral("Invalid numeric value %1").arg(rule.rawValue);
            }
            return false;
        }
        return true;
    }
    case DynamicCrateFieldType::DateTime: {
        ParsedDateTimeValue parsedDateTimeValue;
        if (!parseDateTimeValue(rawValue, &parsedDateTimeValue)) {
            if (pError) {
                *pError = QStringLiteral("Invalid date/time value %1").arg(rule.rawValue);
            }
            return false;
        }
        return true;
    }
    }
    return false;
}

bool validateDynamicCrateDefinition(
        const DynamicCrateDefinition& definition,
        QString* pError) {
    if (definition.sectionName.trimmed().isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Configuration section name is empty");
        }
        return false;
    }
    if (definition.name.trimmed().isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Dynamic crate name is empty");
        }
        return false;
    }
    if (definition.rules.isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("A dynamic crate needs at least one rule");
        }
        return false;
    }

    for (int i = 0; i < definition.rules.size(); ++i) {
        QString ruleError;
        if (!validateDynamicCrateRule(definition.rules.at(i), &ruleError)) {
            if (pError) {
                *pError = QStringLiteral("Rule %1 is invalid: %2")
                                  .arg(QString::number(i + 1), ruleError);
            }
            return false;
        }
    }
    return true;
}

QString sanitizeDynamicCrateSectionName(const QString& name) {
    QStringList words;
    QString currentWord;
    const QString normalized = name.trimmed().normalized(QString::NormalizationForm_KD);
    for (const QChar& ch : normalized) {
        switch (ch.category()) {
        case QChar::Mark_NonSpacing:
        case QChar::Mark_SpacingCombining:
        case QChar::Mark_Enclosing:
            continue;
        default:
            break;
        }

        if (ch.isLetterOrNumber() && ch.unicode() < 128) {
            currentWord.append(ch);
        } else if (!currentWord.isEmpty()) {
            words.append(currentWord);
            currentWord.clear();
        }
    }
    if (!currentWord.isEmpty()) {
        words.append(currentWord);
    }

    QString sectionName;
    for (QString word : std::as_const(words)) {
        word = cleanedAsciiToken(word);
        if (word.isEmpty()) {
            continue;
        }
        word[0] = word.at(0).toUpper();
        sectionName.append(word);
    }
    if (sectionName.isEmpty()) {
        sectionName = QStringLiteral("DynamicCrate");
    }
    if (sectionName.at(0).isDigit()) {
        sectionName.prepend(QStringLiteral("DynamicCrate"));
    }
    return sectionName;
}

QString uniqueDynamicCrateSectionName(
        const QString& preferredSectionName,
        const QList<DynamicCrateDefinition>& definitions,
        const QString& preservedSectionName) {
    QString baseSectionName = preferredSectionName.trimmed();
    if (baseSectionName.isEmpty()) {
        baseSectionName = QStringLiteral("DynamicCrate");
    }

    QString candidate = baseSectionName;
    int suffix = 2;
    auto hasConflict = [&](const QString& sectionName) {
        for (const auto& definition : definitions) {
            if (!preservedSectionName.isEmpty() &&
                    definition.sectionName.compare(
                            preservedSectionName,
                            Qt::CaseInsensitive) == 0) {
                continue;
            }
            if (definition.sectionName.compare(sectionName, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };

    while (hasConflict(candidate)) {
        candidate = QStringLiteral("%1%2")
                            .arg(baseSectionName, QString::number(suffix++));
    }
    return candidate;
}

bool saveDynamicCrateDefinitions(
        const QString& filePath,
        const QList<DynamicCrateDefinition>& definitions,
        QString* pError) {
    if (filePath.trimmed().isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Dynamic crate config file path is empty");
        }
        return false;
    }

    for (const auto& definition : definitions) {
        QString validationError;
        if (!validateDynamicCrateDefinition(definition, &validationError)) {
            if (pError) {
                *pError = QStringLiteral("Dynamic crate %1 is invalid: %2")
                                  .arg(definition.name, validationError);
            }
            return false;
        }
    }

    ConfigObject<ConfigValue> config(filePath);
    const QStringList groups = config.getGroups().values();
    for (const auto& group : groups) {
        const QList<ConfigKey> keys = config.getKeysWithGroup(group);
        for (const auto& key : keys) {
            config.remove(key);
        }
    }

    for (const auto& definition : definitions) {
        const QString group = QStringLiteral("[%1]").arg(definition.sectionName);
        config.setValue(ConfigKey(group, QStringLiteral("name")), definition.name);
        config.setValue(
                ConfigKey(group, QStringLiteral("match")),
                dynamicCrateMatchModeToConfigString(definition.matchMode));
        for (int i = 0; i < definition.rules.size(); ++i) {
            config.setValue(
                    ConfigKey(group, QStringLiteral("rule_%1").arg(i + 1)),
                    serializeDynamicCrateRule(definition.rules.at(i)));
        }
    }

    if (!config.save()) {
        if (pError) {
            *pError = QStringLiteral("Failed to write %1").arg(QFileInfo(filePath).fileName());
        }
        return false;
    }
    return true;
}

QList<DynamicCrateDefinition> DynamicCrateConfigLoader::load() const {
    QList<DynamicCrateDefinition> definitions;
    if (m_filePath.isEmpty() || !QFile::exists(m_filePath)) {
        return definitions;
    }

    ConfigObject<ConfigValue> config(m_filePath);
    QStringList groups = config.getGroups().values();
    std::sort(groups.begin(), groups.end());

    for (const QString& group : groups) {
        const QString sectionName = stripGroupBrackets(group);
        DynamicCrateDefinition definition;
        definition.sectionName = sectionName;
        definition.name = config.getValueString(ConfigKey(group, QStringLiteral("name"))).trimmed();
        if (definition.name.isEmpty()) {
            qWarning() << "Ignoring dynamic crate" << sectionName
                       << "because it is missing a name";
            continue;
        }

        if (!parseMatchMode(
                    config.getValueString(ConfigKey(group, QStringLiteral("match"))),
                    &definition.matchMode)) {
            qWarning() << "Ignoring dynamic crate" << sectionName
                       << "because it has an invalid match mode";
            continue;
        }

        QList<ConfigKey> groupKeys = config.getKeysWithGroup(group);
        std::sort(groupKeys.begin(), groupKeys.end(), [](const ConfigKey& lhs, const ConfigKey& rhs) {
            return ruleItemIndex(lhs.item) < ruleItemIndex(rhs.item);
        });

        bool hasRuleError = false;
        for (const ConfigKey& key : groupKeys) {
            if (ruleItemIndex(key.item) < 0) {
                continue;
            }

            DynamicCrateRule rule;
            QString parseError;
            if (!parseRuleValue(config.getValueString(key), &rule, &parseError)) {
                qWarning() << "Ignoring dynamic crate" << sectionName
                           << "because rule" << key.item << "is invalid:" << parseError;
                hasRuleError = true;
                break;
            }
            definition.rules.append(rule);
        }

        if (hasRuleError) {
            continue;
        }
        if (definition.rules.isEmpty()) {
            qWarning() << "Ignoring dynamic crate" << sectionName
                       << "because it has no rules";
            continue;
        }

        definitions.append(definition);
    }

    return definitions;
}

QString DynamicCrateConfigLoader::defaultConfigFilePath(
        const UserSettings& userSettings) {
    return QDir(userSettings.getSettingsPath()).filePath(kConfigFileName);
}

bool DynamicCrateSqlBuilder::buildWhereClause(
        const DynamicCrateDefinition& definition,
        QString* pWhereClause,
        QString* pError) const {
    VERIFY_OR_DEBUG_ASSERT(pWhereClause != nullptr) {
        return false;
    }
    if (!definition.isValid()) {
        if (pError) {
            *pError = QStringLiteral("Definition is invalid");
        }
        return false;
    }

    FieldEscaper fieldEscaper(m_database);
    QStringList clauses;
    clauses.reserve(definition.rules.size());
    for (const auto& rule : definition.rules) {
        QString clause;
        if (!buildRuleClause(rule, fieldEscaper, &clause, pError)) {
            return false;
        }
        clauses.append(QChar('(') + clause + QChar(')'));
    }

    if (clauses.isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("No valid rule clauses were generated");
        }
        return false;
    }

    const QString joinOperator = definition.matchMode == DynamicCrateMatchMode::Any
            ? QStringLiteral(" OR ")
            : QStringLiteral(" AND ");
    *pWhereClause = clauses.join(joinOperator);
    return true;
}

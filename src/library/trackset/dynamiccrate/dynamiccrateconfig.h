#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

#include "preferences/usersettings.h"

enum class DynamicCrateMatchMode {
    Any,
    All,
};

enum class DynamicCrateFieldType {
    Text,
    Number,
    DateTime,
};

enum class DynamicCrateOperator {
    Eq,
    NotEq,
    Contains,
    NotContains,
    StartsWith,
    NotStartsWith,
    Lt,
    Lte,
    Gt,
    Gte,
};

struct DynamicCrateRule {
    QString fieldName;
    DynamicCrateFieldType fieldType;
    DynamicCrateOperator op;
    QString rawValue;
};

struct DynamicCrateDefinition {
    QString sectionName;
    QString name;
    DynamicCrateMatchMode matchMode;
    QList<DynamicCrateRule> rules;

    bool isValid() const {
        return !sectionName.isEmpty() &&
                !name.isEmpty() &&
                !rules.isEmpty();
    }
};

class DynamicCrateConfigLoader {
  public:
    explicit DynamicCrateConfigLoader(QString filePath)
            : m_filePath(std::move(filePath)) {
    }

    QList<DynamicCrateDefinition> load() const;

    static QString defaultConfigFilePath(const UserSettings& userSettings);

  private:
    QString m_filePath;
};

class DynamicCrateSqlBuilder {
  public:
    explicit DynamicCrateSqlBuilder(QSqlDatabase database)
            : m_database(std::move(database)) {
    }

    bool buildWhereClause(
            const DynamicCrateDefinition& definition,
            QString* pWhereClause,
            QString* pError = nullptr) const;

  private:
    QSqlDatabase m_database;
};

QString dynamicCrateMatchModeToConfigString(DynamicCrateMatchMode matchMode);
QString dynamicCrateOperatorToConfigString(DynamicCrateOperator op);
QStringList dynamicCrateAvailableFields();
bool dynamicCrateFieldTypeForField(
        const QString& fieldName,
        DynamicCrateFieldType* pFieldType);
QList<DynamicCrateOperator> dynamicCrateSupportedOperators(
        DynamicCrateFieldType fieldType);
QString serializeDynamicCrateRule(const DynamicCrateRule& rule);
bool validateDynamicCrateRule(
        const DynamicCrateRule& rule,
        QString* pError = nullptr);
bool validateDynamicCrateDefinition(
        const DynamicCrateDefinition& definition,
        QString* pError = nullptr);
QString sanitizeDynamicCrateSectionName(const QString& name);
QString uniqueDynamicCrateSectionName(
        const QString& preferredSectionName,
        const QList<DynamicCrateDefinition>& definitions,
        const QString& preservedSectionName = QString());
bool saveDynamicCrateDefinitions(
        const QString& filePath,
        const QList<DynamicCrateDefinition>& definitions,
        QString* pError = nullptr);

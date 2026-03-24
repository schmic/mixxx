#include <algorithm>
#include <QFile>
#include <QSqlDatabase>
#include <QTextStream>

#include "library/trackset/dynamiccrate/dynamiccrateconfig.h"
#include "test/mixxxtest.h"

namespace {

class DynamicCrateConfigTest : public MixxxTest {
  protected:
    QString writeConfigFile(const QString& fileContents) const {
        const QString filePath = getTestDataDir().filePath("dynamic_crates.cfg");
        QFile file(filePath);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream stream(&file);
        stream << fileContents;
        file.close();
        return filePath;
    }
};

TEST_F(DynamicCrateConfigTest, LoadValidDefinitionsAndIgnoreInvalidOnes) {
    const QString filePath = writeConfigFile(QStringLiteral(
            "[DynamicCrate_1]\n"
            "name House\n"
            "match any\n"
            "rule_1 genre == \"House\"\n"
            "rule_2 comment ~= \"#house\"\n"
            "\n"
            "[DynamicCrate_2]\n"
            "name Broken\n"
            "match all\n"
            "rule_1 unknown_field == \"X\"\n"));

    DynamicCrateConfigLoader loader(filePath);
    const QList<DynamicCrateDefinition> definitions = loader.load();

    ASSERT_EQ(1, definitions.size());
    EXPECT_QSTRING_EQ("DynamicCrate_1", definitions.first().sectionName);
    EXPECT_QSTRING_EQ("House", definitions.first().name);
    EXPECT_EQ(DynamicCrateMatchMode::Any, definitions.first().matchMode);
    ASSERT_EQ(2, definitions.first().rules.size());
    EXPECT_QSTRING_EQ("genre", definitions.first().rules.first().fieldName);
    EXPECT_EQ(DynamicCrateOperator::Eq, definitions.first().rules.first().op);
}

TEST_F(DynamicCrateConfigTest, BuildSqlForTextAndDateRules) {
    const QString filePath = writeConfigFile(QStringLiteral(
            "[DynamicCrate_1]\n"
            "name RecentlyAddedHouse\n"
            "match all\n"
            "rule_1 genre ^= \"House\"\n"
            "rule_2 date_added >= now-7d\n"));

    DynamicCrateConfigLoader loader(filePath);
    const QList<DynamicCrateDefinition> definitions = loader.load();
    ASSERT_EQ(1, definitions.size());

    const QString connectionName =
            QStringLiteral("dynamiccrate_sql_test_%1").arg(reinterpret_cast<quintptr>(this));
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(QStringLiteral(":memory:"));
        ASSERT_TRUE(database.open());

        DynamicCrateSqlBuilder builder(database);
        QString whereClause;
        QString error;
        EXPECT_TRUE(builder.buildWhereClause(definitions.first(), &whereClause, &error))
                << error.toStdString();
        EXPECT_TRUE(whereClause.contains(QStringLiteral("lower(library.genre) LIKE lower(")));
        EXPECT_TRUE(whereClause.contains(QStringLiteral("library.datetime_added >=")));
        EXPECT_TRUE(whereClause.contains(QStringLiteral(" AND ")));

        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

TEST_F(DynamicCrateConfigTest, BuildSqlForDateEqualityAndNegatedNumericOperator) {
    const QString filePath = writeConfigFile(QStringLiteral(
            "[DynamicCrate_1]\n"
            "name RecentFavorites\n"
            "match all\n"
            "rule_1 date_added == 2026-03-24\n"
            "rule_2 times_played !< 5\n"));

    DynamicCrateConfigLoader loader(filePath);
    const QList<DynamicCrateDefinition> definitions = loader.load();
    ASSERT_EQ(1, definitions.size());

    const QString connectionName =
            QStringLiteral("dynamiccrate_sql_test_date_%1").arg(reinterpret_cast<quintptr>(this));
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(QStringLiteral(":memory:"));
        ASSERT_TRUE(database.open());

        DynamicCrateSqlBuilder builder(database);
        QString whereClause;
        QString error;
        EXPECT_TRUE(builder.buildWhereClause(definitions.first(), &whereClause, &error))
                << error.toStdString();
        EXPECT_TRUE(whereClause.contains(QStringLiteral("library.datetime_added >=")));
        EXPECT_TRUE(whereClause.contains(QStringLiteral("library.datetime_added <")));
        EXPECT_TRUE(whereClause.contains(QStringLiteral("library.timesplayed >= 5")));
        EXPECT_FALSE(whereClause.contains(QStringLiteral("T00:00:00")));

        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

TEST_F(DynamicCrateConfigTest, BuildSqlForTrackLocationAndAdditionalLibraryFields) {
    const QString filePath = writeConfigFile(QStringLiteral(
            "[DynamicCrate_1]\n"
            "name FileBacked\n"
            "match all\n"
            "rule_1 filename ^= \"mix\"\n"
            "rule_2 directory ~= \"/Music/\"\n"
            "rule_3 channels == 2\n"
            "rule_4 bpm_lock == 1\n"));

    DynamicCrateConfigLoader loader(filePath);
    const QList<DynamicCrateDefinition> definitions = loader.load();
    ASSERT_EQ(1, definitions.size());

    const QString connectionName =
            QStringLiteral("dynamiccrate_sql_test_location_%1").arg(reinterpret_cast<quintptr>(this));
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(QStringLiteral(":memory:"));
        ASSERT_TRUE(database.open());

        DynamicCrateSqlBuilder builder(database);
        QString whereClause;
        QString error;
        EXPECT_TRUE(builder.buildWhereClause(definitions.first(), &whereClause, &error))
                << error.toStdString();
        EXPECT_TRUE(whereClause.contains(QStringLiteral("track_locations.filename")));
        EXPECT_TRUE(whereClause.contains(QStringLiteral("track_locations.directory")));
        EXPECT_TRUE(whereClause.contains(QStringLiteral("library.channels = 2")));
        EXPECT_TRUE(whereClause.contains(QStringLiteral("library.bpm_lock = 1")));

        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

TEST_F(DynamicCrateConfigTest, SanitizeAndUniquifySectionNames) {
    EXPECT_QSTRING_EQ("FooCrate", sanitizeDynamicCrateSectionName(" Foo Crate "));
    EXPECT_QSTRING_EQ("AOUSet", sanitizeDynamicCrateSectionName("ÄÖÜ set"));
    EXPECT_QSTRING_EQ("DynamicCrate42", sanitizeDynamicCrateSectionName("42"));

    DynamicCrateDefinition existing;
    existing.sectionName = "FooCrate";
    existing.name = "Foo Crate";
    existing.matchMode = DynamicCrateMatchMode::Any;
    existing.rules.append({"genre", DynamicCrateFieldType::Text, DynamicCrateOperator::Eq, "House"});

    const QList<DynamicCrateDefinition> definitions = {existing};
    EXPECT_QSTRING_EQ(
            "FooCrate2",
            uniqueDynamicCrateSectionName("FooCrate", definitions));
    EXPECT_QSTRING_EQ(
            "FooCrate",
            uniqueDynamicCrateSectionName("FooCrate", definitions, "FooCrate"));
}

TEST_F(DynamicCrateConfigTest, AvailableFieldsAreSortedAlphabetically) {
    const QStringList fields = dynamicCrateAvailableFields();
    QStringList sortedFields = fields;
    std::sort(sortedFields.begin(), sortedFields.end(), [](const QString& lhs, const QString& rhs) {
        return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
    });
    EXPECT_EQ(sortedFields, fields);
}

TEST_F(DynamicCrateConfigTest, SaveAndReloadDefinitionsRoundTrip) {
    const QString filePath = getTestDataDir().filePath("dynamic_crates.cfg");

    DynamicCrateDefinition definition;
    definition.sectionName = "RecentHouse";
    definition.name = "Recent House";
    definition.matchMode = DynamicCrateMatchMode::All;
    definition.rules = {
            {"genre", DynamicCrateFieldType::Text, DynamicCrateOperator::Contains, "Deep House"},
            {"date_added", DynamicCrateFieldType::DateTime, DynamicCrateOperator::Gte, "now-7d"},
            {"rating", DynamicCrateFieldType::Number, DynamicCrateOperator::Gte, "4"},
    };

    QString error;
    ASSERT_TRUE(saveDynamicCrateDefinitions(filePath, {definition}, &error))
            << error.toStdString();

    DynamicCrateConfigLoader loader(filePath);
    const QList<DynamicCrateDefinition> loadedDefinitions = loader.load();
    ASSERT_EQ(1, loadedDefinitions.size());
    EXPECT_QSTRING_EQ("RecentHouse", loadedDefinitions.first().sectionName);
    EXPECT_QSTRING_EQ("Recent House", loadedDefinitions.first().name);
    EXPECT_EQ(DynamicCrateMatchMode::All, loadedDefinitions.first().matchMode);
    ASSERT_EQ(3, loadedDefinitions.first().rules.size());
    EXPECT_QSTRING_EQ("genre", loadedDefinitions.first().rules.at(0).fieldName);
    EXPECT_EQ(DynamicCrateOperator::Contains, loadedDefinitions.first().rules.at(0).op);
    EXPECT_QSTRING_EQ("Deep House", loadedDefinitions.first().rules.at(0).rawValue);
    EXPECT_QSTRING_EQ("now-7d", loadedDefinitions.first().rules.at(1).rawValue);
    EXPECT_QSTRING_EQ("4", loadedDefinitions.first().rules.at(2).rawValue);
}

} // namespace

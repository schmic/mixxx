#include "controllers/controllermappinginfoenumerator.h"

#include <gtest/gtest.h>

#include <QFile>
#include <QFileInfo>

#include "controllers/defs_controllers.h"
#include "test/mixxxtest.h"

namespace {

constexpr auto kMappingXml = R"(<?xml version="1.0" encoding="utf-8"?>
<MixxxControllerPreset>
    <info>
        <name>%1</name>
    </info>
</MixxxControllerPreset>
)";

void writeMappingFile(const QString& path, const QString& name) {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(file.write(QString(kMappingXml).arg(name).toUtf8()),
            QString(kMappingXml).arg(name).toUtf8().size());
}

} // namespace

class MappingInfoEnumeratorTest : public MixxxTest {
};

TEST_F(MappingInfoEnumeratorTest, EnumeratesMappingsInSubdirectories) {
    const QString rootPath = getTestDataDir().filePath("controllers");
    const QString nestedDirPath = getTestDataDir().filePath("controllers/vendor/device");
    ASSERT_TRUE(QDir().mkpath(nestedDirPath));

    const QString rootMidiPath = getTestDataDir().filePath("controllers/root.midi.xml");
    const QString nestedMidiPath =
            getTestDataDir().filePath("controllers/vendor/device/nested.midi.xml");
    const QString nestedHidPath =
            getTestDataDir().filePath("controllers/vendor/device/nested.hid.xml");
    const QString nestedBulkPath =
            getTestDataDir().filePath("controllers/vendor/device/nested.bulk.xml");
    const QString ignoredPath =
            getTestDataDir().filePath("controllers/vendor/device/ignored.txt");

    writeMappingFile(rootMidiPath, QStringLiteral("Root MIDI"));
    writeMappingFile(nestedMidiPath, QStringLiteral("Nested MIDI"));
    writeMappingFile(nestedHidPath, QStringLiteral("Nested HID"));
    writeMappingFile(nestedBulkPath, QStringLiteral("Nested BULK"));
    writeMappingFile(ignoredPath, QStringLiteral("Ignored"));

    MappingInfoEnumerator enumerator(rootPath);

    const QList<MappingInfo> midiMappings =
            enumerator.getMappingsByExtension(MIDI_MAPPING_EXTENSION);
    ASSERT_EQ(2, midiMappings.size());
    EXPECT_EQ(QFileInfo(rootMidiPath).absoluteFilePath(), midiMappings[0].getPath());
    EXPECT_EQ(QFileInfo(nestedMidiPath).absoluteFilePath(), midiMappings[1].getPath());

    const QList<MappingInfo> hidMappings =
            enumerator.getMappingsByExtension(HID_MAPPING_EXTENSION);
    ASSERT_EQ(1, hidMappings.size());
    EXPECT_EQ(QFileInfo(nestedHidPath).absoluteFilePath(), hidMappings[0].getPath());

    const QList<MappingInfo> bulkMappings =
            enumerator.getMappingsByExtension(BULK_MAPPING_EXTENSION);
    ASSERT_EQ(1, bulkMappings.size());
    EXPECT_EQ(QFileInfo(nestedBulkPath).absoluteFilePath(), bulkMappings[0].getPath());
}

#ifdef Q_OS_LINUX
TEST_F(MappingInfoEnumeratorTest, EnumeratesMappingsInSymlinkedSubdirectories) {
    const QString rootPath = getTestDataDir().filePath(QStringLiteral("controllers"));
    const QString targetPath = getTestDataDir().filePath(QStringLiteral("external/vendor/device"));
    const QString linkPath = QDir(rootPath).filePath(QStringLiteral("linked"));
    ASSERT_TRUE(QDir().mkpath(rootPath));
    ASSERT_TRUE(QDir().mkpath(targetPath));
    ASSERT_TRUE(QFile::link(QDir(targetPath).absolutePath(), linkPath));
    ASSERT_TRUE(QFile::link(rootPath, QDir(targetPath).filePath(QStringLiteral("loop"))));
    ASSERT_TRUE(QFile::link(getTestDataDir().filePath(QStringLiteral("missing")),
            QDir(rootPath).filePath(QStringLiteral("broken.midi.xml"))));

    for (const QString& extension :
            {QStringLiteral(MIDI_MAPPING_EXTENSION),
                    QStringLiteral(HID_MAPPING_EXTENSION),
                    QStringLiteral(BULK_MAPPING_EXTENSION)}) {
        const QString fileName = QStringLiteral("nested") + extension;
        writeMappingFile(QDir(targetPath).filePath(fileName), QStringLiteral("Linked mapping"));
    }

    MappingInfoEnumerator enumerator(rootPath);

    for (const QString& extension :
            {QStringLiteral(MIDI_MAPPING_EXTENSION),
                    QStringLiteral(HID_MAPPING_EXTENSION),
                    QStringLiteral(BULK_MAPPING_EXTENSION)}) {
        const QList<MappingInfo> mappings = enumerator.getMappingsByExtension(extension);
        ASSERT_EQ(1, mappings.size());
        EXPECT_EQ(QDir(linkPath).filePath(QStringLiteral("nested") + extension),
                mappings[0].getPath());
        EXPECT_EQ(QDir(linkPath).absolutePath(), mappings[0].getDirPath());
        EXPECT_TRUE(mappings[0].isValid());
    }
}

TEST_F(MappingInfoEnumeratorTest, EnumeratesSymlinkedMappingFiles) {
    const QString rootPath = getTestDataDir().filePath(QStringLiteral("controllers/vendor"));
    const QString targetPath = getTestDataDir().filePath(QStringLiteral("external.xml"));
    const QString linkPath = QDir(rootPath).filePath(QStringLiteral("linked.midi.xml"));
    ASSERT_TRUE(QDir().mkpath(rootPath));
    writeMappingFile(targetPath, QStringLiteral("Linked MIDI"));
    ASSERT_TRUE(QFile::link(targetPath, linkPath));

    MappingInfoEnumerator enumerator(
            getTestDataDir().filePath(QStringLiteral("controllers")));

    const QList<MappingInfo> mappings =
            enumerator.getMappingsByExtension(MIDI_MAPPING_EXTENSION);
    ASSERT_EQ(1, mappings.size());
    EXPECT_EQ(linkPath, mappings[0].getPath());
    EXPECT_EQ(rootPath, mappings[0].getDirPath());
    EXPECT_TRUE(mappings[0].isValid());
}
#endif

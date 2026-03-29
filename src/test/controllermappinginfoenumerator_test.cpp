#include <gtest/gtest.h>

#include <QFile>
#include <QFileInfo>

#include "controllers/defs_controllers.h"
#include "controllers/controllermappinginfoenumerator.h"
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

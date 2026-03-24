#include "library/trackset/dynamiccrate/dynamiccratetablemodel.h"

#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QSqlQuery>

#include "library/basetrackcache.h"
#include "library/dao/trackdao.h"
#include "test/librarytest.h"
#include "track/track.h"

namespace {
class DynamicCrateTableModelTest : public LibraryTest {
  protected:
    DynamicCrateTableModelTest() {
        const QStringList columns = {"id", "artist", "title", "genre", "comment"};
        QSqlQuery query(internalCollection()->database());
        EXPECT_TRUE(query.exec(
                "CREATE TEMPORARY VIEW dynamic_test_cache AS "
                "SELECT id, artist, title, genre, comment FROM library"));
        internalCollection()->connectTrackSource(QSharedPointer<BaseTrackCache>::create(
                internalCollection(), "dynamic_test_cache", "id", columns, QStringList{"artist", "title", "genre", "comment"}, true));
    }

    TrackPointer addTrack(const QString& file, const QString& title, const QString& genre) {
        auto track = getOrAddTrackByLocation(getTestDir().filePath(file));
        EXPECT_TRUE(track);
        if (track) {
            track->setTitle(title);
            track->setArtist("Same artist");
            track->updateGenre(genre);
            EXPECT_TRUE(internalCollection()->getTrackDAO().saveTrack(track.get()));
        }
        if (!track) {
            return {};
        }
        const auto id = track->getId();
        track.reset();
        return trackCollectionManager()->getTrackById(id);
    }

    DynamicCrateDefinition definition() const {
        return {"DynamicCrate_1", "House", DynamicCrateMatchMode::All, {{"genre", DynamicCrateFieldType::Text, DynamicCrateOperator::Eq, "House"}}};
    }

    TrackPointer firstTrack() {
        return addTrack("id3-test-data/cover-test-øé~ł€˚-png.mp3", "Alpha", "House");
    }

    TrackPointer secondTrack(const QString& genre = "House") {
        return addTrack("id3-test-data/cover-test-øé~ł€˚-vbr.mp3", "Beta", genre);
    }
};

TEST_F(DynamicCrateTableModelTest, UnchangedMembershipPreservesRowsAndPersistentSelection) {
    const auto first = firstTrack();
    const auto second = secondTrack();
    ASSERT_TRUE(first && second);
    DynamicCrateTableModel model(nullptr, trackCollectionManager());
    ASSERT_TRUE(model.selectDynamicCrate(definition()));
    ASSERT_EQ(2, model.rowCount());
    model.sort(model.fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_GENRE), Qt::AscendingOrder);
    const auto selected = QPersistentModelIndex(model.index(1, 0));
    const auto selectedId = model.getTrackId(selected);
    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy moved(&model, &QAbstractItemModel::rowsMoved);
    QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
    second->setComment("Loaded into preview");
    ASSERT_TRUE(model.refreshSelectedDynamicCrate({second->getId()}));
    ASSERT_TRUE(model.refreshSelectedDynamicCrate());
    EXPECT_EQ(0, inserted.count());
    EXPECT_EQ(0, removed.count());
    EXPECT_EQ(0, moved.count());
    EXPECT_EQ(0, reset.count());
    EXPECT_EQ(1, selected.row());
    EXPECT_EQ(selectedId, model.getTrackId(selected));
    EXPECT_EQ(first->getId(), model.getTrackId(model.index(0, 0)));
    model.select();
    EXPECT_EQ(first->getId(), model.getTrackId(model.index(0, 0)));
}

TEST_F(DynamicCrateTableModelTest, AddsPreviouslyExcludedTrackAndRemovesOnlyChangedTrack) {
    const auto first = firstTrack();
    const auto second = secondTrack("Techno");
    ASSERT_TRUE(first && second);
    DynamicCrateTableModel model(nullptr, trackCollectionManager());
    ASSERT_TRUE(model.selectDynamicCrate(definition()));
    ASSERT_EQ(1, model.rowCount());
    const auto selected = QPersistentModelIndex(model.index(0, 0));
    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
    second->updateGenre("House");
    ASSERT_TRUE(internalCollection()->getTrackDAO().saveTrack(second.get()));
    ASSERT_TRUE(model.refreshSelectedDynamicCrate({second->getId()}));
    ASSERT_EQ(2, model.rowCount());
    EXPECT_EQ(1, inserted.count());
    EXPECT_EQ(first->getId(), model.getTrackId(selected));
    second->updateGenre("Techno");
    ASSERT_TRUE(internalCollection()->getTrackDAO().saveTrack(second.get()));
    ASSERT_TRUE(model.refreshSelectedDynamicCrate({second->getId()}));
    EXPECT_EQ(1, model.rowCount());
    EXPECT_EQ(1, removed.count());
    EXPECT_EQ(first->getId(), model.getTrackId(selected));
}

TEST_F(DynamicCrateTableModelTest, SortKeyChangeMovesRowAndPreservesItsPersistentIndex) {
    const auto first = firstTrack();
    const auto second = secondTrack();
    ASSERT_TRUE(first && second);
    DynamicCrateTableModel model(nullptr, trackCollectionManager());
    ASSERT_TRUE(model.selectDynamicCrate(definition()));
    model.sort(model.fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_TITLE), Qt::AscendingOrder);
    const auto selected = QPersistentModelIndex(model.index(1, 0));
    ASSERT_EQ(second->getId(), model.getTrackId(selected));
    QSignalSpy moved(&model, &QAbstractItemModel::rowsMoved);
    second->setTitle("Aardvark");
    ASSERT_TRUE(model.refreshSelectedDynamicCrate({second->getId()}));
    EXPECT_EQ(1, moved.count());
    EXPECT_EQ(0, selected.row());
    EXPECT_EQ(second->getId(), model.getTrackId(selected));
}

TEST_F(DynamicCrateTableModelTest, RefreshRespectsSearchAndHandlesAnEmptyCrate) {
    const auto first = firstTrack();
    const auto second = secondTrack();
    ASSERT_TRUE(first && second);
    DynamicCrateTableModel model(nullptr, trackCollectionManager());
    ASSERT_TRUE(model.selectDynamicCrate(definition()));
    model.search("title:Alpha");
    ASSERT_EQ(1, model.rowCount());
    second->setTitle("Alpha Two");
    ASSERT_TRUE(model.refreshSelectedDynamicCrate({second->getId()}));
    ASSERT_EQ(2, model.rowCount());
    second->setTitle("Beta");
    ASSERT_TRUE(model.refreshSelectedDynamicCrate({second->getId()}));
    ASSERT_EQ(1, model.rowCount());
    first->updateGenre("Techno");
    ASSERT_TRUE(internalCollection()->getTrackDAO().saveTrack(first.get()));
    ASSERT_TRUE(model.refreshSelectedDynamicCrate());
    ASSERT_EQ(0, model.rowCount());
    first->updateGenre("House");
    ASSERT_TRUE(internalCollection()->getTrackDAO().saveTrack(first.get()));
    ASSERT_TRUE(model.refreshSelectedDynamicCrate({first->getId()}));
    ASSERT_EQ(1, model.rowCount());
    EXPECT_EQ(first->getId(), model.getTrackId(model.index(0, 0)));
}

}

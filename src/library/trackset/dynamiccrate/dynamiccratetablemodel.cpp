#include "library/trackset/dynamiccrate/dynamiccratetablemodel.h"

#include <QSqlQuery>

#include "library/dao/trackschema.h"
#include "library/queryutil.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "moc_dynamiccratetablemodel.cpp"

namespace {

const QString kModelName = QStringLiteral("dynamiccrate");

QString viewNameForSectionName(const QString& sectionName) {
    return QStringLiteral("dynamic_crate_%1")
            .arg(QString::number(qHash(sectionName), 16));
}

} // namespace

DynamicCrateTableModel::DynamicCrateTableModel(
        QObject* parent,
        TrackCollectionManager* pTrackCollectionManager)
        : TrackSetTableModel(
                  parent,
                  pTrackCollectionManager,
                  "mixxx.db.model.dynamiccrate") {
}

bool DynamicCrateTableModel::selectDynamicCrate(
        const DynamicCrateDefinition& definition,
        bool forceRefresh) {
    if (!definition.isValid()) {
        qWarning() << "Refusing to select invalid dynamic crate definition";
        return false;
    }

    if (!forceRefresh && definition.sectionName == m_selectedDefinition.sectionName) {
        return true;
    }

    if (m_selectedDefinition.isValid()) {
        const QString currentSearchText = currentSearch();
        if (!currentSearchText.trimmed().isEmpty()) {
            m_searchTexts.insert(m_selectedDefinition.sectionName, currentSearchText);
        } else {
            m_searchTexts.remove(m_selectedDefinition.sectionName);
        }
    }

    m_selectedDefinition = definition;
    if (!recreateSelectedView()) {
        return false;
    }

    QStringList columns;
    columns << LIBRARYTABLE_ID
            << LIBRARYTABLE_PREVIEW
            << LIBRARYTABLE_COVERART;
    setTable(currentViewName(),
            LIBRARYTABLE_ID,
            columns,
            m_pTrackCollectionManager->internalCollection()->getTrackSource());
    setSearch(m_searchTexts.value(m_selectedDefinition.sectionName));
    setDefaultSort(fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_ARTIST), Qt::AscendingOrder);
    select();
    return true;
}

bool DynamicCrateTableModel::refreshSelectedDynamicCrate() {
    if (!m_selectedDefinition.isValid()) {
        return false;
    }
    if (!recreateSelectedView()) {
        return false;
    }
    select();
    return true;
}

bool DynamicCrateTableModel::recreateSelectedView() {
    VERIFY_OR_DEBUG_ASSERT(m_selectedDefinition.isValid()) {
        return false;
    }

    QString whereClause;
    DynamicCrateSqlBuilder sqlBuilder(m_database);
    QString buildError;
    if (!sqlBuilder.buildWhereClause(m_selectedDefinition, &whereClause, &buildError)) {
        qWarning() << "Failed to build SQL for dynamic crate"
                   << m_selectedDefinition.sectionName << ":" << buildError;
        return false;
    }

    const QString viewName = currentViewName();
    QSqlQuery dropViewQuery(m_database);
    dropViewQuery.prepare(QStringLiteral("DROP VIEW IF EXISTS %1").arg(viewName));
    if (!dropViewQuery.exec()) {
        LOG_FAILED_QUERY(dropViewQuery);
        return false;
    }

    const QString createViewQueryString = QStringLiteral(
            "CREATE TEMPORARY VIEW %1 AS "
            "SELECT library.id AS %2, "
            "'' AS %3, "
            "library.coverart_digest AS %4 "
            "FROM library "
            "INNER JOIN track_locations "
            "ON library.location = track_locations.id "
            "WHERE library.mixxx_deleted = 0 "
            "AND track_locations.fs_deleted = 0 "
            "AND (%5)")
                                                  .arg(viewName,
                                                          LIBRARYTABLE_ID,
                                                          LIBRARYTABLE_PREVIEW,
                                                          LIBRARYTABLE_COVERART,
                                                          whereClause);
    QSqlQuery createViewQuery(m_database);
    createViewQuery.prepare(createViewQueryString);
    if (!createViewQuery.exec()) {
        LOG_FAILED_QUERY(createViewQuery);
        return false;
    }

    return true;
}

QString DynamicCrateTableModel::currentViewName() const {
    if (!m_selectedDefinition.isValid()) {
        return QStringLiteral("dynamic_crate_empty");
    }
    return viewNameForSectionName(m_selectedDefinition.sectionName);
}

TrackModel::Capabilities DynamicCrateTableModel::getCapabilities() const {
    return Capability::AddToAutoDJ |
            Capability::EditMetadata |
            Capability::LoadToDeck |
            Capability::LoadToSampler |
            Capability::LoadToPreviewDeck |
            Capability::Hide |
            Capability::ResetPlayed |
            Capability::RemoveFromDisk |
            Capability::Analyze |
            Capability::Properties |
            Capability::Sorting;
}

void DynamicCrateTableModel::removeTracks(const QModelIndexList& indices) {
    Q_UNUSED(indices);
    qWarning() << "Refusing to remove tracks directly from a dynamic crate";
}

int DynamicCrateTableModel::addTracksWithTrackIds(
        const QModelIndex& index, const QList<TrackId>& tracks, int* pOutInsertionPos) {
    Q_UNUSED(index);
    Q_UNUSED(tracks);
    if (pOutInsertionPos != nullptr) {
        *pOutInsertionPos = 0;
    }
    return 0;
}

bool DynamicCrateTableModel::isLocked() {
    return true;
}

QString DynamicCrateTableModel::modelKey(bool noSearch) const {
    if (!m_selectedDefinition.isValid()) {
        return noSearch ? kModelName : kModelName + QChar('#') + currentSearch();
    }
    if (noSearch) {
        return kModelName + QChar(':') + m_selectedDefinition.sectionName;
    }
    return kModelName + QChar(':') +
            m_selectedDefinition.sectionName +
            QChar('#') +
            currentSearch();
}

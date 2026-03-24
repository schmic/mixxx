#pragma once

#include <QHash>

#include "library/trackset/dynamiccrate/dynamiccrateconfig.h"
#include "library/trackset/tracksettablemodel.h"

class DynamicCrateTableModel final : public TrackSetTableModel {
    Q_OBJECT

  public:
    DynamicCrateTableModel(
            QObject* parent,
            TrackCollectionManager* pTrackCollectionManager);
    ~DynamicCrateTableModel() final = default;

    bool selectDynamicCrate(
            const DynamicCrateDefinition& definition,
            bool forceRefresh = false);
    bool refreshSelectedDynamicCrate();

    bool hasSelectedDynamicCrate() const {
        return m_selectedDefinition.isValid();
    }
    QString selectedSectionName() const {
        return m_selectedDefinition.sectionName;
    }

    void removeTracks(const QModelIndexList& indices) final;
    int addTracksWithTrackIds(
            const QModelIndex& index,
            const QList<TrackId>& tracks,
            int* pOutInsertionPos) final;
    bool isLocked() final;

    Capabilities getCapabilities() const final;
    QString modelKey(bool noSearch) const override;

  private:
    bool recreateSelectedView();
    QString currentViewName() const;

    DynamicCrateDefinition m_selectedDefinition;
    QHash<QString, QString> m_searchTexts;
};

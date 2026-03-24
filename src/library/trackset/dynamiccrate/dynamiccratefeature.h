#pragma once

#include <QSet>
#include <QModelIndex>
#include <QPointer>
#include <QUrl>

#include "library/trackset/basetracksetfeature.h"
#include "library/trackset/dynamiccrate/dynamiccrateconfig.h"
#include "library/trackset/dynamiccrate/dynamiccratetablemodel.h"
#include "track/trackid.h"
#include "util/parented_ptr.h"

class QAction;
class QPoint;
class WLibrarySidebar;
class WLibraryTextBrowser;

class DynamicCrateFeature final : public BaseTrackSetFeature {
    Q_OBJECT

  public:
    DynamicCrateFeature(
            Library* pLibrary,
            UserSettingsPointer pConfig);
    ~DynamicCrateFeature() override = default;

    QVariant title() override;

    void bindLibraryWidget(
            WLibrary* libraryWidget,
            KeyboardEventFilter* keyboard) override;
    void bindSidebarWidget(WLibrarySidebar* pSidebarWidget) override;

    TreeItemModel* sidebarModel() const override;

  public slots:
    void activate() override;
    void activateChild(const QModelIndex& index) override;
    void onRightClick(const QPoint& globalPos) override;
    void onRightClickChild(const QPoint& globalPos, const QModelIndex& index) override;
    void deleteItem(const QModelIndex& index) override;
    void renameItem(const QModelIndex& index) override;

  private slots:
    void slotCreateDynamicCrate();
    void slotDuplicateDynamicCrate();
    void slotEditDynamicCrate();
    void slotDeleteDynamicCrate();
    void slotReloadDynamicCrates();
    void slotTracksChanged(const QSet<TrackId>& trackIds);
    void slotTracksAdded(const QSet<TrackId>& trackIds);
    void slotTracksRemoved(const QSet<TrackId>& trackIds);
    void slotMultipleTracksChanged();
    void htmlLinkClicked(const QUrl& link);

  private:
    struct AggregateSummary {
        uint trackCount = 0;
        double trackDuration = 0.0;
        bool valid = false;
    };

    void initActions();
    void connectTrackCollection();
    QModelIndex rebuildChildModel(const QString& selectedSectionName = QString());
    bool activateDefinition(
            const DynamicCrateDefinition& definition,
            const QModelIndex& index,
            bool forceRefresh = false);
    AggregateSummary computeAggregateSummary(const DynamicCrateDefinition& definition) const;
    QString formatLabel(const DynamicCrateDefinition& definition) const;
    void updateChildLabels();
    const DynamicCrateDefinition* findDefinitionBySection(const QString& sectionName) const;
    const DynamicCrateDefinition* definitionFromIndex(const QModelIndex& index) const;
    bool reloadAndActivateSection(const QString& sectionName);
    bool saveDefinitionsAndActivate(
            const QList<DynamicCrateDefinition>& definitions,
            const QString& sectionNameToActivate);
    QString proposeDuplicateName(const QString& existingName) const;
    QModelIndex indexFromSectionName(const QString& sectionName) const;
    QString sectionNameFromIndex(const QModelIndex& index) const;
    QString formatRootViewHtml() const;
    void refreshActiveDynamicCrate();

    QList<DynamicCrateDefinition> m_definitions;
    DynamicCrateTableModel m_tableModel;
    QModelIndex m_lastClickedIndex;
    QModelIndex m_lastRightClickedIndex;
    QPointer<WLibrarySidebar> m_pSidebarWidget;

    parented_ptr<QAction> m_pCreateDynamicCrateAction;
    parented_ptr<QAction> m_pDuplicateDynamicCrateAction;
    parented_ptr<QAction> m_pEditDynamicCrateAction;
    parented_ptr<QAction> m_pDeleteDynamicCrateAction;
    parented_ptr<QAction> m_pReloadDynamicCratesAction;
};

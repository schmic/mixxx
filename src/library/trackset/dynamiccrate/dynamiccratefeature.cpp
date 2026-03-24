#include "library/trackset/dynamiccrate/dynamiccratefeature.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QSqlQuery>
#include <QUrl>

#include "library/library.h"
#include "library/queryutil.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "library/treeitem.h"
#include "library/trackset/dynamiccrate/dynamiccrateeditordialog.h"
#include "moc_dynamiccratefeature.cpp"
#include "util/duration.h"
#include "widget/wlibrary.h"
#include "widget/wlibrarysidebar.h"
#include "widget/wlibrarytextbrowser.h"

namespace {

constexpr QLatin1String kRootViewName("DYNAMICCRATEHOME");

} // namespace

DynamicCrateFeature::DynamicCrateFeature(
        Library* pLibrary,
        UserSettingsPointer pConfig)
        : BaseTrackSetFeature(
                  pLibrary,
                  pConfig,
                  kRootViewName,
                  QStringLiteral("dynamic_crates")),
          m_tableModel(this, pLibrary->trackCollectionManager()) {
    initActions();
    m_pSidebarModel->setRootItem(TreeItem::newRoot(this));
    connectTrackCollection();
    slotReloadDynamicCrates();
}

QVariant DynamicCrateFeature::title() {
    return tr("Dynamic Crates");
}

void DynamicCrateFeature::bindLibraryWidget(
        WLibrary* libraryWidget,
        KeyboardEventFilter* keyboard) {
    Q_UNUSED(keyboard);
    auto* pTextBrowser = new WLibraryTextBrowser(libraryWidget);
    pTextBrowser->setHtml(formatRootViewHtml());
    pTextBrowser->setOpenLinks(false);
    connect(pTextBrowser,
            &WLibraryTextBrowser::anchorClicked,
            this,
            &DynamicCrateFeature::htmlLinkClicked);
    libraryWidget->registerView(kRootViewName, pTextBrowser);
}

void DynamicCrateFeature::bindSidebarWidget(WLibrarySidebar* pSidebarWidget) {
    m_pSidebarWidget = pSidebarWidget;
}

TreeItemModel* DynamicCrateFeature::sidebarModel() const {
    return m_pSidebarModel;
}

void DynamicCrateFeature::activate() {
    m_lastClickedIndex = QModelIndex();
    BaseTrackSetFeature::activate();
}

void DynamicCrateFeature::activateChild(const QModelIndex& index) {
    const DynamicCrateDefinition* pDefinition = definitionFromIndex(index);
    if (pDefinition == nullptr) {
        return;
    }
    activateDefinition(*pDefinition, index);
}

bool DynamicCrateFeature::activateDefinition(
        const DynamicCrateDefinition& definition,
        const QModelIndex& index,
        bool forceRefresh) {
    if (!m_tableModel.selectDynamicCrate(definition, forceRefresh)) {
        return false;
    }

    m_lastClickedIndex = index;
    emit saveModelState();
    emit showTrackModel(&m_tableModel);
    emit enableCoverArtDisplay(true);
    return true;
}

void DynamicCrateFeature::onRightClick(const QPoint& globalPos) {
    m_lastRightClickedIndex = QModelIndex();
    QMenu menu(m_pSidebarWidget);
    menu.addAction(m_pCreateDynamicCrateAction.get());
    menu.addSeparator();
    menu.addAction(m_pReloadDynamicCratesAction.get());
    menu.exec(globalPos);
}

void DynamicCrateFeature::onRightClickChild(
        const QPoint& globalPos, const QModelIndex& index) {
    m_lastRightClickedIndex = index;
    QMenu menu(m_pSidebarWidget);
    menu.addAction(m_pCreateDynamicCrateAction.get());
    menu.addAction(m_pDuplicateDynamicCrateAction.get());
    menu.addAction(m_pEditDynamicCrateAction.get());
    menu.addAction(m_pDeleteDynamicCrateAction.get());
    menu.addSeparator();
    menu.addAction(m_pReloadDynamicCratesAction.get());
    menu.exec(globalPos);
}

void DynamicCrateFeature::deleteItem(const QModelIndex& index) {
    m_lastRightClickedIndex = index;
    slotDeleteDynamicCrate();
}

void DynamicCrateFeature::renameItem(const QModelIndex& index) {
    m_lastRightClickedIndex = index;
    slotEditDynamicCrate();
}

void DynamicCrateFeature::initActions() {
    m_pCreateDynamicCrateAction = make_parented<QAction>(tr("New"), this);
    connect(m_pCreateDynamicCrateAction.get(),
            &QAction::triggered,
            this,
            &DynamicCrateFeature::slotCreateDynamicCrate);

    m_pDuplicateDynamicCrateAction = make_parented<QAction>(tr("Duplicate"), this);
    connect(m_pDuplicateDynamicCrateAction.get(),
            &QAction::triggered,
            this,
            &DynamicCrateFeature::slotDuplicateDynamicCrate);

    m_pEditDynamicCrateAction = make_parented<QAction>(tr("Edit"), this);
    connect(m_pEditDynamicCrateAction.get(),
            &QAction::triggered,
            this,
            &DynamicCrateFeature::slotEditDynamicCrate);

    m_pDeleteDynamicCrateAction = make_parented<QAction>(tr("Delete"), this);
    connect(m_pDeleteDynamicCrateAction.get(),
            &QAction::triggered,
            this,
            &DynamicCrateFeature::slotDeleteDynamicCrate);

    m_pReloadDynamicCratesAction = make_parented<QAction>(tr("Reload Dynamic Crates"), this);
    connect(m_pReloadDynamicCratesAction.get(),
            &QAction::triggered,
            this,
            &DynamicCrateFeature::slotReloadDynamicCrates);
}

void DynamicCrateFeature::slotCreateDynamicCrate() {
    DynamicCrateEditorDialog dialog(m_pSidebarWidget, m_definitions);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QList<DynamicCrateDefinition> definitions = m_definitions;
    definitions.append(dialog.definition());
    saveDefinitionsAndActivate(definitions, dialog.definition().sectionName);
}

void DynamicCrateFeature::slotDuplicateDynamicCrate() {
    const DynamicCrateDefinition* pDefinition = definitionFromIndex(m_lastRightClickedIndex);
    if (pDefinition == nullptr) {
        return;
    }

    DynamicCrateDefinition duplicatedDefinition = *pDefinition;
    duplicatedDefinition.sectionName.clear();
    duplicatedDefinition.name = proposeDuplicateName(pDefinition->name);

    DynamicCrateEditorDialog dialog(m_pSidebarWidget, m_definitions, &duplicatedDefinition);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QList<DynamicCrateDefinition> definitions = m_definitions;
    definitions.append(dialog.definition());
    saveDefinitionsAndActivate(definitions, dialog.definition().sectionName);
}

void DynamicCrateFeature::slotEditDynamicCrate() {
    const DynamicCrateDefinition* pDefinition = definitionFromIndex(m_lastRightClickedIndex);
    if (pDefinition == nullptr) {
        return;
    }

    DynamicCrateEditorDialog dialog(m_pSidebarWidget, m_definitions, pDefinition);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QList<DynamicCrateDefinition> definitions = m_definitions;
    for (auto& definition : definitions) {
        if (definition.sectionName == pDefinition->sectionName) {
            definition = dialog.definition();
            saveDefinitionsAndActivate(definitions, dialog.definition().sectionName);
            return;
        }
    }
}

void DynamicCrateFeature::slotDeleteDynamicCrate() {
    const DynamicCrateDefinition* pDefinition = definitionFromIndex(m_lastRightClickedIndex);
    if (pDefinition == nullptr) {
        return;
    }

    const QMessageBox::StandardButton button = QMessageBox::question(
            m_pSidebarWidget,
            tr("Confirm Deletion"),
            tr("Do you really want to delete dynamic crate <b>%1</b>?")
                    .arg(pDefinition->name.toHtmlEscaped()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
    if (button != QMessageBox::Yes) {
        return;
    }

    QList<DynamicCrateDefinition> definitions = m_definitions;
    for (auto it = definitions.begin(); it != definitions.end(); ++it) {
        if (it->sectionName == pDefinition->sectionName) {
            definitions.erase(it);
            break;
        }
    }

    QString error;
    const QString configPath =
            DynamicCrateConfigLoader::defaultConfigFilePath(*m_pConfig);
    if (!saveDynamicCrateDefinitions(configPath, definitions, &error)) {
        QMessageBox::warning(
                m_pSidebarWidget,
                tr("Deleting Dynamic Crate Failed"),
                error);
        return;
    }

    slotReloadDynamicCrates();
}

void DynamicCrateFeature::connectTrackCollection() {
    auto* pTrackCollection = m_pLibrary->trackCollectionManager()->internalCollection();
    connect(pTrackCollection,
            &TrackCollection::tracksChanged,
            this,
            &DynamicCrateFeature::slotTracksChanged);
    connect(pTrackCollection,
            &TrackCollection::tracksAdded,
            this,
            &DynamicCrateFeature::slotTracksAdded);
    connect(pTrackCollection,
            &TrackCollection::tracksRemoved,
            this,
            &DynamicCrateFeature::slotTracksRemoved);
    connect(pTrackCollection,
            &TrackCollection::multipleTracksChanged,
            this,
            &DynamicCrateFeature::slotMultipleTracksChanged);
    connect(m_pLibrary->trackCollectionManager(),
            &TrackCollectionManager::libraryScanFinished,
            this,
            &DynamicCrateFeature::slotMultipleTracksChanged);
}

void DynamicCrateFeature::slotReloadDynamicCrates() {
    const QString selectedSectionName = sectionNameFromIndex(m_lastClickedIndex);

    DynamicCrateConfigLoader configLoader(
            DynamicCrateConfigLoader::defaultConfigFilePath(*m_pConfig));
    m_definitions = configLoader.load();
    QModelIndex selectedIndex = rebuildChildModel(selectedSectionName);

    if (selectedIndex.isValid()) {
        const DynamicCrateDefinition* pDefinition = definitionFromIndex(selectedIndex);
        if (pDefinition != nullptr && m_tableModel.hasSelectedDynamicCrate()) {
            if (activateDefinition(*pDefinition, selectedIndex, true)) {
                emit featureSelect(this, selectedIndex);
            }
        }
        return;
    }

    if (m_tableModel.hasSelectedDynamicCrate()) {
        m_lastClickedIndex = QModelIndex();
        BaseTrackSetFeature::activate();
    }
}

QModelIndex DynamicCrateFeature::rebuildChildModel(const QString& selectedSectionName) {
    TreeItem* pRootItem = m_pSidebarModel->getRootItem();
    VERIFY_OR_DEBUG_ASSERT(pRootItem != nullptr) {
        return QModelIndex();
    }

    m_pSidebarModel->removeRows(0, pRootItem->childRows());

    std::vector<std::unique_ptr<TreeItem>> treeRows;
    treeRows.reserve(m_definitions.size());
    int selectedRow = TreeItem::kInvalidRow;
    for (const auto& definition : std::as_const(m_definitions)) {
        auto pTreeItem = TreeItem::newRoot(this);
        pTreeItem->setLabel(formatLabel(definition));
        pTreeItem->setData(definition.sectionName);
        if (definition.sectionName == selectedSectionName) {
            selectedRow = static_cast<int>(treeRows.size());
        }
        treeRows.push_back(std::move(pTreeItem));
    }

    m_pSidebarModel->insertTreeItemRows(std::move(treeRows), 0);

    if (selectedRow == TreeItem::kInvalidRow) {
        return QModelIndex();
    }
    return m_pSidebarModel->index(selectedRow, 0);
}

DynamicCrateFeature::AggregateSummary DynamicCrateFeature::computeAggregateSummary(
        const DynamicCrateDefinition& definition) const {
    AggregateSummary summary;

    auto* pTrackCollection = m_pLibrary->trackCollectionManager()->internalCollection();
    DynamicCrateSqlBuilder sqlBuilder(pTrackCollection->database());
    QString whereClause;
    QString error;
    if (!sqlBuilder.buildWhereClause(definition, &whereClause, &error)) {
        qWarning() << "Failed to build aggregate SQL for dynamic crate"
                   << definition.sectionName << ":" << error;
        return summary;
    }

    QSqlQuery query(pTrackCollection->database());
    query.prepare(QStringLiteral(
            "SELECT COUNT(library.id), COALESCE(SUM(library.duration), 0) "
            "FROM library "
            "INNER JOIN track_locations "
            "ON library.location = track_locations.id "
            "WHERE library.mixxx_deleted = 0 "
            "AND track_locations.fs_deleted = 0 "
            "AND (%1)")
                          .arg(whereClause));
    if (!query.exec()) {
        LOG_FAILED_QUERY(query);
        return summary;
    }
    if (!query.next()) {
        qWarning() << "Failed to read aggregate result for dynamic crate"
                   << definition.sectionName;
        return summary;
    }

    summary.trackCount = query.value(0).toUInt();
    summary.trackDuration = query.value(1).toDouble();
    summary.valid = true;
    return summary;
}

QString DynamicCrateFeature::formatLabel(const DynamicCrateDefinition& definition) const {
    const AggregateSummary summary = computeAggregateSummary(definition);
    if (!summary.valid) {
        return definition.name;
    }
    return QStringLiteral("%1 (%2) %3")
            .arg(definition.name,
                    QString::number(summary.trackCount),
                    mixxx::Duration::formatTime(
                            summary.trackDuration,
                            mixxx::Duration::Precision::SECONDS));
}

void DynamicCrateFeature::updateChildLabels() {
    for (const auto& definition : std::as_const(m_definitions)) {
        const QModelIndex index = indexFromSectionName(definition.sectionName);
        if (!index.isValid()) {
            continue;
        }
        TreeItem* pTreeItem = m_pSidebarModel->getItem(index);
        if (pTreeItem == nullptr) {
            continue;
        }
        pTreeItem->setLabel(formatLabel(definition));
        m_pSidebarModel->dataChanged(index, index);
    }
}

const DynamicCrateDefinition* DynamicCrateFeature::findDefinitionBySection(
        const QString& sectionName) const {
    if (sectionName.isEmpty()) {
        return nullptr;
    }
    for (const auto& definition : m_definitions) {
        if (definition.sectionName == sectionName) {
            return &definition;
        }
    }
    return nullptr;
}

const DynamicCrateDefinition* DynamicCrateFeature::definitionFromIndex(const QModelIndex& index) const {
    if (!index.isValid()) {
        return nullptr;
    }
    return findDefinitionBySection(sectionNameFromIndex(index));
}

bool DynamicCrateFeature::reloadAndActivateSection(const QString& sectionName) {
    DynamicCrateConfigLoader configLoader(
            DynamicCrateConfigLoader::defaultConfigFilePath(*m_pConfig));
    m_definitions = configLoader.load();

    const QModelIndex index = rebuildChildModel(sectionName);
    if (!index.isValid()) {
        m_lastClickedIndex = QModelIndex();
        BaseTrackSetFeature::activate();
        return false;
    }

    const DynamicCrateDefinition* pDefinition = definitionFromIndex(index);
    if (pDefinition == nullptr) {
        return false;
    }
    if (!activateDefinition(*pDefinition, index, true)) {
        return false;
    }
    emit featureSelect(this, index);
    return true;
}

bool DynamicCrateFeature::saveDefinitionsAndActivate(
        const QList<DynamicCrateDefinition>& definitions,
        const QString& sectionNameToActivate) {
    QString error;
    const QString configPath =
            DynamicCrateConfigLoader::defaultConfigFilePath(*m_pConfig);
    if (!saveDynamicCrateDefinitions(configPath, definitions, &error)) {
        QMessageBox::warning(
                m_pSidebarWidget,
                tr("Saving Dynamic Crate Failed"),
                error);
        return false;
    }
    return reloadAndActivateSection(sectionNameToActivate);
}

QString DynamicCrateFeature::proposeDuplicateName(const QString& existingName) const {
    QString candidate = QStringLiteral("%1 %2").arg(existingName, tr("Copy"));
    int suffix = 2;
    auto hasConflict = [&](const QString& name) {
        for (const auto& definition : m_definitions) {
            if (definition.name.compare(name, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };

    while (hasConflict(candidate)) {
        candidate = QStringLiteral("%1 %2 %3")
                            .arg(existingName, tr("Copy"), QString::number(suffix++));
    }
    return candidate;
}

QModelIndex DynamicCrateFeature::indexFromSectionName(const QString& sectionName) const {
    TreeItem* pRootItem = m_pSidebarModel->getRootItem();
    VERIFY_OR_DEBUG_ASSERT(pRootItem != nullptr) {
        return QModelIndex();
    }

    for (int row = 0; row < pRootItem->childRows(); ++row) {
        const QModelIndex index = m_pSidebarModel->index(row, 0);
        if (sectionNameFromIndex(index) == sectionName) {
            return index;
        }
    }
    return QModelIndex();
}

QString DynamicCrateFeature::formatRootViewHtml() const {
    const QString configPath =
            DynamicCrateConfigLoader::defaultConfigFilePath(*m_pConfig);
    QString html;
    html.append(QStringLiteral("<h2>%1</h2>").arg(tr("Dynamic Crates")));
    html.append(QStringLiteral("<p>%1</p>")
                        .arg(tr("Dynamic Crates can be edited in the UI and are stored in a separate configuration file.")));
    html.append(QStringLiteral("<p><code>%1</code></p>").arg(configPath.toHtmlEscaped()));
    html.append(QStringLiteral("<a style=\"color:#0496FF;\" href=\"reload\">%1</a>")
                        .arg(tr("Reload Dynamic Crates")));
    return html;
}

QString DynamicCrateFeature::sectionNameFromIndex(const QModelIndex& index) const {
    if (!index.isValid()) {
        return QString();
    }
    auto* pTreeItem = static_cast<TreeItem*>(index.internalPointer());
    if (pTreeItem == nullptr) {
        return QString();
    }
    return pTreeItem->getData().toString();
}

void DynamicCrateFeature::refreshActiveDynamicCrate() {
    updateChildLabels();

    if (!m_lastClickedIndex.isValid() || !m_tableModel.hasSelectedDynamicCrate()) {
        return;
    }

    const QString selectedSectionName = sectionNameFromIndex(m_lastClickedIndex);
    const DynamicCrateDefinition* pDefinition = findDefinitionBySection(selectedSectionName);
    if (pDefinition == nullptr) {
        return;
    }

    if (m_tableModel.selectedSectionName() != selectedSectionName) {
        activateDefinition(*pDefinition, m_lastClickedIndex);
        return;
    }

    m_tableModel.refreshSelectedDynamicCrate();
}

void DynamicCrateFeature::slotTracksChanged(const QSet<TrackId>& trackIds) {
    Q_UNUSED(trackIds);
    refreshActiveDynamicCrate();
}

void DynamicCrateFeature::slotTracksAdded(const QSet<TrackId>& trackIds) {
    Q_UNUSED(trackIds);
    refreshActiveDynamicCrate();
}

void DynamicCrateFeature::slotTracksRemoved(const QSet<TrackId>& trackIds) {
    Q_UNUSED(trackIds);
    refreshActiveDynamicCrate();
}

void DynamicCrateFeature::slotMultipleTracksChanged() {
    refreshActiveDynamicCrate();
}

void DynamicCrateFeature::htmlLinkClicked(const QUrl& link) {
    if (link == QUrl(QStringLiteral("reload"))) {
        slotReloadDynamicCrates();
    }
}

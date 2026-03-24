#include "library/trackset/dynamiccrate/dynamiccrateeditordialog.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

#include "moc_dynamiccrateeditordialog.cpp"

namespace {

QString translatedFieldGroup(const char* text) {
    return QCoreApplication::translate("DynamicCrateEditorDialog", text);
}

QString fieldGroupName(const QString& fieldName) {
    static const QSet<QString> kMetadataFields = {
            QStringLiteral("album"),
            QStringLiteral("album_artist"),
            QStringLiteral("albumartist"),
            QStringLiteral("artist"),
            QStringLiteral("comment"),
            QStringLiteral("composer"),
            QStringLiteral("genre"),
            QStringLiteral("grouping"),
            QStringLiteral("key"),
            QStringLiteral("key_id"),
            QStringLiteral("title"),
            QStringLiteral("track"),
            QStringLiteral("track_number"),
            QStringLiteral("tracknumber"),
            QStringLiteral("url"),
            QStringLiteral("year"),
    };

    if (kMetadataFields.contains(fieldName)) {
        return translatedFieldGroup("Metadata");
    }
    return translatedFieldGroup("Library");
}

QList<QPair<QString, QStringList>> groupedFields() {
    QMap<QString, QStringList> groups;
    for (const auto& field : dynamicCrateAvailableFields()) {
        groups[fieldGroupName(field)].append(field);
    }

    QList<QPair<QString, QStringList>> orderedGroups;
    const QStringList groupOrder = {
            translatedFieldGroup("Metadata"),
            translatedFieldGroup("Library"),
    };
    for (const auto& groupName : groupOrder) {
        auto fields = groups.take(groupName);
        std::sort(fields.begin(), fields.end(), [](const QString& lhs, const QString& rhs) {
            return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
        });
        orderedGroups.append({groupName, fields});
    }
    return orderedGroups;
}

void populateFieldCombo(QComboBox* pFieldCombo) {
    bool firstGroup = true;
    for (const auto& group : groupedFields()) {
        if (group.second.isEmpty()) {
            continue;
        }
        if (!firstGroup) {
            pFieldCombo->insertSeparator(pFieldCombo->count());
        }
        firstGroup = false;
        for (const auto& field : group.second) {
            pFieldCombo->addItem(field);
        }
    }
}

DynamicCrateRule defaultRule() {
    DynamicCrateRule rule;
    const QStringList fields = dynamicCrateAvailableFields();
    if (fields.isEmpty()) {
        return rule;
    }
    rule.fieldName = fields.first();
    dynamicCrateFieldTypeForField(rule.fieldName, &rule.fieldType);
    const QList<DynamicCrateOperator> operators =
            dynamicCrateSupportedOperators(rule.fieldType);
    if (!operators.isEmpty()) {
        rule.op = operators.first();
    }
    return rule;
}

} // namespace

DynamicCrateEditorDialog::DynamicCrateEditorDialog(
        QWidget* parent,
        const QList<DynamicCrateDefinition>& existingDefinitions,
        const DynamicCrateDefinition* pInitialDefinition)
        : QDialog(parent),
          m_existingDefinitions(existingDefinitions),
          m_originalSectionName(
                  pInitialDefinition != nullptr
                          ? pInitialDefinition->sectionName
                          : QString()),
          m_pNameEdit(make_parented<QLineEdit>(this)),
          m_pMatchModeCombo(make_parented<QComboBox>(this)),
          m_pSectionPreviewLabel(make_parented<QLabel>(this)),
          m_pRulesTable(make_parented<QTableWidget>(this)),
          m_pValidationLabel(make_parented<QLabel>(this)),
          m_pSaveButton(nullptr),
          m_pRemoveRuleButton(nullptr) {
    setModal(true);
    setMinimumSize(720, 420);
    setWindowTitle(
            pInitialDefinition != nullptr
                    ? tr("Edit Dynamic Crate")
                    : tr("New Dynamic Crate"));

    m_pMatchModeCombo->addItem(
            tr("Match Any"),
            static_cast<int>(DynamicCrateMatchMode::Any));
    m_pMatchModeCombo->addItem(
            tr("Match All"),
            static_cast<int>(DynamicCrateMatchMode::All));

    m_pRulesTable->setColumnCount(3);
    m_pRulesTable->setHorizontalHeaderLabels(
            {tr("Field"), tr("Operator"), tr("Value")});
    m_pRulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pRulesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pRulesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pRulesTable->verticalHeader()->setVisible(false);
    m_pRulesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pRulesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pRulesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    m_pValidationLabel->setWordWrap(true);
    m_pValidationLabel->setStyleSheet(QStringLiteral("color: #c0392b;"));

    auto* pRuleButtons = new QDialogButtonBox(Qt::Horizontal, this);
    pRuleButtons->setCenterButtons(false);
    QPushButton* pAddRuleButton =
            pRuleButtons->addButton(tr("Add Rule"), QDialogButtonBox::ActionRole);
    m_pRemoveRuleButton =
            pRuleButtons->addButton(tr("Remove Rule"), QDialogButtonBox::ActionRole);

    auto* pDialogButtons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            this);
    m_pSaveButton = pDialogButtons->button(QDialogButtonBox::Ok);
    m_pSaveButton->setText(tr("Save"));

    auto* pFormLayout = new QFormLayout();
    pFormLayout->addRow(tr("Name"), m_pNameEdit.get());
    pFormLayout->addRow(tr("Match"), m_pMatchModeCombo.get());
    pFormLayout->addRow(tr("Config Section"), m_pSectionPreviewLabel.get());

    auto* pLayout = new QVBoxLayout();
    pLayout->addLayout(pFormLayout);
    pLayout->addWidget(m_pRulesTable.get(), 1);
    pLayout->addWidget(pRuleButtons);
    pLayout->addWidget(m_pValidationLabel.get());
    pLayout->addWidget(pDialogButtons);
    setLayout(pLayout);

    connect(pAddRuleButton,
            &QPushButton::clicked,
            this,
            &DynamicCrateEditorDialog::slotAddRule);
    connect(m_pRemoveRuleButton,
            &QPushButton::clicked,
            this,
            &DynamicCrateEditorDialog::slotRemoveSelectedRules);
    connect(pDialogButtons,
            &QDialogButtonBox::accepted,
            this,
            &DynamicCrateEditorDialog::accept);
    connect(pDialogButtons,
            &QDialogButtonBox::rejected,
            this,
            &DynamicCrateEditorDialog::reject);
    connect(m_pNameEdit.get(),
            &QLineEdit::textChanged,
            this,
            &DynamicCrateEditorDialog::slotInputsChanged);
    connect(m_pMatchModeCombo.get(),
            &QComboBox::currentIndexChanged,
            this,
            &DynamicCrateEditorDialog::slotInputsChanged);
    connect(m_pRulesTable->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &DynamicCrateEditorDialog::slotSelectionChanged);

    if (pInitialDefinition != nullptr) {
        m_pNameEdit->setText(pInitialDefinition->name);
        m_pMatchModeCombo->setCurrentIndex(
                m_pMatchModeCombo->findData(
                        static_cast<int>(pInitialDefinition->matchMode)));
        for (const auto& rule : pInitialDefinition->rules) {
            appendRuleRow(&rule);
        }
    } else {
        m_pNameEdit->setText(tr("New Dynamic Crate"));
        appendRuleRow();
    }

    updateUiState();
}

void DynamicCrateEditorDialog::accept() {
    DynamicCrateDefinition definition;
    QString error;
    if (!buildDefinition(&definition, &error)) {
        QMessageBox::warning(
                this,
                tr("Saving Dynamic Crate Failed"),
                error);
        return;
    }

    m_definition = definition;
    QDialog::accept();
}

void DynamicCrateEditorDialog::slotAddRule() {
    appendRuleRow();
    m_pRulesTable->setCurrentCell(m_pRulesTable->rowCount() - 1, 0);
    updateUiState();
}

void DynamicCrateEditorDialog::slotRemoveSelectedRules() {
    const QModelIndexList selectedRows = m_pRulesTable->selectionModel()->selectedRows();
    for (int i = selectedRows.size() - 1; i >= 0; --i) {
        m_pRulesTable->removeRow(selectedRows.at(i).row());
    }
    updateUiState();
}

void DynamicCrateEditorDialog::slotRuleFieldChanged() {
    auto* pFieldCombo = qobject_cast<QComboBox*>(sender());
    if (pFieldCombo == nullptr) {
        return;
    }
    const int row = rowForSenderWidget(pFieldCombo);
    if (row < 0) {
        return;
    }

    DynamicCrateFieldType fieldType;
    if (!dynamicCrateFieldTypeForField(pFieldCombo->currentText(), &fieldType)) {
        return;
    }
    const QList<DynamicCrateOperator> supportedOperators =
            dynamicCrateSupportedOperators(fieldType);
    const DynamicCrateOperator preferredOperator = supportedOperators.isEmpty()
            ? DynamicCrateOperator::Eq
            : supportedOperators.first();
    updateOperatorComboForRow(row, preferredOperator);
    updateUiState();
}

void DynamicCrateEditorDialog::slotInputsChanged() {
    updateUiState();
}

void DynamicCrateEditorDialog::slotSelectionChanged() {
    updateUiState();
}

void DynamicCrateEditorDialog::appendRuleRow(const DynamicCrateRule* pRule) {
    const DynamicCrateRule initialRule = pRule != nullptr ? *pRule : defaultRule();
    const int row = m_pRulesTable->rowCount();
    m_pRulesTable->insertRow(row);

    auto* pFieldCombo = new QComboBox(m_pRulesTable.get());
    populateFieldCombo(pFieldCombo);
    pFieldCombo->setCurrentText(initialRule.fieldName);
    m_pRulesTable->setCellWidget(row, 0, pFieldCombo);

    auto* pOperatorCombo = new QComboBox(m_pRulesTable.get());
    m_pRulesTable->setCellWidget(row, 1, pOperatorCombo);
    updateOperatorComboForRow(
            row,
            dynamicCrateOperatorToConfigString(initialRule.op).isEmpty()
                    ? DynamicCrateOperator::Eq
                    : initialRule.op);

    auto* pValueEdit = new QLineEdit(m_pRulesTable.get());
    pValueEdit->setText(initialRule.rawValue);
    m_pRulesTable->setCellWidget(row, 2, pValueEdit);

    connect(pFieldCombo,
            &QComboBox::currentTextChanged,
            this,
            [this]() {
                slotRuleFieldChanged();
            });
    connect(pOperatorCombo,
            &QComboBox::currentTextChanged,
            this,
            &DynamicCrateEditorDialog::slotInputsChanged);
    connect(pValueEdit,
            &QLineEdit::textChanged,
            this,
            &DynamicCrateEditorDialog::slotInputsChanged);
}

void DynamicCrateEditorDialog::updateOperatorComboForRow(
        int row,
        DynamicCrateOperator preferredOperator) {
    auto* pFieldCombo = qobject_cast<QComboBox*>(m_pRulesTable->cellWidget(row, 0));
    auto* pOperatorCombo = qobject_cast<QComboBox*>(m_pRulesTable->cellWidget(row, 1));
    if (pFieldCombo == nullptr || pOperatorCombo == nullptr) {
        return;
    }

    DynamicCrateFieldType fieldType;
    if (!dynamicCrateFieldTypeForField(pFieldCombo->currentText(), &fieldType)) {
        return;
    }

    const QList<DynamicCrateOperator> supportedOperators =
            dynamicCrateSupportedOperators(fieldType);
    const QSignalBlocker blocker(pOperatorCombo);
    pOperatorCombo->clear();
    for (const auto& op : supportedOperators) {
        pOperatorCombo->addItem(
                dynamicCrateOperatorToConfigString(op),
                static_cast<int>(op));
    }

    int preferredIndex = pOperatorCombo->findData(static_cast<int>(preferredOperator));
    if (preferredIndex < 0) {
        preferredIndex = 0;
    }
    pOperatorCombo->setCurrentIndex(preferredIndex);
}

int DynamicCrateEditorDialog::rowForSenderWidget(QWidget* pWidget) const {
    for (int row = 0; row < m_pRulesTable->rowCount(); ++row) {
        for (int column = 0; column < m_pRulesTable->columnCount(); ++column) {
            if (m_pRulesTable->cellWidget(row, column) == pWidget) {
                return row;
            }
        }
    }
    return -1;
}

bool DynamicCrateEditorDialog::buildDefinition(
        DynamicCrateDefinition* pDefinition,
        QString* pError) const {
    DynamicCrateDefinition definition;
    definition.name = m_pNameEdit->text().trimmed();
    definition.matchMode = static_cast<DynamicCrateMatchMode>(
            m_pMatchModeCombo->currentData().toInt());

    if (definition.name.isEmpty()) {
        if (pError) {
            *pError = tr("A dynamic crate cannot have a blank name.");
        }
        return false;
    }

    for (const auto& existingDefinition : m_existingDefinitions) {
        if (!m_originalSectionName.isEmpty() &&
                existingDefinition.sectionName.compare(
                        m_originalSectionName,
                        Qt::CaseInsensitive) == 0) {
            continue;
        }
        if (existingDefinition.name.compare(definition.name, Qt::CaseInsensitive) == 0) {
            if (pError) {
                *pError = tr("A dynamic crate by that name already exists.");
            }
            return false;
        }
    }

    definition.sectionName = uniqueDynamicCrateSectionName(
            sanitizeDynamicCrateSectionName(definition.name),
            m_existingDefinitions,
            m_originalSectionName);

    for (int row = 0; row < m_pRulesTable->rowCount(); ++row) {
        auto* pFieldCombo = qobject_cast<QComboBox*>(m_pRulesTable->cellWidget(row, 0));
        auto* pOperatorCombo = qobject_cast<QComboBox*>(m_pRulesTable->cellWidget(row, 1));
        auto* pValueEdit = qobject_cast<QLineEdit*>(m_pRulesTable->cellWidget(row, 2));
        if (pFieldCombo == nullptr || pOperatorCombo == nullptr || pValueEdit == nullptr) {
            if (pError) {
                *pError = tr("Rule %1 is incomplete.").arg(row + 1);
            }
            return false;
        }

        DynamicCrateRule rule;
        rule.fieldName = pFieldCombo->currentText().trimmed();
        rule.op = static_cast<DynamicCrateOperator>(pOperatorCombo->currentData().toInt());
        rule.rawValue = pValueEdit->text().trimmed();
        if (!dynamicCrateFieldTypeForField(rule.fieldName, &rule.fieldType)) {
            if (pError) {
                *pError = tr("Rule %1 uses an unknown field.").arg(row + 1);
            }
            return false;
        }

        QString ruleError;
        if (!validateDynamicCrateRule(rule, &ruleError)) {
            if (pError) {
                *pError = tr("Rule %1 is invalid: %2").arg(row + 1).arg(ruleError);
            }
            return false;
        }
        definition.rules.append(rule);
    }

    QString validationError;
    if (!validateDynamicCrateDefinition(definition, &validationError)) {
        if (pError) {
            *pError = validationError;
        }
        return false;
    }

    if (pDefinition != nullptr) {
        *pDefinition = definition;
    }
    return true;
}

QString DynamicCrateEditorDialog::previewSectionName() const {
    return QStringLiteral("[%1]")
            .arg(uniqueDynamicCrateSectionName(
                    sanitizeDynamicCrateSectionName(m_pNameEdit->text()),
                    m_existingDefinitions,
                    m_originalSectionName));
}

void DynamicCrateEditorDialog::updateUiState() {
    m_pSectionPreviewLabel->setText(previewSectionName());
    m_pRemoveRuleButton->setEnabled(m_pRulesTable->selectionModel()->hasSelection());

    QString error;
    DynamicCrateDefinition definition;
    const bool valid = buildDefinition(&definition, &error);
    m_pSaveButton->setEnabled(valid);
    m_pValidationLabel->setText(valid ? QString() : error);
}

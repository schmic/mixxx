#pragma once

#include <QDialog>
#include <QList>

#include "library/trackset/dynamiccrate/dynamiccrateconfig.h"
#include "util/parented_ptr.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QWidget;

class DynamicCrateEditorDialog final : public QDialog {
    Q_OBJECT

  public:
    DynamicCrateEditorDialog(
            QWidget* parent,
            const QList<DynamicCrateDefinition>& existingDefinitions,
            const DynamicCrateDefinition* pInitialDefinition = nullptr);

    const DynamicCrateDefinition& definition() const {
        return m_definition;
    }

  public slots:
    void accept() override;

  private slots:
    void slotAddRule();
    void slotRemoveSelectedRules();
    void slotRuleFieldChanged();
    void slotInputsChanged();
    void slotSelectionChanged();

  private:
    void appendRuleRow(const DynamicCrateRule* pRule = nullptr);
    void updateOperatorComboForRow(
            int row,
            DynamicCrateOperator preferredOperator);
    int rowForSenderWidget(QWidget* pWidget) const;
    bool buildDefinition(
            DynamicCrateDefinition* pDefinition,
            QString* pError) const;
    QString previewSectionName() const;
    void updateUiState();

    const QList<DynamicCrateDefinition> m_existingDefinitions;
    const QString m_originalSectionName;
    DynamicCrateDefinition m_definition;

    parented_ptr<QLineEdit> m_pNameEdit;
    parented_ptr<QComboBox> m_pMatchModeCombo;
    parented_ptr<QLabel> m_pSectionPreviewLabel;
    parented_ptr<QTableWidget> m_pRulesTable;
    parented_ptr<QLabel> m_pValidationLabel;

    QPushButton* m_pSaveButton;
    QPushButton* m_pRemoveRuleButton;
};

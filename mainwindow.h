#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/QTableWidget>
#include "settingswindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void initializeAfterShowing();

private slots:
    void on_pushButtonAddBloodSample_clicked();

    void on_actionSettingsSaveAs_triggered();

    void on_actionOpenPatientDataFile_triggered();

    void on_pushButtonAddChemoAndMed_clicked();

    void on_checkBoxVisualizationShowLeukocytes_stateChanged(int arg1);

    void on_checkBoxVisualizationShowErythrocytes_stateChanged(int arg1);

    void on_checkBoxVisualizationShowHemoglobin_stateChanged(int arg1);

    void on_checkBoxVisualizationShowThrombocytes_stateChanged(int arg1);

    void on_checkBoxVisualizationShowMedicamentationAndChemoTherapy_stateChanged(int arg1);

    void on_actionSettings_triggered();

    void on_tableWidgetBloodSamples_cellChanged(int row, int column);

    void on_tableWidgetChemoAndMeds_cellChanged(int row, int column);

    void on_tabWidget_currentChanged(int index);

    void on_lineEditPatientName_textEdited(const QString &arg1);

    void on_lineEditPatientDateOfBirth_textEdited(const QString &arg1);

    void on_pushButtonDeleteSelectedBloodSample_clicked();

    void on_pushButtonDeleteSelectedChemoAndMed_clicked();

private:
    Ui::MainWindow *ui;
    QString m_previousPatientDataFileName;
    SettingsWindow m_settingsWindow;
    bool m_tableDataChangedSinceLastVisualizationPlot;
    bool m_patientDataChangedSinceLastSave;
    bool m_loadingPatientDataInProgress;
    void loadPatientDataFile(QString&);
    void saveSettingsFile();
    void ensureTableWidgetCellsAreNotNull(QTableWidget*);
    qsizetype deleteSelectedTableRows(QTableWidget&);
    void plotVisualization();
};
#endif // MAINWINDOW_H

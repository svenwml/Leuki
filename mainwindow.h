#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/QTableWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonNewBloodSample_clicked();

    void savePatientDataFileAs();

    void on_actionOpenPatientDataFile_triggered();

    void on_pushButtonNewChemoAndMed_clicked();

    void on_checkBoxVisualizationShowLeukocytes_stateChanged(int arg1);

    void on_checkBoxVisualizationShowErythrocytes_stateChanged(int arg1);

    void on_checkBoxVisualizationShowHemoglobin_stateChanged(int arg1);

    void on_checkBoxVisualizationShowThrombocytes_stateChanged(int arg1);

    void on_checkBoxVisualizationShowMedicamentationAndChemoTherapy_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
    void ensureTableWidgetCellsAreNotNull(QTableWidget*);
    void plotVisualization();
};
#endif // MAINWINDOW_H

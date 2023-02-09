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

private:
    Ui::MainWindow *ui;
    void ensureTableWidgetCellsAreNotNull(QTableWidget*);
};
#endif // MAINWINDOW_H

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>
#include <regex>

const char* leukiSettingsDefault =
#include "leukiSettingsDefault.txt"
;

const static QVector<QString> tabWidgetTabs
{
    "General Information",
    "Blood Samples",
    "Chemo Therapy / Medicamentation",
    "Visualization"
};

const static QVector<QString> tableWidgetBloodSamplesColumns
{
    "Date",
    "Leukocytes",
    "Erythrocytes",
    "Hemoglobin",
    "Thrombocytes"
};

const static QVector<QString> tableWidgetChemoAndMedsColumns
{
    "Date (Start)",
    "Days",
    "Name",
    "Dose per Day"
};

const static unsigned int heightVisualizationTextLabelPixels = 45;
const static unsigned int lengthVisualizationArrowPixels = 15;

const static unsigned int hoursPerDay = 24;
const static unsigned int secondsPerHour = 3600;
const static unsigned int secondsPerDay = hoursPerDay * secondsPerHour;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tableDataChangedSinceLastVisualizationPlot(false)
    , m_patientDataChangedSinceLastSave(false)
    , m_internalTableModificationsInProgress(false)
{
    ui->setupUi(this);

    // Load settings file first.

    QFile settingsFile;
    settingsFile.setFileName(QDir::currentPath() + "/" + "leukiSettings.json");

    // If settings file does not exist, create a new file with default settings.
    if(!settingsFile.exists())
    {
        settingsFile.open(QIODevice::WriteOnly | QIODevice::Text);
        settingsFile.write(leukiSettingsDefault);
        settingsFile.close();
    }

    settingsFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString settingsString = settingsFile.readAll();
    settingsFile.close();
    QJsonDocument settingsJsonDocument = QJsonDocument::fromJson(settingsString.toUtf8());
    QJsonObject settingsJsonObject = settingsJsonDocument.object();

    if(settingsJsonObject["previousPatientDataFileName"].isString())
    {
        m_previousPatientDataFileName = settingsJsonObject["previousPatientDataFileName"].toString();
    }

    if(settingsJsonObject["activeTabIndex"].isDouble())
    {
        ui->tabWidget->setCurrentIndex(settingsJsonObject["activeTabIndex"].toInt());
    }

    if(settingsJsonObject["autoLoadPatientDataFileOnStartup"].isBool())
    {
        SettingsWindow::settings_t settings;
        settings.autoLoadPatientDataFileOnStartup = settingsJsonObject["autoLoadPatientDataFileOnStartup"].toBool();
        m_settingsWindow.setSettings(settings);
    }

    if(settingsJsonObject["visualizationShowLeukocytes"].isBool())
    {
        if(settingsJsonObject["visualizationShowLeukocytes"].toBool())
        {
            ui->checkBoxVisualizationShowLeukocytes->setCheckState(Qt::CheckState::Checked);
        }
        else
        {
            ui->checkBoxVisualizationShowLeukocytes->setCheckState(Qt::CheckState::Unchecked);
        }
    }

    if(settingsJsonObject["visualizationShowErythrocytes"].isBool())
    {
        if(settingsJsonObject["visualizationShowErythrocytes"].toBool())
        {
            ui->checkBoxVisualizationShowErythrocytes->setCheckState(Qt::CheckState::Checked);
        }
        else
        {
            ui->checkBoxVisualizationShowErythrocytes->setCheckState(Qt::CheckState::Unchecked);
        }
    }

    if(settingsJsonObject["visualizationShowHemoglobin"].isBool())
    {
        if(settingsJsonObject["visualizationShowHemoglobin"].toBool())
        {
            ui->checkBoxVisualizationShowHemoglobin->setCheckState(Qt::CheckState::Checked);
        }
        else
        {
            ui->checkBoxVisualizationShowHemoglobin->setCheckState(Qt::CheckState::Unchecked);
        }
    }

    if(settingsJsonObject["visualizationShowThrombocytes"].isBool())
    {
        if(settingsJsonObject["visualizationShowThrombocytes"].toBool())
        {
            ui->checkBoxVisualizationShowThrombocytes->setCheckState(Qt::CheckState::Checked);
        }
        else
        {
            ui->checkBoxVisualizationShowThrombocytes->setCheckState(Qt::CheckState::Unchecked);
        }
    }

    if(settingsJsonObject["visualizationShowMedicamentationAndChemoTherapy"].isBool())
    {
        if(settingsJsonObject["visualizationShowMedicamentationAndChemoTherapy"].toBool())
        {
            ui->checkBoxVisualizationShowMedicamentationAndChemoTherapy->setCheckState(Qt::CheckState::Checked);
        }
        else
        {
            ui->checkBoxVisualizationShowMedicamentationAndChemoTherapy->setCheckState(Qt::CheckState::Unchecked);
        }
    }

    // Prepare tables.

    ui->tableWidgetBloodSamples->setColumnCount(static_cast<int>(tableWidgetBloodSamplesColumns.size()));

    for(auto i = 0; i < tableWidgetBloodSamplesColumns.size(); i++)
    {
        ui->tableWidgetBloodSamples->setHorizontalHeaderItem(i, new QTableWidgetItem(tableWidgetBloodSamplesColumns[i]));
    }

    ui->tableWidgetBloodSamples->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->tableWidgetChemoAndMeds->setColumnCount(static_cast<int>(tableWidgetChemoAndMedsColumns.size()));

    for(auto i = 0; i < tableWidgetChemoAndMedsColumns.size(); i++)
    {
        ui->tableWidgetChemoAndMeds->setHorizontalHeaderItem(i, new QTableWidgetItem(tableWidgetChemoAndMedsColumns[i]));
    }

    ui->tableWidgetChemoAndMeds->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Setup plot.

    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iSelectLegend | QCP::iSelectPlottables);

    ui->customPlot->xAxis->setLabel("Date");

    // Configure horizontal axis to show date.
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("dd.MM.yyyy");
    ui->customPlot->xAxis->setTicker(dateTicker);

    // Initialize with current date.
    double currentSecondsSinceEpoch = QDateTime::currentSecsSinceEpoch();
    ui->customPlot->xAxis->setRange(currentSecondsSinceEpoch, currentSecondsSinceEpoch + 1);
}

MainWindow::~MainWindow()
{
    if(m_patientDataChangedSinceLastSave)
    {
        askPatientDataFileSave();
    }

    saveSettingsFile();
    delete ui;
}

// Initialization steps, must not be called before constructor and show() have been called
// in order to make plot axes scaling detection work properly.
void MainWindow::initializeAfterShowing()
{
    // Auto-load previously opened patient data file if this setting is activated and a valid
    // previous file name exists.
    if(m_settingsWindow.getSettings().autoLoadPatientDataFileOnStartup && QFile::exists(m_previousPatientDataFileName))
    {
        loadPatientDataFile(m_previousPatientDataFileName);
    }

    // This is a workaround. When visualization is not opened at this point (i.e. other tab is
    // selected on startup), axes scaling detection does not work properly so we have to replot
    // the next time the tab is opened.
    if(ui->tabWidget->currentIndex() != 3)
    {
        m_tableDataChangedSinceLastVisualizationPlot = true;
    }
}

// Loads the patient data file, fills all forms and triggers visualization plot.
void MainWindow::loadPatientDataFile(QString& patientDataFileName)
{
    m_internalTableModificationsInProgress = true;
    m_patientDataChangedSinceLastSave = false;

    ui->labelPatientDataFile->setText(patientDataFileName);

    // Store the file name so it can be written to the settings file on program exit
    // and be restored at the next program execution.
    m_previousPatientDataFileName = patientDataFileName;

    QFile patientDataFile;
    patientDataFile.setFileName(patientDataFileName);
    patientDataFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString patientDataString = patientDataFile.readAll();
    patientDataFile.close();
    QJsonDocument patientDataJsonDocument = QJsonDocument::fromJson(patientDataString.toUtf8());
    QJsonObject patientDataJsonObject = patientDataJsonDocument.object();

    // Fill forms with given patient data.

    ui->lineEditPatientName->setText(patientDataJsonObject["name"].toString());
    ui->lineEditPatientDateOfBirth->setText(patientDataJsonObject["dateOfBirth"].toString());
    ui->lineEditPatientSize->setText(patientDataJsonObject["size"].toString());
    ui->lineEditPatientWeight->setText(patientDataJsonObject["weight"].toString());
    ui->lineEditPatientBodySurface->setText(patientDataJsonObject["bodySurface"].toString());

    auto bloodSamplesArraySize = patientDataJsonObject["bloodSamples"].toArray().size();

    ui->tableWidgetBloodSamples->setRowCount(bloodSamplesArraySize);

    for(auto i = 0; i < bloodSamplesArraySize; i++)
    {
        // Date
        QString dateString = patientDataJsonObject["bloodSamples"][i]["date"].toString();
        ui->tableWidgetBloodSamples->setItem(i,
                                             tableWidgetBloodSamplesColumns.indexOf("Date"),
                                             new QTableWidgetItem(dateString));

        // We expect the values to be of type double. If not, user may have entered nothing so we expect an
        // empty string so we ignore the value for the graph.

        // Leukocytes

        if(patientDataJsonObject["bloodSamples"][i]["leukocytes"].isDouble())
        {
            double leukocytes = patientDataJsonObject["bloodSamples"][i]["leukocytes"].toDouble();
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Leukocytes"),
                                                 new QTableWidgetItem(QString::number(leukocytes)));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["leukocytes"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Leukocytes"),
                                                 new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["leukocytes"].toString()));
        }

        // Erythrocytes

        if(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].isDouble())
        {
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Erythrocytes"),
                                                 new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].toDouble())));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Erythrocytes"),
                                                 new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].toString()));
        }

        // Hemoglobin

        if(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].isDouble())
        {
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Hemoglobin"),
                                                 new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].toDouble())));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Hemoglobin"),
                                                 new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].toString()));
        }

        // Thrombocytes

        if(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].isDouble())
        {
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Thrombocytes"),
                                                 new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].toDouble())));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i,
                                                 tableWidgetBloodSamplesColumns.indexOf("Thrombocytes"),
                                                 new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].toString()));
        }
    }

    auto chemoAndMedsArraySize = patientDataJsonObject["chemoTherapyAndMedicamentation"].toArray().size();

    ui->tableWidgetChemoAndMeds->setRowCount(chemoAndMedsArraySize);

    for(auto i = 0; i < chemoAndMedsArraySize; i++)
    {
        // Date
        QString dateString = patientDataJsonObject["chemoTherapyAndMedicamentation"][i]["date"].toString();
        ui->tableWidgetChemoAndMeds->setItem(i, tableWidgetChemoAndMedsColumns.indexOf("Date (Start)"), new QTableWidgetItem(dateString));

        // Name
        QString nameString = patientDataJsonObject["chemoTherapyAndMedicamentation"][i]["name"].toString();
        ui->tableWidgetChemoAndMeds->setItem(i,
                                             tableWidgetChemoAndMedsColumns.indexOf("Name"),
                                             new QTableWidgetItem(nameString));

        // Dose
        QString doseString = patientDataJsonObject["chemoTherapyAndMedicamentation"][i]["dose"].toString();
        ui->tableWidgetChemoAndMeds->setItem(i,
                                             tableWidgetChemoAndMedsColumns.indexOf("Dose per Day"),
                                             new QTableWidgetItem(doseString));

        // Days
        QString daysString = patientDataJsonObject["chemoTherapyAndMedicamentation"][i]["days"].toString();
        ui->tableWidgetChemoAndMeds->setItem(i,
                                             tableWidgetChemoAndMedsColumns.indexOf("Days"),
                                             new QTableWidgetItem(daysString));
    }

    m_internalTableModificationsInProgress = false;

    plotVisualization();
}

// Saves the settings file after writing the current settings.
void MainWindow::saveSettingsFile()
{
    QFile settingsFile;
    settingsFile.setFileName(QDir::currentPath() + "/" + "leukiSettings.json");

    settingsFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString settingsString = settingsFile.readAll();
    settingsFile.close();
    QJsonDocument settingsJsonDocument = QJsonDocument::fromJson(settingsString.toUtf8());
    QJsonObject settingsJsonObject = settingsJsonDocument.object();

    settingsFile.open(QIODevice::WriteOnly | QIODevice::Text);

    settingsJsonObject["previousPatientDataFileName"] = m_previousPatientDataFileName;
    settingsJsonObject["activeTabIndex"] = static_cast<double>(ui->tabWidget->currentIndex());

    auto settings = m_settingsWindow.getSettings();
    settingsJsonObject["autoLoadPatientDataFileOnStartup"] = settings.autoLoadPatientDataFileOnStartup;

    settingsJsonObject["visualizationShowLeukocytes"] = (ui->checkBoxVisualizationShowLeukocytes->checkState() == Qt::CheckState::Checked);
    settingsJsonObject["visualizationShowErythrocytes"] = (ui->checkBoxVisualizationShowErythrocytes->checkState() == Qt::CheckState::Checked);
    settingsJsonObject["visualizationShowHemoglobin"] = (ui->checkBoxVisualizationShowHemoglobin->checkState() == Qt::CheckState::Checked);
    settingsJsonObject["visualizationShowThrombocytes"] = (ui->checkBoxVisualizationShowThrombocytes->checkState() == Qt::CheckState::Checked);
    settingsJsonObject["visualizationShowMedicamentationAndChemoTherapy"] = (ui->checkBoxVisualizationShowMedicamentationAndChemoTherapy->checkState() == Qt::CheckState::Checked);

    settingsJsonDocument.setObject(settingsJsonObject);

    settingsFile.write(settingsJsonDocument.toJson());

    settingsFile.close();
}

// Replaces all empty (null) cell contents of the passed table with an empty string.
void MainWindow::ensureTableWidgetCellsAreNotNull(QTableWidget& tableWidget)
{
    for(auto row = 0; row < tableWidget.rowCount(); row++)
    {
        for(auto column = 0; column < tableWidget.columnCount(); column++)
        {
            if(!tableWidget.item(row, column))
            {
                tableWidget.setItem(row, column, new QTableWidgetItem(""));
            }
        }
    }
}

// Deletes the selected rows of the passed table.
// Returns the number of deleted rows.
qsizetype MainWindow::deleteSelectedTableRows(QTableWidget& qTableWidget)
{
    // Check if one or more items are selected.
    if(qTableWidget.selectionModel()->hasSelection())
    {
       auto selectedRows = qTableWidget.selectionModel()->selectedRows();

       // Selected rows are stored in selection order, so find the first selected row's index
       // first by iterating through all selected rows.
       int firstRowToDeleteIndex = INT_MAX;

       for(auto i = 0; i < selectedRows.count(); i++)
       {
           if(selectedRows[i].row() < firstRowToDeleteIndex)
           {
               firstRowToDeleteIndex = selectedRows[i].row();
           }
       }

       for(auto i = 0; i < selectedRows.count(); i++)
       {
           // The item below the previously gets it's index, so use the same index for all
           // items to delete.
           qTableWidget.removeRow(firstRowToDeleteIndex);
       }

       return selectedRows.count();
    }

    return 0;
}

// Sorts the passed row of the passed table in the table so that table is sorted date ascending.
void MainWindow::sortEditedTableRow(QTableWidget& table, int row, int dateColumn)
{
    // Re-Sort if required.
    auto dateOfEditedRow = QDateTime::fromString(table.item(row, dateColumn)->text(), "dd.MM.yyyy").toSecsSinceEpoch();

    int rowToMoveNewItemTo = 0;

    for(int i = 0; i < table.rowCount() - 1; i++)
    {
        auto dateOfCurrentRow = QDateTime::fromString(table.item(i, dateColumn)->text(), "dd.MM.yyyy").toSecsSinceEpoch();
        long long dateOfNextRow = LLONG_MAX;

        if(i < table.rowCount() - 2)
        {
           dateOfNextRow = QDateTime::fromString(table.item(i + 1, dateColumn)->text(), "dd.MM.yyyy").toSecsSinceEpoch();
        }

        // Edited item must be placed before first item.
        if(i == 0 && dateOfEditedRow < dateOfCurrentRow)
        {
            rowToMoveNewItemTo = i;
        }
        // Edited item must be placed between two items.
        else if(dateOfEditedRow >= dateOfCurrentRow && dateOfEditedRow < dateOfNextRow)
        {
            rowToMoveNewItemTo = i + 1;
        }
    }

    if(row != rowToMoveNewItemTo)
    {
        m_internalTableModificationsInProgress = true;

        // Insert a new row at the destination row position.
        table.insertRow(rowToMoveNewItemTo);

        // Copy new user-added row's contents to the inserted new row.
        for(auto i = 0; i < table.columnCount(); i++)
        {
            // At this point, increase row by one for accessing the new added row
            // since it's index has been increased by inserting a new row above.
            if(table.item(row + 1, i))
            {
                table.setItem(rowToMoveNewItemTo,
                              i,
                              new QTableWidgetItem(table.item(row + 1, i)->text()));
            }
        }

        // Delete new user-added row after copying.
        table.removeRow(row + 1);

        // Scroll to new added row.
        table.scrollTo(table.model()->index(rowToMoveNewItemTo, 0));

        m_internalTableModificationsInProgress = false;
    }
}

bool MainWindow::checkDateFormat(QString dateString)
{
    auto dateStdString = dateString.toStdString();
    std::regex regex("\\b\\d{2}[.]\\d{2}[.]\\d{4}\\b");
    std::smatch match;

    auto ret = std::regex_match(dateStdString, match, regex);

    return ret;
}

// Handles a user-initiated change in a date table cell. Triggers date validation
// and sorting of the table.
void MainWindow::handleDateCellChange(QTableWidget& table, int row, int column)
{
    QString dateString = table.item(row, column)->text();

    if(!checkDateFormat(dateString))
    {
        QMessageBox::information(this,
                                 "Leuki - Invalid Date Entry",
                                 "Warning: Table Row " + QString::number(row + 1) +
                                 " contains an invalid date entry (" +
                                 dateString +
                                 ")! Format must be dd.MM.yyyy .");
    }
    else if (!m_internalTableModificationsInProgress)
    {
        sortEditedTableRow(table,
                           row,
                           column);
    }
}

void MainWindow::askPatientDataFileSave()
{
    auto ret = QMessageBox::question(this,
                                     "Leuki - Patient Data Changed",
                                     "Patient data has been changed since last save! Save first? Otherwise, unsaved data will be lost!",
                                     QMessageBox::Yes|QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        on_actionSettingsSaveAs_triggered();
    }
}

// (Re-)Plots the visualization.
void MainWindow::plotVisualization()
{
    // Clear everything before (re)plotting.
    ui->customPlot->clearGraphs();
    ui->customPlot->clearItems();
    ui->customPlot->clearPlottables();
    ui->customPlot->clearFocus();
    ui->customPlot->clearMask();

    QString yAxisLabel;

    if(ui->checkBoxVisualizationShowLeukocytes->isChecked())
    {
        yAxisLabel = yAxisLabel + ", Leukocytes [Giga/l]";
    }

    if(ui->checkBoxVisualizationShowErythrocytes->isChecked())
    {
        yAxisLabel = yAxisLabel + ", Erythrocytes [Tera / l]";
    }

    if(ui->checkBoxVisualizationShowHemoglobin->isChecked())
    {
        yAxisLabel = yAxisLabel + ", Hemoglobin [g/dl]";
    }

    if(ui->checkBoxVisualizationShowThrombocytes->isChecked())
    {
        yAxisLabel = yAxisLabel + ", Thrombocytes [Giga/l]";
    }

    // Remove leading ", "
    yAxisLabel.remove(0, std::string(", ").length());

    ui->customPlot->yAxis->setLabel(yAxisLabel);

    auto bloodSamplesCount = ui->tableWidgetBloodSamples->rowCount();
    double yAxisMax = 0.0;

    // At this point, assume that the first column (index 0) is the date column.
    for(auto column = tableWidgetBloodSamplesColumns.indexOf("Date") + 1; column < tableWidgetBloodSamplesColumns.size(); column++)
    {
        ui->customPlot->addGraph();
        ui->customPlot->graph(column - 1)->setLineStyle(QCPGraph::lsLine);
        ui->customPlot->graph(column - 1)->setScatterStyle(QCPScatterStyle::ssStar);

        if(column == tableWidgetBloodSamplesColumns.indexOf("Leukocytes"))
        {
            if(!ui->checkBoxVisualizationShowLeukocytes->isChecked())
            {
                continue;
            }

            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::blue));
        }
        else if(column == tableWidgetBloodSamplesColumns.indexOf("Erythrocytes"))
        {
            if(!ui->checkBoxVisualizationShowErythrocytes->isChecked())
            {
                continue;
            }

            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::red));
        }
        else if(column == tableWidgetBloodSamplesColumns.indexOf("Hemoglobin"))
        {
            if(!ui->checkBoxVisualizationShowHemoglobin->isChecked())
            {
                continue;
            }

            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::magenta));
        }
        else if(column == tableWidgetBloodSamplesColumns.indexOf("Thrombocytes"))
        {
            if(!ui->checkBoxVisualizationShowThrombocytes->isChecked())
            {
                continue;
            }

            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::darkYellow));
        }
        else
        {
            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::black));
        }

        QVector<QCPGraphData> graphData;

        for(auto bloodSampleIndex = 0; bloodSampleIndex < bloodSamplesCount; bloodSampleIndex++)
        {
            if(// Ignore cells of rows with an invalid date.
               ui->tableWidgetBloodSamples->item(bloodSampleIndex, tableWidgetBloodSamplesColumns.indexOf("Date")) &&
               checkDateFormat(ui->tableWidgetBloodSamples->item(bloodSampleIndex, tableWidgetBloodSamplesColumns.indexOf("Date"))->text()) &&
               // Ignore empty cells.
               ui->tableWidgetBloodSamples->item(bloodSampleIndex, column) &&
               ui->tableWidgetBloodSamples->item(bloodSampleIndex, column)->text() != "")
            {
                QCPGraphData graphPoint;

                graphPoint.key = QDateTime::fromString(ui->tableWidgetBloodSamples->item(bloodSampleIndex, tableWidgetBloodSamplesColumns.indexOf("Date"))->text(), "dd.MM.yyyy").toSecsSinceEpoch();
                graphPoint.value = ui->tableWidgetBloodSamples->item(bloodSampleIndex, column)->text().toDouble();

                graphData.append(graphPoint);

                if(graphPoint.value > yAxisMax)
                {
                    yAxisMax = graphPoint.value;
                }
            }
        }

        ui->customPlot->graph(column - 1)->data()->set(graphData);
    }

    // Plot (date axis range)
    if(bloodSamplesCount)
    {
        double firstBloodSampleDateSecsSinceEpoch = 0.0;
        bool entryFound = false;

        for(auto i = 0; i < ui->tableWidgetBloodSamples->rowCount(); i++)
        {
            if(ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Date")) &&
               checkDateFormat(ui->tableWidgetBloodSamples->item(i, 0)->text()))
            {
                firstBloodSampleDateSecsSinceEpoch = QDateTime::fromString(ui->tableWidgetBloodSamples->item(0, tableWidgetBloodSamplesColumns.indexOf("Date"))->text(), "dd.MM.yyyy").toSecsSinceEpoch();
                entryFound = true;
                break;
            }
        }

        double lastBloodSampleDateSecsSinceEpoch = 0.0;

        if(entryFound)
        {
            entryFound = false;

            for(auto i = ui->tableWidgetBloodSamples->rowCount() - 1; i >= 0; i--)
            {
                if(ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Date")) &&
                   checkDateFormat(ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Date"))->text()))
                {
                    lastBloodSampleDateSecsSinceEpoch = QDateTime::fromString(ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Date"))->text(), "dd.MM.yyyy").toSecsSinceEpoch();
                    entryFound = true;
                    break;
                }
            }
        }

        if(!entryFound)
        {
            QMessageBox::information(this,
                                     "Leuki - No Valid Date Entries",
                                     "Warning: No valid date entries for plot x-axes scaling found!");

            return;
        }

        ui->customPlot->xAxis->setRange(firstBloodSampleDateSecsSinceEpoch - secondsPerDay, lastBloodSampleDateSecsSinceEpoch + secondsPerDay);
    }

    ui->customPlot->yAxis->setRange(0, yAxisMax);

    unsigned int maxTextLabelsStacked = 0;

    if(ui->checkBoxVisualizationShowMedicamentationAndChemoTherapy->isChecked())
    {
        m_textLabelStatistics.clear();

        auto chemoAndMedsCount = ui->tableWidgetChemoAndMeds->rowCount();

        for(auto i = 0; i < chemoAndMedsCount; i++)
        {
            auto secondsSinceEpoch = QDateTime::fromString(ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Date (Start)"))->text(), "dd.MM.yyyy").toSecsSinceEpoch();
            int days = 1;

            if(ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Days")))
            {
                bool conversionSuccessful = false;
                int ret = ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Days"))->text().toInt(&conversionSuccessful);

                if(conversionSuccessful)
                {
                    days = ret;
                }
            }

            // Text Label

            QCPItemText *textLabel = new QCPItemText(ui->customPlot);
            textLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
            textLabel->position->setType(QCPItemPosition::ptPlotCoords);

            // Check how many labels are already placed at the current x-axis position.
            unsigned int textLabelsAtCurrentXAxisPosition = 0;
            bool entryFound = false;

            for(auto i = 0; i < m_textLabelStatistics.size(); i++)
            {
                if(m_textLabelStatistics[i].dateSecondsSinceEpoch == secondsSinceEpoch)
                {
                    m_textLabelStatistics[i].textLabelCount++;
                    textLabelsAtCurrentXAxisPosition = m_textLabelStatistics[i].textLabelCount;
                    entryFound = true;
                    break;
                }
            }

            if(!entryFound)
            {
                text_label_statistics_item_t m_textLabelStatisticsItem;
                m_textLabelStatisticsItem.dateSecondsSinceEpoch = secondsSinceEpoch;
                m_textLabelStatisticsItem.textLabelCount = 1;
                textLabelsAtCurrentXAxisPosition = 1;

                m_textLabelStatistics.push_back(m_textLabelStatisticsItem);
            }

            // Place the label in the middle of it's time span.
            textLabel->position->setPixelPosition(QPointF(ui->customPlot->xAxis->coordToPixel(static_cast<double>(secondsSinceEpoch) +
                                                                                              (static_cast<double>(days - 1) * 0.5) *
                                                                                              static_cast<double>(secondsPerDay)),
                                                                                              ui->customPlot->yAxis->coordToPixel(0) +
                                                                                              lengthVisualizationArrowPixels +
                                                                                              heightVisualizationTextLabelPixels *
                                                                                              (textLabelsAtCurrentXAxisPosition - 1)));

            if(textLabelsAtCurrentXAxisPosition > maxTextLabelsStacked)
            {
                maxTextLabelsStacked = textLabelsAtCurrentXAxisPosition;
            }

            QString name = "";
            QString dose = "";

            // Check cells for content before evaluating to avoid nullptr-access.

            if(ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Name")))
            {
                name = ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Name"))->text();
            }

            if(ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Dose per Day")))
            {
                dose = ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Dose per Day"))->text();
            }

            textLabel->setText(name + "\n" + dose);
            textLabel->setPen(QPen(Qt::black));

            for(auto i = 0; i < days; i++)
            {
                // Arrow from text label to x-axis
                QCPItemLine *arrow = new QCPItemLine(ui->customPlot);
                arrow->start->setParentAnchor(textLabel->top);
                arrow->end->setCoords(secondsSinceEpoch + i * secondsPerDay, 0);
                arrow->setHead(QCPLineEnding::esSpikeArrow);
            }
        }
    }

    auto yAxisPixelsPerStep = abs(ui->customPlot->yAxis->coordToPixel(1) - ui->customPlot->yAxis->coordToPixel(0));
    auto yAxisMin = - ((heightVisualizationTextLabelPixels * maxTextLabelsStacked + lengthVisualizationArrowPixels) / yAxisPixelsPerStep);

    ui->customPlot->yAxis->setRange(yAxisMin, yAxisMax);

    ui->customPlot->replot();
}

void MainWindow::on_pushButtonAddBloodSample_clicked()
{
    ui->tableWidgetBloodSamples->setRowCount(ui->tableWidgetBloodSamples->rowCount() + 1);

    // Scroll to new added row.
    ui->tableWidgetBloodSamples->scrollTo(ui->tableWidgetBloodSamples->model()->index(ui->tableWidgetBloodSamples->rowCount() - 1, 0));
}

void MainWindow::on_actionSettingsSaveAs_triggered()
{
    QFileInfo patientDataFileInfo(m_previousPatientDataFileName);

    QString patientDataFileName = QFileDialog::getSaveFileName(this,
                                                               tr("Save File"),
                                                               patientDataFileInfo.absolutePath(),
                                                               tr("JSON (*.json)"));

    // If the file save dialog has been cancelled by the user, file name is "", so length is 0.
    if(!patientDataFileName.length())
    {
        return;
    }

    // Collect and write all data to the selected patient data file.

    QJsonDocument patientDataJsonDocument;
    QJsonObject patientDataJsonObject;

    patientDataJsonObject["name"] = ui->lineEditPatientName->text();
    patientDataJsonObject["dateOfBirth"] = ui->lineEditPatientDateOfBirth->text();
    patientDataJsonObject["size"] = ui->lineEditPatientSize->text();
    patientDataJsonObject["weight"] = ui->lineEditPatientWeight->text();
    patientDataJsonObject["bodySurface"] = ui->lineEditPatientBodySurface->text();

    // Make sure that each cell of the blood samples table contains some text.
    ensureTableWidgetCellsAreNotNull(*(ui->tableWidgetBloodSamples));

    QJsonArray bloodSamplesArray;
    auto bloodSamplesArraySize = ui->tableWidgetBloodSamples->rowCount();

    for(auto i = 0; i < bloodSamplesArraySize; i++)
    {
        QJsonObject bloodSamplesJsonObject;

        bloodSamplesJsonObject["date"] = ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Date"))->text();

        bool conversionSuccessful = false;

        bloodSamplesJsonObject["leukocytes"] = ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Leukocytes"))->text().toDouble(&conversionSuccessful);

        if(!conversionSuccessful)
        {
            bloodSamplesJsonObject["leukocytes"] = "";
        }

        bloodSamplesJsonObject["erythrocytes"] = ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Erythrocytes"))->text().toDouble(&conversionSuccessful);

        if(!conversionSuccessful)
        {
            bloodSamplesJsonObject["erythrocytes"] = "";
        }

        bloodSamplesJsonObject["hemoglobin"] = ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Hemoglobin"))->text().toDouble(&conversionSuccessful);

        if(!conversionSuccessful)
        {
            bloodSamplesJsonObject["hemoglobin"] = "";
        }

        bloodSamplesJsonObject["thrombocytes"] = ui->tableWidgetBloodSamples->item(i, tableWidgetBloodSamplesColumns.indexOf("Thrombocytes"))->text().toDouble(&conversionSuccessful);

        if(!conversionSuccessful)
        {
            bloodSamplesJsonObject["thrombocytes"] = "";
        }

        bloodSamplesArray.push_back(bloodSamplesJsonObject);
    }

    patientDataJsonObject["bloodSamples"] = bloodSamplesArray;

    // Make sure that each cell of the chemo and meds table contains some text.
    ensureTableWidgetCellsAreNotNull(*(ui->tableWidgetChemoAndMeds));

    QJsonArray chemoAndMedsArray;
    auto chemoAndMedsArraySize = ui->tableWidgetChemoAndMeds->rowCount();

    for(auto i = 0; i < chemoAndMedsArraySize; i++)
    {
        QJsonObject chemoAndMedsJsonObject;

        chemoAndMedsJsonObject["date"] = ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Date (Start)"))->text();
        chemoAndMedsJsonObject["name"] = ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Name"))->text();
        chemoAndMedsJsonObject["dose"] = ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Dose per Day"))->text();
        chemoAndMedsJsonObject["days"] = ui->tableWidgetChemoAndMeds->item(i, tableWidgetChemoAndMedsColumns.indexOf("Days"))->text();

        chemoAndMedsArray.push_back(chemoAndMedsJsonObject);
    }

    patientDataJsonObject["chemoTherapyAndMedicamentation"] = chemoAndMedsArray;

    patientDataJsonDocument.setObject(patientDataJsonObject);

    QFile patientDataFile;
    patientDataFile.setFileName(patientDataFileName);
    patientDataFile.open(QIODevice::WriteOnly | QIODevice::Text);
    patientDataFile.write(patientDataJsonDocument.toJson());
    patientDataFile.close();

    m_patientDataChangedSinceLastSave = false;
}

void MainWindow::on_actionOpenPatientDataFile_triggered()
{
    // Ask for saving patient data file first if there are unsaved changes.
    if(m_patientDataChangedSinceLastSave)
    {
        askPatientDataFileSave();
    }

    // Load patient data from patient data file.

    QFileInfo patientDataFileInfo(m_previousPatientDataFileName);

    QString patientDataFileName = QFileDialog::getOpenFileName(this,
                                                               tr("Open File"),
                                                               patientDataFileInfo.absolutePath(),
                                                               tr("JSON (*.json)"));

    // If the file open dialog has been cancelled by the user, file name is "", so length is 0.
    if(patientDataFileName.length())
    {
        loadPatientDataFile(patientDataFileName);
    }
}

void MainWindow::on_pushButtonAddChemoAndMed_clicked()
{
    ui->tableWidgetChemoAndMeds->setRowCount(ui->tableWidgetChemoAndMeds->rowCount() + 1);

    // Scroll to new added row.
    ui->tableWidgetChemoAndMeds->scrollTo(ui->tableWidgetChemoAndMeds->model()->index(ui->tableWidgetChemoAndMeds->rowCount() - 1, 0));
}

void MainWindow::on_checkBoxVisualizationShowLeukocytes_stateChanged(int arg1)
{
    plotVisualization();
}

void MainWindow::on_checkBoxVisualizationShowErythrocytes_stateChanged(int arg1)
{
    plotVisualization();
}

void MainWindow::on_checkBoxVisualizationShowHemoglobin_stateChanged(int arg1)
{
    plotVisualization();
}

void MainWindow::on_checkBoxVisualizationShowThrombocytes_stateChanged(int arg1)
{
    plotVisualization();
}

void MainWindow::on_checkBoxVisualizationShowMedicamentationAndChemoTherapy_stateChanged(int arg1)
{
    plotVisualization();
}

void MainWindow::on_actionSettings_triggered()
{
    m_settingsWindow.show();
}

void MainWindow::on_tableWidgetBloodSamples_cellChanged(int row, int column)
{
    // Check date format, must be dd.MM.yyyy .
    if(column == tableWidgetBloodSamplesColumns.indexOf("Date"))
    {
        handleDateCellChange(*(ui->tableWidgetBloodSamples), row, column);
    }

    // If the cell data is not changed by the application itself during patient data
    // file loading, set the respective flags.
    if(!m_internalTableModificationsInProgress)
    {
        m_tableDataChangedSinceLastVisualizationPlot = true;
        m_patientDataChangedSinceLastSave = true;
    }
}

void MainWindow::on_tableWidgetChemoAndMeds_cellChanged(int row, int column)
{
    // Check date format, must be dd.MM.yyyy .
    if(column == tableWidgetChemoAndMedsColumns.indexOf("Date (Start)"))
    {
        handleDateCellChange(*(ui->tableWidgetChemoAndMeds), row, column);
    }

    // If the cell data is not changed by the application itself during patient data
    // file loading, set the respective flags.
    if(!m_internalTableModificationsInProgress)
    {
        m_tableDataChangedSinceLastVisualizationPlot = true;
        m_patientDataChangedSinceLastSave = true;
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    // If visualization tab is clicked, replot if table data has been changed since last plot.
    if(index == tabWidgetTabs.indexOf("Visualization") && m_tableDataChangedSinceLastVisualizationPlot)
    {
        m_tableDataChangedSinceLastVisualizationPlot = false;
        plotVisualization();
    }
}

void MainWindow::on_lineEditPatientName_textEdited(const QString &arg1)
{
    m_patientDataChangedSinceLastSave = true;
}

void MainWindow::on_lineEditPatientDateOfBirth_textEdited(const QString &arg1)
{
    m_patientDataChangedSinceLastSave = true;
}

void MainWindow::on_lineEditPatientSize_textEdited(const QString &arg1)
{
    m_patientDataChangedSinceLastSave = true;
}

void MainWindow::on_lineEditPatientWeight_textEdited(const QString &arg1)
{
    m_patientDataChangedSinceLastSave = true;
}

void MainWindow::on_lineEditPatientBodySurface_textEdited(const QString &arg1)
{
    m_patientDataChangedSinceLastSave = true;
}

void MainWindow::on_pushButtonDeleteSelectedBloodSample_clicked()
{
    auto ret = deleteSelectedTableRows(*(ui->tableWidgetBloodSamples));

    if(ret)
    {
        m_tableDataChangedSinceLastVisualizationPlot = true;
        m_patientDataChangedSinceLastSave = true;
    }
}

void MainWindow::on_pushButtonDeleteSelectedChemoAndMed_clicked()
{
    auto ret = deleteSelectedTableRows(*(ui->tableWidgetChemoAndMeds));

    if(ret)
    {
        m_tableDataChangedSinceLastVisualizationPlot = true;
        m_patientDataChangedSinceLastSave = true;
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(0,
                       "Leuki - About",
                       "Leuki is published under the GPL-3.0 license. For information on licenses and source code, visit: <a href='https://github.com/svenwml/Leuki'>https://github.com/svenwml/Leuki</a>");
}


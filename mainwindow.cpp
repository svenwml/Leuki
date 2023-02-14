#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>

const char* leukiSettingsDefault =
#include "leukiSettingsDefault.txt"
;

const static unsigned int heightVisualizationTextLabelPixels = 45;
const static unsigned int lengthVisualizationArrowPixels = 15;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tableDataChangedSinceLastVisualizationPlot(false)
    , m_patientDataChangedSinceLastSave(false)
    , m_loadingPatientDataInProgress(false)
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

    ui->tableWidgetBloodSamples->setColumnCount(5);
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(0, new QTableWidgetItem("Date"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(1, new QTableWidgetItem("Leukocytes"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(2, new QTableWidgetItem("Erythrocytes"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(3, new QTableWidgetItem("Hemoglobin"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(4, new QTableWidgetItem("Thrombocytes"));

    ui->tableWidgetChemoAndMeds->setColumnCount(3);
    ui->tableWidgetChemoAndMeds->setHorizontalHeaderItem(0, new QTableWidgetItem("Date"));
    ui->tableWidgetChemoAndMeds->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
    ui->tableWidgetChemoAndMeds->setHorizontalHeaderItem(2, new QTableWidgetItem("Dose"));

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
    m_loadingPatientDataInProgress = true;

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

    auto bloodSamplesArraySize = patientDataJsonObject["bloodSamples"].toArray().size();

    ui->tableWidgetBloodSamples->setRowCount(bloodSamplesArraySize);

    for(auto i = 0; i < bloodSamplesArraySize; i++)
    {
        // Date
        QString dateString = patientDataJsonObject["bloodSamples"][i]["date"].toString();
        ui->tableWidgetBloodSamples->setItem(i, 0, new QTableWidgetItem(dateString));

        // We expect the values to be of type double. If not, user may have entered nothing so we expect an
        // empty string so we ignore the value for the graph.

        // Leukocytes

        if(patientDataJsonObject["bloodSamples"][i]["leukocytes"].isDouble())
        {
            double leukocytes = patientDataJsonObject["bloodSamples"][i]["leukocytes"].toDouble();
            ui->tableWidgetBloodSamples->setItem(i, 1, new QTableWidgetItem(QString::number(leukocytes)));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["leukocytes"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i, 1, new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["leukocytes"].toString()));
        }

        // Erythrocytes

        if(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].isDouble())
        {
            ui->tableWidgetBloodSamples->setItem(i, 2, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].toDouble())));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i, 2, new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].toString()));
        }

        // Hemoglobin

        if(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].isDouble())
        {
            ui->tableWidgetBloodSamples->setItem(i, 3, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].toDouble())));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i, 3, new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].toString()));
        }

        // Thrombocytes

        if(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].isDouble())
        {
            ui->tableWidgetBloodSamples->setItem(i, 4, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].toDouble())));
        }
        else if(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].isString())
        {
            ui->tableWidgetBloodSamples->setItem(i, 4, new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].toString()));
        }
    }

    auto chemoAndMedsArraySize = patientDataJsonObject["chemoTherapyAndMedicamentation"].toArray().size();

    ui->tableWidgetChemoAndMeds->setRowCount(chemoAndMedsArraySize);

    for(auto i = 0; i < chemoAndMedsArraySize; i++)
    {
        // Date
        QString dateString = patientDataJsonObject["chemoTherapyAndMedicamentation"][i]["date"].toString();
        ui->tableWidgetChemoAndMeds->setItem(i, 0, new QTableWidgetItem(dateString));

        // Name
        QString nameString = patientDataJsonObject["chemoTherapyAndMedicamentation"][i]["name"].toString();
        ui->tableWidgetChemoAndMeds->setItem(i, 1, new QTableWidgetItem(nameString));

        // Dose
        QString doseString = patientDataJsonObject["chemoTherapyAndMedicamentation"][i]["dose"].toString();
        ui->tableWidgetChemoAndMeds->setItem(i, 2, new QTableWidgetItem(doseString));
    }

    m_loadingPatientDataInProgress = false;

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
            if(!ui->tableWidgetBloodSamples->item(row, column))
            {
                ui->tableWidgetBloodSamples->setItem(row, column, new QTableWidgetItem(""));
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

void MainWindow::askPatientDataFileSave()
{
    auto ret = QMessageBox::question(this,
                                     "Leuki - Patient Data Changed",
                                     "Patient data has been changed since last save! Save first? Otherwise, data will be lost!",
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
    yAxisLabel.remove(0, 2);

    ui->customPlot->yAxis->setLabel(yAxisLabel);

    auto bloodSamplesCount = ui->tableWidgetBloodSamples->rowCount();
    double yAxisMax = 0.0;

    for(auto column = 1; column < 5; column++)
    {
        ui->customPlot->addGraph();
        ui->customPlot->graph(column - 1)->setLineStyle(QCPGraph::lsLine);
        ui->customPlot->graph(column - 1)->setScatterStyle(QCPScatterStyle::ssStar);

        if(column == 1)
        {
            if(!ui->checkBoxVisualizationShowLeukocytes->isChecked())
            {
                continue;
            }

            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::blue));
        }
        else if(column == 2)
        {
            if(!ui->checkBoxVisualizationShowErythrocytes->isChecked())
            {
                continue;
            }

            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::red));
        }
        else if(column == 3)
        {
            if(!ui->checkBoxVisualizationShowHemoglobin->isChecked())
            {
                continue;
            }

            ui->customPlot->graph(column - 1)->setPen(QPen(Qt::magenta));
        }
        else if(column == 4)
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
            // Ignore empty cells.
            if(ui->tableWidgetBloodSamples->item(bloodSampleIndex, column)->text() != "")
            {
                QCPGraphData graphPoint;

                graphPoint.key = QDateTime::fromString(ui->tableWidgetBloodSamples->item(bloodSampleIndex, 0)->text(), "dd.MM.yyyy").toSecsSinceEpoch();
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
        double firstBloodSampleDateSecsSinceEpoch = QDateTime::fromString(ui->tableWidgetBloodSamples->item(0, 0)->text(), "dd.MM.yyyy").toSecsSinceEpoch();
        double lastBloodSampleDateSecsSinceEpoch = QDateTime::fromString(ui->tableWidgetBloodSamples->item(bloodSamplesCount - 1, 0)->text(), "dd.MM.yyyy").toSecsSinceEpoch();
        ui->customPlot->xAxis->setRange(firstBloodSampleDateSecsSinceEpoch - 24*3600, lastBloodSampleDateSecsSinceEpoch + 24*3600);
    }

    ui->customPlot->yAxis->setRange(0, yAxisMax);

    if(ui->checkBoxVisualizationShowMedicamentationAndChemoTherapy->isChecked())
    {
        auto chemoAndMedsCount = ui->tableWidgetChemoAndMeds->rowCount();

        for(auto i = 0; i < chemoAndMedsCount; i++)
        {
            auto secondsSinceEpoch = QDateTime::fromString(ui->tableWidgetChemoAndMeds->item(i, 0)->text(), "dd.MM.yyyy").toSecsSinceEpoch();

            // Text Label
            QCPItemText *textLabel = new QCPItemText(ui->customPlot);
            textLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
            textLabel->position->setType(QCPItemPosition::ptPlotCoords);
            textLabel->position->setPixelPosition(QPointF(ui->customPlot->xAxis->coordToPixel(secondsSinceEpoch),
                                                          ui->customPlot->yAxis->coordToPixel(0) + lengthVisualizationArrowPixels));
            textLabel->setText(ui->tableWidgetChemoAndMeds->item(i, 1)->text() + "\n" + ui->tableWidgetChemoAndMeds->item(i, 2)->text());
            textLabel->setPen(QPen(Qt::black));

            // Arrow from text label to x-axis
            QCPItemLine *arrow = new QCPItemLine(ui->customPlot);
            arrow->start->setParentAnchor(textLabel->top);
            arrow->end->setCoords(secondsSinceEpoch, 0);
            arrow->setHead(QCPLineEnding::esSpikeArrow);
        }
    }

    auto yAxisPixelsPerStep = abs(ui->customPlot->yAxis->coordToPixel(1) - ui->customPlot->yAxis->coordToPixel(0));
    auto yAxisMin = - ((heightVisualizationTextLabelPixels + lengthVisualizationArrowPixels) / yAxisPixelsPerStep);

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

    QFile patientDataFile;
    patientDataFile.setFileName(patientDataFileName);
    patientDataFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QJsonDocument patientDataJsonDocument;
    QJsonObject patientDataJsonObject;

    patientDataJsonObject["name"] = ui->lineEditPatientName->text();
    patientDataJsonObject["dateOfBirth"] = ui->lineEditPatientDateOfBirth->text();

    // Make sure that each cell of the blood samples table contains some text.
    ensureTableWidgetCellsAreNotNull(*(ui->tableWidgetBloodSamples));

    QJsonArray bloodSamplesArray;
    auto bloodSamplesArraySize = ui->tableWidgetBloodSamples->rowCount();

    for(auto i = 0; i < bloodSamplesArraySize; i++)
    {
        QJsonObject bloodSamplesJsonObject;

        bloodSamplesJsonObject["date"] = ui->tableWidgetBloodSamples->item(i, 0)->text();

        bool conversionSuccessful = false;

        bloodSamplesJsonObject["leukocytes"] = ui->tableWidgetBloodSamples->item(i, 1)->text().toDouble(&conversionSuccessful);

        if(!conversionSuccessful)
        {
            bloodSamplesJsonObject["leukocytes"] = "";
        }

        bloodSamplesJsonObject["erythrocytes"] = ui->tableWidgetBloodSamples->item(i, 2)->text().toDouble(&conversionSuccessful);

        if(!conversionSuccessful)
        {
            bloodSamplesJsonObject["erythrocytes"] = "";
        }

        bloodSamplesJsonObject["hemoglobin"] = ui->tableWidgetBloodSamples->item(i, 3)->text().toDouble(&conversionSuccessful);

        if(!conversionSuccessful)
        {
            bloodSamplesJsonObject["hemoglobin"] = "";
        }

        bloodSamplesJsonObject["thrombocytes"] = ui->tableWidgetBloodSamples->item(i, 4)->text().toDouble(&conversionSuccessful);

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

        chemoAndMedsJsonObject["date"] = ui->tableWidgetChemoAndMeds->item(i, 0)->text();
        chemoAndMedsJsonObject["name"] = ui->tableWidgetChemoAndMeds->item(i, 1)->text();
        chemoAndMedsJsonObject["dose"] = ui->tableWidgetChemoAndMeds->item(i, 2)->text();

        chemoAndMedsArray.push_back(chemoAndMedsJsonObject);
    }

    patientDataJsonObject["chemoTherapyAndMedicamentation"] = chemoAndMedsArray;

    patientDataJsonDocument.setObject(patientDataJsonObject);

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

    QString patientDataFileName = QFileDialog::getOpenFileName(this, tr("Open File"),
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
    // If the cell data is not changed by the application itself during patient data
    // file loading, set the respective flags.
    if(!m_loadingPatientDataInProgress)
    {
        m_tableDataChangedSinceLastVisualizationPlot = true;
        m_patientDataChangedSinceLastSave = true;
    }
}


void MainWindow::on_tableWidgetChemoAndMeds_cellChanged(int row, int column)
{
    // If the cell data is not changed by the application itself during patient data
    // file loading, set the respective flags.
    if(!m_loadingPatientDataInProgress)
    {
        m_tableDataChangedSinceLastVisualizationPlot = true;
        m_patientDataChangedSinceLastSave = true;
    }
}


void MainWindow::on_tabWidget_currentChanged(int index)
{
    // If visualization tab is clicked, replot if table data has been changed since last plot.
    if(index == 3 && m_tableDataChangedSinceLastVisualizationPlot)
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


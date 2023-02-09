#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionSettingsSaveAs, SIGNAL(triggered()), this, SLOT(savePatientDataFileAs()));

//    // Load settings file first.
//
//    QFile settingsFile;
//    settingsFile.setFileName(QDir::currentPath() + "/" + "leukiSettings.json");
//    settingsFile.open(QIODevice::ReadOnly | QIODevice::Text);
//    QString settingsString = settingsFile.readAll();
//    settingsFile.close();
//    QJsonDocument settingsJsonDocument = QJsonDocument::fromJson(settingsString.toUtf8());
//    QJsonObject settingsJsonObject = settingsJsonDocument.object();

//    auto patientDataFileName = settingsJsonObject["patientDataFilePath"].toString();

//    if(patientDataFileName == "")
//    {
//        QMessageBox messageBoxNoFileSelected;
//        messageBoxNoFileSelected.setText("No patient data file given! Check Leuki settings file.");
//        messageBoxNoFileSelected.exec();
//        exit(0);
//    }

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
    ui->customPlot->yAxis->setLabel("Leukocytes");

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
    delete ui;
}

void MainWindow::ensureTableWidgetCellsAreNotNull(QTableWidget* tableWidget)
{
    for(auto row = 0; row < tableWidget->rowCount(); row++)
    {
        for(auto column = 0; column < tableWidget->columnCount(); column++)
        {
            if(!ui->tableWidgetBloodSamples->item(row, column))
            {
                ui->tableWidgetBloodSamples->setItem(row, column, new QTableWidgetItem(""));
            }
        }
    }
}

void MainWindow::on_pushButtonNewBloodSample_clicked()
{
    ui->tableWidgetBloodSamples->setRowCount(ui->tableWidgetBloodSamples->rowCount() + 1);
}

void MainWindow::savePatientDataFileAs()
{
    QString patientDataFileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("JSON (*.json)"));

    // Collect and write all data to the selected patient data file.

    QFile patientDataFile;
    patientDataFile.setFileName(patientDataFileName);
    patientDataFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QJsonDocument patientDataJsonDocument;
    QJsonObject patientDataJsonObject;

    patientDataJsonObject["name"] = ui->lineEditPatientName->text();
    patientDataJsonObject["dateOfBirth"] = ui->lineEditPatientDateOfBirth->text();

    // Make sure that each cell of the blood samples table contains some text.
    ensureTableWidgetCellsAreNotNull(ui->tableWidgetBloodSamples);

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
    ensureTableWidgetCellsAreNotNull(ui->tableWidgetChemoAndMeds);

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
}

void MainWindow::on_actionOpenPatientDataFile_triggered()
{
    // Load patient data from patient data file.

    QString patientDataFileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "",
                                                    tr("JSON (*.json)"));

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

    QVector<QCPGraphData> leukocytesGraphData(bloodSamplesArraySize);
    ui->customPlot->addGraph();
    ui->customPlot->graph()->setLineStyle(QCPGraph::lsLine);
    ui->customPlot->graph()->setPen(QPen(Qt::blue));

    double leukocytesMax = 0.0;

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
            leukocytesGraphData[i].key = QDateTime::fromString(dateString, "dd.MM.yyyy").toSecsSinceEpoch();
            leukocytesGraphData[i].value = leukocytes;

            if(leukocytes > leukocytesMax)
            {
                leukocytesMax = leukocytes;
            }
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

        // Text Label
        QCPItemText *textLabel = new QCPItemText(ui->customPlot);
        textLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
        textLabel->position->setType(QCPItemPosition::ptPlotCoords);
        auto secondsSinceEpoch = QDateTime::fromString(dateString, "dd.MM.yyyy").toSecsSinceEpoch();
        textLabel->position->setCoords(secondsSinceEpoch, leukocytesMax + 10);
        textLabel->setText(nameString + "\n" + doseString);
        textLabel->setPen(QPen(Qt::black));

        // Arrow from text label to x-axis
        QCPItemLine *arrow = new QCPItemLine(ui->customPlot);
        arrow->start->setParentAnchor(textLabel->bottom);
        arrow->end->setCoords(secondsSinceEpoch, 0);
        arrow->setHead(QCPLineEnding::esSpikeArrow);
    }

    // Plot (date axis range)
    if(bloodSamplesArraySize)
    {
        double firstBloodSampleDateSecsSinceEpoch = QDateTime::fromString(patientDataJsonObject["bloodSamples"][0]["date"].toString(), "dd.MM.yyyy").toSecsSinceEpoch();
        double lastBloodSampleDateSecsSinceEpoch = QDateTime::fromString(patientDataJsonObject["bloodSamples"][bloodSamplesArraySize - 1]["date"].toString(), "dd.MM.yyyy").toSecsSinceEpoch();
        ui->customPlot->xAxis->setRange(firstBloodSampleDateSecsSinceEpoch - 24*3600, lastBloodSampleDateSecsSinceEpoch + 24*3600);
    }

    ui->customPlot->yAxis->setRange(0, leukocytesMax);
    ui->customPlot->rescaleAxes();
    ui->customPlot->graph()->data()->set(leukocytesGraphData);
    ui->customPlot->replot();
}


void MainWindow::on_pushButtonNewChemoAndMed_clicked()
{
    ui->tableWidgetChemoAndMeds->setRowCount(ui->tableWidgetChemoAndMeds->rowCount() + 1);
}


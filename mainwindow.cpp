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

    QJsonArray bloodSamplesArray;
    auto bloodSamplesArraySize = ui->tableWidgetBloodSamples->rowCount();

    for(int i = 0; i < bloodSamplesArraySize; i++)
    {
        QJsonObject bloodSamplesJsonObject;

        bloodSamplesJsonObject["date"] = ui->tableWidgetBloodSamples->item(i, 0)->text();
        bloodSamplesJsonObject["leukocytes"] = ui->tableWidgetBloodSamples->item(i, 1)->text().toDouble();
        bloodSamplesJsonObject["erythrocytes"] = ui->tableWidgetBloodSamples->item(i, 2)->text().toDouble();
        bloodSamplesJsonObject["hemoglobin"] = ui->tableWidgetBloodSamples->item(i, 3)->text().toDouble();
        bloodSamplesJsonObject["thrombocytes"] = ui->tableWidgetBloodSamples->item(i, 4)->text().toDouble();

        bloodSamplesArray.push_back(bloodSamplesJsonObject);
    }

    patientDataJsonObject["bloodSamples"] = bloodSamplesArray;

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

    ui->tableWidgetBloodSamples->setColumnCount(5);
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(0, new QTableWidgetItem("Date"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(1, new QTableWidgetItem("Leukocytes"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(2, new QTableWidgetItem("Erythrocytes"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(3, new QTableWidgetItem("Hemoglobin"));
    ui->tableWidgetBloodSamples->setHorizontalHeaderItem(4, new QTableWidgetItem("Thrombocytes"));

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

        // Leukocytes

        double leukocytes = patientDataJsonObject["bloodSamples"][i]["leukocytes"].toDouble();
        ui->tableWidgetBloodSamples->setItem(i, 1, new QTableWidgetItem(QString::number(leukocytes)));
        leukocytesGraphData[i].key = QDateTime::fromString(dateString, "dd.MM.yyyy").toSecsSinceEpoch();
        leukocytesGraphData[i].value = leukocytes;

        if(leukocytes > leukocytesMax)
        {
            leukocytesMax = leukocytes;
        }

        // Erythrocytes
        ui->tableWidgetBloodSamples->setItem(i, 2, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].toDouble())));
        // Hemoglobin
        ui->tableWidgetBloodSamples->setItem(i, 3, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].toDouble())));
        // Thrombocytes
        ui->tableWidgetBloodSamples->setItem(i, 4, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].toDouble())));
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


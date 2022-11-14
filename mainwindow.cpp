#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Load settings file first.

    QFile settingsFile;
    settingsFile.setFileName(QDir::currentPath() + "/" + "leukiSettings.json");
    settingsFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString settingsString = settingsFile.readAll();
    settingsFile.close();
    QJsonDocument settingsJsonDocument = QJsonDocument::fromJson(settingsString.toUtf8());
    QJsonObject settingsJsonObject = settingsJsonDocument.object();

    auto patientDataFileName = settingsJsonObject["patientDataFilePath"].toString();

    if(patientDataFileName == "")
    {
        QMessageBox messageBoxNoFileSelected;
        messageBoxNoFileSelected.setText("No patient data file given! Check Leuki settings file.");
        messageBoxNoFileSelected.exec();
        exit(0);
    }

    // Load patient data from patient data file.

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

    for(auto i = 0; i < bloodSamplesArraySize; i++)
    {
        // Date
        ui->tableWidgetBloodSamples->setItem(i, 0, new QTableWidgetItem(patientDataJsonObject["bloodSamples"][i]["date"].toString()));
        // Leukocytes
        ui->tableWidgetBloodSamples->setItem(i, 1, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["leukocytes"].toDouble())));
        // Erythrocytes
        ui->tableWidgetBloodSamples->setItem(i, 2, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["erythrocytes"].toDouble())));
        // Hemoglobin
        ui->tableWidgetBloodSamples->setItem(i, 3, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["hemoglobin"].toDouble())));
        // Thrombocytes
        ui->tableWidgetBloodSamples->setItem(i, 4, new QTableWidgetItem(QString::number(patientDataJsonObject["bloodSamples"][i]["thrombocytes"].toDouble())));
    }

    // Setup plot.

    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iSelectLegend | QCP::iSelectPlottables);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButtonNewBloodSample_clicked()
{
    ui->tableWidgetBloodSamples->setRowCount(ui->tableWidgetBloodSamples->rowCount() + 1);
}


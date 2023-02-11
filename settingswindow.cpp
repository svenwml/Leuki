#include "settingswindow.h"
#include "ui_settingswindow.h"

SettingsWindow::SettingsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    m_settings.autoLoadPatientDataFileOnStartup = false;
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::setSettings(settings_t& settings)
{
    m_settings = settings;

    if(settings.autoLoadPatientDataFileOnStartup)
    {
        ui->checkBoxAutoLoadPatientDataFileOnStartup->setCheckState(Qt::CheckState::Checked);
    }
    else
    {
        ui->checkBoxAutoLoadPatientDataFileOnStartup->setCheckState(Qt::CheckState::Unchecked);
    }
}

SettingsWindow::settings_t& SettingsWindow::getSettings()
{
    return m_settings;
}

void SettingsWindow::on_buttonBox_rejected()
{
    // Restore previous settings.

    if(m_settings.autoLoadPatientDataFileOnStartup)
    {
        ui->checkBoxAutoLoadPatientDataFileOnStartup->setCheckState(Qt::CheckState::Checked);
    }
    else
    {
        ui->checkBoxAutoLoadPatientDataFileOnStartup->setCheckState(Qt::CheckState::Unchecked);
    }
}

void SettingsWindow::on_buttonBox_accepted()
{
    // Store settings.

    m_settings.autoLoadPatientDataFileOnStartup = ui->checkBoxAutoLoadPatientDataFileOnStartup->isChecked();
}


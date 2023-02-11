#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow();

    typedef struct
    {
        bool autoLoadPatientDataFileOnStartup;
    } settings_t;

    void setSettings(settings_t& settings);
    settings_t& getSettings();

private slots:
    void on_buttonBox_rejected();

    void on_buttonBox_accepted();

private:
    Ui::SettingsWindow *ui;
    settings_t m_settings;
};

#endif // SETTINGSWINDOW_H

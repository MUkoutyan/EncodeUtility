/* =====================================================
* EncodeUtility
* Copyright (C) 2019 Koutyan
*
* EncodeUtility is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ===================================================== */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include "Encoder/EncoderInterface.h"
#include "DialogAppSettings.h"
#include <QUndoCommand>

namespace Ui {
class MainWindow;
}

class MetadataTable;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;

    void openFiles(QStringList pathList);
    void loadProjectFile(QString projFilePath);

public slots:
    void Encode();
    void SaveProjectFile(QString saveFilePath);
    void CheckEnableEncodeButton();

private:
    void showEvent(QShowEvent* e) override;

    void SaveSettingFile(QString key, QVariant value);
    void LoadSettingFile();
    bool CheckEncoder();
    void WindowsEncodeProcess();

    void CreateBatchEntryWidgets();

    Ui::MainWindow *ui;
    MetadataTable* metadataTable;
    QString artworkPath;
    QLabel* aboutLabel;
    QMenu* tableMenu;
    QMenu* artworkMenu;
    QString wavOutputPath;
    QString imageOutputPath;
    int processedCount;
    int numEncodingMusic;
    int numEncodingFile;
    DialogAppSettings* settings;
    QList<QWidget*> widgetListDisableDuringEncode;
    QString lastLoadProject;
    QString currentWorkDirectory;
    QWidget* batchEntryWidget;
    QStringList batchParameters;

    bool showAtFirst;

    struct EncoderComponents{
        std::shared_ptr<EncoderInterface> encoder;
        QCheckBox* enableCheck;
        QLabel* baseFolder;
        QLineEdit* outputPath;
    };

    std::vector<EncoderComponents> encoderComponents;


};

#endif // MAINWINDOW_H

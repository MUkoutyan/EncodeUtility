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

#ifndef DIALOGAPPSETTINGS_H
#define DIALOGAPPSETTINGS_H

#include <QDialog>

namespace Ui {
class DialogAppSettings;
}

class DialogAppSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogAppSettings(QWidget *parent = 0);
    ~DialogAppSettings();

    void SetLastOpenedDirectory(QString path);
    QString GetLastOpenedDirectory() const;

    QString GetFFmpegPath() const;
    QString GetAACEncodeSetting() const;
    QString GetMP3EncodeSetting() const;

    bool IsAddTrackNoForTitle() const;
    QString DelimiterForTrackNo() const;
    int GetNumOfDigit() const;

private:
    Ui::DialogAppSettings *ui;
    QString lastOpenedDir;
};

#endif // DIALOGAPPSETTINGS_H

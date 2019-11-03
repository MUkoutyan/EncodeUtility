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

#include "DialogAppSettings.h"
#include "ui_DialogAppSettings.h"
#include <QSettings>

DialogAppSettings::DialogAppSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAppSettings)
{
    ui->setupUi(this);


    QSettings settings(qApp->applicationDirPath()+"/setting.ini", QSettings::IniFormat);
    if(settings.value("ffmpegPath").isValid() == false){
        settings.setProperty("ffmpegPath", QVariant(this->ui->ffmpeg_path->text()));
    }
    else{
        this->ui->ffmpeg_path->setText(settings.value("ffmpegPath").toString());
    }

    if(settings.value("m4aOption").isValid() == false){
        settings.setProperty("m4aOption", QVariant(this->ui->m4a_args->text()));
    }
    else{
        this->ui->m4a_args->setText(settings.value("m4aOption").toString());
    }
    if(settings.value("mp3Option").isValid() == false){
        settings.setProperty("mp3Option", QVariant(this->ui->mp3_args->text()));
    }
    else{
        this->ui->mp3_args->setText(settings.value("mp3Option").toString());
    }
}

DialogAppSettings::~DialogAppSettings()
{
    delete ui;
}

QString DialogAppSettings::GetFFmpegPath() const
{
    return this->ui->ffmpeg_path->text();
}

QString DialogAppSettings::GetAACEncodeSetting() const
{
    QString param = this->ui->m4a_args->text();
    param.replace("${input}", "\"%1\"");
    param.replace("${metadata}", "%2");
    param.replace("${output}", "\"%3\"");
    return param;
}

QString DialogAppSettings::GetMP3EncodeSetting() const
{
    QString param = this->ui->mp3_args->text();
    param.replace("${input}", "\"%1\"");
    param.replace("${metadata}", "%2");
    param.replace("${output}", "\"%3\"");
    return param;
}

bool DialogAppSettings::IsAddTrackNoForTitle() const
{
    return this->ui->check_addTrackNo->isChecked();
}

QString DialogAppSettings::DelimiterForTrackNo() const
{
    return this->ui->track_no_delimiter->text();
}

int DialogAppSettings::GetNumOfDigit() const
{
    return this->ui->num_of_digit->value();
}

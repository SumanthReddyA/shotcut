/*
 * Copyright (c) 2014-2020 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
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
 */

#include "util.h"

#include <QFileInfo>
#include <QWidget>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMap>
#include <QDoubleSpinBox>
#include <QTemporaryFile>
#include <QApplication>
#include <QCryptographicHash>

#include <MltProducer.h>
#include <Logger.h>
#include "shotcut_mlt_properties.h"
#include "qmltypes/qmlapplication.h"
#include "proxymanager.h"

QString Util::baseName(const QString &filePath)
{
    QString s = filePath;
    // Only if absolute path and not a URI.
    if (s.startsWith('/') || s.midRef(1, 2) == ":/" || s.midRef(1, 2) == ":\\")
        s = QFileInfo(s).fileName();
    return s;
}

void Util::setColorsToHighlight(QWidget* widget, QPalette::ColorRole role)
{
    if (role == QPalette::Base) {
        widget->setStyleSheet(
                    "QLineEdit {"
                        "font-weight: bold;"
                        "background-color: palette(highlight);"
                        "color: palette(highlighted-text);"
                        "selection-background-color: palette(alternate-base);"
                        "selection-color: palette(text);"
                    "}"
                    "QLineEdit:hover {"
                        "border: 2px solid palette(button-text);"
                    "}"
        );
    } else {
        QPalette palette = QApplication::palette();
        palette.setColor(role, palette.color(palette.Highlight));
        palette.setColor(role == QPalette::Button ? QPalette::ButtonText : QPalette::WindowText,
            palette.color(palette.HighlightedText));
        widget->setPalette(palette);
        widget->setAutoFillBackground(true);
    }
}

void Util::showInFolder(const QString& path)
{
    QFileInfo info(path);
#if defined(Q_OS_WIN)
    QStringList args;
    if (!info.isDir())
        args << "/select,";
    args << QDir::toNativeSeparators(path);
    if (QProcess::startDetached("explorer", args))
        return;
#elif defined(Q_OS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + path + "\"";
    args << "-e";
    args << "end tell";
#if !defined(QT_DEBUG)
    args << "-e";
    args << "return";
#endif
    if (!QProcess::execute("/usr/bin/osascript", args))
        return;
#endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir()? path : info.path()));
}

bool Util::warnIfNotWritable(const QString& filePath, QWidget* parent, const QString& caption, bool remove)
{
    // Returns true if not writable.
    if (!filePath.isEmpty() && !filePath.contains("://")) {
        // Do a hard check by writing to the file.
        QFile file(filePath);
        file.open(QIODevice::WriteOnly | QIODevice::Append);
        if (file.write("") < 0) {
            QFileInfo fi(filePath);
            QMessageBox::warning(parent, caption,
                                 QObject::tr("Unable to write file %1\n"
                                    "Perhaps you do not have permission.\n"
                                    "Try again with a different folder.")
                                 .arg(fi.fileName()));
            return true;
        } else if (remove) {
            file.remove();
        }
    }
    return false;
}

QString Util::producerTitle(const Mlt::Producer& producer)
{
    QString result;
    Mlt::Producer& p = const_cast<Mlt::Producer&>(producer);
    if (!p.is_valid() || p.is_blank()) return result;
    if (p.get(kShotcutTransitionProperty))
        return QObject::tr("Transition");
    if (p.get(kTrackNameProperty))
        return QObject::tr("Track: %1").arg(QString::fromUtf8(p.get(kTrackNameProperty)));
    if (tractor_type == p.type())
        return QObject::tr("Master");
    if (p.get(kShotcutCaptionProperty))
        return QString::fromUtf8(p.get(kShotcutCaptionProperty));
    return Util::baseName(ProxyManager::resource(p));
}

QString Util::removeFileScheme(QUrl& url)
{
    QString path = url.url();
    if (url.scheme() == "file")
        path = url.toString(QUrl::PreferLocalFile);
    return QUrl::fromPercentEncoding(path.toUtf8());
}

static inline bool isValidGoProFirstFilePrefix(const QFileInfo& info)
{
    QStringList list {"GOPR", "GH01", "GS01"};
    return list.contains(info.baseName().toUpper());
}

static inline bool isValidGoProPrefix(const QFileInfo& info)
{
    QStringList list {"GP", "GH", "GS"};
    return list.contains(info.baseName().toUpper());
}

static inline bool isValidGoProSuffix(const QFileInfo& info)
{
    QStringList list {"MP4", "LRV", "360"};
    return list.contains(info.suffix().toUpper());
}

QStringList Util::sortedFileList(const QList<QUrl>& urls)
{
    QStringList result;
    QMap<QString, QStringList> goproFiles;

    // First look for GoPro main files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) && isValidGoProFirstFilePrefix(fi))
            goproFiles[fi.baseName().mid(4)] << fi.filePath();
    }
    // Then, look for GoPro split files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) && isValidGoProPrefix(fi)) {
            QString goproNumber = fi.baseName().mid(4);
            // Only if there is a matching main GoPro file.
            if (goproFiles.contains(goproNumber) && goproFiles[goproNumber].size())
                goproFiles[goproNumber] << fi.filePath();
        }
    }
    // Next, sort the GoPro files.
    foreach (QString goproNumber, goproFiles.keys())
        goproFiles[goproNumber].sort(Qt::CaseSensitive);
    // Finally, build the list of all files.
    // Add all the GoPro files first.
    foreach (QStringList paths, goproFiles)
        result << paths;
    // Add all the non-GoPro files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) &&
                (isValidGoProFirstFilePrefix(fi) || isValidGoProPrefix(fi))) {
            QString goproNumber = fi.baseName().mid(4);
            if (goproFiles.contains(goproNumber) && goproFiles[goproNumber].contains(fi.filePath()))
                continue;
        }
        result << fi.filePath();
    }
    return result;
}

int Util::coerceMultiple(int value, int multiple)
{
    return (value + multiple - 1) / multiple * multiple;
}

QList<QUrl> Util::expandDirectories(const QList<QUrl>& urls)
{
    QList<QUrl> result;
    foreach (QUrl url, urls) {
        QString path = Util::removeFileScheme(url);
        QFileInfo fi(path);
        if (fi.isDir()) {
            QDir dir(path);
            foreach (QFileInfo fi, dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name))
                result << fi.filePath();
        } else {
            result << url;
        }
    }
    return result;
}

bool Util::isDecimalPoint(QChar ch)
{
    // See https://en.wikipedia.org/wiki/Decimal_separator#Unicode_characters
    return ch == '.' || ch == ',' || ch == '\'' || ch == ' '
        || ch == QChar(0x00B7) || ch == QChar(0x2009) || ch == QChar(0x202F)
        || ch == QChar(0x02D9) || ch == QChar(0x066B) || ch == QChar(0x066C)
        || ch == QChar(0x2396);
}

bool Util::isNumeric(QString& str)
{
    for (int i = 0; i < str.size(); ++i) {
        QCharRef ch = str[i];
        if (ch != '+' && ch != '-' && ch.toLower() != 'e'
            && !isDecimalPoint(ch) && !ch.isDigit())
            return false;
    }
    return true;
}

bool Util::convertNumericString(QString& str, QChar decimalPoint)
{
    // Returns true if the string was changed.
    bool result = false;
    if (isNumeric(str)) {
        for (int i = 0; i < str.size(); ++i) {
            QCharRef ch = str[i];
            if (ch != decimalPoint && isDecimalPoint(ch)) {
                ch = decimalPoint;
                result = true;
            }
        }
    }
    return result;
}

bool Util::convertDecimalPoints(QString& str, QChar decimalPoint)
{
    // Returns true if the string was changed.
    bool result = false;
    if (!str.contains(decimalPoint)) {
        for (int i = 0; i < str.size(); ++i) {
            QCharRef ch = str[i];
            // Space is used as a delimiter for rect fields and possibly elsewhere.
            if (ch != decimalPoint && ch != ' ' && isDecimalPoint(ch)) {
                ch = decimalPoint;
                result = true;
            }
        }
    }
    return result;
}

void Util::showFrameRateDialog(const QString& caption, int numerator, QDoubleSpinBox* spinner, QWidget *parent)
{
    double fps = numerator / 1001.0;
    QMessageBox dialog(QMessageBox::Question, caption,
                       QObject::tr("The value you entered is very similar to the common,\n"
                          "more standard %1 = %2/1001.\n\n"
                          "Do you want to use %1 = %2/1001 instead?")
                          .arg(fps, 0, 'f', 6).arg(numerator),
                       QMessageBox::No | QMessageBox::Yes,
                       parent);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QMessageBox::Yes) {
        spinner->setValue(fps);
    }
}

QTemporaryFile* Util::writableTemporaryFile(const QString& filePath, const QString& templateName)
{
    // filePath should already be checked writable.
    QFileInfo info(filePath);
    QString templateFileName = templateName.isEmpty()?
        QString("%1.XXXXXX").arg(QCoreApplication::applicationName()) : templateName;

    // First, try the system temp dir.
    QString templateFilePath = QDir(QDir::tempPath()).filePath(templateFileName);
    QScopedPointer<QTemporaryFile> tmp(new QTemporaryFile(templateFilePath));

    if (!tmp->open() || tmp->write("") < 0) {
        // Otherwise, use the directory provided.
        return new QTemporaryFile(info.dir().filePath(templateFileName));
    } else {
        return tmp.take();
    }
}

void Util::applyCustomProperties(Mlt::Producer& destination, Mlt::Producer& source, int in, int out)
{
    Mlt::Properties p(destination);
    p.clear("force_progressive");
    p.clear("force_tff");
    p.clear("force_aspect_ratio");
    p.clear("video_delay");
    p.clear("color_range");
    p.clear("speed");
    p.clear("warp_speed");
    p.clear("warp_pitch");
    p.clear(kAspectRatioNumerator);
    p.clear(kAspectRatioDenominator);
    p.clear(kCommentProperty);
    p.clear(kShotcutProducerProperty);
    p.clear(kDefaultAudioIndexProperty);
    p.clear(kOriginalInProperty);
    p.clear(kOriginalOutProperty);
    if (!p.get_int(kIsProxyProperty))
        p.clear(kOriginalResourceProperty);
    p.clear(kDisableProxyProperty);
    destination.pass_list(source, "mlt_service, audio_index, video_index, force_progressive, force_tff,"
                       "force_aspect_ratio, video_delay, color_range, warp_speed, warp_pitch,"
                       kAspectRatioNumerator ","
                       kAspectRatioDenominator ","
                       kCommentProperty ","
                       kShotcutProducerProperty ","
                       kDefaultAudioIndexProperty ","
                       kOriginalInProperty ","
                       kOriginalOutProperty ","
                       kOriginalResourceProperty ","
                       kDisableProxyProperty);
    if (!destination.get("_shotcut:resource")) {
        destination.set("_shotcut:resource", destination.get("resource"));
        destination.set("_shotcut:length", destination.get("length"));
    }
    QString resource = ProxyManager::resource(destination);
    if (!qstrcmp("timewarp", source.get("mlt_service"))) {
        auto speed = qAbs(source.get_double("warp_speed"));
        auto caption = QString("%1 (%2x)").arg(Util::baseName(resource)).arg(speed);
        destination.set(kShotcutCaptionProperty, caption.toUtf8().constData());

        resource = destination.get("_shotcut:resource");
        destination.set("warp_resource", resource.toUtf8().constData());
        resource = QString("%1:%2:%3").arg("timewarp").arg(source.get("warp_speed")).arg(resource);
        destination.set("resource", resource.toUtf8().constData());
        double speedRatio = 1.0 / speed;
        int length = qRound(destination.get_length() * speedRatio);
        destination.set("length", destination.frames_to_time(length, mlt_time_clock));
    } else {
        auto caption = Util::baseName(resource);
        destination.set(kShotcutCaptionProperty, caption.toUtf8().constData());

        p.clear("warp_resource");
        destination.set("resource", destination.get("_shotcut:resource"));
        destination.set("length", destination.get("_shotcut:length"));
    }
    destination.set_in_and_out(in, out);
}

QString Util::getFileHash(const QString& path)
{
    // This routine is intentionally copied from Kdenlive.
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData;
         // 1 MB = 1 second per 450 files (or faster)
         // 10 MB = 9 seconds per 450 files (or faster)
        if (file.size() > 1000000*2) {
            fileData = file.read(1000000);
            if (file.seek(file.size() - 1000000))
                fileData.append(file.readAll());
        } else {
            fileData = file.readAll();
        }
        file.close();
        return QCryptographicHash::hash(fileData, QCryptographicHash::Md5).toHex();
    }
    return QString();
}

QString Util::getHash(Mlt::Properties& properties)
{
    QString hash = properties.get(kShotcutHashProperty);
    if (hash.isEmpty()) {
        QString service = properties.get("mlt_service");
        QString resource = QString::fromUtf8(properties.get("resource"));

        if (properties.get_int(kIsProxyProperty) && properties.get(kOriginalResourceProperty))
            resource = QString::fromUtf8(properties.get(kOriginalResourceProperty));
        else if (service == "timewarp")
            resource = QString::fromUtf8(properties.get("warp_resource"));
        else if (service == "vidstab")
            resource = QString::fromUtf8(properties.get("filename"));
        hash = getFileHash(resource);
        if (!hash.isEmpty())
            properties.set(kShotcutHashProperty, hash.toLatin1().constData());
    }
    return hash;
}

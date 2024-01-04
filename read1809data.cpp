#include "read1809data.h"

Read1809Data::Read1809Data(QObject *parent)
    : Config{parent}{
    connect(timer, &QTimer::timeout, this, &Read1809Data::readDataline);

    buffer->setBuffer(&bufferArray);
}

void Read1809Data::readFolder(QString folder)
{
}

void Read1809Data::readFile(QString filename)
{
    tempfile.clear();

    if (filename.contains(".zip", Qt::CaseInsensitive)) { // zipped file, unzip to temp file before reading
        QStringList filelist = JlCompress::getFileList(filename);
        if (!filelist.isEmpty()) {
            if (filelist.size() > 1) {
                qDebug() << "Multiple files in zip, I have no idea how to handle this!" << filelist;
            }
            tempfile = getLogFolder() + "/" + filelist.first();
        }
        JlCompress::extractFiles(buffer, filelist, getLogFolder());
    }

    if (tempfile.isEmpty()) file.setFileName(filename);
    else file.setFileName(tempfile);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open the 1809 file" << file.fileName();
    }
    else if (file.size() < 200) {
        qDebug() << "1809: Too small file, aborting";
    }

    else { // read line by line, fill out header values
        locationName.clear();
        latitude.clear();
        longitude.clear();
        freqStart = 0, freqStop = 0; // kHz
        filterBW = 0; // kHz
        isDBuV = true;
        date = QDate();
        datapoints = 0;
        scanTime = 0;
        maxHoldTime = 0;
        isHeaderRead = false;

        while(!file.atEnd()) {
            // Header
            QString line = file.readLine();
            if (line.contains("filetype", Qt::CaseInsensitive)) {
                if (line.contains("standard data exchange", Qt::CaseInsensitive)) isMobile = false;
                else isMobile = true;
            }
            else if (line.contains("locationname", Qt::CaseInsensitive)) {
                locationName = line.mid(line.indexOf(' ')).trimmed();
            }
            else if (line.contains("latitude", Qt::CaseInsensitive)) {
                latitude = line.mid(line.indexOf(' ')).trimmed();
            }
            else if (line.contains("longitude", Qt::CaseInsensitive)) {
                longitude = line.mid(line.indexOf(' ')).trimmed();
            }
            else if (line.contains("freqstart", Qt::CaseInsensitive) || line.contains("freqstop", Qt::CaseInsensitive)) {
                bool ok = false;
                unsigned long tmp;
                QStringList list = line.split(' ');
                if (list.size() == 2) tmp = list[1].toULong(&ok);
                if (ok && line.contains("freqstart", Qt::CaseInsensitive)) freqStart = tmp;
                else if (ok && line.contains("freqstop", Qt::CaseInsensitive)) freqStop = tmp;
                else qDebug() << "Conversion error in 1809 header frequency info, check content";
            }
            else if (line.contains("filterbandwidth", Qt::CaseInsensitive)) {
                bool ok = false;
                unsigned int tmp;
                QStringList list = line.split(' ');
                if (list.size() == 2) tmp = list[1].toFloat(&ok);
                if (ok) filterBW = tmp;
            }
            else if (line.contains("levelunits", Qt::CaseInsensitive)) {
                if (line.contains("dbuv", Qt::CaseInsensitive)) isDBuV = true;
                else isDBuV = false;
            }
            else if (line.contains("date", Qt::CaseInsensitive)) {
                QStringList list = line.split(' ');
                if (list.size() == 2) {
                    QStringList str = list[1].split('-');
                    if (str.size() == 3) {
                        int yy = str[0].toInt();
                        int MM = str[1].toInt();
                        int dd = str[2].toInt();
                        date = QDate(yy, MM, dd);
                    }
                }
            }
            else if (line.contains("datapoints", Qt::CaseInsensitive)) {
                bool ok = false;
                unsigned long tmp;
                QStringList list = line.split(' ');
                if (list.size() == 2) tmp = list[1].toUInt(&ok);
                if (ok) datapoints = tmp;
            }
            else if (line.contains("scantime", Qt::CaseInsensitive)) {
                bool ok = false;
                float tmp;
                QStringList list = line.split(' ');
                if (list.size() == 2) tmp = list[1].toFloat(&ok);
                if (ok) scanTime = tmp;
            }
            else if (line.contains("maxholdtime", Qt::CaseInsensitive)) {
                QStringList commaSep = line.split(',');
                int a;
                for (a = 0; a < commaSep.size(); a++) {
                    if (commaSep[a].contains("maxholdtime", Qt::CaseInsensitive)) break; // found correct part
                }
                bool ok = false;
                float tmp;
                QStringList list = commaSep[a].trimmed().split(' ');
                if (list.size() >= 2) tmp = list[1].toFloat(&ok);
                if (ok) maxHoldTime = tmp;
                isHeaderRead = true;
            }

            // Header checks
            if (isHeaderRead && (locationName.isEmpty() || latitude.isEmpty() || longitude.isEmpty() || freqStart == 0 || freqStop == 0 ||
                                 (int)(filterBW * 1000) == 0 || !date.isValid() || datapoints == 0 || (int)(scanTime * 1000) == 0 || (int)(maxHoldTime * 1000) == 0)) {
                qDebug() << "1809 read: header error, aborting";
                emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Cannot read header data from 1809 file, aborting");
                break;
            }
            else if (isHeaderRead) { // all good, emit header data signals and start reading data
                emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Playback of location " + locationName + ", " + (isMobile? "mobile recording, ":"") + "recorded " + date.toString("dd.MM.yy") +
                                                                           ". Position " + latitude + " " + longitude + ", frequency range " + QString::number(freqStart / 1e3, 'f', 3) +
                                                                           "-" + QString::number(freqStop / 1e3, 'f', 3) + " MHz, resolution " + QString::number(filterBW) + "kHz."
                                   );
                emit playbackRunning(true);
                emit freqRangeUsed(freqStart / 1e3, freqStop / 1e3);
                emit tracesPerSec(1.0 / maxHoldTime);
                emit resUsed(filterBW);
                (void)isDBuV; // TODO
                setInstrStartFreq(freqStart / 1e3);
                setInstrStopFreq(freqStop / 1e3);
                setInstrResolution(QString::number(filterBW, 'f', 3));
                // Data readout starts now
                timer->start(maxHoldTime * 1000);
                break;
            }
        }
    }
}

void Read1809Data::readDataline()
{
    if (file.isOpen() && !file.atEnd()) {
        QString line = file.readLine();
        QStringList traceLine = line.split(',');
        int a = 0;
        QList<qint16> trace;

        if (traceLine.size() > 3) {
            traceTime = QTime::fromString(traceLine[a++]);
            if (isMobile) {
                latitude = traceLine[a++];
                longitude = traceLine[a++];
                //emit positionUpdate(true, latitude, longitude);
            }
            for (int i = a; i < traceLine.size(); i++)
                trace.append(traceLine[i].toInt() * 10);
        }
        if (trace.size() != datapoints) {
            qDebug() << "Wrong number of datapoints in" << file.fileName() << ", expected/received" << datapoints << trace.size();
        }
        else {
            emit newTrace(trace);
        }
    }
    else {
        qDebug() << "1809 playback: Closing file";
        file.close();
        timer->stop();
        if (!tempfile.isEmpty()) QFile::remove(getLogFolder() + "/" + tempfile);
        emit playbackRunning(false);
        emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Reached end of file");
    }
}

void Read1809Data::openFile()
{

}

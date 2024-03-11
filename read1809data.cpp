#include "read1809data.h"
#include "opencv2/imgcodecs.hpp"

Read1809Data::Read1809Data(QObject *parent)
    : Config{parent}{
    connect(timer, &QTimer::timeout, this, &Read1809Data::readDataline);

    buffer->setBuffer(&bufferArray);

    //wdg->resize(250, 150);
    wdg->setWindowTitle("1809 playback controller");

    btnRewind->setText("Rewind");
    btnStartPause->setText("Play");
    btnStop->setText("Stop");
    btnClose->setText("Close");

    layout->addWidget(slider, 0, 0, 0, 4);
    layout->addWidget(btnRewind, 1, 0);
    layout->addWidget(btnStartPause, 1, 1);
    layout->addWidget(btnStop, 1, 2);
    layout->addWidget(btnClose, 1, 3);
    wdg->setLayout(layout);

    //wdg->adjustSize();

    connect(btnStartPause, &QPushButton::clicked, this, [this] {
        if (timer->isActive()) pause();
        else play();
    });
    connect(btnRewind, &QPushButton::clicked, this, &Read1809Data::rewind);
    connect(btnClose, &QPushButton::clicked, this, &Read1809Data::closeFile);
    connect(slider, &QSlider::sliderMoved, this, &Read1809Data::seek);
    connect(btnStop, &QPushButton::clicked, this, &Read1809Data::stop);
    slider->setMinimum(0);
    slider->setMaximum(100);
    wdg->setAttribute(Qt::WA_QuitOnClose);
    wdg->setMinimumHeight(150);
}

void Read1809Data::readFolder(QString folder)
{
    filenumber = 0;
    QDir selFolder(folder);
    QStringList filelist = selFolder.entryList(QStringList() << "*.zip" << "*.ZIP" << "*.cef" << "*.CEF", QDir::Files);
    qDebug() << "ReadFolder: Found these files:" << filelist;

    foreach (QString filename, filelist) {
        readAndConvert(folder, folder + "/" + filename);
    }
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
            qDebug() << tempfile << filelist;
        }
        JlCompress::extractFiles(filename, filelist, getLogFolder());
    }

    if (tempfile.isEmpty()) file.setFileName(filename);
    else file.setFileName(tempfile);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open the 1809 file" << file.fileName() << file.errorString();
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
        note.clear();

        while(!file.atEnd()) {
            // Header
            QString line = QString::fromLatin1(file.readLine());
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
                float tmp;
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
            else if (line.contains("note", Qt::CaseInsensitive)) {
                QStringList list = line.split("Note");
                if (list.size() > 1) {
                    QString conv = QString::fromLatin1(list[1].toLocal8Bit());
                    note += conv + " ";
                }
            }

            // Header checks
            if (file.peek(30).count(':') == 2) { // ugly hack, but this means the next line will contain a timestamp, so end of header
                isHeaderRead = true;
                if (locationName.isEmpty()) {
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "No location name in 1809 file");
                    isHeaderRead = false;
                }
                if (latitude.isEmpty() || longitude.isEmpty()) {
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "No position data in 1809 file, trying without");
                    //isHeaderRead = false; // Non-critical
                }
                if (freqStart == 0 || freqStop == 0) {
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Missing frequency range in 1809 file");
                    isHeaderRead = false;
                }
                if ((int)(filterBW * 1000) == 0) {
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Missing resolution value in 1809 file");
                    isHeaderRead = false;
                }
                if (datapoints == 0) {
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Missing number of datapoints in 1809 file");
                    isHeaderRead = false;
                }
                if ((int)(scanTime * 1000) == 0) {
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Missing scantime value in 1809 file");
                    //isHeaderRead = false; // Non-critical
                }
                if ((int)(maxHoldTime * 1000) == 0) {
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Missing maxhold time value in 1809 file, setting to 1 sec");
                    //isHeaderRead = false;
                    maxHoldTime = 1;
                }
                if (isHeaderRead) { // all good, emit header data signals and start reading data
                    emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Playback of location " +
                                                                               locationName + ", " + (isMobile? "mobile recording, ":"") + "recorded " +
                                                                               date.toString("dd.MM.yy") +
                                                                               ". Position " + latitude + " " + longitude +
                                                                               ", frequency range " + QString::number(freqStart / 1e3, 'f', 3) +
                                                                               "-" + QString::number(freqStop / 1e3, 'f', 3) + " MHz, resolution " +
                                                                               QString::number(filterBW, 'f', 3) + " kHz. Notes: " +
                                                                               note
                                       );
                    emit freqRangeUsed(freqStart / 1e3, freqStop / 1e3);
                    emit tracesPerSec(1.0 / maxHoldTime);
                    emit resUsed(filterBW);
                    (void)isDBuV; // TODO
                    setInstrStartFreq(freqStart / 1e3);
                    setInstrStopFreq(freqStop / 1e3);
                    setInstrResolution(QString::number(filterBW, 'f'));
                    // Data readout starts now
                    playbackStartPosition = file.pos();
                    wdg->show();            // start controller widget
                    break;
                }
                else { // Header failed somehow, giving up
                    closeFile();
                }
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
        /*if (trace.size() != datapoints) { // nvmd, not important
            qDebug() << "Wrong number of datapoints in" << file.fileName() << ", expected/received" << datapoints << trace.size();
        }*/
        if (trace.size() == datapoints) {
            updPlaybackPosition();
            emit newTrace(trace);
        }
    }
    else {
        stop();
    }
}

void Read1809Data::openFile()
{

}

void Read1809Data::closeFile()
{
    stop();
    qDebug() << "1809 playback: Closing file";
    file.close();
    timer->stop();
    if (!tempfile.isEmpty()) QFile::remove(tempfile);
    emit playbackRunning(false);
    //emit toIncidentLog(NOTIFY::TYPE::PLAYBACK, "Playback", "Reached end of file");
    wdg->close();
}

void Read1809Data::play()
{
    if (!timer->isActive() && file.isOpen()) {
        timer->start(maxHoldTime * 1000);
        btnStartPause->setText("Pause");
        emit playbackRunning(true);
    }
}

void Read1809Data::pause()
{
    if (timer->isActive() && file.isOpen()) {
        timer->stop();
        btnStartPause->setText("Play");
        emit playbackRunning(false);
    }
}

void Read1809Data::stop()
{
    rewind();
    pause();
}

void Read1809Data::rewind()
{
    //timer->stop();
    //btnStartPause->setText("Play");
    file.seek(playbackStartPosition);
    updPlaybackPosition();
}

void Read1809Data::updPlaybackPosition()
{
    slider->setValue((int)((file.pos() - playbackStartPosition) * 100 / file.size()));
}

void Read1809Data::seek(int pos)
{
    file.seek((pos * file.size() / 100) + playbackStartPosition);
}

void Read1809Data::readAndConvert(QString folder, QString filename)
{
    QFile file;
    QString tempfile;

    if (filename.contains(".zip", Qt::CaseInsensitive)) { // zipped file, unzip to temp file before reading
        QStringList filelist = JlCompress::getFileList(filename);
        if (!filelist.isEmpty()) {
            if (filelist.size() > 1) {
                qDebug() << "Multiple files in zip, I have no idea how to handle this!" << filelist;
            }
            tempfile = getLogFolder() + "/" + filelist.first();
            qDebug() << tempfile << filelist;
        }
        JlCompress::extractFiles(filename, filelist, getLogFolder());
    }

    if (tempfile.isEmpty()) file.setFileName(filename);
    else file.setFileName(tempfile);

    /*if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open the 1809 file" << file.fileName() << file.errorString();
    }*/
    if (file.size() < 200) {
        qDebug() << "1809: Too small file, aborting";
    }
    else {
        std::vector < std::vector<qint16>> array;
        QTime start;
        bool isMobile = false;
        QString note;

        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) { // read whole file at once, line by line
                QString line = file.readLine();
                if (line.contains("filetype", Qt::CaseInsensitive)) {
                    if (line.contains("standard data exchange", Qt::CaseInsensitive)) isMobile = false;
                    else isMobile = true;
                }
                else if (line.contains("datapoints", Qt::CaseInsensitive)) {
                    bool ok = false;
                    unsigned long tmp;
                    QStringList list = line.split(' ');
                    if (list.size() == 2) tmp = list[1].toUInt(&ok);
                    if (ok) datapoints = tmp;
                }
                else if (line.contains("note", Qt::CaseInsensitive)) {
                    QStringList list = line.split("Note");
                    if (list.size() > 1) {
                        QString conv = QString::fromLatin1(list[1].toLocal8Bit());
                        note += conv + " ";
                    }
                }


                else if (line.count(':') == 2 && line.size() > 100) { // ugly hack, just skip header and start reading from first timestamp
                    if (!start.isValid()) { // First line
                        QStringList timedata = line.split(':');
                        if (timedata.size() > 2) {
                            start.setHMS(timedata[0].toInt(), timedata[1].toInt(), timedata[2].split(',')[0].toInt());
                            if (start.isValid()) {
                                qDebug() << "First line timestamp read successfully" << start.toString();
                            }
                            else {
                                qDebug() << "Timestamp parsing error, aborting";
                                break;
                            }
                        }
                    }
                    QStringList traceLine = line.split(',');
                    int a = 0;
                    std::vector<qint16> trace;

                    if (traceLine.size() > 3) {
                        traceTime = QTime::fromString(traceLine[a++]);
                        if (isMobile) a += 2;

                        for (int i = a; i < traceLine.size(); i++)
                            trace.push_back(traceLine[i].toInt());
                    }
                    if (trace.size() == datapoints) array.push_back(trace);
                }
            }
            file.close();
            qint16 min, max, avg;
            findMinAvgMax(array, &min, &max, &avg);
            if (max < 30) max = 30;
            //if (abs(min - avg) > 30) min = avg - 30;
            min = avg / 2; // avg val = white pixel, anything less = still white

            cv::Mat frame((int)array.size(), (int)array.front().size(), CV_8UC3, cv::Scalar());

            for (int i = 0; i < array.size(); i++) {
                for (int j = 0, k = 0; j < array[0].size(); j++, k++) {
                    if (max - min == 0) max += 1;
                    int val = (1.5 * 255 * (array[i][j] - min)) / (max - min);
                    if (val < 0) val = 0;
                    if (val > 255) val = 255;
                    frame.at<uchar>(i, k++) = 255 - val;
                    frame.at<uchar>(i, k++) = 255 - val;
                    frame.at<uchar>(i, k) = 255 - val;
                }
            }

            cv::Mat frameResized;
            cv::resize(frame, frameResized, cv::Size(369, 369), 0, 0, cv::INTER_CUBIC);
            qDebug() << note;
            if (note.contains("Selvsving")) note = "oscillation";
            else if (note.contains("Jammer")) note = "jammer";
            else if (note.contains("Diverse")) note = "other";
            else if (note.contains("pulsgreie")) note = "wideband";
            else note = "unknown";
            cv::imwrite(QString(folder + "/" + QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + note + ".png").toStdString(), frameResized);
        }
        else {
            qDebug() << "Couldn't open file" << filename << ", giving up";
        }
    }
    if (!tempfile.isEmpty()) QFile::remove(tempfile);
}

void Read1809Data::findMinAvgMax(const std::vector<std::vector<qint16> > &buffer, qint16 *min, qint16 *max, qint16 *avg)
{
    qint16 _min = 32767, _max = -32767;
    long _avg = 0;

    for (int i=0; i<buffer.size(); i++) { // find min and max values, used for normalizing values
        for (int j=0; j<buffer[i].size(); j++) {
            if (_min > buffer[i][j]) _min = buffer[i][j];
            else if (_max < buffer[i][j]) _max = buffer[i][j];
            _avg += buffer[i][j];
        }
    }
    *min = _min;
    *max = _max;
    *avg = (qint16)(_avg / ((long)buffer.size() * (long)buffer[0].size()));
}

/*
 * Copyright (C) 2023  UBports Foundation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * screenrecorder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <chrono>

#include "controller.h"
#include "buffer.h"

#if defined(__aarch64__)
#define ARCH_TRIPLET "aarch64-linux-gnu"
#elif defined(__arm__)
#define ARCH_TRIPLET "arm-linux-gnueabihf"
#elif defined(__x86_64__)
#define ARCH_TRIPLET "x86_64-linux-gnu"
#elif defined(__i386__)
#define ARCH_TRIPLET "i386-linux-gnu"
#else
#error "No supported architecture detected"
#endif

Controller::Controller() : m_editing{false}, m_micInput{false}
{
    // make directory on launch so users can restart before starting a recording
    // TODO: remove once Lomiri does that itself
    QDir().mkpath(QFileInfo(INDICATOR_PATH).dir().absolutePath());
}

Controller::~Controller() { }

void Controller::start(float scale, float framerate, bool microphoneInput)
{
    m_micInput = microphoneInput;
    m_capture = QSharedPointer<CaptureMir>(new CaptureMir());
    m_encoder = QSharedPointer<AndroidH264Encoder>(new AndroidH264Encoder());
    m_mux = QSharedPointer<MuxMp4>(new MuxMp4());
    m_recorder.setup(m_encoder, m_capture, m_mux);

    auto config = AndroidH264Encoder::defaultConfig();
    m_capture->init();
    config.width = m_capture->width();
    config.height = m_capture->height();
    config.output_scale = scale;
#if 0
    if (microphoneInput) {
        m_mux->setupAudioTrack();
    }
#endif
    m_encoder->configure(config);

    const auto dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    {
        QDir target(dir);
        if (!target.exists())
            target.mkpath(dir);
    }

    m_fileName = dir + QStringLiteral("/screen_recording_") +
                         QDateTime::currentDateTime().toString("yyyy_MM_dd__hh_mm_ss_zzz") +
                         QStringLiteral(".mp4");
    m_tmpFileName = dir + QStringLiteral("/tmp.mp4");
    m_tmpWavName = dir + QStringLiteral("/tmp.wav");

    if (microphoneInput)
        m_parecord.start("/usr/bin/parecord", QStringList() << m_tmpWavName);
    m_mux->start(m_tmpFileName, m_capture->width(), m_capture->height());
    m_recorder.start(framerate, microphoneInput);
}

void Controller::stop()
{
    m_recorder.stop();
    m_mux->stop();
    if (m_parecord.state() != QProcess::NotRunning) {
        m_parecord.kill();
        m_parecord.waitForFinished();
    }

    if (m_micInput)
        mergeVideoAndAudio();
    else
        QFile::rename(m_tmpFileName, m_fileName);

    Q_EMIT fileSaved(m_fileName);
}

void Controller::cleanSpace()
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));

    if (!dir.exists())
        return;

    QDirIterator it(dir);

    while (it.hasNext()) {
        const auto path = it.next();
        qInfo() << "Deleting stale file" << path << QFile(path).remove();
    }
}

void Controller::cutVideo(const QString path, qint64 from, qint64 to)
{
    m_editing = true;
    Q_EMIT editingChanged();

    const QString editedFile = path + QStringLiteral("_cut.mp4");

    // Correction to match seek playback start
    from = std::max<qint64>(from - 1000, 0);

    QStringList args;
    args << "-y"
         << "-ss" << QString::number(from / 1000)
         << "-to" << QString::number(to / 1000)
         << "-i" << path
         << "-c" << "copy" << editedFile;

    QProcess ffmpeg;
    connect(&ffmpeg, &QProcess::readyReadStandardOutput, this, [&]() {
        qDebug() << ffmpeg.readAllStandardOutput();
    }, Qt::DirectConnection);
    connect(&ffmpeg, &QProcess::readyReadStandardError, this, [&]() {
        qDebug() << ffmpeg.readAllStandardError();
    }, Qt::DirectConnection);

    static const QString ffmpegPath = QStringLiteral("./lib/" ARCH_TRIPLET "/bin/ffmpeg");
    ffmpeg.start(ffmpegPath, args);
    ffmpeg.waitForFinished();

    m_editing = false;
    Q_EMIT editingChanged();

    if (ffmpeg.exitCode() == 0)
        Q_EMIT editedFileSaved(editedFile);
}

bool Controller::isEditing()
{
    return m_editing;
}

void Controller::mergeVideoAndAudio()
{
    QStringList args;
    args << "-y"
         << "-i" << m_tmpFileName
         << "-i" << m_tmpWavName
         << "-vcodec" << "copy"
         << m_fileName;

    QProcess ffmpeg;
    connect(&ffmpeg, &QProcess::readyReadStandardOutput, this, [&]() {
        qDebug() << ffmpeg.readAllStandardOutput();
    }, Qt::DirectConnection);
    connect(&ffmpeg, &QProcess::readyReadStandardError, this, [&]() {
        qDebug() << ffmpeg.readAllStandardError();
    }, Qt::DirectConnection);

    static const QString ffmpegPath = QStringLiteral("./lib/" ARCH_TRIPLET "/bin/ffmpeg");
    ffmpeg.start(ffmpegPath, args);
    ffmpeg.waitForFinished();
}

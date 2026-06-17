#include "net/VoiceRecorder.h"

#include <QMediaCaptureSession>
#include <QAudioInput>
#include <QMediaRecorder>
#include <QMediaFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QUrl>
#include <QDir>
#include <QDateTime>

VoiceRecorder::VoiceRecorder(QObject* parent)
    : QObject(parent),
      session_(new QMediaCaptureSession(this)),
      input_(new QAudioInput(this)),
      recorder_(new QMediaRecorder(this)) {
    session_->setAudioInput(input_);
    session_->setRecorder(recorder_);

    QMediaFormat format;
    format.setFileFormat(QMediaFormat::MPEG4);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    recorder_->setMediaFormat(format);
    recorder_->setQuality(QMediaRecorder::HighQuality);

    connect(recorder_, &QMediaRecorder::recorderStateChanged, this,
            [this](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::StoppedState) {
            if (canceled_) return;
            const QString path = recorder_->actualLocation().toLocalFile();
            if (path.isEmpty()) {
                emit error(QStringLiteral("Запись не сохранена"));
                return;
            }
            emit recordingFinished(path, QStringLiteral("audio/mp4"));
        }
    });
    connect(recorder_, &QMediaRecorder::errorOccurred, this,
            [this](QMediaRecorder::Error, const QString& msg) {
        if (!canceled_) emit error(msg.isEmpty() ? QStringLiteral("Ошибка записи") : msg);
    });
}

bool VoiceRecorder::start() {
    if (QMediaDevices::defaultAudioInput().isNull()) {
        emit error(QStringLiteral("Микрофон не найден"));
        return false;
    }
    canceled_ = false;
    const QString path = QDir::temp().filePath(
        QStringLiteral("xipher_voice_%1.m4a").arg(QDateTime::currentMSecsSinceEpoch()));
    recorder_->setOutputLocation(QUrl::fromLocalFile(path));
    recorder_->record();
    return true;
}

void VoiceRecorder::stop() {
    canceled_ = false;
    if (recorder_->recorderState() != QMediaRecorder::StoppedState)
        recorder_->stop();
}

void VoiceRecorder::cancel() {
    canceled_ = true;
    if (recorder_->recorderState() != QMediaRecorder::StoppedState)
        recorder_->stop();
}

bool VoiceRecorder::isRecording() const {
    return recorder_->recorderState() == QMediaRecorder::RecordingState;
}

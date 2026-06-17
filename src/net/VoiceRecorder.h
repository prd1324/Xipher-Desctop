#pragma once
#include <QObject>
#include <QString>

class QMediaCaptureSession;
class QAudioInput;
class QMediaRecorder;

// ─────────────────────────────────────────────────────────────────────────────
//  VoiceRecorder — запись голосового сообщения через Qt Multimedia в файл
//  MPEG-4/AAC (.m4a). По окончании отдаёт путь к файлу и MIME для загрузки.
// ─────────────────────────────────────────────────────────────────────────────
class VoiceRecorder : public QObject {
    Q_OBJECT
public:
    explicit VoiceRecorder(QObject* parent = nullptr);

    bool start();     // начать запись (false — нет устройства ввода)
    void stop();      // завершить → recordingFinished(path, mime)
    void cancel();    // отменить без сигнала
    bool isRecording() const;

signals:
    void recordingFinished(const QString& filePath, const QString& mimeType);
    void error(const QString& message);

private:
    QMediaCaptureSession* session_;
    QAudioInput*          input_;
    QMediaRecorder*       recorder_;
    bool                  canceled_ = false;
};

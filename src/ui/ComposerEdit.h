#pragma once
#include <QPlainTextEdit>
#include <QImage>

// ─────────────────────────────────────────────────────────────────────────────
//  ComposerEdit — многострочное поле ввода (как в Telegram Desktop):
//   • Enter — отправить (signal sendRequested), Shift+Enter — перенос строки;
//   • авто-высота от 1 до ~6 строк;
//   • вставка картинки из буфера обмена (Ctrl+V) → signal imagePasted.
// ─────────────────────────────────────────────────────────────────────────────
class ComposerEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit ComposerEdit(QWidget* parent = nullptr);

signals:
    void sendRequested();
    void imagePasted(const QImage& image);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void insertFromMimeData(const QMimeData* src) override;

private:
    void adjustHeight();
    int  minH_ = 44;
    int  maxH_ = 150;
};

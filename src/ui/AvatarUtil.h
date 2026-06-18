#pragma once
#include <QString>
#include <QPixmap>

class QLabel;

// ─────────────────────────────────────────────────────────────────────────────
//  Avatar — круглые аватары: буква-плейсхолдер на градиенте + асинхронная
//  подгрузка картинки по avatar_url (с токеном для /files), кэш по URL.
// ─────────────────────────────────────────────────────────────────────────────
namespace Avatar {

QPixmap roundLetter(const QString& text, int size);

// Ставит на label круглый аватар: сразу буква, затем картинка (если url задан).
void setRound(QLabel* label, const QString& url, const QString& fallbackText, int size);

} // namespace Avatar

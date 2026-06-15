#pragma once
#include <QWidget>

// ─────────────────────────────────────────────────────────────────────────────
//  BackgroundWidget — рисует фон экранов входа/регистрации 1:1 с login.css:
//  базовый #05060f + два радиальных «свечения» (фиолетовое и синее) + лёгкая
//  виньетка. Карточки и контент кладутся поверх обычным layout'ом.
// ─────────────────────────────────────────────────────────────────────────────
class BackgroundWidget : public QWidget {
    Q_OBJECT
public:
    explicit BackgroundWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

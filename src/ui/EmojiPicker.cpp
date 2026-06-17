#include "ui/EmojiPicker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QScrollBar>

namespace {
// Категории эмодзи (space-separated). Большой пак, рендер через Segoe UI Emoji.
const char* kSmileys =
"😀 😃 😄 😁 😆 😅 😂 🤣 🥲 😊 😇 🙂 🙃 😉 😌 😍 🥰 😘 😗 😙 😚 😋 😛 😝 😜 🤪 🤨 🧐 🤓 😎 🥸 🤩 🥳 "
"😏 😒 😞 😔 😟 😕 🙁 ☹️ 😣 😖 😫 😩 🥺 😢 😭 😤 😠 😡 🤬 🤯 😳 🥵 🥶 😱 😨 😰 😥 😓 🤗 🤔 🤭 🤫 🤥 "
"😶 😐 😑 😬 🙄 😯 😦 😧 😮 😲 🥱 😴 🤤 😪 😵 🤐 🥴 🤢 🤮 🤧 😷 🤒 🤕 🤑 🤠 😈 👿 👹 👺 🤡 💩 👻 💀 "
"☠️ 👽 👾 🤖 🎃 😺 😸 😹 😻 😼 😽 🙀 😿 😾 "
"👍 👎 👌 🤌 🤏 ✌️ 🤞 🤟 🤘 🤙 👈 👉 👆 👇 ☝️ ✋ 🤚 🖐 🖖 👋 🤝 🙏 ✍️ 💅 🤳 💪 🦾 🖕 ✊ 👊 🤛 🤜 👏 🙌 👐 🤲 "
"❤️ 🧡 💛 💚 💙 💜 🖤 🤍 🤎 💔 ❣️ 💕 💞 💓 💗 💖 💘 💝 💟";

const char* kAnimals =
"🐶 🐱 🐭 🐹 🐰 🦊 🐻 🐼 🐨 🐯 🦁 🐮 🐷 🐸 🐵 🙈 🙉 🙊 🐒 🐔 🐧 🐦 🐤 🦆 🦅 🦉 🦇 🐺 🐗 🐴 🦄 🐝 🐛 🦋 🐌 🐞 🐜 "
"🦗 🕷 🦂 🐢 🐍 🦎 🦖 🦕 🐙 🦑 🦐 🦞 🦀 🐡 🐠 🐟 🐬 🐳 🐋 🦈 🐊 🐅 🐆 🦓 🦍 🦧 🐘 🦛 🦏 🐪 🐫 🦒 🦘 🐃 🐂 🐄 🐎 "
"🐖 🐏 🐑 🦙 🐐 🦌 🐕 🐩 🦮 🐈 🐓 🦃 🦚 🦜 🦢 🦩 🕊 🐇 🦝 🦨 🦡 🦦 🦥 🐁 🐀 🐿 🦔 "
"🌵 🎄 🌲 🌳 🌴 🌱 🌿 ☘️ 🍀 🎍 🎋 🍃 🍂 🍁 🍄 🐚 🌾 💐 🌷 🌹 🥀 🌺 🌸 🌼 🌻 🌞 🌝 🌛 🌜 🌙 ⭐ 🌟 ✨ ⚡ ☄️ 💥 🔥 🌈 ☀️ "
"⛅ ☁️ 🌧 ⛈ 🌩 🌨 ❄️ ☃️ ⛄ 🌬 💨 💧 💦 ☔";

const char* kFood =
"🍏 🍎 🍐 🍊 🍋 🍌 🍉 🍇 🍓 🫐 🍈 🍒 🍑 🥭 🍍 🥥 🥝 🍅 🍆 🥑 🥦 🥬 🥒 🌶 🫑 🌽 🥕 🫒 🧄 🧅 🥔 🍠 🥐 🥯 🍞 🥖 🥨 🧀 🥚 "
"🍳 🧈 🥞 🧇 🥓 🥩 🍗 🍖 🌭 🍔 🍟 🍕 🥪 🥙 🧆 🌮 🌯 🥗 🥘 🍝 🍜 🍲 🍛 🍣 🍱 🥟 🦪 🍤 🍙 🍚 🍘 🍥 🥠 🍢 🍡 🍧 🍨 🍦 🥧 "
"🧁 🍰 🎂 🍮 🍭 🍬 🍫 🍿 🍩 🍪 🌰 🥜 🍯 🥛 🍼 ☕ 🍵 🧃 🥤 🧋 🍶 🍺 🍻 🥂 🍷 🥃 🍸 🍹 🧉 🍾 🧊";

const char* kActivity =
"⚽ 🏀 🏈 ⚾ 🥎 🎾 🏐 🏉 🥏 🎱 🪀 🏓 🏸 🏒 🏑 🥍 🏏 🥅 ⛳ 🪁 🏹 🎣 🤿 🥊 🥋 🎽 🛹 🛷 ⛸ 🥌 🎿 ⛷ 🏂 🪂 🏋️ 🤼 🤸 ⛹️ 🤺 🤾 "
"🏌️ 🏇 🧘 🏄 🏊 🤽 🚣 🧗 🚵 🚴 🏆 🥇 🥈 🥉 🏅 🎖 🏵 🎗 🎫 🎟 🎪 🤹 🎭 🩰 🎨 🎬 🎤 🎧 🎼 🎹 🥁 🎷 🎺 🎸 🪕 🎻 🎲 ♟ 🎯 🎳 🎮 🎰 🧩";

const char* kTravel =
"🚗 🚕 🚙 🚌 🚎 🏎 🚓 🚑 🚒 🚐 🚚 🚛 🚜 🛴 🚲 🛵 🏍 🛺 🚨 🚔 🚍 🚘 🚖 🚡 🚠 🚟 🚃 🚋 🚞 🚝 🚄 🚅 🚈 🚂 🚆 🚇 🚊 🚉 ✈️ 🛫 🛬 "
"🛩 💺 🛰 🚀 🛸 🚁 🛶 ⛵ 🚤 🛥 🛳 ⛴ 🚢 ⚓ ⛽ 🚧 🚦 🚥 🗺 🗿 🗽 🗼 🏰 🏯 🏟 🎡 🎢 🎠 ⛲ ⛱ 🏖 🏝 🏜 🌋 ⛰ 🏔 🗻 🏕 ⛺ 🏠 🏡 🏢 🏬 "
"🏣 🏥 🏦 🏨 🏪 🏫 🏩 💒 🏛 ⛪ 🕌 🕍 🛕 🕋 ⛩ 🌅 🌄 🌠 🎇 🎆 🌇 🌆 🏙 🌃 🌌 🌉 🌁";

const char* kObjects =
"⌚ 📱 📲 💻 ⌨️ 🖥 🖨 🖱 🕹 💽 💾 💿 📀 📼 📷 📸 📹 🎥 📽 🎞 📞 ☎️ 📟 📠 📺 📻 🎙 🎛 🧭 ⏱ ⏲ ⏰ 🕰 ⌛ ⏳ 📡 🔋 🔌 💡 🔦 🕯 🪔 🧯 "
"🛢 💸 💵 💴 💶 💷 💰 💳 💎 ⚖️ 🧰 🔧 🔨 ⚒ 🛠 ⛏ 🔩 ⚙️ 🧱 ⛓ 🧲 🔫 💣 🧨 🪓 🔪 🗡 ⚔️ 🛡 🚬 ⚰️ ⚱️ 🏺 🔮 📿 🧿 💈 ⚗️ 🔭 🔬 🕳 💊 💉 🩸 🧬 🦠 🧫 🧪 "
"🌡 🧹 🧺 🧻 🚽 🚰 🚿 🛁 🛀 🧼 🪒 🧽 🧴 🛎 🔑 🗝 🚪 🪑 🛋 🛏 🛌 🧸 🖼 🛍 🛒 🎁 🎈 🎏 🎀 🎊 🎉 🏮 🎐 🧧 ✉️ 📩 📨 📧 💌 📥 📦 📫 📮 📜 📃 📄 📊 📈 📉 "
"🗒 📆 📅 🗃 🗳 🗄 📋 📁 📂 🗂 🗞 📰 📓 📔 📒 📕 📗 📘 📙 📚 📖 🔖 🧷 🔗 📎 🖇 📐 📏 🧮 📌 📍 ✂️ 🖊 🖋 ✒️ 🖌 🖍 📝 ✏️ 🔍 🔎 🔏 🔐 🔒 🔓";

const char* kSymbols =
"❤️ 🧡 💛 💚 💙 💜 🖤 🤍 🤎 💔 ❣️ 💕 💞 💓 💗 💖 💘 💝 💟 ☮️ ✝️ ☪️ 🕉 ☸️ ✡️ 🔯 🕎 ☯️ ☦️ 🛐 ⛎ ♈ ♉ ♊ ♋ ♌ ♍ ♎ ♏ ♐ ♑ ♒ ♓ 🆔 ⚛️ "
"🉑 ☢️ ☣️ 📴 📳 🈶 🈚 🈸 🈺 🈷 ✴️ 🆚 💮 🉐 ㊙️ ㊗️ 🈴 🈵 🈹 🈲 🅰️ 🅱️ 🆎 🆑 🅾️ 🆘 ❌ ⭕ 🛑 ⛔ 📛 🚫 💯 💢 ♨️ 🚷 🚯 🚳 🚱 🔞 📵 🚭 ❗ ❕ ❓ ❔ ‼️ ⁉️ "
"🔅 🔆 〽️ ⚠️ 🚸 🔱 ⚜️ 🔰 ♻️ ✅ 🈯 💹 ❇️ ✳️ ❎ 🌐 💠 Ⓜ️ 🌀 💤 🏧 🚾 ♿ 🅿️ 🛂 🛃 🛄 🛅 🚹 🚺 🚼 🚻 🚮 🎦 📶 🈁 🔣 ℹ️ 🔤 🔡 🔠 🆖 🆗 🆙 🆒 🆕 🆓 "
"0️⃣ 1️⃣ 2️⃣ 3️⃣ 4️⃣ 5️⃣ 6️⃣ 7️⃣ 8️⃣ 9️⃣ 🔟 🔢 ▶️ ⏸ ⏯ ⏹ ⏺ ⏭ ⏮ ⏩ ⏪ 🔀 🔁 🔂 🔄 🔃 ➕ ➖ ➗ ✖️ ♾ 💲 💱 ™️ © ® 〰️ ➰ ➿ 🔚 🔙 🔛 🔝 🔜 ✔️ ☑️ 🔘 "
"🔴 🟠 🟡 🟢 🔵 🟣 ⚫ ⚪ 🟤 🔺 🔻 🔸 🔹 🔶 🔷 🔳 🔲 ▪️ ▫️ ◾ ◽ ◼️ ◻️ 🟥 🟧 🟨 🟩 🟦 🟪 ⬛ ⬜ 🟫 🔔 🔕 📣 📢 💬 💭 🗯 ♠️ ♣️ ♥️ ♦️ 🃏 🎴 🀄 "
"🕐 🕑 🕒 🕓 🕔 🕕 🕖 🕗 🕘 🕙 🕚 🕛";

const char* kFlags =
"🏳️ 🏴 🏁 🚩 🏳️‍🌈 🏴‍☠️ 🇷🇺 🇺🇸 🇬🇧 🇩🇪 🇫🇷 🇪🇸 🇮🇹 🇯🇵 🇨🇳 🇰🇷 🇮🇳 🇧🇷 🇨🇦 🇦🇺 🇺🇦 🇰🇿 🇧🇾 🇵🇱 🇹🇷 🇳🇱 🇨🇭 🇸🇪 🇳🇴 🇫🇮 🇩🇰 🇵🇹 🇬🇷 🇨🇿 🇦🇹 🇧🇪 🇮🇪 🇲🇽 🇦🇷 🇮🇩 🇹🇭 🇻🇳 🇵🇭 🇪🇬 🇿🇦 🇸🇦 🇦🇪 🇮🇱";

const char* kTabIcons[] = {"😀", "🐻", "🍔", "⚽", "🚗", "💡", "🔣", "🏳️"};
}

EmojiPicker::EmojiPicker(QWidget* parent) : QFrame(parent) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setObjectName(QStringLiteral("emojiPicker"));
    setFixedSize(372, 420);
    setStyleSheet(QStringLiteral(R"QSS(
#emojiPicker { background:#1A1822; border:1px solid rgba(255,255,255,0.12); border-radius:14px; }
QPushButton.emoji {
    border:none; background:transparent; font-size:22px; border-radius:8px;
    font-family:"Segoe UI Emoji","Apple Color Emoji",sans-serif;
}
QPushButton.emoji:hover { background:rgba(255,255,255,0.10); }
QPushButton.tab {
    border:none; background:transparent; font-size:18px; padding:6px; border-radius:8px;
    font-family:"Segoe UI Emoji",sans-serif;
}
QPushButton.tab:hover { background:rgba(255,255,255,0.08); }
QPushButton.tab:checked { background:rgba(139,92,246,0.22); }
QScrollArea { background:transparent; border:none; }
QScrollBar:vertical { background:transparent; width:8px; margin:2px; }
QScrollBar::handle:vertical { background:rgba(255,255,255,0.14); border-radius:4px; min-height:30px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
)QSS"));

    categories_ = {
        QString::fromUtf8(kSmileys).split(' ', Qt::SkipEmptyParts),
        QString::fromUtf8(kAnimals).split(' ', Qt::SkipEmptyParts),
        QString::fromUtf8(kFood).split(' ', Qt::SkipEmptyParts),
        QString::fromUtf8(kActivity).split(' ', Qt::SkipEmptyParts),
        QString::fromUtf8(kTravel).split(' ', Qt::SkipEmptyParts),
        QString::fromUtf8(kObjects).split(' ', Qt::SkipEmptyParts),
        QString::fromUtf8(kSymbols).split(' ', Qt::SkipEmptyParts),
        QString::fromUtf8(kFlags).split(' ', Qt::SkipEmptyParts),
    };

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // Вкладки категорий
    auto* tabRow = new QHBoxLayout();
    tabRow->setSpacing(2);
    for (int i = 0; i < categories_.size(); ++i) {
        auto* t = new QPushButton(QString::fromUtf8(kTabIcons[i]), this);
        t->setProperty("class", "tab");
        t->setCheckable(true);
        t->setCursor(Qt::PointingHandCursor);
        connect(t, &QPushButton::clicked, this, [this, i]() { showCategory(i); });
        tabs_.append(t);
        tabRow->addWidget(t);
    }
    root->addLayout(tabRow);

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    grid_ = new QWidget();
    grid_->setStyleSheet(QStringLiteral("background:transparent;"));
    scroll_->setWidget(grid_);
    root->addWidget(scroll_, 1);

    showCategory(0);
}

void EmojiPicker::showCategory(int index) {
    current_ = index;
    for (int i = 0; i < tabs_.size(); ++i) tabs_[i]->setChecked(i == index);

    // Перестраиваем сетку
    if (grid_->layout()) {
        QLayoutItem* it;
        while ((it = grid_->layout()->takeAt(0)) != nullptr) {
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }
        delete grid_->layout();
    }
    auto* g = new QGridLayout(grid_);
    g->setContentsMargins(2, 2, 2, 2);
    g->setSpacing(2);

    const QStringList& list = categories_[index];
    const int cols = 8;
    for (int i = 0; i < list.size(); ++i) {
        auto* b = new QPushButton(list[i], grid_);
        b->setProperty("class", "emoji");
        b->setFixedSize(40, 40);
        b->setCursor(Qt::PointingHandCursor);
        const QString e = list[i];
        connect(b, &QPushButton::clicked, this, [this, e]() { emit emojiPicked(e); });
        g->addWidget(b, i / cols, i % cols);
    }
    scroll_->verticalScrollBar()->setValue(0);
}

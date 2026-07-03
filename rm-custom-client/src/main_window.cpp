#include "main_window.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QTextStream>
#include <QtGui/QFont>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPixmap>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace {

QLabel* makeVideoLabel(const QString& title) {
    auto* label = new QLabel(title);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumSize(640, 360);
    label->setStyleSheet(QStringLiteral("background:#050816;color:#b8c2d8;border:1px solid rgba(122, 162, 247, 0.28);border-radius:16px;"));
    label->setMouseTracking(true);
    label->setScaledContents(false);
    return label;
}

QFrame* makeVideoCard(QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setFrameShape(QFrame::NoFrame);
    card->setObjectName(QStringLiteral("videoCard"));
    return card;
}

QLabel* makeVideoTitle(const QString& title, const QString& subtitle) {
    auto* label = new QLabel(QStringLiteral("<div style='font-size:18px;font-weight:700;'>%1</div><div style='font-size:12px;color:#8c96ad;'>%2</div>")
                                 .arg(title)
                                 .arg(subtitle));
    label->setTextFormat(Qt::RichText);
    return label;
}

QLabel* makeVideoHint(const QString& text) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setStyleSheet(QStringLiteral("color:#7f8aa3;font-size:12px;"));
    return label;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("rootCentral"));
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(14);

    setStyleSheet(QStringLiteral(
        "QWidget#rootCentral { background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #07111f, stop:0.55 #0c1628, stop:1 #111a2d); }"
        "QGroupBox { color:#d9e3f5; border:1px solid rgba(121,136,168,0.35); border-radius:14px; margin-top:12px; padding:14px 12px 12px 12px; background:rgba(7,12,22,0.72); }"
        "QGroupBox::title { subcontrol-origin: margin; left:14px; padding:0 6px; color:#8fb3ff; }"
        "QLabel { color:#dfe7f7; }"
        "QLineEdit, QPlainTextEdit { background:rgba(3,8,18,0.86); color:#dfe7f7; border:1px solid rgba(121,136,168,0.35); border-radius:10px; padding:8px; }"
        "QPushButton { background:#1f6feb; color:white; border:none; border-radius:10px; padding:8px 14px; font-weight:600; }"
        "QPushButton:hover { background:#388bfd; }"
    ));

    videoLayout_ = new QHBoxLayout();
    videoLayout_->setSpacing(14);
    officialVideoLabel_ = makeVideoLabel(QStringLiteral("Official HEVC Stream"));
    heroVideoLabel_ = makeVideoLabel(QStringLiteral("Hero Custom Stream"));
    officialVideoCard_ = makeVideoCard(central);
    heroVideoCard_ = makeVideoCard(central);
    officialVideoTitleLabel_ = makeVideoTitle(QStringLiteral("官方图传"), QStringLiteral("Official stream"));
    heroVideoTitleLabel_ = makeVideoTitle(QStringLiteral("相机画面"), QStringLiteral("Hero camera"));
    officialVideoHintLabel_ = makeVideoHint(QStringLiteral("点击放大 / 再点恢复"));
    heroVideoHintLabel_ = makeVideoHint(QStringLiteral("点击放大 / 再点恢复"));

    auto* officialVideoCardLayout = new QVBoxLayout(officialVideoCard_);
    officialVideoCardLayout->setContentsMargins(14, 14, 14, 14);
    officialVideoCardLayout->setSpacing(10);
    auto* officialHeaderLayout = new QHBoxLayout();
    officialHeaderLayout->addWidget(officialVideoTitleLabel_, 1);
    officialHeaderLayout->addWidget(officialVideoHintLabel_);
    officialVideoCardLayout->addLayout(officialHeaderLayout);
    officialVideoCardLayout->addWidget(officialVideoLabel_, 1);

    auto* heroVideoCardLayout = new QVBoxLayout(heroVideoCard_);
    heroVideoCardLayout->setContentsMargins(14, 14, 14, 14);
    heroVideoCardLayout->setSpacing(10);
    auto* heroHeaderLayout = new QHBoxLayout();
    heroHeaderLayout->addWidget(heroVideoTitleLabel_, 1);
    heroHeaderLayout->addWidget(heroVideoHintLabel_);
    heroVideoCardLayout->addLayout(heroHeaderLayout);
    heroVideoCardLayout->addWidget(heroVideoLabel_, 1);

    officialVideoLabel_->installEventFilter(this);
    heroVideoLabel_->installEventFilter(this);
    officialVideoCard_->installEventFilter(this);
    heroVideoCard_->installEventFilter(this);
    videoLayout_->addWidget(officialVideoCard_, 1);
    videoLayout_->addWidget(heroVideoCard_, 1);

    auto* statusGroup = new QGroupBox(QStringLiteral("Status"), central);
    auto* statusLayout = new QGridLayout(statusGroup);
    connectionStatusLabel_ = new QLabel(QStringLiteral("MQTT disconnected"), statusGroup);
    gameStatusLabel_ = new QLabel(QStringLiteral("Game: -"), statusGroup);
    dynamicStatusLabel_ = new QLabel(QStringLiteral("Dynamic: -"), statusGroup);
    moduleStatusLabel_ = new QLabel(QStringLiteral("Modules: -"), statusGroup);
    positionStatusLabel_ = new QLabel(QStringLiteral("Position: -"), statusGroup);
    heroConfigStatusLabel_ = new QLabel(QStringLiteral("Hero stream: waiting config"), statusGroup);
    statusLayout->addWidget(connectionStatusLabel_, 0, 0, 1, 2);
    statusLayout->addWidget(gameStatusLabel_, 1, 0, 1, 2);
    statusLayout->addWidget(dynamicStatusLabel_, 2, 0, 1, 2);
    statusLayout->addWidget(moduleStatusLabel_, 3, 0, 1, 2);
    statusLayout->addWidget(positionStatusLabel_, 4, 0, 1, 2);
    statusLayout->addWidget(heroConfigStatusLabel_, 5, 0, 1, 2);

    auto* controlGroup = new QGroupBox(QStringLiteral("Custom Control"), central);
    auto* controlLayout = new QHBoxLayout(controlGroup);
    customControlLineEdit_ = new QLineEdit(controlGroup);
    customControlLineEdit_->setPlaceholderText(QStringLiteral("Hex bytes, e.g. 01 02 A0 FF"));
    auto* sendButton = new QPushButton(QStringLiteral("Send"), controlGroup);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendCustomControl);
    controlLayout->addWidget(customControlLineEdit_, 1);
    controlLayout->addWidget(sendButton);

    logView_ = new QPlainTextEdit(central);
    logView_->setReadOnly(true);
    logView_->setMaximumBlockCount(300);

    rootLayout->addLayout(videoLayout_, 4);
    rootLayout->addWidget(statusGroup);
    rootLayout->addWidget(controlGroup);
    rootLayout->addWidget(logView_, 1);

    setCentralWidget(central);
    setWindowTitle(QStringLiteral("RM Custom Client"));
    resize(1440, 920);
    setFocusPolicy(Qt::StrongFocus);

    applyVideoLayoutMode();

    controlTimer_.setInterval(13);
    connect(&controlTimer_, &QTimer::timeout, this, &MainWindow::emitControlFrame);
    controlTimer_.start();
}

void MainWindow::updateOfficialFrame(const QImage& image) {
    lastOfficialFrame_ = image;
    setVideoLabelImage(officialVideoLabel_, image);
}

void MainWindow::updateHeroFrame(const QImage& image) {
    lastHeroFrame_ = image;
    setVideoLabelImage(heroVideoLabel_, image);
}

void MainWindow::updateGameStatus(const QString& text) {
    gameStatusLabel_->setText(text);
}

void MainWindow::updateDynamicStatus(const QString& text) {
    dynamicStatusLabel_->setText(text);
}

void MainWindow::updateModuleStatus(const QString& text) {
    moduleStatusLabel_->setText(text);
}

void MainWindow::updatePositionStatus(const QString& text) {
    positionStatusLabel_->setText(text);
}

void MainWindow::updateHeroConfigStatus(const QString& text) {
    heroConfigStatusLabel_->setText(text);
}

void MainWindow::updateConnectionStatus(const QString& text) {
    connectionStatusLabel_->setText(text);
}

void MainWindow::appendLog(const QString& text) {
    const QString line = QStringLiteral("[%1] %2")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")))
                             .arg(text);
    logView_->appendPlainText(line);

    QTextStream stream(stderr, QIODevice::WriteOnly);
    stream << line << Qt::endl;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return;
    }
    currentControl_.keyboardMask |= keyMaskForQtKey(event->key());
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return;
    }
    currentControl_.keyboardMask &= ~keyMaskForQtKey(event->key());
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    const bool isOfficialTarget = watched == officialVideoLabel_ || watched == officialVideoCard_;
    const bool isHeroTarget = watched == heroVideoLabel_ || watched == heroVideoCard_;
    if (!isOfficialTarget && !isHeroTarget) {
        return QMainWindow::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (isOfficialTarget) {
                focusVideoPane(videoLayoutMode_ == VideoLayoutMode::OfficialFocused ? VideoLayoutMode::Split : VideoLayoutMode::OfficialFocused);
            } else if (isHeroTarget) {
                focusVideoPane(videoLayoutMode_ == VideoLayoutMode::HeroFocused ? VideoLayoutMode::Split : VideoLayoutMode::HeroFocused);
            }
        }
    }

    if (!isOfficialTarget) {
        return QMainWindow::eventFilter(watched, event);
    }

    switch (event->type()) {
        case QEvent::MouseMove: {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (hasLastMousePos_) {
                const QPoint delta = mouseEvent->pos() - lastMousePos_;
                currentControl_.mouseX += delta.x();
                currentControl_.mouseY -= delta.y();
            }
            lastMousePos_ = mouseEvent->pos();
            hasLastMousePos_ = true;
            return true;
        }
        case QEvent::Leave:
            hasLastMousePos_ = false;
            return false;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            const bool pressed = event->type() == QEvent::MouseButtonPress;
            if (mouseEvent->button() == Qt::LeftButton && watched == officialVideoLabel_) {
                currentControl_.leftButtonDown = pressed;
            } else if (mouseEvent->button() == Qt::RightButton) {
                currentControl_.rightButtonDown = pressed;
            } else if (mouseEvent->button() == Qt::MiddleButton) {
                currentControl_.midButtonDown = pressed;
            }
            return true;
        }
        case QEvent::Wheel: {
            auto* wheelEvent = static_cast<QWheelEvent*>(event);
            currentControl_.mouseZ += wheelEvent->angleDelta().y() / 120;
            return true;
        }
        default:
            return QMainWindow::eventFilter(watched, event);
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    refreshRenderedFrames();
}

void MainWindow::emitControlFrame() {
    emit controlFrameReady(currentControl_);
    currentControl_.mouseX = 0;
    currentControl_.mouseY = 0;
    currentControl_.mouseZ = 0;
}

void MainWindow::sendCustomControl() {
    QByteArray hex = customControlLineEdit_->text().toLatin1();
    hex.replace(" ", "");
    const QByteArray bytes = QByteArray::fromHex(hex);
    if (bytes.isEmpty()) {
        appendLog(QStringLiteral("[警告] 未输入有效的 CustomControl 十六进制数据，本次未发送"));
        return;
    }
    emit customControlRequested(bytes);
    appendLog(QStringLiteral("[信息] 已发送 CustomControl，自定义数据长度 %1 字节").arg(bytes.size()));
}

void MainWindow::setVideoLabelImage(QLabel* label, const QImage& image) {
    if (image.isNull()) {
        return;
    }
    label->setPixmap(QPixmap::fromImage(image).scaled(label->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
}

void MainWindow::focusVideoPane(VideoLayoutMode mode) {
    if (videoLayoutMode_ == mode) {
        return;
    }
    videoLayoutMode_ = mode;
    applyVideoLayoutMode();
    refreshRenderedFrames();
}

void MainWindow::applyVideoLayoutMode() {
    int officialStretch = 1;
    int heroStretch = 1;
    switch (videoLayoutMode_) {
        case VideoLayoutMode::Split:
            officialStretch = 1;
            heroStretch = 1;
            officialVideoCard_->setMaximumWidth(QWIDGETSIZE_MAX);
            heroVideoCard_->setMaximumWidth(QWIDGETSIZE_MAX);
            officialVideoLabel_->setMinimumSize(640, 360);
            heroVideoLabel_->setMinimumSize(640, 360);
            break;
        case VideoLayoutMode::OfficialFocused:
            officialStretch = 14;
            heroStretch = 1;
            officialVideoCard_->setMaximumWidth(QWIDGETSIZE_MAX);
            heroVideoCard_->setMaximumWidth(280);
            officialVideoLabel_->setMinimumSize(960, 540);
            heroVideoLabel_->setMinimumSize(240, 160);
            break;
        case VideoLayoutMode::HeroFocused:
            officialStretch = 1;
            heroStretch = 14;
            officialVideoCard_->setMaximumWidth(280);
            heroVideoCard_->setMaximumWidth(QWIDGETSIZE_MAX);
            officialVideoLabel_->setMinimumSize(240, 160);
            heroVideoLabel_->setMinimumSize(960, 540);
            break;
    }
    videoLayout_->setStretch(0, officialStretch);
    videoLayout_->setStretch(1, heroStretch);
    updateVideoCardStyles();
}

void MainWindow::refreshRenderedFrames() {
    if (!lastOfficialFrame_.isNull()) {
        setVideoLabelImage(officialVideoLabel_, lastOfficialFrame_);
    }
    if (!lastHeroFrame_.isNull()) {
        setVideoLabelImage(heroVideoLabel_, lastHeroFrame_);
    }
}

void MainWindow::updateVideoCardStyles() {
    const bool officialFocused = videoLayoutMode_ == VideoLayoutMode::OfficialFocused;
    const bool heroFocused = videoLayoutMode_ == VideoLayoutMode::HeroFocused;
    const bool splitMode = videoLayoutMode_ == VideoLayoutMode::Split;

    const QString baseCardStyle = QStringLiteral(
        "QFrame#videoCard { background:rgba(4,10,20,0.78); border:1px solid rgba(125,140,170,0.28); border-radius:20px; }"
    );
    const QString focusedCardStyle = QStringLiteral(
        "QFrame#videoCard { background:rgba(7,16,30,0.92); border:2px solid rgba(88,166,255,0.88); border-radius:20px; }"
    );
    const QString dimmedCardStyle = QStringLiteral(
        "QFrame#videoCard { background:rgba(3,8,16,0.66); border:1px solid rgba(96,109,136,0.24); border-radius:20px; }"
    );

    officialVideoCard_->setStyleSheet(officialFocused ? focusedCardStyle : (heroFocused ? dimmedCardStyle : baseCardStyle));
    heroVideoCard_->setStyleSheet(heroFocused ? focusedCardStyle : (officialFocused ? dimmedCardStyle : baseCardStyle));

    officialVideoHintLabel_->setText(splitMode ? QStringLiteral("点击放大") : (officialFocused ? QStringLiteral("再点恢复双栏") : QStringLiteral("点击切换主画面")));
    heroVideoHintLabel_->setText(splitMode ? QStringLiteral("点击放大") : (heroFocused ? QStringLiteral("再点恢复双栏") : QStringLiteral("点击切换主画面")));
}

quint32 MainWindow::keyMaskForQtKey(int key) {
    switch (key) {
        case Qt::Key_W:
            return 1u << 0;
        case Qt::Key_S:
            return 1u << 1;
        case Qt::Key_A:
            return 1u << 2;
        case Qt::Key_D:
            return 1u << 3;
        case Qt::Key_Shift:
            return 1u << 4;
        case Qt::Key_Control:
            return 1u << 5;
        case Qt::Key_Q:
            return 1u << 6;
        case Qt::Key_E:
            return 1u << 7;
        case Qt::Key_R:
            return 1u << 8;
        case Qt::Key_F:
            return 1u << 9;
        case Qt::Key_G:
            return 1u << 10;
        case Qt::Key_Z:
            return 1u << 11;
        case Qt::Key_X:
            return 1u << 12;
        case Qt::Key_C:
            return 1u << 13;
        case Qt::Key_V:
            return 1u << 14;
        case Qt::Key_B:
            return 1u << 15;
        default:
            return 0;
    }
}

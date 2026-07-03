#pragma once

#include <QtCore/QPoint>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>

#include "control_state.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

signals:
    void controlFrameReady(const ControlState& state);
    void customControlRequested(const QByteArray& payload);

public slots:
    void updateOfficialFrame(const QImage& image);
    void updateHeroFrame(const QImage& image);
    void updateGameStatus(const QString& text);
    void updateDynamicStatus(const QString& text);
    void updateModuleStatus(const QString& text);
    void updatePositionStatus(const QString& text);
    void updateHeroConfigStatus(const QString& text);
    void updateConnectionStatus(const QString& text);
    void appendLog(const QString& text);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void emitControlFrame();
    void sendCustomControl();

private:
    enum class VideoLayoutMode {
        Split,
        OfficialFocused,
        HeroFocused,
    };

    void setVideoLabelImage(QLabel* label, const QImage& image);
    void focusVideoPane(VideoLayoutMode mode);
    void applyVideoLayoutMode();
    void refreshRenderedFrames();
    void updateVideoCardStyles();
    static quint32 keyMaskForQtKey(int key);

    QHBoxLayout* videoLayout_ = nullptr;
    QFrame* officialVideoCard_ = nullptr;
    QFrame* heroVideoCard_ = nullptr;
    QLabel* officialVideoLabel_ = nullptr;
    QLabel* heroVideoLabel_ = nullptr;
    QLabel* officialVideoTitleLabel_ = nullptr;
    QLabel* heroVideoTitleLabel_ = nullptr;
    QLabel* officialVideoHintLabel_ = nullptr;
    QLabel* heroVideoHintLabel_ = nullptr;
    QLabel* connectionStatusLabel_ = nullptr;
    QLabel* gameStatusLabel_ = nullptr;
    QLabel* dynamicStatusLabel_ = nullptr;
    QLabel* moduleStatusLabel_ = nullptr;
    QLabel* positionStatusLabel_ = nullptr;
    QLabel* heroConfigStatusLabel_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QLineEdit* customControlLineEdit_ = nullptr;
    QImage lastOfficialFrame_;
    QImage lastHeroFrame_;
    VideoLayoutMode videoLayoutMode_ = VideoLayoutMode::Split;
    ControlState currentControl_;
    QPoint lastMousePos_;
    bool hasLastMousePos_ = false;
    QTimer controlTimer_;
};

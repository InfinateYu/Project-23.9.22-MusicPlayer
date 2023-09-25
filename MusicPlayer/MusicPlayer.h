#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QMediaPlayer>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QFileDialog>
#include <QTextCOdec>
#include <QStringConverter>
#include <QRegularExpression>
#include <QtWidgets/QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MusicPlayer; }
QT_END_NAMESPACE

class MusicPlayer : public QMainWindow {
    Q_OBJECT

public:
    MusicPlayer(QWidget* parent = nullptr);
    ~MusicPlayer();

public slots:
    void OpenFolder();
    void TogglePlay();
    void CheckMediaStatus(QMediaPlayer::MediaStatus);
    void PlayNext();
    void UpdateProgress();
    void UpdatePosition();
    void UpdateVolume(int);

private:
    QString FormatTime(int);
private:
    QMediaPlayer* player;
    QAudioOutput* output;
    QList<QUrl>* playList;
    QList<QString>* nameList;
    QMap<QString, QMap<int, QString>>* lyricsList;
    QTimer* timer;
    int index;
    int size;

private:
    Ui::MusicPlayer* ui;
};
#endif // MUSICPLAYER_H

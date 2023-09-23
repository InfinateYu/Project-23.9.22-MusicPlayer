#include "MusicPlayer.h"
#include "ui_MusicPlayer.h"

MusicPlayer::MusicPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MusicPlayer) {
    ui->setupUi(this);

    // 设置音乐存储部分
    index = 0;
    size = 0;
    playList = new QList<QUrl>;
    nameList = new QList<QString>;

    // 设置播放器
    player = new QMediaPlayer(this);
    output = new QAudioOutput(this);
    output->setDevice(QMediaDevices::defaultAudioOutput());
    player->setAudioOutput(output);
    player->setVideoOutput(nullptr);
    player->setLoops(QMediaPlayer::Once);

    // 设置计时器
    timer = new QTimer(this);
    timer->start(500);

    // 连接信号与槽
    connect(ui->fileButton, &QPushButton::clicked, this, &MusicPlayer::OpenFolder);
    connect(ui->playButton, &QPushButton::clicked, this, &MusicPlayer::TogglePlay);
    connect(ui->nextButton, &QPushButton::clicked, this, &MusicPlayer::PlayNext);
    connect(timer, &QTimer::timeout, this, &MusicPlayer::UpdateProgress);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::CheckMediaStatus);
}


MusicPlayer::~MusicPlayer() {
    delete player;
    delete timer;
    delete playList;
    delete nameList;
    delete ui;
}

void MusicPlayer::OpenFolder() {
    // 使用QFileDialog::getExistingDirectory静态函数，返回选择的文件夹路径
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("选择音乐文件夹"), "/");

    // 如果没有取消选择，且文件夹路径不为空
    if (!folderPath.isEmpty()) {
        playList->clear();
        nameList->clear();

        // 获取文件夹下的所有音乐文件名
        QDir dir(folderPath);
        QStringList filters;
        filters << "*.mp3" << "*.ogg";
        QStringList fileNames = dir.entryList(filters, QDir::Files);

        // 遍历所有音乐文件名，将它们添加到播放列表中
        for (const QString& fileName : fileNames) {
            // 使用QUrl::fromLocalFile函数，将本地文件路径转换为QUrl对象
            QUrl url = QUrl::fromLocalFile(dir.absoluteFilePath(fileName));
            playList->push_back(url);
            QFileInfo fileInfo(fileName);
            nameList->append(fileInfo.baseName());
        }

        size = playList->size();
        if (size) {
            index = 0;
            player->setSource(playList->at(0));
            ui->musicName->setText(nameList->at(0));
            ui->nowTime->setText(FormatTime(0));
            ui->totalTime->setText(FormatTime(player->duration()));
            ui->playButton->setText(tr("播放"));
            ui->progressBar->setValue(0);
            player->setPosition(0);
            player->stop();
        }
    }
}

void MusicPlayer::TogglePlay() {
    if (playList->empty()) {
        return;
    }
    // 确保加载完成
    if (player->mediaStatus() == QMediaPlayer::BufferedMedia
        || player->mediaStatus() == QMediaPlayer::BufferingMedia
        || player->mediaStatus() == QMediaPlayer::LoadedMedia) {
        // 如果媒体播放器的状态是正在播放
        if (player->playbackState() == QMediaPlayer::PlayingState) {
            player->pause();
            ui->playButton->setText(tr("播放"));
        }
        // 如果媒体播放器的状态是暂停或停止
        else if (player->playbackState() == QMediaPlayer::PausedState
                 || player->playbackState() == QMediaPlayer::StoppedState) {
            player->play();
            ui->playButton->setText(tr("暂停"));
        }
    }
}

void MusicPlayer::CheckMediaStatus(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        index = (index + 1) % size;
        player->setSource(playList->at(index));
        ui->musicName->setText(nameList->at(index));
        player->play();
    }
}

void MusicPlayer::PlayNext() {
    if (playList->empty()) {
        return;
    }
    player->stop();
    ui->playButton->setText(tr("暂停"));
    index = (index + 1) % size;
    player->setSource(playList->at(index));
    player->setPosition(0);
    ui->musicName->setText(nameList->at(index));
    ui->progressBar->setValue(0);
    ui->nowTime->setText(FormatTime(0));
    ui->totalTime->setText(FormatTime(player->duration()));
    player->play();
}

void MusicPlayer::UpdateProgress() {
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        ui->nowTime->setText(FormatTime(player->position()));
        ui->totalTime->setText(FormatTime(player->duration()));
        int progress = player->position() * 10000 / player->duration();
        ui->progressBar->setValue(progress); // 更新进度条的值
    }
}

QString MusicPlayer::FormatTime(int mSecond) {
    if (mSecond < 0)
        return "";
    int second = mSecond / 1000;
    int minute = second / 60;
    second %= 60;
    int hour = minute / 60;
    minute %= 60;
    QString sec = QString("%1").arg(second, 2, 10, QChar('0'));
    QString mnt = QString("%1").arg(minute, 2, 10, QChar('0'));
    QString hor = QString("%1").arg(hour, 2, 10, QChar('0'));
    return hor + ":" + mnt + ":" + sec;
}

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
    lyricsList = new QMap<QString, QMap<int, QString>>;

    // 设置播放器
    player = new QMediaPlayer(this);
    output = new QAudioOutput(this);
    output->setDevice(QMediaDevices::defaultAudioOutput());
    player->setAudioOutput(output);
    player->setVideoOutput(nullptr);
    player->setLoops(QMediaPlayer::Once);

    // 设置计时器
    timer = new QTimer(this);

    // 其他
    ui->musicName->setAlignment(Qt::AlignHCenter);
    ui->lyrics->setAlignment(Qt::AlignHCenter);

    // 连接信号与槽
    connect(ui->fileButton, &QPushButton::clicked, this, &MusicPlayer::OpenFolder);
    connect(ui->playButton, &QPushButton::clicked, this, &MusicPlayer::TogglePlay);
    connect(ui->nextButton, &QPushButton::clicked, this, &MusicPlayer::PlayNext);
    connect(timer, &QTimer::timeout, this, &MusicPlayer::UpdateProgress);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::CheckMediaStatus);
    connect(ui->slider, &QSlider::sliderReleased, this, &MusicPlayer::UpdatePosition);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &MusicPlayer::UpdateVolume);
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
        QStringList musicFilters;
        musicFilters << "*.mp3" << "*.ogg";
        QStringList fileNames = dir.entryList(musicFilters, QDir::Files);

        // 遍历所有音乐文件名，将它们添加到播放列表中
        for (const QString& fileName : fileNames) {
            // 使用QUrl::fromLocalFile函数，将本地文件路径转换为QUrl对象
            QUrl url = QUrl::fromLocalFile(dir.absoluteFilePath(fileName));
            playList->push_back(url);
            QFileInfo fileInfo(fileName);
            nameList->append(fileInfo.baseName());
        }

        // 获取歌词文件
        QStringList lyricFilters;
        lyricFilters << "*.lrc";
        QStringList lyricFileNames = dir.entryList(lyricFilters, QDir::Files);

        // 遍历歌词
        for (const QString& fileName : lyricFileNames) {
            QFile lyrics(dir.absoluteFilePath(fileName));
            if (!lyrics.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            auto data = lyrics.readAll();
            QString allLyrics = data.data();
            QTextCodec *codec = QTextCodec::codecForUtfText(data, nullptr);
            if (codec == nullptr)
                codec = QTextCodec::codecForName("GBK");

            allLyrics = codec->toUnicode(data);
            //qDebug() << allLyrics;
            QStringList qSList = allLyrics.split('\n', Qt::SkipEmptyParts);
            lyrics.close();

            // 每个文件对应一个map
            QMap<int ,QString> lyr;
            // 时间正则处理
            QRegularExpression timeRegExp("\\[(\\d{2}):(\\d{2})\\.(\\d{2})\\]");
            for (auto line : qSList) {
                // 读取一行文本
                QRegularExpressionMatch match = timeRegExp.match(line);
                // 如果找到了时间标签
                if (match.hasMatch()) {
                    // 获取时间标签中的分、秒和毫秒，并转换为整数
                    int min = match.captured(1).toInt();
                    int sec = match.captured(2).toInt();
                    int msec = match.captured(3).toInt();

                    // 计算时间（毫秒）= 分*60*1000 + 秒*1000 + 毫秒
                    int time = min * 60 * 1000 + sec * 1000 + msec;

                    // 去掉时间标签，剩下的就是歌词
                    QString lyric = line.remove(timeRegExp);

                    // 将时间和歌词插入到QMap中
                    lyr.insert(time, lyric);
                }
            }
            QFileInfo fileInfo(fileName);
            lyricsList->insert(fileInfo.baseName(), lyr);
        }

        size = playList->size();
        if (size) {
            index = 0;
            player->setSource(playList->at(0));
            ui->musicName->setText(nameList->at(0));
            ui->nowTime->setText(FormatTime(0));
            ui->totalTime->setText(FormatTime(player->duration()));
            ui->playButton->setText(tr("播放"));
            //ui->progressBar->setValue(0);
            ui->slider->setValue(0);
            player->setPosition(0);
            player->stop();
            ui->lyrics->clear();
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
            timer->stop();
            ui->playButton->setText(tr("播放"));
        }
        // 如果媒体播放器的状态是暂停或停止
        else if (player->playbackState() == QMediaPlayer::PausedState
                 || player->playbackState() == QMediaPlayer::StoppedState) {
            player->play();
            timer->start(500);
            ui->playButton->setText(tr("暂停"));
        }
    }
}

void MusicPlayer::CheckMediaStatus(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        ui->lyrics->clear();
        index = (index + 1) % size;
        player->setSource(playList->at(index));
        ui->musicName->setText(nameList->at(index));
        player->play();
        timer->stop();
        timer->start(500);
    }
}

void MusicPlayer::PlayNext() {
    if (playList->empty()) {
        return;
    }
    player->stop();
    timer->stop();
    ui->lyrics->clear();
    ui->playButton->setText(tr("暂停"));
    index = (index + 1) % size;
    player->setSource(playList->at(index));
    player->setPosition(0);
    ui->musicName->setText(nameList->at(index));
    //ui->progressBar->setValue(0);
    ui->slider->setValue(0);
    ui->nowTime->setText(FormatTime(0));
    ui->totalTime->setText(FormatTime(player->duration()));
    player->play();
    timer->start(500);
}

void MusicPlayer::UpdateProgress() {
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        // 更新进度条
        ui->nowTime->setText(FormatTime(player->position()));
        ui->totalTime->setText(FormatTime(player->duration()));
        int position = player->position() * 10000 / player->duration();
        //ui->progressBar->setValue(progress); // 更新进度条的值
        ui->slider->setValue(position);

        // 更新歌词

        QString musicName = nameList->at(index);
        if (lyricsList->contains(musicName)) {
            QMap<int, QString> lyr = lyricsList->value(musicName);
            int time = player->position();

            int cnt = 0;
            for (auto it = lyr.begin(); it != lyr.end(); it++)
                if (it.key() <= time && it.key() > cnt)
                    cnt = it.key();
            if (lyr.contains(cnt))
                ui->lyrics->setText(lyr[cnt]);

        }
    }
}

void MusicPlayer::UpdatePosition() {
    if (playList->empty()) {
        return;
    }
    int newPos = player->duration() * ui->slider->sliderPosition() / 10000;
    player->setPosition(newPos);
}

QString MusicPlayer::FormatTime(int mSecond) {
    if (mSecond < 0)
        return "";
    int second = mSecond / 1000;
    int minute = second / 60;
    second %= 60;
    minute %= 60;
    QString sec = QString("%1").arg(second, 2, 10, QChar('0'));
    QString mnt = QString("%1").arg(minute, 2, 10, QChar('0'));
    return mnt + ":" + sec;
}

void MusicPlayer::UpdateVolume(int val) {
    float volume = static_cast<float>(val) / 100.0;
    output->setVolume(volume);
}

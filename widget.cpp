#include "widget.h"
#include "./ui_widget.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>
#include <QtConcurrent>
#ifdef Q_OS_WINDOWS
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    hashCalculator.moveToThread(&thread);
    // FIXME: Use function pointer.
    connect(&futureWatcher, SIGNAL(finished()), this,
            SLOT(handleDirSearching()));
    connect(ui->pushButton_open_file, &QPushButton::clicked, this, [this] {
        if (checkAlgorithmList()) {
            const QStringList list = QFileDialog::getOpenFileNames(
                this, tr("Please select a file(s)"));
            if (!list.isEmpty()) {
                setFileList(list);
            }
        }
    });
    connect(ui->pushButton_open_folder, &QPushButton::clicked, this, [this] {
        if (checkAlgorithmList()) {
            const QString path = QFileDialog::getExistingDirectory(
                this, tr("Please select a folder"));
            if (!path.isEmpty()) {
                QFuture<QStringList> future = QtConcurrent::run(
                    [=]() -> QStringList { return getFolderContents(path); });
                futureWatcher.setFuture(future);
            }
        }
    });
    connect(ui->pushButton_clear, &QPushButton::clicked, this, [this] {
        ui->textEdit_log->clear();
        if (!isComputing) {
            ui->progressBar_current->setValue(0);
            ui->progressBar_total->setValue(0);
        }
    });
    connect(ui->pushButton_compare, &QPushButton::clicked, this, [this] {
        const QString targetHash = ui->lineEdit_hash->text().trimmed();
        if (targetHash.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("You have to enter a hash value first."));
        } else {
            bool found = false;
            QTextDocument *textDocument = ui->textEdit_log->document();
            QTextCursor highLightTextCursor(textDocument);
            QTextCursor textCursor(textDocument);
            textCursor.beginEditBlock();
            QTextCharFormat highLightTextCharFormat(
                highLightTextCursor.charFormat());
            highLightTextCharFormat.setForeground(Qt::magenta);
            highLightTextCharFormat.setBackground(Qt::yellow);
            highLightTextCharFormat.setFontItalic(true);
            highLightTextCharFormat.setFontUnderline(true);
            highLightTextCharFormat.setUnderlineColor(Qt::darkGray);
            highLightTextCharFormat.setUnderlineStyle(
                QTextCharFormat::DashDotLine);
            while (!highLightTextCursor.isNull() &&
                   !highLightTextCursor.atEnd()) {
                highLightTextCursor = textDocument->find(
                    targetHash.toUpper(), highLightTextCursor,
                    QTextDocument::FindWholeWords);
                if (!highLightTextCursor.isNull()) {
                    found = true;
                    highLightTextCursor.mergeCharFormat(
                        highLightTextCharFormat);
                }
            }
            textCursor.endEditBlock();
            if (!found) {
                QMessageBox::information(this, tr("Result"),
                                         tr("There is no same hash value."));
            }
        }
    });
    connect(&hashCalculator, &HashCalculator::fileChanged, this,
            [](const QString &_path) { Q_UNUSED(_path) });
    connect(&hashCalculator, &HashCalculator::algorithmListChanged, this,
            [](const QStringList &_algorithmList) { Q_UNUSED(_algorithmList) });
    connect(&hashCalculator, &HashCalculator::hashChanged, this,
            [this](const QString &_algorithm, const QString &_hash) {
                if (!_algorithm.isEmpty() && !_hash.isEmpty()) {
                    ui->textEdit_log->append(
                        tr("<font color=\"#287CE5\"><b>File %1</b></font>: %2")
                            .arg(_algorithm, _hash.toUpper()));
                }
            });
    connect(&hashCalculator, &HashCalculator::progressChanged, this,
            [this](quint32 _progress) {
                ui->progressBar_current->setValue(static_cast<int>(_progress));
            });
    connect(&hashCalculator, &HashCalculator::started, this, [this] {
        ui->textEdit_log->append(
            tr("<font color=\"red\"><b>File path</b></font>: \"%1\"")
                .arg(QDir::toNativeSeparators(hashCalculator.file())));
    });
    connect(&hashCalculator, &HashCalculator::finished, this, [this] {
        ui->textEdit_log->append(
            QLatin1String("<font "
                          "color=\"#14B09F\">----------------------------------"
                          "------------------"
                          "-------------</font>"));
        Q_ASSERT(!fileList.isEmpty());
        fileList.removeLast();
        ui->progressBar_total->setValue(ui->progressBar_total->maximum() -
                                        fileList.count());
        if (fileList.isEmpty()) {
            if (!ui->pushButton_open_file->isEnabled()) {
                ui->pushButton_open_file->setEnabled(true);
            }
            if (!ui->pushButton_open_folder->isEnabled()) {
                ui->pushButton_open_folder->setEnabled(true);
            }
            isComputing = false;
        } else {
            computeFileHash(fileList.constLast());
        }
    });
    thread.start();
}

Widget::~Widget() {
    delete ui;
    ui = nullptr;
    futureWatcherCanceled = true;
    if (futureWatcher.isRunning()) {
        futureWatcher.cancel();
        futureWatcher.waitForFinished();
    }
    hashCalculator.stop();
    thread.quit();
    thread.wait();
}

void Widget::handleDirSearching() {
    if (futureWatcherCanceled) {
        return;
    }
    const QStringList list = futureWatcher.result();
    if (!list.isEmpty()) {
        setFileList(list);
    }
}

void Widget::dragEnterEvent(QDragEnterEvent *event) {
    if (isComputing) {
        event->ignore();
        return;
    }
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    const bool ok = checkAlgorithmList(false);
    if (!ok) {
        ui->textEdit_log->append(
            tr("<font color=\"red\"><b>WARNING: The drag & drop event was "
               "ignored due to no hash algorithms were selected.</b></font>"));
        event->ignore();
        return;
    }
    event->acceptProposedAction();
}

void Widget::dropEvent(QDropEvent *event) {
    if (isComputing) {
        event->ignore();
        return;
    }
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        event->ignore();
        return;
    }
    QStringList list;
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            const QString path = url.toLocalFile();
            const QFileInfo fileInfo(path);
            if (fileInfo.isDir()) {
                const QStringList folderContents = getFolderContents(
                    fileInfo.isSymLink() ? fileInfo.symLinkTarget()
                                         : fileInfo.canonicalFilePath());
                if (!folderContents.isEmpty()) {
                    list.append(folderContents);
                }
            } else if (fileInfo.isFile()) {
                list.append(fileInfo.isSymLink()
                                ? fileInfo.symLinkTarget()
                                : fileInfo.canonicalFilePath());
            }
        }
    }
    if (list.isEmpty()) {
        event->ignore();
        return;
    }
    setFileList(list);
    event->acceptProposedAction();
}

#ifdef Q_OS_WINDOWS
void Widget::showEvent(QShowEvent *event) {
    auto *winTaskbarButton = new QWinTaskbarButton(this);
    winTaskbarButton->setWindow(windowHandle());
    QWinTaskbarProgress *winTaskbarProgress = winTaskbarButton->progress();
    connect(this, &Widget::started, this, [winTaskbarProgress](int count) {
        winTaskbarProgress->reset();
        winTaskbarProgress->setMaximum(count > 1 ? count : 100);
    });
    connect(&hashCalculator, &HashCalculator::started, this,
            [winTaskbarProgress] {
                if (winTaskbarProgress->isPaused() ||
                    winTaskbarProgress->isStopped()) {
                    winTaskbarProgress->resume();
                }
                if (!winTaskbarProgress->isVisible()) {
                    winTaskbarProgress->show();
                }
            });
    connect(&hashCalculator, &HashCalculator::finished, this,
            [this, winTaskbarProgress] {
                bool shouldStop = true;
                if (multiFileMode) {
                    const int progress =
                        winTaskbarProgress->maximum() - fileList.count();
                    shouldStop = (progress >= winTaskbarProgress->maximum());
                    winTaskbarProgress->setValue(progress);
                }
                if (shouldStop) {
                    if (winTaskbarProgress->isVisible()) {
                        winTaskbarProgress->hide();
                    }
                    winTaskbarProgress->reset();
                }
            });
    connect(&hashCalculator, &HashCalculator::progressChanged, this,
            [this, winTaskbarProgress](quint32 progress) {
                if (winTaskbarProgress->isPaused() ||
                    winTaskbarProgress->isStopped()) {
                    winTaskbarProgress->resume();
                }
                if (!multiFileMode) {
                    winTaskbarProgress->setValue(static_cast<int>(progress));
                }
                if (!winTaskbarProgress->isVisible()) {
                    winTaskbarProgress->show();
                }
            });
    QWidget::showEvent(event);
}
#endif

void Widget::computeFileHash(const QString &path) {
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    hashCalculator.reset();
    hashCalculator.setAlgorithmList(algorithmList);
    hashCalculator.setFile(fileList.constLast());
    Q_EMIT hashCalculator.startComputing();
}

void Widget::setFileList(const QStringList &list) {
    if (list.isEmpty()) {
        return;
    }
    fileList = list;
    const int count = fileList.count();
    multiFileMode = (count > 1);
    ui->progressBar_total->setValue(0);
    ui->progressBar_total->setMaximum(count);
    if (ui->pushButton_open_file->isEnabled()) {
        ui->pushButton_open_file->setEnabled(false);
    }
    if (ui->pushButton_open_folder->isEnabled()) {
        ui->pushButton_open_folder->setEnabled(false);
    }
    isComputing = true;
    Q_EMIT started(count);
    computeFileHash(fileList.constLast());
}

QStringList Widget::getFolderContents(const QString &folderPath) const {
    if (folderPath.isEmpty() || !QFileInfo::exists(folderPath) ||
        !QFileInfo(folderPath).isDir()) {
        return {};
    }
    const QFileInfo dirInfo(folderPath);
    const QDir dir(dirInfo.isSymLink() ? dirInfo.symLinkTarget()
                                       : dirInfo.canonicalFilePath());
    const auto fileInfoList = dir.entryInfoList(
        QDir::Files | QDir::Readable | QDir::Hidden | QDir::System, QDir::Name);
    const auto folderInfoList = dir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name);
    if (fileInfoList.isEmpty() && folderInfoList.isEmpty()) {
        return {};
    }
    QStringList stringList = {};
    if (!fileInfoList.isEmpty()) {
        for (const auto &fileInfo : fileInfoList) {
            stringList.append(QDir::toNativeSeparators(
                fileInfo.isSymLink() ? fileInfo.symLinkTarget()
                                     : fileInfo.canonicalFilePath()));
        }
    }
    if (!folderInfoList.isEmpty()) {
        for (const auto &folderInfo : folderInfoList) {
            const QStringList _fileList = getFolderContents(
                folderInfo.isSymLink() ? folderInfo.symLinkTarget()
                                       : folderInfo.canonicalFilePath());
            if (!_fileList.isEmpty()) {
                stringList.append(_fileList);
            }
        }
    }
    return stringList;
}

void Widget::refreshAlgorithmList() {
    if (!algorithmList.isEmpty()) {
        algorithmList.clear();
    }
    if (ui->checkBox_md4->isChecked()) {
        algorithmList.append(QLatin1String("MD4"));
    }
    if (ui->checkBox_md5->isChecked()) {
        algorithmList.append(QLatin1String("MD5"));
    }
    if (ui->checkBox_sha1->isChecked()) {
        algorithmList.append(QLatin1String("SHA-1"));
    }
    if (ui->checkBox_sha2_224->isChecked()) {
        algorithmList.append(QLatin1String("SHA-224"));
    }
    if (ui->checkBox_sha2_256->isChecked()) {
        algorithmList.append(QLatin1String("SHA-256"));
    }
    if (ui->checkBox_sha2_384->isChecked()) {
        algorithmList.append(QLatin1String("SHA-384"));
    }
    if (ui->checkBox_sha2_512->isChecked()) {
        algorithmList.append(QLatin1String("SHA-512"));
    }
    if (ui->checkBox_sha3_224->isChecked()) {
        algorithmList.append(QLatin1String("SHA3-224"));
    }
    if (ui->checkBox_sha3_256->isChecked()) {
        algorithmList.append(QLatin1String("SHA3-256"));
    }
    if (ui->checkBox_sha3_384->isChecked()) {
        algorithmList.append(QLatin1String("SHA3-384"));
    }
    if (ui->checkBox_sha3_512->isChecked()) {
        algorithmList.append(QLatin1String("SHA3-512"));
    }
}

bool Widget::checkAlgorithmList(bool showUI) {
    refreshAlgorithmList();
    if (algorithmList.isEmpty()) {
        if (showUI) {
            QMessageBox::warning(
                this, tr("Warning"),
                tr("You should select at least one hash algorithm."));
        }
        return false;
    }
    return true;
}

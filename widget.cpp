#include "widget.h"
#include "./ui_widget.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QTextStream>
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
        if (checkAlgorithmListMore()) {
            const auto list = QFileDialog::getOpenFileNames(
                this, tr("Please select a file(s)"));
            if (!list.isEmpty()) {
                setFileList(list2vector(list));
            }
        }
    });
    connect(ui->pushButton_open_folder, &QPushButton::clicked, this, [this] {
        if (checkAlgorithmListMore()) {
            const QString path = QFileDialog::getExistingDirectory(
                this, tr("Please select a folder"));
            if (!path.isEmpty()) {
                // Set the isComputing flag to prevent the user from opening
                // files while the application is searching for files.
                isComputing = true;
                ui->pushButton_open_file->setEnabled(false);
                ui->pushButton_open_folder->setEnabled(false);
                ui->pushButton_import->setEnabled(false);
                ui->pushButton_export->setEnabled(false);
                QFuture<QVector<QString>> future =
                    QtConcurrent::run([=]() -> QVector<QString> {
                        return getFolderContents(path);
                    });
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
            QMessageBox::warning(this, tr("Empty hash string"),
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
                QMessageBox::information(this, tr("No search result"),
                                         tr("There is no same hash value."));
            }
        }
    });
    connect(ui->pushButton_import, &QPushButton::clicked, this, [this] {
        if (checkAlgorithmListLess()) {
            const QString _path =
                QFileDialog::getOpenFileName(this, tr("Open a hash file"));
            if (!_path.isEmpty()) {
                verifyHashFile(_path);
            }
        }
    });
    connect(ui->pushButton_export, &QPushButton::clicked, this, [this] {
        Q_ASSERT(hashList.count() == 11);
        bool listIsEmpty = true;
        for (const auto &value : qAsConst(hashList)) {
            if (value.count() > 1) {
                listIsEmpty = false;
                break;
            }
        }
        if (listIsEmpty) {
            QMessageBox::warning(this, tr("No hash data"),
                                 tr("No files have been processed before."));
        } else {
            const QString path =
                QFileDialog::getSaveFileName(this, tr("Save hash file"));
            if (!path.isEmpty()) {
                generateHashFile(path);
            }
        }
    });
    connect(ui->pushButton_about_qt, &QPushButton::clicked, qApp,
            &QApplication::aboutQt);
    connect(ui->lineEdit_hash, &QLineEdit::returnPressed,
            ui->pushButton_compare, &QPushButton::clicked);
    connect(&hashCalculator, &HashCalculator::fileChanged, this,
            [](const QString &_path) { Q_UNUSED(_path) });
    connect(&hashCalculator, &HashCalculator::algorithmListChanged, this,
            [](const QVector<QString> &_algorithmList) {
                Q_UNUSED(_algorithmList)
            });
    connect(
        &hashCalculator, &HashCalculator::hashChanged, this,
        [this](const QString &_path, const QString &_algorithm,
               const QString &_hash, bool _matched) {
            if (!_path.isEmpty() && !_algorithm.isEmpty() && !_hash.isEmpty()) {
                if (verifyMode) {
                    if (_matched) {
                        ui->textEdit_log->append(
                            tr("<font "
                               "color=\"green\"><b>[VERIFIED]</b></font>: "
                               "\"%1\"")
                                .arg(_path));
                    } else {
                        ui->textEdit_log->append(
                            tr("<font color=\"red\"><b>[UNMATCHED]</b></font>: "
                               "\"%1\"")
                                .arg(_path));
                        ui->textEdit_log->append(
                            tr("<font color=\"blue\"><b>File %1</b></font>: %2")
                                .arg(_algorithm, _hash.toUpper()));
                    }
                } else {
                    ui->textEdit_log->append(
                        tr("<font color=\"#287CE5\"><b>File %1</b></font>: %2")
                            .arg(_algorithm, _hash.toUpper()));
                    Q_ASSERT(hashList.count() == 11);
                    const QString __algorithm = _algorithm.toLower().replace(
                        QLatin1Char('-'), QLatin1Char('_'));
                    const auto hashData = qMakePair(_hash, _path);
                    if (__algorithm == QLatin1String("md4")) {
                        hashList[0].append(hashData);
                    } else if (__algorithm == QLatin1String("md5")) {
                        hashList[1].append(hashData);
                    } else if (__algorithm == QLatin1String("sha_1")) {
                        hashList[2].append(hashData);
                    } else if (__algorithm == QLatin1String("sha_224")) {
                        hashList[3].append(hashData);
                    } else if (__algorithm == QLatin1String("sha_256")) {
                        hashList[4].append(hashData);
                    } else if (__algorithm == QLatin1String("sha_384")) {
                        hashList[5].append(hashData);
                    } else if (__algorithm == QLatin1String("sha_512")) {
                        hashList[6].append(hashData);
                    } else if (__algorithm == QLatin1String("sha3_224")) {
                        hashList[7].append(hashData);
                    } else if (__algorithm == QLatin1String("sha3_256")) {
                        hashList[8].append(hashData);
                    } else if (__algorithm == QLatin1String("sha3_384")) {
                        hashList[9].append(hashData);
                    } else if (__algorithm == QLatin1String("sha3_512")) {
                        hashList[10].append(hashData);
                    }
                }
            }
        });
    connect(&hashCalculator, &HashCalculator::progressChanged, this,
            [this](quint32 _progress) {
                ui->progressBar_current->setValue(static_cast<int>(_progress));
            });
    connect(
        &hashCalculator, &HashCalculator::started, this,
        [this](const QString &_path) {
            if (!verifyMode) {
                ui->textEdit_log->append(
                    tr("<font color=\"red\"><b>File path</b></font>: \"%1\"")
                        .arg(QDir::toNativeSeparators(_path)));
            }
        });
    connect(&hashCalculator, &HashCalculator::finished, this, [this] {
        if (!verifyMode) {
            ui->textEdit_log->append(
                QLatin1String("<font "
                              "color=\"#14B09F\">------------------------------"
                              "-----------------------------------</font>"));
        }
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
            if (!ui->pushButton_import->isEnabled()) {
                ui->pushButton_import->setEnabled(true);
            }
            if (!ui->pushButton_export->isEnabled()) {
                ui->pushButton_export->setEnabled(true);
            }
            isComputing = false;
            verifyMode = false;
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

void Widget::verifyHashFile(const QString &filePath,
                            const QString &algorithmName, bool fromCmd) {
    if (filePath.isEmpty() || (fromCmd && algorithmName.isEmpty())) {
        return;
    }
    if (!QFileInfo::exists(filePath) || !QFileInfo(filePath).isFile()) {
        return;
    }
    bool checked = !fromCmd;
    if (fromCmd) {
        const QString _algorithm = algorithmName.toLower().remove(
            QRegularExpression(QLatin1String("(-|_)+")));
        ui->checkBox_md4->setChecked(_algorithm == QLatin1String("md4"));
        ui->checkBox_md5->setChecked(_algorithm == QLatin1String("md5"));
        ui->checkBox_sha1->setChecked(_algorithm == QLatin1String("sha1"));
        ui->checkBox_sha2_224->setChecked(_algorithm ==
                                          QLatin1String("sha224"));
        ui->checkBox_sha2_256->setChecked(_algorithm ==
                                          QLatin1String("sha256"));
        ui->checkBox_sha2_384->setChecked(_algorithm ==
                                          QLatin1String("sha384"));
        ui->checkBox_sha2_512->setChecked(_algorithm ==
                                          QLatin1String("sha512"));
        ui->checkBox_sha3_224->setChecked(_algorithm ==
                                          QLatin1String("sha3224"));
        ui->checkBox_sha3_256->setChecked(_algorithm ==
                                          QLatin1String("sha3256"));
        ui->checkBox_sha3_384->setChecked(_algorithm ==
                                          QLatin1String("sha3384"));
        ui->checkBox_sha3_512->setChecked(_algorithm ==
                                          QLatin1String("sha3512"));
        checked = checkAlgorithmListLess();
    }
    if (checked) {
        PairStringList _fileList = {};
        const QFileInfo fileInfo(filePath);
        QFile file(fileInfo.isSymLink() ? fileInfo.symLinkTarget()
                                        : fileInfo.canonicalFilePath());
        QTextStream textStream(&file);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::critical(
                this, tr("Error"),
                tr("Failed to open file: \"%1\"!").arg(filePath));
            return;
        }
        QString line = QString();
        while (textStream.readLineInto(&line)) {
            if (line.isEmpty()) {
                continue;
            }
            const int spaceIndex = line.indexOf(QLatin1Char(' '));
            const int tabIndex = line.indexOf(QLatin1Char('\t'));
            if ((spaceIndex <= 0) && (tabIndex <= 0)) {
                continue;
            }
            const int index = qMin(spaceIndex <= 0 ? 999999 : spaceIndex,
                                   tabIndex <= 0 ? 999999 : tabIndex);
            if (index >= 999999) {
                continue;
            }
            const QString _hashValue = line.left(index);
            line.remove(0, index + 1);
            const QString _filePath = line.trimmed().remove(QLatin1Char('\"'));
            _fileList.append(qMakePair(_filePath, _hashValue));
        }
        file.close();
        if (!_fileList.isEmpty()) {
            verifyMode = true;
            setFileList(_fileList);
        }
    }
}

void Widget::handleDirSearching() {
    if (futureWatcherCanceled) {
        return;
    }
    const QVector<QString> list = futureWatcher.result();
    if (list.isEmpty()) {
        isComputing = false;
        ui->pushButton_open_file->setEnabled(true);
        ui->pushButton_open_folder->setEnabled(true);
        ui->pushButton_import->setEnabled(true);
        ui->pushButton_export->setEnabled(true);
    } else {
        setFileList(list2vector(list));
    }
}

void Widget::dragEnterEvent(QDragEnterEvent *event) {
    if (isComputing) {
        ui->textEdit_log->append(
            tr("<font color=\"red\"><b>WARNING: The drag & drop event was "
               "ignored due to the computing process is running.</b></font>"));
        event->ignore();
        return;
    }
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    const bool ok = checkAlgorithmListMore(false);
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
    QVector<QString> list;
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            const QFileInfo fileInfo(url.toLocalFile());
            const QString path = fileInfo.isSymLink()
                ? fileInfo.symLinkTarget()
                : fileInfo.canonicalFilePath();
            if (fileInfo.isDir()) {
                const QVector<QString> folderContents = getFolderContents(path);
                if (!folderContents.isEmpty()) {
                    list.append(folderContents);
                }
            } else if (fileInfo.isFile()) {
                list.append(path);
            }
        }
    }
    if (list.isEmpty()) {
        event->ignore();
        return;
    }
    setFileList(list2vector(list));
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

void Widget::computeFileHash(const QPair<QString, QString> &targetFile) {
    const QString path = targetFile.first;
    const QFileInfo fileInfo(path);
    if (path.isEmpty() || !QFileInfo::exists(path) || !fileInfo.isFile()) {
        return;
    }
    hashCalculator.reset();
    hashCalculator.setAlgorithmList(algorithmList);
    hashCalculator.setFile(fileInfo.isSymLink() ? fileInfo.symLinkTarget()
                                                : fileInfo.canonicalFilePath(),
                           targetFile.second);
    Q_EMIT hashCalculator.startComputing();
}

void Widget::setFileList(const PairStringList &list) {
    if (list.isEmpty()) {
        return;
    }
    Q_ASSERT(hashList.count() == 11);
    for (auto &hashData : hashList) {
        if (hashData.count() > 1) {
            hashData.remove(1, hashData.count() - 1);
        }
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
    if (ui->pushButton_import->isEnabled()) {
        ui->pushButton_import->setEnabled(false);
    }
    if (ui->pushButton_export->isEnabled()) {
        ui->pushButton_export->setEnabled(false);
    }
    isComputing = true;
    Q_EMIT started(count);
    computeFileHash(fileList.constLast());
}

QVector<QString> Widget::getFolderContents(const QString &folderPath) const {
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
    QVector<QString> stringList = {};
    if (!fileInfoList.isEmpty()) {
        for (const auto &fileInfo : fileInfoList) {
            stringList.append(QDir::toNativeSeparators(
                fileInfo.isSymLink() ? fileInfo.symLinkTarget()
                                     : fileInfo.canonicalFilePath()));
        }
    }
    if (!folderInfoList.isEmpty()) {
        for (const auto &folderInfo : folderInfoList) {
            const QVector<QString> _fileList = getFolderContents(
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

bool Widget::checkAlgorithmListMore(bool showUI) {
    refreshAlgorithmList();
    if (algorithmList.isEmpty()) {
        if (showUI) {
            QMessageBox::warning(
                this, tr("No algorithms selected"),
                tr("You should select at least one hash algorithm."));
        }
        return false;
    }
    return true;
}

bool Widget::checkAlgorithmListLess(bool showUI) {
    refreshAlgorithmList();
    const int count = algorithmList.count();
    if (count < 1) {
        if (showUI) {
            QMessageBox::warning(this, tr("No algorithms selected"),
                                 tr("You should select a hash algorithm."));
        }
        return false;
    }
    if (count > 1) {
        if (showUI) {
            QMessageBox::warning(
                this, tr("Too many algorithms"),
                tr("You can only select one hash algorithm in this mode."));
        }
        ui->checkBox_md4->setChecked(false);
        ui->checkBox_md5->setChecked(false);
        ui->checkBox_sha1->setChecked(false);
        ui->checkBox_sha2_224->setChecked(false);
        ui->checkBox_sha2_256->setChecked(false);
        ui->checkBox_sha2_384->setChecked(false);
        ui->checkBox_sha2_512->setChecked(false);
        ui->checkBox_sha3_224->setChecked(false);
        ui->checkBox_sha3_256->setChecked(false);
        ui->checkBox_sha3_384->setChecked(false);
        ui->checkBox_sha3_512->setChecked(false);
        return false;
    }
    return true;
}

void Widget::generateHashFile(const QString &fileNameTemplate) {
    if (fileNameTemplate.isEmpty()) {
        return;
    }
    Q_ASSERT(hashList.count() == 11);
    QFile file;
    QTextStream textStream(&file);
    for (const auto &hashData : qAsConst(hashList)) {
        if (hashData.count() > 1) {
            const QString suffix = hashData.constFirst().second;
            if (file.isOpen()) {
                file.close();
            }
            file.setFileName(
                QStringLiteral("%1.%2").arg(fileNameTemplate, suffix));
            if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
                QMessageBox::critical(
                    this, tr("Error"),
                    tr("Failed to open file: \"%1\"!").arg(file.fileName()));
                continue;
            }
            for (int i = 1; i != hashData.count(); ++i) {
                // Use lowered hash string
                // Should we use absolute file path or just file name?
                const auto &data = hashData.at(i);
                const auto &hash = data.first.toLower();
                auto path = QDir::toNativeSeparators(data.second);
                if (!path.startsWith(QLatin1Char('\"'))) {
                    path.prepend(QLatin1Char('\"'));
                }
                if (!path.endsWith(QLatin1Char('\"'))) {
                    path.append(QLatin1Char('\"'));
                }
                textStream << hash << QLatin1Char('\t') << path << endl;
            }
            file.close();
        }
    }
    QMessageBox::information(
        this, tr("Done"),
        tr("All hash values have been written to file(s), please check."));
}

Widget::PairStringList Widget::list2vector(const QStringList &list) const {
    if (list.isEmpty()) {
        return {};
    }
    QVector<QString> _list = {};
    for (const auto &value : list) {
        _list.append(value);
    }
    return list2vector(_list);
}

Widget::PairStringList Widget::list2vector(const QVector<QString> &list) const {
    if (list.isEmpty()) {
        return {};
    }
    PairStringList _list = {};
    for (const auto &value : list) {
        _list.append(qMakePair(value, QString()));
    }
    return _list;
}

#include "widget.h"
#include "./ui_widget.h"
#include <QDateTime>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    hashCalculator.moveToThread(&thread);
    connect(ui->pushButton_open_file, &QPushButton::clicked, this, [this] {
        if (checkAlgorithmList()) {
            const QStringList list =
                QFileDialog::getOpenFileNames(this, tr("Please select a file"));
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
                QStringList list = getFolderContents(path);
                if (!list.isEmpty()) {
                    setFileList(list);
                }
            }
        }
    });
    connect(ui->pushButton_clear, &QPushButton::clicked, this, [this] {
        ui->textEdit_log->clear();
        ui->progressBar_current->setValue(0);
        ui->progressBar_total->setValue(0);
    });
    connect(ui->pushButton_compare, &QPushButton::clicked, this, [this] {
        const QString targetHash = ui->lineEdit_hash->text().trimmed();
        if (targetHash.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("You have to enter a hash string first."));
        } else {
            if (!ui->textEdit_log->find(targetHash,
                                        QTextDocument::FindWholeWords)) {
                QMessageBox::information(this, tr("Result"),
                                         tr("There is no same hash."));
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
                        QStringLiteral("File %1: %2")
                            .arg(_algorithm, _hash.toUpper()));
                }
            });
    connect(&hashCalculator, &HashCalculator::progressChanged, this,
            [this](quint32 _progress) {
                ui->progressBar_current->setValue(static_cast<int>(_progress));
            });
    connect(&hashCalculator, &HashCalculator::started, this,
            [this] { outputFileInfo(hashCalculator.file()); });
    connect(&hashCalculator, &HashCalculator::finished, this, [this] {
        ui->textEdit_log->append(QLatin1String(
            "------------------------------------------------------"));
        Q_ASSERT(!fileList.isEmpty());
        fileList.removeLast();
        ui->progressBar_total->setValue(ui->progressBar_total->maximum() -
                                        fileList.count());
        if (!fileList.isEmpty()) {
            computeFileHash(fileList.constLast());
        }
    });
    thread.start();
}

Widget::~Widget() {
    delete ui;
    hashCalculator.stop();
    thread.quit();
    thread.wait();
}

void Widget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        if (checkAlgorithmList()) {
            event->acceptProposedAction();
        }
    }
}

void Widget::dropEvent(QDropEvent *event) {
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    QStringList list;
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            const QString path = url.toLocalFile();
            const QFileInfo fileInfo(path);
            if (fileInfo.isDir()) {
                const QStringList folderContents = getFolderContents(path);
                if (!folderContents.isEmpty()) {
                    list.append(folderContents);
                }
            } else if (fileInfo.isFile()) {
                list.append(fileInfo.isSymLink() ? fileInfo.symLinkTarget()
                                                 : path);
            }
        }
    }
    if (list.isEmpty()) {
        return;
    }
    setFileList(list);
    event->acceptProposedAction();
}

void Widget::outputFileInfo(const QString &path) {
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    QFileInfo fileInfo(path);
    ui->textEdit_log->append(
        tr("File path: %1").arg(QDir::toNativeSeparators(path)));
    ui->textEdit_log->append(tr("File size: %1 bytes").arg(fileInfo.size()));
    ui->textEdit_log->append(tr("File create time: %1")
                                 .arg(fileInfo.birthTime().toString(
                                     QLatin1String("yyyy/MM/dd HH:mm:ss"))));
    ui->textEdit_log->append(tr("File last modify time: %1")
                                 .arg(fileInfo.lastModified().toString(
                                     QLatin1String("yyyy/MM/dd HH:mm:ss"))));
}

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
    ui->progressBar_total->setValue(0);
    ui->progressBar_total->setMaximum(fileList.count());
    computeFileHash(fileList.constLast());
}

QStringList Widget::getFolderContents(const QString &folderPath) const {
    if (folderPath.isEmpty() || !QFileInfo::exists(folderPath) ||
        !QFileInfo(folderPath).isDir()) {
        return {};
    }
    const auto fileInfoList = QDir(folderPath)
                                  .entryInfoList(QDir::Files | QDir::Readable,
                                                 QDir::Name | QDir::Reversed);
    if (fileInfoList.isEmpty()) {
        return {};
    }
    QStringList stringList;
    for (const auto &fileInfo : fileInfoList) {
        stringList.append(QDir::toNativeSeparators(
            fileInfo.isSymLink() ? fileInfo.symLinkTarget()
                                 : fileInfo.canonicalFilePath()));
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

bool Widget::checkAlgorithmList() {
    refreshAlgorithmList();
    if (algorithmList.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("You should select at least one algorithm."));
        return false;
    }
    return true;
}

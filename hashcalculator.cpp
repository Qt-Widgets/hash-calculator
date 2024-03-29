#include "hashcalculator.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QtConcurrent>

HashCalculator::HashCalculator(QObject *parent) : QObject(parent) {}

void HashCalculator::setFile(const QString &path, const QString &targetHash) {
    QMutexLocker mutexLocker(&mutex);
    if (QDir::toNativeSeparators(path) ==
        QDir::toNativeSeparators(targetFile.fileName())) {
        return;
    }
    if (path.isEmpty() || !QFileInfo::exists(path) ||
        !QFileInfo(path).isFile()) {
        qWarning().noquote()
            << "Path is empty or file does not exist or path is not a file:"
            << path;
        return;
    }
    if (!targetHash.isEmpty()) {
        targetHashValue = targetHash.toLower();
    }
    if (targetFile.isOpen()) {
        targetFile.close();
    }
    const QString _path = QDir::toNativeSeparators(path);
    targetFile.setFileName(_path);
    Q_EMIT fileChanged(_path);
}

void HashCalculator::calculateHashValue() {
    if (shouldStop) {
        return;
    }
    if (hashAlgorithmList.isEmpty()) {
        qWarning().noquote() << "Algorithm list cannot be empty.";
        return;
    }
    if (targetFile.isOpen()) {
        targetFile.close();
    }
    const QString _filePath = file();
    Q_EMIT started(_filePath);
    for (const auto &algorithmString : qAsConst(hashAlgorithmList)) {
        QMutexLocker mutexLocker(&mutex);
        if (shouldStop) {
            return;
        }
        if (!targetFile.open(QFile::ReadOnly)) {
            qWarning().noquote()
                << "Failed to open file:" << targetFile.errorString();
            return;
        }
        hashAlgorithm = algorithmString.toUpper().replace(QLatin1Char('_'),
                                                          QLatin1Char('-'));
        QCryptographicHash cryptographicHash(str2enum(hashAlgorithm));
        QByteArray buffer;
        qint64 loadedDataSize = 0;
        while (!(buffer = targetFile.read(10 * 1024 * 1024)).isEmpty()) {
            if (shouldStop) {
                return;
            }
            cryptographicHash.addData(buffer);
            computeProgress = static_cast<quint32>(
                (loadedDataSize += buffer.size()) * 100.0 / targetFile.size());
            Q_EMIT progressChanged(computeProgress);
        }
        targetFile.close();
        computeResult = QLatin1String(cryptographicHash.result().toHex());
        computeResult = computeResult.toLower();
        Q_EMIT hashChanged(_filePath, hashAlgorithm, computeResult,
                           targetHashValue.isEmpty()
                               ? true
                               : (targetHashValue == computeResult));
    }
    Q_EMIT finished(_filePath);
}

void HashCalculator::reset() {
    QMutexLocker mutexLocker(&mutex);
    targetHashValue.clear();
    if (targetFile.isOpen()) {
        targetFile.close();
    }
    Q_EMIT fileChanged(QString());
    if (!hashAlgorithmList.isEmpty()) {
        hashAlgorithmList.clear();
    }
    hashAlgorithmList << QLatin1String("MD5") << QLatin1String("SHA-256");
    Q_EMIT algorithmListChanged(hashAlgorithmList);
    computeProgress = 0;
    Q_EMIT progressChanged(0);
    computeResult.clear();
    Q_EMIT hashChanged(QString(), QString(), QString(), false);
    hashAlgorithm = QLatin1String("MD5");
    shouldStop = false;
}

void HashCalculator::stop() {
    QMutexLocker mutexLocker(&mutex);
    shouldStop = true;
}

QCryptographicHash::Algorithm HashCalculator::str2enum(const QString &string) {
    if (string.isEmpty()) {
        return QCryptographicHash::Algorithm::Md5;
    }
    const QString algorithm =
        string.toLower().replace(QLatin1Char('-'), QLatin1Char('_'));
    if (algorithm == QLatin1String("md4")) {
        return QCryptographicHash::Algorithm::Md4;
    }
    if (algorithm == QLatin1String("md5")) {
        return QCryptographicHash::Algorithm::Md5;
    }
    if (algorithm == QLatin1String("sha_1")) {
        return QCryptographicHash::Algorithm::Sha1;
    }
    if (algorithm == QLatin1String("sha_224")) {
        return QCryptographicHash::Algorithm::Sha224;
    }
    if (algorithm == QLatin1String("sha_256")) {
        return QCryptographicHash::Algorithm::Sha256;
    }
    if (algorithm == QLatin1String("sha_384")) {
        return QCryptographicHash::Algorithm::Sha384;
    }
    if (algorithm == QLatin1String("sha_512")) {
        return QCryptographicHash::Algorithm::Sha512;
    }
    if (algorithm == QLatin1String("sha3_224")) {
        return QCryptographicHash::Algorithm::Sha3_224;
    }
    if (algorithm == QLatin1String("sha3_256")) {
        return QCryptographicHash::Algorithm::Sha3_256;
    }
    if (algorithm == QLatin1String("sha3_384")) {
        return QCryptographicHash::Algorithm::Sha3_384;
    }
    if (algorithm == QLatin1String("sha3_512")) {
        return QCryptographicHash::Algorithm::Sha3_512;
    }
    return QCryptographicHash::Algorithm::Md5;
}

QString HashCalculator::hash() const { return computeResult; }

QString HashCalculator::file() const {
    return QDir::toNativeSeparators(targetFile.fileName());
}

quint32 HashCalculator::progress() const { return computeProgress; }

QVector<QString> HashCalculator::algorithmList() const {
    return hashAlgorithmList;
}

void HashCalculator::setAlgorithmList(const QVector<QString> &list) {
    QMutexLocker mutexLocker(&mutex);
    if (list.isEmpty()) {
        return;
    }
    hashAlgorithmList = list;
    Q_EMIT algorithmListChanged(hashAlgorithmList);
}

QString HashCalculator::currentAlgorithm() const { return hashAlgorithm; }

void HashCalculator::compute() {
    if (shouldStop) {
        return;
    }
    QtConcurrent::run([this]() { calculateHashValue(); });
}

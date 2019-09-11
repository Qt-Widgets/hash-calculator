#include "hashcalculator.h"
#include <QDebug>
#include <QDir>

HashCalculator::HashCalculator(QObject *parent) : QObject(parent) {
    connect(this, &HashCalculator::startComputing, this,
            &HashCalculator::compute);
}

void HashCalculator::setFile(const QString &path) {
    if (QDir::toNativeSeparators(path) ==
        QDir::toNativeSeparators(targetFile.fileName())) {
        return;
    }
    if (path.isEmpty() || !QFile::exists(path)) {
        qDebug().noquote() << "Path is empty or file does not exist.";
        return;
    }
    if (targetFile.isOpen()) {
        targetFile.close();
    }
    targetFile.setFileName(QDir::toNativeSeparators(path));
    Q_EMIT fileChanged();
}

void HashCalculator::setAlgorithm(const QString &algorithm) {
    const QString _algorithm =
        algorithm.toLower().replace(QLatin1Char('-'), QLatin1Char('_'));
    if (_algorithm ==
        this->algorithm().toLower().replace(QLatin1Char('-'),
                                            QLatin1Char('_'))) {
        return;
    }
    if (_algorithm == QLatin1String("md4")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Md4;
    } else if (_algorithm == QLatin1String("md5")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Md5;
    } else if (_algorithm == QLatin1String("sha_1")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha1;
    } else if (_algorithm == QLatin1String("sha_224")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha224;
    } else if (_algorithm == QLatin1String("sha_256")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha256;
    } else if (_algorithm == QLatin1String("sha_384")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha384;
    } else if (_algorithm == QLatin1String("sha_512")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha512;
    } else if (_algorithm == QLatin1String("sha3_224")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha3_224;
    } else if (_algorithm == QLatin1String("sha3_256")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha3_256;
    } else if (_algorithm == QLatin1String("sha3_384")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha3_384;
    } else if (_algorithm == QLatin1String("sha3_512")) {
        hashAlgorithm = QCryptographicHash::Algorithm::Sha3_512;
    } else {
        hashAlgorithm = QCryptographicHash::Algorithm::Md4;
    }
    Q_EMIT algorithmChanged();
}

void HashCalculator::compute() {
    if (targetFile.isOpen()) {
        targetFile.close();
    }
    if (!targetFile.open(QFile::ReadOnly)) {
        qDebug().noquote() << "Failed to open file:"
                           << targetFile.errorString();
        return;
    }
    Q_EMIT started();
    QCryptographicHash cryptographicHash(hashAlgorithm);
    QByteArray buffer;
    qint64 loadedDataSize = 0;
    while (!(buffer = targetFile.read(10 * 1024 * 1024)).isEmpty()) {
        cryptographicHash.addData(buffer);
        computeProgress = static_cast<quint32>(
            (loadedDataSize += buffer.size()) * 100 / targetFile.size());
        Q_EMIT progressChanged();
    }
    targetFile.close();
    computeResult = QLatin1String(cryptographicHash.result().toHex());
    Q_EMIT finished();
    Q_EMIT hashChanged();
}

void HashCalculator::reset() {
    if (targetFile.isOpen()) {
        targetFile.close();
    }
    Q_EMIT fileChanged();
    hashAlgorithm = QCryptographicHash::Md4;
    Q_EMIT algorithmChanged();
    computeProgress = 0;
    Q_EMIT progressChanged();
    computeResult.clear();
    Q_EMIT hashChanged();
}

QString HashCalculator::hash() const { return computeResult; }

QString HashCalculator::file() const {
    return QDir::toNativeSeparators(targetFile.fileName());
}

QString HashCalculator::algorithm() const {
    switch (hashAlgorithm) {
    case QCryptographicHash::Md4:
        return QLatin1String("MD4");
    case QCryptographicHash::Md5:
        return QLatin1String("MD5");
    case QCryptographicHash::Sha1:
        return QLatin1String("SHA-1");
    case QCryptographicHash::Sha224:
        return QLatin1String("SHA-224");
    case QCryptographicHash::Sha256:
        return QLatin1String("SHA-256");
    case QCryptographicHash::Sha384:
        return QLatin1String("SHA-384");
    case QCryptographicHash::Sha512:
        return QLatin1String("SHA-512");
    case QCryptographicHash::Sha3_224:
        return QLatin1String("SHA3-224");
    case QCryptographicHash::Sha3_256:
        return QLatin1String("SHA3-256");
    case QCryptographicHash::Sha3_384:
        return QLatin1String("SHA3-384");
    case QCryptographicHash::Sha3_512:
        return QLatin1String("SHA3-512");
    default:
        return QLatin1String("MD4");
    }
}

quint32 HashCalculator::progress() const { return computeProgress; }

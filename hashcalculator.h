#pragma once

#include <QCryptographicHash>
#include <QFile>
#include <QObject>

class HashCalculator : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString algorithm READ algorithm WRITE setAlgorithm NOTIFY
                   algorithmChanged)
    Q_PROPERTY(QString hash READ hash NOTIFY hashChanged)
    Q_PROPERTY(quint32 progress READ progress NOTIFY progressChanged)

Q_SIGNALS:
    void fileChanged();
    void algorithmChanged();
    void hashChanged();
    void progressChanged();

    // Emitted when computing just started
    void started();
    // Emitted when computing just finished
    void finished();

    // Emit this signal = call compute()
    void startComputing();

public:
    explicit HashCalculator(QObject *parent = nullptr);

    [[nodiscard]] QString file() const;
    void setFile(const QString &path);

    [[nodiscard]] QString algorithm() const;
    void setAlgorithm(const QString &algorithm);

    [[nodiscard]] QString hash() const;

    [[nodiscard]] quint32 progress() const;

    Q_INVOKABLE void compute();
    Q_INVOKABLE void reset();

private:
    QCryptographicHash::Algorithm hashAlgorithm =
        QCryptographicHash::Algorithm::Md4;
    QFile targetFile;
    QString computeResult = QString();
    quint32 computeProgress = 0;
};

#pragma once

#include <QCryptographicHash>
#include <QFile>
#include <QMutex>
#include <QObject>
#include <QVector>

class HashCalculator : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString hash READ hash NOTIFY hashChanged)
    Q_PROPERTY(quint32 progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QVector<QString> algorithmList READ algorithmList WRITE
                   setAlgorithmList NOTIFY algorithmListChanged)

Q_SIGNALS:
    void fileChanged(const QString &);
    void hashChanged(const QString &, const QString &, const QString &, bool);
    void progressChanged(quint32);
    void algorithmListChanged(const QVector<QString> &);

    // Emitted when computing just started
    void started(const QString &);
    // Emitted when computing just finished
    void finished(const QString &);

public:
    explicit HashCalculator(QObject *parent = nullptr);

    [[nodiscard]] QString file() const;
    void setFile(const QString &path, const QString &targetHash = QString());

    [[nodiscard]] QString hash() const;

    [[nodiscard]] quint32 progress() const;

    [[nodiscard]] QVector<QString> algorithmList() const;
    void setAlgorithmList(const QVector<QString> &list);

    [[nodiscard]] QString currentAlgorithm() const;

    Q_INVOKABLE void compute();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void stop();

private:
    void calculateHashValue();
    QCryptographicHash::Algorithm str2enum(const QString &string);

private:
    QVector<QString> hashAlgorithmList = {QLatin1String("MD5"),
                                          QLatin1String("SHA-256")};
    QFile targetFile;
    QString computeResult = QString(), hashAlgorithm = QLatin1String("MD5"),
            targetHashValue = QString();
    quint32 computeProgress = 0;
    bool shouldStop = false;
    QMutex mutex;
};

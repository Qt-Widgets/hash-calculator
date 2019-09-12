#pragma once

#include <QCryptographicHash>
#include <QFile>
#include <QObject>

class HashCalculator : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString hash READ hash NOTIFY hashChanged)
    Q_PROPERTY(quint32 progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QStringList algorithmList READ algorithmList WRITE
                   setAlgorithmList NOTIFY algorithmListChanged)

Q_SIGNALS:
    void fileChanged(const QString &);
    void hashChanged(const QString &, const QString &);
    void progressChanged(quint32);
    void algorithmListChanged(const QStringList &);

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

    [[nodiscard]] QString hash() const;

    [[nodiscard]] quint32 progress() const;

    [[nodiscard]] QStringList algorithmList() const;
    void setAlgorithmList(const QStringList &list);

    [[nodiscard]] QString currentAlgorithm() const;

    Q_INVOKABLE void compute();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void stop();

private:
    QCryptographicHash::Algorithm str2enum(const QString &string);

private:
    QStringList hashAlgorithmList = {QLatin1String("MD4")};
    QFile targetFile;
    QString computeResult = QString(), hashAlgorithm = QLatin1String("MD4");
    quint32 computeProgress = 0;
    bool shouldStop = false;
};

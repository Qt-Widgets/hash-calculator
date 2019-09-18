#pragma once

#include "hashcalculator.h"
#include <QPair>
#include <QVector>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Widget)

    using PairStringList = QVector<QPair<QString, QString>>;

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

    void verifyHashFile(const QString &filePath,
                        const QString &algorithmName = QString(),
                        bool fromCmd = false, bool silent = false);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
#ifdef Q_OS_WINDOWS
    void showEvent(QShowEvent *event) override;
#endif

private:
    void computeFileHash(const QPair<QString, QString> &targetFile);
    void setFileList(const PairStringList &list);
    [[nodiscard]] QVector<QString>
    getFolderContents(const QString &folderPath) const;
    void refreshAlgorithmList();
    bool checkAlgorithmListMore(bool showUI = true);
    bool checkAlgorithmListLess(bool showUI = true);
    void generateHashFile(const QString &fileNameTemplate);
    PairStringList list2vector(const QStringList &list) const;
    PairStringList list2vector(const QVector<QString> &list) const;

private:
    Ui::Widget *ui = nullptr;
    HashCalculator hashCalculator;
    QVector<QString> algorithmList = {};
    bool isComputing = false, multiFileMode = false, verifyMode = false,
         silentMode = false;
    PairStringList fileList = {};
    QVector<PairStringList> hashList = {
        // Algorithm name, hash file suffix
        {qMakePair(QLatin1String("MD4"), QLatin1String("md4"))},
        {qMakePair(QLatin1String("MD5"), QLatin1String("md5"))},
        {qMakePair(QLatin1String("SHA-1"), QLatin1String("sha1"))},
        {qMakePair(QLatin1String("SHA-224"), QLatin1String("sha224"))},
        {qMakePair(QLatin1String("SHA-256"), QLatin1String("sha256"))},
        {qMakePair(QLatin1String("SHA-384"), QLatin1String("sha384"))},
        {qMakePair(QLatin1String("SHA-512"), QLatin1String("sha512"))},
        {qMakePair(QLatin1String("SHA3-224"), QLatin1String("sha3_224"))},
        {qMakePair(QLatin1String("SHA3-256"), QLatin1String("sha3_256"))},
        {qMakePair(QLatin1String("SHA3-384"), QLatin1String("sha3_384"))},
        {qMakePair(QLatin1String("SHA3-512"), QLatin1String("sha3_512"))}};
    int totalFileCount = 0, unmatchedFileCount = 0;

Q_SIGNALS:
    void started(int);
    void private_setFileList(const PairStringList &);
};

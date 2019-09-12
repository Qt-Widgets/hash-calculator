#pragma once

#include "hashcalculator.h"
#include <QThread>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Widget)

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void computeFileHash(const QString &path);
    void setFileList(const QStringList &list);
    [[nodiscard]] QStringList
    getFolderContents(const QString &folderPath) const;
    void refreshAlgorithmList();
    bool checkAlgorithmList();

private:
    Ui::Widget *ui = nullptr;
    HashCalculator hashCalculator;
    QThread thread;
    QStringList fileList = {}, algorithmList = {};
};

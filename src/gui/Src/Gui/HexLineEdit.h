#pragma once

#include <QLineEdit>

// Qt6 moved QTextCodec into the Core5Compat module and no longer pulls it in
// transitively; forward-declare it here since only pointers are used.
class QTextCodec;

namespace Ui
{
    class HexLineEdit;
}

class HexLineEdit : public QLineEdit
{
    Q_OBJECT

public:

    explicit HexLineEdit(QWidget* parent = nullptr);
    ~HexLineEdit();

    void keyPressEvent(QKeyEvent* event);

    void setData(const QByteArray & data);
    QByteArray data();

    void setEncoding(QTextCodec* encoding);
    QTextCodec* encoding();

    void setKeepSize(const bool enabled);
    bool keepSize();

    void setOverwriteMode(bool overwriteMode);
    bool overwriteMode();

signals:
    void dataEdited();

private slots:
    void updateData(const QString & arg1);

private:
    Ui::HexLineEdit* ui;

    QByteArray mData;
    QTextCodec* mEncoding;
    bool mKeepSize;
    bool mOverwriteMode;

    QByteArray toEncodedData(const QString & text);
};

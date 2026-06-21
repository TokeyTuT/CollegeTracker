#ifndef PHOTOCROPDIALOG_H
#define PHOTOCROPDIALOG_H

#include <QDialog>
#include <QPixmap>

class PhotoCropDialog : public QDialog {
public:
    explicit PhotoCropDialog(const QPixmap &source,
                             QWidget *parent = nullptr);

    QPixmap croppedPixmap() const;

private:
    class CropCanvas;
    CropCanvas *m_canvas = nullptr;
};

#endif // PHOTOCROPDIALOG_H

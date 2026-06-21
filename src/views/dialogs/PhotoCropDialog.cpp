#include "PhotoCropDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSlider>
#include <QTransform>
#include <QVBoxLayout>

class PhotoCropDialog::CropCanvas : public QWidget {
public:
    explicit CropCanvas(const QPixmap &source, QWidget *parent = nullptr)
        : QWidget(parent), m_source(source) {
        setMinimumSize(420, 420);
        setMouseTracking(true);
        setCursor(Qt::OpenHandCursor);
        rebuildDisplayPixmap();
    }

    void setRadius(int radius) {
        m_radius = radius;
        constrainCenter();
        update();
    }

    int radius() const {
        return m_radius;
    }

    void setRotation(int degrees) {
        m_rotation = degrees;
        rebuildDisplayPixmap();
        m_center = QPointF(width() / 2.0, height() / 2.0);
        constrainCenter();
        update();
    }

    int rotation() const {
        return m_rotation;
    }

    QPixmap croppedPixmap() const {
        if (m_display.isNull())
            return {};
        QSize scaledSize = m_display.size();
        scaledSize.scale(size(), Qt::KeepAspectRatio);
        const int imageX = (width() - scaledSize.width()) / 2;
        const int imageY = (height() - scaledSize.height()) / 2;
        const double fx = (m_center.x() - imageX) / scaledSize.width();
        const double fy = (m_center.y() - imageY) / scaledSize.height();
        const double radiusRatio =
            m_radius /
            static_cast<double>(qMin(scaledSize.width(), scaledSize.height()));
        const int sourceCenterX =
            qBound(0, qRound(fx * m_display.width()), m_display.width());
        const int sourceCenterY =
            qBound(0, qRound(fy * m_display.height()), m_display.height());
        const int sourceRadius =
            qBound(10,
                   qRound(radiusRatio *
                          qMin(m_display.width(), m_display.height())),
                   qMin(m_display.width(), m_display.height()) / 2);
        const int cropX = qMax(0, sourceCenterX - sourceRadius);
        const int cropY = qMax(0, sourceCenterY - sourceRadius);
        const int cropWidth =
            qMin(sourceRadius * 2, m_display.width() - cropX);
        const int cropHeight =
            qMin(sourceRadius * 2, m_display.height() - cropY);
        const int side = qMin(cropWidth, cropHeight);
        return m_display.copy(cropX, cropY, side, side);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if (m_center.isNull())
            m_center = QPointF(width() / 2.0, height() / 2.0);
        constrainCenter();
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        if (!m_display.isNull()) {
            QSize scaledSize = m_display.size();
            scaledSize.scale(size(), Qt::KeepAspectRatio);
            const int x = (width() - scaledSize.width()) / 2;
            const int y = (height() - scaledSize.height()) / 2;
            painter.drawPixmap(x, y, scaledSize.width(), scaledSize.height(),
                               m_display);
        }

        QPainterPath fullArea;
        fullArea.addRect(rect());
        QPainterPath selection;
        selection.addEllipse(m_center, m_radius, m_radius);
        painter.fillPath(fullArea.subtracted(selection),
                         QColor(0, 0, 0, 120));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 255, 255, 220), 3));
        painter.drawEllipse(m_center, m_radius, m_radius);
        painter.setPen(QPen(QColor(31, 107, 91), 2));
        painter.drawEllipse(m_center, m_radius + 1, m_radius + 1);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton)
            return;
        m_dragging = true;
        m_dragOffset = m_center - event->pos();
        setCursor(Qt::ClosedHandCursor);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!m_dragging)
            return;
        m_center = event->pos() + m_dragOffset;
        constrainCenter();
        update();
    }

    void mouseReleaseEvent(QMouseEvent *) override {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
    }

private:
    QPixmap m_source;
    QPixmap m_display;
    QPointF m_center;
    QPointF m_dragOffset;
    int m_radius = 90;
    int m_rotation = 0;
    bool m_dragging = false;

    void rebuildDisplayPixmap() {
        if (m_rotation == 0) {
            m_display = m_source;
            return;
        }
        QTransform transform;
        transform.rotate(m_rotation);
        m_display =
            m_source.transformed(transform, Qt::SmoothTransformation);
    }

    void constrainCenter() {
        m_center.setX(
            qBound(static_cast<double>(m_radius), m_center.x(),
                   static_cast<double>(qMax(m_radius, width() - m_radius))));
        m_center.setY(
            qBound(static_cast<double>(m_radius), m_center.y(),
                   static_cast<double>(qMax(m_radius, height() - m_radius))));
    }
};

PhotoCropDialog::PhotoCropDialog(const QPixmap &source, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("框选头像 — 拖动圆形选区定位"));
    setMinimumSize(520, 650);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(14);
    auto *title = new QLabel(QStringLiteral("拖动圆形选区，框选头像区域"));
    title->setStyleSheet(
        QStringLiteral("font-size:18px;font-weight:800;color:#0F172A;"));
    layout->addWidget(title);

    m_canvas = new CropCanvas(source, this);
    layout->addWidget(m_canvas, 1);

    auto *radiusRow = new QHBoxLayout;
    radiusRow->addWidget(new QLabel(QStringLiteral("选区大小：")));
    auto *radiusSlider = new QSlider(Qt::Horizontal, this);
    radiusSlider->setRange(40, 160);
    radiusSlider->setValue(m_canvas->radius());
    auto *radiusValue =
        new QLabel(QStringLiteral("%1px").arg(m_canvas->radius()));
    radiusRow->addWidget(radiusSlider, 1);
    radiusRow->addWidget(radiusValue);
    layout->addLayout(radiusRow);
    connect(radiusSlider, &QSlider::valueChanged, this,
            [this, radiusValue](int value) {
                m_canvas->setRadius(value);
                radiusValue->setText(QStringLiteral("%1px").arg(value));
            });

    auto *rotationRow = new QHBoxLayout;
    rotationRow->addWidget(new QLabel(QStringLiteral("旋转角度：")));
    auto *leftButton = new QPushButton(QStringLiteral("↺ -90°"), this);
    auto *rotationSlider = new QSlider(Qt::Horizontal, this);
    rotationSlider->setRange(-180, 180);
    auto *rotationValue = new QLabel(QStringLiteral("0°"), this);
    auto *rightButton = new QPushButton(QStringLiteral("↻ +90°"), this);
    auto *resetButton = new QPushButton(QStringLiteral("⟲ 0°"), this);
    rotationRow->addWidget(leftButton);
    rotationRow->addWidget(rotationSlider, 1);
    rotationRow->addWidget(rotationValue);
    rotationRow->addWidget(rightButton);
    rotationRow->addWidget(resetButton);
    layout->addLayout(rotationRow);
    connect(rotationSlider, &QSlider::valueChanged, this,
            [this, rotationValue](int value) {
                m_canvas->setRotation(value);
                rotationValue->setText(QStringLiteral("%1°").arg(value));
            });
    connect(leftButton, &QPushButton::clicked, this,
            [this, rotationSlider]() {
                int value = m_canvas->rotation() - 90;
                if (value < -180)
                    value += 360;
                rotationSlider->setValue(value);
            });
    connect(rightButton, &QPushButton::clicked, this,
            [this, rotationSlider]() {
                int value = m_canvas->rotation() + 90;
                if (value > 180)
                    value -= 360;
                rotationSlider->setValue(value);
            });
    connect(resetButton, &QPushButton::clicked, this,
            [rotationSlider]() { rotationSlider->setValue(0); });

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)
        ->setText(QStringLiteral("取消"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QPixmap PhotoCropDialog::croppedPixmap() const {
    return m_canvas ? m_canvas->croppedPixmap() : QPixmap();
}

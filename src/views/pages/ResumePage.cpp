#include "ResumePage.h"

#include "AppDataPaths.h"
#include "AvatarUtils.h"
#include "DatabaseMannager.h"
#include "PhotoCropDialog.h"
#include "ResumeExporter.h"
#include "ResumeTemplateRegistry.h"
#include "Theme.h"
#include "User.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {

constexpr qreal kHiResScale = 4.0;

QFrame *sectionCard(const QString &title) {
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("resumeSectionCard"));
    card->setFrameShape(QFrame::NoFrame);
    card->setStyleSheet(QStringLiteral(
        "QFrame#resumeSectionCard { background:#FFFEFA;"
        "border:1px solid #DED8CC;border-radius:12px; }"));
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(13, 148, 136, 14));
    card->setGraphicsEffect(shadow);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 13, 20, 13);
    layout->setSpacing(6);
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("resumeSectionTitle"));
    titleLabel->setStyleSheet(
        QStringLiteral("font-size:17px;font-weight:800;color:#25332F;"));
    layout->addWidget(titleLabel);
    return card;
}

QIcon paperIcon(const QString &resource) {
    constexpr int width = 170;
    constexpr int height = 226;
    const QPixmap source(resource);
    QPixmap canvas(qCeil(width * kHiResScale),
                   qCeil(height * kHiResScale));
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(35, 45, 42, 25));
    painter.drawRoundedRect(QRectF(10 * kHiResScale, 8 * kHiResScale,
                                   154 * kHiResScale, 214 * kHiResScale),
                            6 * kHiResScale, 6 * kHiResScale);
    painter.drawPixmap(
        QRectF(5 * kHiResScale, 3 * kHiResScale, 154 * kHiResScale,
               214 * kHiResScale),
        source, source.rect());
    canvas.setDevicePixelRatio(kHiResScale);
    return QIcon(canvas);
}

} // namespace

ResumePage::ResumePage(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("profilePage"));
    m_exporter = new ResumeExporter(this);
    buildUi();
    qApp->installEventFilter(this);
    refresh();
}

ResumePage::~ResumePage() {
    qApp->removeEventFilter(this);
}

void ResumePage::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 6, 0, 0);
    mainLayout->setSpacing(12);

    auto *photoCard = sectionCard(QStringLiteral("个人照片"));
    auto *photoLayout = static_cast<QVBoxLayout *>(photoCard->layout());
    auto *photoHeader = new QHBoxLayout;
    m_photoPreview = new QLabel(photoCard);
    m_photoPreview->setFixedSize(96, 96);
    m_photoPreview->setAlignment(Qt::AlignCenter);
    m_photoPreview->setStyleSheet(QStringLiteral(
        "background:#F4F1EA;border:1px dashed #D6D0C4;"
        "border-radius:48px;color:#7A827E;"));
    m_photoPreview->setPixmap(
        AvatarUtils::circularAvatar(QString(), 96));
    auto *choosePhotoButton =
        new QPushButton(QStringLiteral("导入照片"), photoCard);
    choosePhotoButton->setObjectName(QStringLiteral("selectPhotoBtn"));
    photoHeader->addWidget(m_photoPreview);
    photoHeader->addStretch();
    photoHeader->addWidget(choosePhotoButton);
    photoLayout->addLayout(photoHeader);
    photoCard->hide();

    auto *templateGallery = new QFrame(this);
    templateGallery->setObjectName(QStringLiteral("resumeTemplateGallery"));
    auto *galleryLayout = new QVBoxLayout(templateGallery);
    galleryLayout->setContentsMargins(0, 2, 0, 8);
    galleryLayout->setSpacing(14);
    auto *galleryHeader = new QHBoxLayout;
    auto *galleryTitle = new QLabel(QStringLiteral("⌄  模板"));
    galleryTitle->setStyleSheet(
        QStringLiteral("color:#56615D;font-size:18px;font-weight:800;"));
    m_templateDescription =
        new QLabel(QStringLiteral("传统学术排版，信息清晰，适合通用申请。"));
    m_templateDescription->setStyleSheet(QStringLiteral(
        "color:#7A827E;font-size:12px;font-weight:600;padding-top:2px;"));
    auto *galleryHint =
        new QLabel(QStringLiteral("点击切换 · 按空格放大 / 返回"));
    galleryHint->setStyleSheet(QStringLiteral(
        "color:#1F6B5B;font-size:11px;font-weight:700;padding-top:2px;"));
    galleryHeader->addWidget(galleryTitle);
    galleryHeader->addWidget(m_templateDescription, 1);
    galleryHeader->addWidget(galleryHint);
    galleryLayout->addLayout(galleryHeader);

    m_templateCombo = new QComboBox(this);
    m_templateCombo->hide();

    const QList<ResumeTemplateDefinition> &resumeTemplates =
        ResumeTemplateRegistry::templates();
    auto *group = new QButtonGroup(templateGallery);
    group->setExclusive(true);
    auto *previewRow = new QHBoxLayout;
    previewRow->setContentsMargins(6, 0, 6, 0);
    previewRow->setSpacing(28);
    for (int index = 0; index < resumeTemplates.size(); ++index) {
        const ResumeTemplateDefinition &resumeTemplate =
            resumeTemplates.at(index);
        m_templateCombo->addItem(resumeTemplate.displayName,
                                 resumeTemplate.id);
        auto *button = new QToolButton(templateGallery);
        button->setObjectName(QStringLiteral("resumeTemplatePreview"));
        button->setCheckable(true);
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setIcon(paperIcon(resumeTemplate.previewResource));
        button->setIconSize(QSize(170, 226));
        button->setText(resumeTemplate.displayName);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setFixedHeight(264);
        button->setStyleSheet(QStringLiteral(
            "QToolButton{background:transparent;color:#37443F;border:none;"
            "border-bottom:3px solid transparent;padding:2px 4px 7px;"
            "font-size:13px;font-weight:700;}"
            "QToolButton:hover{color:#1F6B5B;}"
            "QToolButton:checked{color:#174F44;border-bottom:3px solid "
            "#D97745;font-weight:850;}"));
        group->addButton(button, index);
        m_templateCards.append(button);
        previewRow->addWidget(button, 1);
        connect(button, &QToolButton::clicked, this,
                [this, index]() { m_templateCombo->setCurrentIndex(index); });
    }
    galleryLayout->addLayout(previewRow);
    mainLayout->addWidget(templateGallery);

    QFrame *skillsCard = sectionCard(QStringLiteral("技术能力"));
    skillsCard->setFixedHeight(116);
    auto *skillsLayout = static_cast<QVBoxLayout *>(skillsCard->layout());
    auto *skillsTitle =
        skillsCard->findChild<QLabel *>(QStringLiteral("resumeSectionTitle"));
    skillsLayout->removeWidget(skillsTitle);
    auto *skillsHeader = new QHBoxLayout;
    skillsHeader->addWidget(skillsTitle);
    skillsHeader->addStretch();
    auto *editSkillsButton =
        new QPushButton(QStringLiteral("编辑"), skillsCard);
    editSkillsButton->setObjectName(QStringLiteral("editSkillsBtn"));
    editSkillsButton->setProperty("variant", "secondary");
    skillsHeader->addWidget(editSkillsButton);
    skillsLayout->addLayout(skillsHeader);
    m_skillsLabel =
        new QLabel(
            QStringLiteral(
                "技能树还是空的。点击编辑，点亮 Qt 法术、火把维护或史莱姆沟通。"),
            skillsCard);
    m_skillsLabel->setWordWrap(true);
    m_skillsLabel->setStyleSheet(QStringLiteral(
        "color:#59635F;font-size:13px;font-weight:550;padding:2px 0;"));
    skillsLayout->addWidget(m_skillsLabel);
    mainLayout->addWidget(skillsCard);

    QFrame *summaryCard = sectionCard(QStringLiteral("个人总结"));
    summaryCard->setFixedHeight(124);
    auto *summaryLayout = static_cast<QVBoxLayout *>(summaryCard->layout());
    auto *summaryTitle =
        summaryCard->findChild<QLabel *>(QStringLiteral("resumeSectionTitle"));
    summaryLayout->removeWidget(summaryTitle);
    auto *summaryHeader = new QHBoxLayout;
    summaryHeader->addWidget(summaryTitle);
    summaryHeader->addStretch();
    auto *editSummaryButton =
        new QPushButton(QStringLiteral("编辑"), summaryCard);
    editSummaryButton->setObjectName(QStringLiteral("editSummaryBtn"));
    editSummaryButton->setProperty("variant", "secondary");
    summaryHeader->addWidget(editSummaryButton);
    summaryLayout->addLayout(summaryHeader);
    m_summaryLabel =
        new QLabel(QStringLiteral(
                       "这只哥布林还没写冒险者自述。点击编辑，讲讲你的专业、"
                       "特长和远征目标。"),
                   summaryCard);
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setStyleSheet(QStringLiteral(
        "color:#59635F;font-size:13px;font-weight:550;padding:2px 0;"));
    summaryLayout->addWidget(m_summaryLabel);
    mainLayout->addWidget(summaryCard);

    auto *actions = new QHBoxLayout;
    actions->addStretch();
    auto *previewButton =
        new QPushButton(QStringLiteral("在浏览器中预览"), this);
    m_exportPdfButton = new QPushButton(QStringLiteral("导出 PDF"), this);
    previewButton->setObjectName(QStringLiteral("previewResumeBtn"));
    m_exportPdfButton->setObjectName(QStringLiteral("exportResumePdfBtn"));
    previewButton->setProperty("variant", "secondary");
    actions->addWidget(previewButton);
    actions->addWidget(m_exportPdfButton);
    mainLayout->addLayout(actions);
    mainLayout->addStretch();

    m_previewOverlay = new QWidget(this);
    m_previewOverlay->setObjectName(QStringLiteral("resumePreviewOverlay"));
    m_previewOverlay->setAttribute(Qt::WA_StyledBackground, true);
    m_previewOverlay->setStyleSheet(QStringLiteral(
        "QWidget#resumePreviewOverlay{background:rgba(18,28,25,232);}"
        "QLabel{background:transparent;color:#FFF9F1;}"
        "QLabel#resumePreviewLargeLbl{background:#FFF;border:1px solid "
        "rgba(255,255,255,0.32);}"));
    auto *overlayLayout = new QVBoxLayout(m_previewOverlay);
    overlayLayout->setContentsMargins(34, 24, 34, 28);
    auto *overlayHeader = new QHBoxLayout;
    m_previewTitleLabel = new QLabel(QStringLiteral("简历模板"));
    auto *overlayHint = new QLabel(QStringLiteral("再按一次空格返回"));
    overlayHeader->addWidget(m_previewTitleLabel);
    overlayHeader->addStretch();
    overlayHeader->addWidget(overlayHint);
    overlayLayout->addLayout(overlayHeader);
    m_previewLargeLabel = new QLabel;
    m_previewLargeLabel->setObjectName(
        QStringLiteral("resumePreviewLargeLbl"));
    m_previewLargeLabel->setAlignment(Qt::AlignCenter);
    overlayLayout->addWidget(m_previewLargeLabel, 1, Qt::AlignCenter);
    m_previewOverlay->hide();

    connect(choosePhotoButton, &QPushButton::clicked, this,
            [this]() { choosePhoto(this); });
    connect(editSkillsButton, &QPushButton::clicked, this,
            &ResumePage::editSkills);
    connect(editSummaryButton, &QPushButton::clicked, this,
            &ResumePage::editSummary);
    connect(m_templateCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]() {
                for (int i = 0; i < m_templateCards.size(); ++i)
                    m_templateCards.at(i)->setChecked(
                        i == m_templateCombo->currentIndex());
                updateTemplateDescription();
                if (m_previewOverlay->isVisible())
                    updateTemplatePreview();
                saveProfile();
            });
    connect(previewButton, &QPushButton::clicked, this, [this]() {
        saveProfile();
        QString error;
        if (!m_exporter->openPreview(User::getInstance().getId(), &error))
            QMessageBox::warning(this, QStringLiteral("预览失败"), error);
    });
    connect(m_exportPdfButton, &QPushButton::clicked, this, [this]() {
        saveProfile();
        QString username = User::getInstance().getUsername().trimmed();
        username.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")),
                         QStringLiteral("_"));
        if (username.isEmpty())
            username = QStringLiteral("Resume");
        QString directory = QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation);
        if (directory.isEmpty())
            directory = QDir::homePath();
        QString filePath = QFileDialog::getSaveFileName(
            this, QStringLiteral("导出简历 PDF"),
            QDir(directory).filePath(username + QStringLiteral("的Resume.pdf")),
            QStringLiteral("PDF 文件 (*.pdf)"));
        if (filePath.isEmpty())
            return;
        if (!filePath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
            filePath += QStringLiteral(".pdf");
        m_exportPdfButton->setEnabled(false);
        m_exportPdfButton->setText(QStringLiteral("正在导出..."));
        m_exporter->exportPdf(User::getInstance().getId(), filePath);
    });
    connect(m_exporter, &ResumeExporter::pdfExportFinished, this,
            [this](bool success, const QString &filePath,
                   const QString &error) {
                m_exportPdfButton->setEnabled(true);
                m_exportPdfButton->setText(QStringLiteral("导出 PDF"));
                if (success)
                    QMessageBox::information(
                        this, QStringLiteral("导出成功"),
                        QStringLiteral("简历已导出到：\n%1").arg(filePath));
                else
                    QMessageBox::critical(this, QStringLiteral("导出失败"),
                                          error);
            });
}

QString ResumePage::photoPath() const {
    return m_photoPath;
}

void ResumePage::choosePhoto(QWidget *dialogParent) {
    if (!dialogParent)
        dialogParent = this;
    const QString filePath = QFileDialog::getOpenFileName(
        dialogParent, QStringLiteral("导入个人照片"), QString(),
        QStringLiteral("图片 (*.jpg *.jpeg *.png);;所有文件 (*)"));
    if (filePath.isEmpty())
        return;
    const QPixmap source(filePath);
    if (source.isNull()) {
        QMessageBox::warning(dialogParent, QStringLiteral("提示"),
                             QStringLiteral("无法读取所选图片"));
        return;
    }

    PhotoCropDialog cropDialog(source, dialogParent);
    if (cropDialog.exec() != QDialog::Accepted)
        return;
    const QPixmap selectedCrop = cropDialog.croppedPixmap();
    if (selectedCrop.isNull()) {
        QMessageBox::warning(dialogParent, QStringLiteral("提示"),
                             QStringLiteral("无法裁剪所选图片"));
        return;
    }

    const QString photosDir = AppDataPaths::photosDirectory();
    if (!QDir().mkpath(photosDir)) {
        QMessageBox::warning(dialogParent, QStringLiteral("提示"),
                             QStringLiteral("无法创建照片目录"));
        return;
    }
    const QString fileName =
        QStringLiteral("user_%1_photo.jpg")
            .arg(User::getInstance().getId());
    const QString destination = QDir(photosDir).filePath(fileName);
    const QPixmap cropped =
        selectedCrop.scaled(800, 800, Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation);
    if (!cropped.save(destination, "JPEG", 92)) {
        QMessageBox::warning(dialogParent, QStringLiteral("提示"),
                             QStringLiteral("照片保存失败"));
        return;
    }
    m_photoPath = AppDataPaths::storedPhotoPath(fileName);
    m_photoPreview->setPixmap(AvatarUtils::circularPixmap(cropped, 96));
    m_photoPreview->setText(QString());
    saveProfile();
    emit photoChanged(m_photoPath);
}

void ResumePage::removePhoto() {
    if (!m_photoPath.isEmpty()) {
        QFile::remove(AppDataPaths::resolveStoredPath(m_photoPath));
    }
    m_photoPath.clear();
    m_photoPreview->setPixmap(
        AvatarUtils::circularAvatar(QString(), 96));
    m_photoPreview->setText(QString());
    saveProfile();
    emit photoChanged(m_photoPath);
}

void ResumePage::saveProfile() {
    const int userId = User::getInstance().getId();
    if (userId <= 0)
        return;
    QVariantMap profile =
        DatabaseManager::getInstance().getResumeProfile(userId);
    profile[QStringLiteral("skills")] = m_skillsText;
    profile[QStringLiteral("summary")] = m_summaryText;
    profile[QStringLiteral("photo_path")] = m_photoPath;
    profile[QStringLiteral("template_id")] =
        m_templateCombo->currentData().toString();
    DatabaseManager::getInstance().updateResumeProfile(userId, profile);
}

void ResumePage::refresh() {
    const int userId = User::getInstance().getId();
    if (userId <= 0)
        return;
    const QVariantMap profile =
        DatabaseManager::getInstance().getResumeProfile(userId);
    m_skillsText = profile.value(QStringLiteral("skills")).toString();
    m_summaryText = profile.value(QStringLiteral("summary")).toString();
    m_photoPath = profile.value(QStringLiteral("photo_path")).toString();
    m_skillsLabel->setText(
        m_skillsText.isEmpty()
            ? QStringLiteral(
                  "技能树还是空的。点击编辑，点亮 Qt 法术、火把维护或史莱姆沟通。")
            : m_skillsText.split('\n', Qt::SkipEmptyParts)
                  .join(QStringLiteral("  ·  ")));
    m_summaryLabel->setText(
        m_summaryText.isEmpty()
            ? QStringLiteral(
                  "这只哥布林还没写冒险者自述。点击编辑，讲讲你的专业、"
                  "特长和远征目标。")
            : m_summaryText);

    const QString templateId = ResumeTemplateRegistry::normalizedId(
        profile.value(QStringLiteral("template_id")).toString());
    int index = m_templateCombo->findData(templateId);
    if (index < 0)
        index = 0;
    {
        const QSignalBlocker blocker(m_templateCombo);
        m_templateCombo->setCurrentIndex(index);
    }
    for (int i = 0; i < m_templateCards.size(); ++i)
        m_templateCards.at(i)->setChecked(i == index);
    updateTemplateDescription();

    m_photoPreview->setPixmap(
        AvatarUtils::circularAvatar(m_photoPath, 96));
    m_photoPreview->setText(QString());
    emit photoChanged(m_photoPath);
}

void ResumePage::editSkills() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("编辑技术能力"));
    dialog.setMinimumSize(480, 360);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 18);
    layout->setSpacing(14);
    auto *hint = new QLabel(QStringLiteral(
        "每行填写一个技能，例如：C++ / Qt / SQLite / 洞穴勘探"));
    hint->setWordWrap(true);
    auto *edit = new QPlainTextEdit;
    edit->setPlainText(m_skillsText);
    edit->setPlaceholderText(
        QStringLiteral("例如：火把维护\n史莱姆沟通\nQt Widgets"));
    edit->setStyleSheet(Theme::inputStyle());
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)
        ->setText(QStringLiteral("取消"));
    layout->addWidget(hint);
    layout->addWidget(edit, 1);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        m_skillsText = edit->toPlainText().trimmed();
        m_skillsLabel->setText(
            m_skillsText.isEmpty()
                ? QStringLiteral(
                      "技能树还是空的。点击编辑，点亮 Qt 法术、火把维护或"
                      "史莱姆沟通。")
                : m_skillsText.split('\n', Qt::SkipEmptyParts)
                      .join(QStringLiteral("  ·  ")));
        saveProfile();
    }
}

void ResumePage::editSummary() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("编辑个人总结"));
    dialog.setMinimumSize(480, 420);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 18);
    layout->setSpacing(14);
    auto *hint = new QLabel(
        QStringLiteral("简要介绍专业背景、核心优势和你的冒险目标"));
    hint->setWordWrap(true);
    auto *edit = new QPlainTextEdit;
    edit->setPlainText(m_summaryText);
    edit->setPlaceholderText(QStringLiteral(
        "例如：来自首都哥布林大学，擅长把混乱洞穴整理成清晰的数据结构……"));
    edit->setStyleSheet(Theme::inputStyle());
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)
        ->setText(QStringLiteral("取消"));
    layout->addWidget(hint);
    layout->addWidget(edit, 1);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        m_summaryText = edit->toPlainText().trimmed();
        m_summaryLabel->setText(
            m_summaryText.isEmpty()
                ? QStringLiteral(
                      "这只哥布林还没写冒险者自述。点击编辑，讲讲你的专业、"
                      "特长和远征目标。")
                : m_summaryText);
        saveProfile();
    }
}

void ResumePage::updateTemplateDescription() {
    const ResumeTemplateDefinition &resumeTemplate =
        ResumeTemplateRegistry::findById(
            m_templateCombo->currentData().toString());
    m_templateDescription->setText(resumeTemplate.description);
}

bool ResumePage::eventFilter(QObject *watched, QEvent *event) {
    Q_UNUSED(watched);
    const bool canPreview =
        isVisible() && QApplication::activeModalWidget() == nullptr &&
        (QApplication::activeWindow() == window() ||
         m_previewOverlay->isVisible());
    if (canPreview && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            if (!keyEvent->isAutoRepeat()) {
                if (m_previewOverlay->isVisible())
                    hideTemplatePreview();
                else
                    showTemplatePreview();
            }
            return true;
        }
    }
    if (canPreview && event->type() == QEvent::KeyRelease) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space)
            return true;
    }
    if (event->type() == QEvent::ApplicationDeactivate ||
        event->type() == QEvent::WindowDeactivate)
        hideTemplatePreview();
    return QWidget::eventFilter(watched, event);
}

void ResumePage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_previewOverlay) {
        m_previewOverlay->setGeometry(rect());
        if (m_previewOverlay->isVisible())
            updateTemplatePreview();
    }
}

void ResumePage::showTemplatePreview() {
    m_previewOverlay->setGeometry(rect());
    updateTemplatePreview();
    m_previewOverlay->show();
    m_previewOverlay->raise();
}

void ResumePage::hideTemplatePreview() {
    if (m_previewOverlay)
        m_previewOverlay->hide();
}

void ResumePage::updateTemplatePreview() {
    const int index = qMax(0, m_templateCombo->currentIndex());
    const ResumeTemplateDefinition &resumeTemplate =
        ResumeTemplateRegistry::findById(
            m_templateCombo->itemData(index).toString());
    const QPixmap source(resumeTemplate.previewResource);
    if (source.isNull())
        return;
    m_previewTitleLabel->setText(
        m_templateCombo->itemText(index) + QStringLiteral(" · 模板预览"));
    const int maxWidth = qMax(260, m_previewOverlay->width() - 180);
    const int maxHeight = qMax(360, m_previewOverlay->height() - 105);
    QSize logicalSize = source.size();
    logicalSize.scale(maxWidth, maxHeight, Qt::KeepAspectRatio);
    const QSize pixelSize(qCeil(logicalSize.width() * kHiResScale),
                          qCeil(logicalSize.height() * kHiResScale));
    QPixmap preview =
        source.scaled(pixelSize, Qt::KeepAspectRatio,
                      Qt::SmoothTransformation);
    preview.setDevicePixelRatio(kHiResScale);
    m_previewLargeLabel->setFixedSize(logicalSize);
    m_previewLargeLabel->setPixmap(preview);
}

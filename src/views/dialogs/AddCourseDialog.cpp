#include "AddCourseDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

AddCourseDialog::AddCourseDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("添加课程");
    setModal(true);
    setFixedSize(500, 510);
    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #F4F1EA;
            font-family: "Avenir Next", "PingFang SC", sans-serif;
        }
        QLabel#eyebrow {
            color: #D97745; font-size: 11px; font-weight: 850;
            letter-spacing: 2px;
        }
        QLabel#title { color: #17201D; font-size: 26px; font-weight: 850; }
        QLabel#subtitle { color: #68716D; font-size: 13px; }
        QLabel#fieldLabel {
            color: #46524E; font-size: 12px; font-weight: 750;
        }
        QLineEdit, QComboBox, QDoubleSpinBox {
            min-height: 42px; background: #FFFEFA; color: #17201D;
            border: 1px solid #D6D0C4; border-radius: 9px;
            padding: 0 12px; font-size: 15px;
            selection-background-color: #BFD6CD;
        }
        QLineEdit:hover, QComboBox:hover, QDoubleSpinBox:hover {
            border-color: #9DABA5;
        }
        QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus {
            border: 2px solid #1F6B5B;
        }
        QComboBox::drop-down { border: none; width: 30px; }
        QLabel#gpaPreview {
            background: #DDEBE6; color: #1F6B5B;
            border-radius: 9px; padding: 0 13px;
            font-size: 14px; font-weight: 800;
        }
        QCheckBox {
            color: #46524E; font-size: 13px; spacing: 9px;
        }
        QPushButton {
            min-height: 42px; min-width: 104px; border-radius: 9px;
            padding: 0 18px; font-size: 14px; font-weight: 750;
        }
    )QSS"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(38, 30, 38, 28);
    layout->setSpacing(0);

    auto *eyebrow = new QLabel("NEW COURSE", this);
    eyebrow->setObjectName("eyebrow");
    layout->addWidget(eyebrow);
    layout->addSpacing(7);

    auto *title = new QLabel("添加一门课程", this);
    title->setObjectName("title");
    layout->addWidget(title);
    layout->addSpacing(5);

    auto *subtitle = new QLabel("成绩会自动换算为绩点，并计入首页统计。", this);
    subtitle->setObjectName("subtitle");
    layout->addWidget(subtitle);
    layout->addSpacing(24);

    auto *form = new QGridLayout;
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(8);

    auto addField = [this, form](int row, int column, const QString &text,
                                 QWidget *field) {
        auto *label = new QLabel(text, this);
        label->setObjectName("fieldLabel");
        form->addWidget(label, row * 2, column);
        form->addWidget(field, row * 2 + 1, column);
    };

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("例如：哥布林高等数学");
    addField(0, 0, "课程名称", nameEdit);
    form->addWidget(nameEdit, 1, 0, 1, 2);

    creditSpin = new QDoubleSpinBox(this);
    creditSpin->setRange(0.5, 10.0);
    creditSpin->setSingleStep(0.5);
    creditSpin->setValue(2.0);
    creditSpin->setSuffix(" 学分");
    scoreSpin = new QDoubleSpinBox(this);
    scoreSpin->setRange(0.0, 100.0);
    scoreSpin->setValue(85.0);
    scoreSpin->setSuffix(" 分");
    addField(1, 0, "学分", creditSpin);
    addField(1, 1, "成绩", scoreSpin);

    semesterCombo = new QComboBox(this);
    semesterCombo->addItems({"大一上", "大一下", "大二上", "大二下",
                             "大三上", "大三下", "大四上", "大四下"});
    gpaPreviewLbl = new QLabel(this);
    gpaPreviewLbl->setObjectName("gpaPreview");
    gpaPreviewLbl->setAlignment(Qt::AlignCenter);
    addField(2, 0, "学期", semesterCombo);
    addField(2, 1, "自动绩点", gpaPreviewLbl);
    layout->addLayout(form);

    layout->addSpacing(18);
    coreCourseCheck = new QCheckBox("在导出的简历中列为核心课程", this);
    coreCourseCheck->setToolTip("勾选后，这门课程会出现在简历的“核心课程”中");
    layout->addWidget(coreCourseCheck);
    layout->addStretch();

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("添加课程");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(
        "QPushButton { background:#1F6B5B; color:#FFF;"
        "border:1px solid #1F6B5B; }"
        "QPushButton:hover { background:#174F44; border-color:#174F44; }");
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(
        "QPushButton { background:transparent; color:#59635F;"
        "border:1px solid #B8C4BE; }"
        "QPushButton:hover { background:#E6ECE8; }");
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    connect(scoreSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { updateGpaPreview(); });

    updateGpaPreview();
    nameEdit->setFocus();
}

double AddCourseDialog::scoreToGpa(double score) const {
    if (score >= 90.0) return 4.0;
    if (score >= 85.0) return 3.7;
    if (score >= 82.0) return 3.3;
    if (score >= 78.0) return 3.0;
    if (score >= 75.0) return 2.7;
    if (score >= 72.0) return 2.3;
    if (score >= 68.0) return 2.0;
    if (score >= 64.0) return 1.5;
    if (score >= 60.0) return 1.0;
    return 0.0;
}

void AddCourseDialog::updateGpaPreview() {
    if (gpaPreviewLbl)
        gpaPreviewLbl->setText(
            QString::number(scoreToGpa(scoreSpin->value()), 'f', 1));
}

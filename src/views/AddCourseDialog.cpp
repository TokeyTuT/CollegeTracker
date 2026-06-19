#include "AddCourseDialog.h"
#include "Theme.h"



AddCourseDialog::AddCourseDialog(QWidget *parent):QDialog(parent){
    using namespace Theme;

    setWindowTitle("添加课程记录");
    setMinimumWidth(420);

    setStyleSheet(QString("QDialog { background: %1; }").arg(Color::surfaceVariant));

    // 使用表单布局，整齐美观
    QFormLayout *layout = new QFormLayout(this);
    layout->setContentsMargins(32, 28, 32, 24);
    layout->setHorizontalSpacing(16);
    layout->setVerticalSpacing(14);
    layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto labelStyle = QString("font-size: %1px; color: %2; font-weight: %3;")
                          .arg(TypeScale::body).arg(Color::onSurfaceVar).arg(FontWeight::bold);

    auto inputStyle = QString(
        "min-height: 36px; border: 1px solid %1; border-radius: %2px;"
        " padding: 0 12px; font-size: %3px; background: #FFFFFF; color: %4;"
    ).arg(Color::outline).arg(Radius::sm).arg(TypeScale::body).arg(Color::onSurface);

    // 课程名称输入框
    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("例如：数据结构");
    nameEdit->setStyleSheet(inputStyle);
    auto *nameLbl = new QLabel("课程名称：", this);
    nameLbl->setStyleSheet(labelStyle);
    layout->addRow(nameLbl, nameEdit);

    // 学分输入框
    creditSpin = new QDoubleSpinBox(this);
    creditSpin->setRange(0.5, 10.0);
    creditSpin->setSingleStep(0.5);
    creditSpin->setValue(2.0);
    creditSpin->setStyleSheet(inputStyle);
    auto *creditLbl = new QLabel("学分：", this);
    creditLbl->setStyleSheet(labelStyle);
    layout->addRow(creditLbl, creditSpin);

    // 成绩输入框
    scoreSpin = new QDoubleSpinBox(this);
    scoreSpin->setRange(0.0, 100.0);
    scoreSpin->setValue(0.0);
    scoreSpin->setStyleSheet(inputStyle);
    auto *scoreLbl = new QLabel("成绩：", this);
    scoreLbl->setStyleSheet(labelStyle);
    layout->addRow(scoreLbl, scoreSpin);

    // 自动换算单门课程绩点
    gpaPreviewLbl = new QLabel("绩点：0.0", this);
    gpaPreviewLbl->setStyleSheet(
        QString("color: %1; font-weight: %2; font-size: %3px;")
            .arg(Color::primary).arg(FontWeight::bold).arg(TypeScale::body));
    auto *gpaLbl = new QLabel("自动绩点：", this);
    gpaLbl->setStyleSheet(labelStyle);
    layout->addRow(gpaLbl, gpaPreviewLbl);

    // 课程时间
    semesterCombo = new QComboBox(this);
    semesterCombo->addItems({"大一上", "大一下", "大二上", "大二下", "大三上", "大三下", "大四上", "大四下"});
    semesterCombo->setStyleSheet(inputStyle);
    auto *semLbl = new QLabel("学期：", this);
    semLbl->setStyleSheet(labelStyle);
    layout->addRow(semLbl, semesterCombo);

    // 核心课程会被展示在导出的简历中
    coreCourseCheck = new QCheckBox("作为简历中的主要课程", this);
    coreCourseCheck->setChecked(false);
    coreCourseCheck->setToolTip("勾选后，这门课程会出现在简历的“核心课程”中");
    coreCourseCheck->setStyleSheet(
        QString("font-size: %1px; color: %2;")
            .arg(TypeScale::body).arg(Color::onSurface));
    auto *coreCourseLbl = new QLabel("核心课程：", this);
    coreCourseLbl->setStyleSheet(labelStyle);
    layout->addRow(coreCourseLbl, coreCourseCheck);

    // 确认/取消按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->setStyleSheet(
        QString("QPushButton { min-height: 36px; border-radius: %1px;"
                "  font-size: %2px; padding: 0 24px; }").arg(Radius::sm).arg(TypeScale::body));
    layout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(scoreSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double){ updateGpaPreview(); });
    updateGpaPreview();

    // 默认焦点在名称输入框
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
    if (!gpaPreviewLbl) return;
    gpaPreviewLbl->setText(QString("绩点：%1").arg(scoreToGpa(scoreSpin->value()), 0, 'f', 1));
}

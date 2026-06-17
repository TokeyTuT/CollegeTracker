#include"AddCourseDialog.h"



AddCourseDialog::AddCourseDialog(QWidget *parent):QDialog(parent){
    setWindowTitle("添加课程记录");

    // 使用表单布局，整齐美观
    QFormLayout *layout = new QFormLayout(this);

    // 课程名称输入框
    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("例如：数据结构");
    layout->addRow("课程名称:", nameEdit);

    // 学分输入框
    creditSpin = new QDoubleSpinBox(this);
    creditSpin->setRange(0.5, 10.0);
    creditSpin->setSingleStep(0.5);
    creditSpin->setValue(2.0);
    layout->addRow("学分:", creditSpin);

    // 成绩输入框
    scoreSpin = new QDoubleSpinBox(this);
    scoreSpin->setRange(0.0, 100.0);
    scoreSpin->setValue(0.0);
    layout->addRow("成绩:", scoreSpin);

    // 自动换算单门课程绩点
    gpaPreviewLbl = new QLabel("绩点：0.0", this);
    gpaPreviewLbl->setStyleSheet("color:#C8943E; font-weight:700;");
    layout->addRow("自动绩点:", gpaPreviewLbl);

    // 课程时间
    semesterCombo = new QComboBox(this);
    semesterCombo->addItems({"大一上", "大一下", "大二上", "大二下", "大三上", "大三下", "大四上", "大四下"});
    layout->addRow("学期:", semesterCombo);

    //  确认/取消按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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

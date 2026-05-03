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

    // 默认焦点在名称输入框
    nameEdit->setFocus();
}

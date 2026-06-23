#ifndef ADDCOURSEDIALOG_H
#define ADDCOURSEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>

class AddCourseDialog : public QDialog {
    Q_OBJECT
public:
    AddCourseDialog(QWidget *parent = nullptr);
    // 外部获取数据的接口
    QString getName() const { return nameEdit->text(); }
    double getCredit() const { return creditSpin->value(); }
    double getScore() const { return scoreSpin->value(); }
    QString getSemester() const{return semesterCombo->currentText();}
    int getSemesterOrder() const { return semesterCombo->currentIndex(); }
    bool isCoreCourse() const { return coreCourseCheck->isChecked(); }
private:
    double scoreToGpa(double score) const;
    void updateGpaPreview();
    QLineEdit *nameEdit;
    QDoubleSpinBox *creditSpin;
    QDoubleSpinBox *scoreSpin;
    QComboBox *semesterCombo;
    QCheckBox *coreCourseCheck;
    QLabel *gpaPreviewLbl;
};

#endif

#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

class RegisterDialog : public QDialog {
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);

private slots:
    void onRegisterClicked();
    void onBackClicked();

private:
    void showMessage(const QString &msg, bool isError);

    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_confirmEdit;
    QComboBox *m_gradeCombo;
    QComboBox *m_genderCombo;
    QLineEdit *m_majorEdit;
    QPushButton *m_registerBtn;
    QPushButton *m_backBtn;
    QLabel *m_messageLabel;
};

#endif // REGISTERDIALOG_H

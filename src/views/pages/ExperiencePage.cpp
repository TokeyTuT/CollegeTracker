#include "ExperiencePage.h"

#include "CsvUtils.h"
#include "User.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QTableView>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

void setupTableView(QTableView *tableView) {
    tableView->setAlternatingRowColors(true);
    tableView->setShowGrid(false);
    tableView->verticalHeader()->setVisible(false);
    tableView->verticalHeader()->setDefaultSectionSize(42);
    tableView->horizontalHeader()->setMinimumHeight(44);
    tableView->horizontalHeader()->setStretchLastSection(true);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::DoubleClicked);
}

QLabel *formLabel(const QString &text, QWidget *parent) {
    return new QLabel(text, parent);
}

} // namespace

ExperiencePage::ExperiencePage(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("expPage"));
    buildUi();
    setupModels();
    setExperienceTabActive(true);
}

void ExperiencePage::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(14);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    m_experienceTabButton =
        new QPushButton(QStringLiteral("课外活动"), this);
    m_awardTabButton = new QPushButton(QStringLiteral("个人荣誉"), this);
    m_importButton = new QPushButton(QStringLiteral("导入 CSV"), this);
    m_exportButton = new QPushButton(QStringLiteral("导出 CSV"), this);
    m_helpButton = new QPushButton(QStringLiteral("?"), this);

    m_experienceTabButton->setObjectName(QStringLiteral("expTabBtn"));
    m_awardTabButton->setObjectName(QStringLiteral("awardTabBtn"));
    m_importButton->setObjectName(QStringLiteral("importExpCsvBtn"));
    m_exportButton->setObjectName(QStringLiteral("exportExpCsvBtn"));
    m_helpButton->setObjectName(QStringLiteral("csvHelpExpBtn"));
    m_importButton->setProperty("variant", "secondary");
    m_exportButton->setProperty("variant", "secondary");
    m_helpButton->setProperty("variant", "tool");
    m_helpButton->setFixedSize(40, 40);
    m_helpButton->setToolTip(QStringLiteral("查看 CSV 格式说明"));

    toolbar->addWidget(m_experienceTabButton);
    toolbar->addWidget(m_awardTabButton);
    toolbar->addStretch();
    toolbar->addWidget(m_importButton);
    toolbar->addWidget(m_exportButton);
    toolbar->addWidget(m_helpButton);
    mainLayout->addLayout(toolbar);

    m_experienceTable = new QTableView(this);
    m_experienceTable->setObjectName(QStringLiteral("expTableView"));
    m_awardTable = new QTableView(this);
    m_awardTable->setObjectName(QStringLiteral("awardTableView"));
    setupTableView(m_experienceTable);
    setupTableView(m_awardTable);
    m_experienceTable->setMinimumHeight(250);
    m_awardTable->setMinimumHeight(250);
    mainLayout->addWidget(m_experienceTable, 1);
    mainLayout->addWidget(m_awardTable, 1);

    m_experienceForm = new QFrame(this);
    m_experienceForm->setObjectName(QStringLiteral("addExpFrame"));
    m_experienceForm->setFixedHeight(224);
    auto *experienceLayout = new QVBoxLayout(m_experienceForm);
    experienceLayout->setContentsMargins(20, 18, 20, 18);
    experienceLayout->setSpacing(12);
    auto *experienceFormTitle =
        new QLabel(QStringLiteral("新增经历"), m_experienceForm);
    experienceFormTitle->setObjectName(QStringLiteral("addExpTitle"));
    experienceLayout->addWidget(experienceFormTitle);

    auto *experienceGrid = new QGridLayout;
    experienceGrid->setHorizontalSpacing(12);
    experienceGrid->setVerticalSpacing(10);
    m_experienceType = new QComboBox(m_experienceForm);
    m_experienceType->addItems(
        {QStringLiteral("实习"), QStringLiteral("竞赛"),
         QStringLiteral("项目"), QStringLiteral("其他")});
    m_experienceTitle = new QLineEdit(m_experienceForm);
    m_experienceTitle->setPlaceholderText(
        QStringLiteral("示例：哥布林学生会财务总管"));
    m_experienceDate = new QDateEdit(m_experienceForm);
    m_experienceDate->setCalendarPopup(true);
    m_experienceDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_experienceDate->setDate(QDate::currentDate());
    m_experienceDescription = new QLineEdit(m_experienceForm);
    m_experienceDescription->setPlaceholderText(
        QStringLiteral("示例：管理三箱金币与五支火把"));

    experienceGrid->addWidget(formLabel(QStringLiteral("类型"),
                                        m_experienceForm),
                              0, 0);
    experienceGrid->addWidget(m_experienceType, 0, 1);
    experienceGrid->addWidget(formLabel(QStringLiteral("标题"),
                                        m_experienceForm),
                              0, 2);
    experienceGrid->addWidget(m_experienceTitle, 0, 3);
    experienceGrid->addWidget(formLabel(QStringLiteral("日期"),
                                        m_experienceForm),
                              1, 0);
    experienceGrid->addWidget(m_experienceDate, 1, 1);
    experienceGrid->addWidget(formLabel(QStringLiteral("描述"),
                                        m_experienceForm),
                              1, 2);
    experienceGrid->addWidget(m_experienceDescription, 1, 3);
    experienceLayout->addLayout(experienceGrid);

    auto *experienceActions = new QHBoxLayout;
    auto *deleteExperienceButton =
        new QPushButton(QStringLiteral("删除所选"), m_experienceForm);
    auto *addExperienceButton =
        new QPushButton(QStringLiteral("＋ 添加经历"), m_experienceForm);
    deleteExperienceButton->setObjectName(QStringLiteral("DelExpBtn"));
    addExperienceButton->setObjectName(QStringLiteral("addExpBtn"));
    deleteExperienceButton->setProperty("variant", "danger");
    addExperienceButton->setProperty("variant", "primary");
    experienceActions->addStretch();
    experienceActions->addWidget(deleteExperienceButton);
    experienceActions->addWidget(addExperienceButton);
    experienceLayout->addLayout(experienceActions);
    mainLayout->addWidget(m_experienceForm);

    m_awardForm = new QFrame(this);
    m_awardForm->setObjectName(QStringLiteral("addAwardFrame"));
    m_awardForm->setFixedHeight(224);
    auto *awardLayout = new QVBoxLayout(m_awardForm);
    awardLayout->setContentsMargins(20, 18, 20, 18);
    awardLayout->setSpacing(12);
    auto *awardFormTitle =
        new QLabel(QStringLiteral("新增荣誉"), m_awardForm);
    awardFormTitle->setObjectName(QStringLiteral("addAwardTitle"));
    awardLayout->addWidget(awardFormTitle);

    auto *awardGrid = new QGridLayout;
    awardGrid->setHorizontalSpacing(12);
    awardGrid->setVerticalSpacing(10);
    m_awardName = new QLineEdit(m_awardForm);
    m_awardName->setPlaceholderText(
        QStringLiteral("示例：年度最具潜力哥布林奖"));
    m_awardDate = new QDateEdit(m_awardForm);
    m_awardDate->setCalendarPopup(true);
    m_awardDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_awardDate->setDate(QDate::currentDate());
    m_awardLevel = new QComboBox(m_awardForm);
    m_awardLevel->addItems(
        {QStringLiteral("国家级"), QStringLiteral("省级"),
         QStringLiteral("校级"), QStringLiteral("院级")});
    m_awardAmount = new QDoubleSpinBox(m_awardForm);
    m_awardAmount->setPrefix(QStringLiteral("¥ "));
    m_awardAmount->setRange(0.0, 999999.0);
    m_awardAmount->setDecimals(0);
    m_awardAmount->setSingleStep(500.0);
    m_awardAmount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    awardGrid->addWidget(formLabel(QStringLiteral("奖项名称"), m_awardForm),
                         0, 0);
    awardGrid->addWidget(m_awardName, 0, 1);
    awardGrid->addWidget(formLabel(QStringLiteral("获奖日期"), m_awardForm),
                         0, 2);
    awardGrid->addWidget(m_awardDate, 0, 3);
    awardGrid->addWidget(formLabel(QStringLiteral("荣誉级别"), m_awardForm),
                         1, 0);
    awardGrid->addWidget(m_awardLevel, 1, 1);
    awardGrid->addWidget(formLabel(QStringLiteral("奖金金额"), m_awardForm),
                         1, 2);
    awardGrid->addWidget(m_awardAmount, 1, 3);
    awardLayout->addLayout(awardGrid);

    auto *awardActions = new QHBoxLayout;
    auto *deleteAwardButton =
        new QPushButton(QStringLiteral("删除所选"), m_awardForm);
    auto *addAwardButton =
        new QPushButton(QStringLiteral("＋ 添加荣誉"), m_awardForm);
    deleteAwardButton->setObjectName(QStringLiteral("delAwardBtn"));
    addAwardButton->setObjectName(QStringLiteral("addAwardBtn"));
    deleteAwardButton->setProperty("variant", "danger");
    addAwardButton->setProperty("variant", "primary");
    awardActions->addStretch();
    awardActions->addWidget(deleteAwardButton);
    awardActions->addWidget(addAwardButton);
    awardLayout->addLayout(awardActions);
    mainLayout->addWidget(m_awardForm);

    connect(m_experienceTabButton, &QPushButton::clicked, this,
            [this]() { setExperienceTabActive(true); });
    connect(m_awardTabButton, &QPushButton::clicked, this,
            [this]() { setExperienceTabActive(false); });
    connect(addExperienceButton, &QPushButton::clicked, this,
            &ExperiencePage::addExperience);
    connect(deleteExperienceButton, &QPushButton::clicked, this,
            &ExperiencePage::deleteExperience);
    connect(addAwardButton, &QPushButton::clicked, this,
            &ExperiencePage::addAward);
    connect(deleteAwardButton, &QPushButton::clicked, this,
            &ExperiencePage::deleteAward);
    connect(m_helpButton, &QPushButton::clicked, this,
            &ExperiencePage::showCsvHelp);
    connect(m_importButton, &QPushButton::clicked, this, [this]() {
        const QString title =
            m_experienceTabActive ? QStringLiteral("导入经历 CSV")
                                  : QStringLiteral("导入荣誉 CSV");
        const QString filePath = QFileDialog::getOpenFileName(
            this, title, QString(),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));
        if (filePath.isEmpty())
            return;
        if (m_experienceTabActive)
            importExperiencesFromCsv(filePath);
        else
            importAwardsFromCsv(filePath);
    });
    connect(m_exportButton, &QPushButton::clicked, this, [this]() {
        const QString prefix =
            m_experienceTabActive ? QStringLiteral("experiences")
                                  : QStringLiteral("awards");
        const QString title =
            m_experienceTabActive ? QStringLiteral("导出经历 CSV")
                                  : QStringLiteral("导出荣誉 CSV");
        const QString filePath = QFileDialog::getSaveFileName(
            this, title,
            QStringLiteral("%1_%2.csv")
                .arg(prefix,
                     QDate::currentDate().toString(
                         QStringLiteral("yyyyMMdd"))),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));
        if (filePath.isEmpty())
            return;
        if (m_experienceTabActive)
            exportExperiencesToCsv(filePath);
        else
            exportAwardsToCsv(filePath);
    });
}

void ExperiencePage::setupModels() {
    const int userId = User::getInstance().getId();

    m_experienceModel = new QSqlTableModel(this);
    m_experienceModel->setTable(QStringLiteral("experiences"));
    m_experienceModel->setEditStrategy(QSqlTableModel::OnRowChange);
    m_experienceModel->setFilter(
        QStringLiteral("user_id = %1").arg(userId));
    m_experienceModel->setHeaderData(2, Qt::Horizontal,
                                     QStringLiteral("标题"));
    m_experienceModel->setHeaderData(3, Qt::Horizontal,
                                     QStringLiteral("类型"));
    m_experienceModel->setHeaderData(4, Qt::Horizontal,
                                     QStringLiteral("时间"));
    m_experienceModel->setHeaderData(5, Qt::Horizontal,
                                     QStringLiteral("描述"));
    m_experienceModel->setSort(4, Qt::AscendingOrder);
    m_experienceTable->setModel(m_experienceModel);
    m_experienceTable->setColumnHidden(0, true);
    m_experienceTable->setColumnHidden(1, true);
    for (int column = 6; column < m_experienceModel->columnCount(); ++column)
        m_experienceTable->setColumnHidden(column, true);
    m_experienceModel->select();

    m_awardModel = new QSqlTableModel(this);
    m_awardModel->setTable(QStringLiteral("awards"));
    m_awardModel->setEditStrategy(QSqlTableModel::OnRowChange);
    m_awardModel->setFilter(QStringLiteral("user_id = %1").arg(userId));
    m_awardModel->setHeaderData(2, Qt::Horizontal,
                                QStringLiteral("奖项名称"));
    m_awardModel->setHeaderData(3, Qt::Horizontal,
                                QStringLiteral("荣誉级别"));
    m_awardModel->setHeaderData(4, Qt::Horizontal,
                                QStringLiteral("获奖时间"));
    m_awardModel->setHeaderData(5, Qt::Horizontal,
                                QStringLiteral("奖金金额"));
    m_awardModel->setSort(4, Qt::AscendingOrder);
    m_awardTable->setModel(m_awardModel);
    m_awardTable->setColumnHidden(0, true);
    m_awardTable->setColumnHidden(1, true);
    for (int column = 6; column < m_awardModel->columnCount(); ++column)
        m_awardTable->setColumnHidden(column, true);
    m_awardModel->select();

    connect(m_experienceModel, &QSqlTableModel::dataChanged, this,
            &ExperiencePage::dataChanged);
    connect(m_awardModel, &QSqlTableModel::dataChanged, this,
            &ExperiencePage::dataChanged);
}

void ExperiencePage::setExperienceTabActive(bool experienceActive) {
    m_experienceTabActive = experienceActive;
    m_experienceTable->setVisible(experienceActive);
    m_experienceForm->setVisible(experienceActive);
    m_awardTable->setVisible(!experienceActive);
    m_awardForm->setVisible(!experienceActive);

    for (QPushButton *button :
         {m_experienceTabButton, m_awardTabButton}) {
        button->setProperty(
            "active",
            button == (experienceActive ? m_experienceTabButton
                                        : m_awardTabButton));
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
}

void ExperiencePage::refresh() {
    if (m_experienceModel)
        m_experienceModel->select();
    if (m_awardModel)
        m_awardModel->select();
}

void ExperiencePage::addExperience() {
    if (m_experienceDate->date() > QDate::currentDate()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("不能选择未来日期！"));
        return;
    }
    if (m_experienceTitle->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("标题不能为空！"));
        return;
    }

    const int row = m_experienceModel->rowCount();
    if (!m_experienceModel->insertRow(row))
        return;
    m_experienceModel->setData(m_experienceModel->index(row, 1),
                               User::getInstance().getId());
    m_experienceModel->setData(m_experienceModel->index(row, 2),
                               m_experienceTitle->text().trimmed());
    m_experienceModel->setData(m_experienceModel->index(row, 3),
                               m_experienceType->currentText());
    m_experienceModel->setData(
        m_experienceModel->index(row, 4),
        m_experienceDate->date().toString(QStringLiteral("yyyy-MM-dd")));
    m_experienceModel->setData(
        m_experienceModel->index(row, 5),
        m_experienceDescription->text().trimmed());
    if (!m_experienceModel->submitAll()) {
        m_experienceModel->revertAll();
        QMessageBox::critical(this, QStringLiteral("添加失败"),
                              m_experienceModel->lastError().text());
        return;
    }

    m_experienceTitle->clear();
    m_experienceDescription->clear();
    m_experienceDate->setDate(QDate::currentDate());
    m_experienceType->setCurrentIndex(0);
    m_experienceTable->selectRow(row);
    emit dataChanged();
}

void ExperiencePage::deleteExperience() {
    const QModelIndex currentIndex = m_experienceTable->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请先选择一行内容"));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认删除"),
                              QStringLiteral("确定要删除该经历吗？")) !=
        QMessageBox::Yes)
        return;

    m_experienceModel->removeRow(currentIndex.row());
    if (!m_experienceModel->submitAll()) {
        m_experienceModel->revertAll();
        QMessageBox::critical(this, QStringLiteral("删除失败"),
                              m_experienceModel->lastError().text());
        return;
    }
    refresh();
    emit dataChanged();
}

void ExperiencePage::addAward() {
    if (m_awardDate->date() > QDate::currentDate()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("不能选择未来日期！"));
        return;
    }
    if (m_awardName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("奖项名称不能为空！"));
        return;
    }

    const int row = m_awardModel->rowCount();
    if (!m_awardModel->insertRow(row))
        return;
    m_awardModel->setData(m_awardModel->index(row, 1),
                          User::getInstance().getId());
    m_awardModel->setData(m_awardModel->index(row, 2),
                          m_awardName->text().trimmed());
    m_awardModel->setData(m_awardModel->index(row, 3),
                          m_awardLevel->currentText());
    m_awardModel->setData(
        m_awardModel->index(row, 4),
        m_awardDate->date().toString(QStringLiteral("yyyy-MM-dd")));
    m_awardModel->setData(m_awardModel->index(row, 5),
                          m_awardAmount->value());
    if (!m_awardModel->submitAll()) {
        m_awardModel->revertAll();
        QMessageBox::critical(this, QStringLiteral("添加失败"),
                              m_awardModel->lastError().text());
        return;
    }

    m_awardName->clear();
    m_awardDate->setDate(QDate::currentDate());
    m_awardAmount->setValue(0.0);
    m_awardTable->selectRow(row);
    emit dataChanged();
}

void ExperiencePage::deleteAward() {
    const QModelIndex currentIndex = m_awardTable->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请先选择一行内容"));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认删除"),
                              QStringLiteral("确定要删除该荣誉吗？")) !=
        QMessageBox::Yes)
        return;

    m_awardModel->removeRow(currentIndex.row());
    if (!m_awardModel->submitAll()) {
        m_awardModel->revertAll();
        QMessageBox::critical(this, QStringLiteral("删除失败"),
                              m_awardModel->lastError().text());
        return;
    }
    refresh();
    emit dataChanged();
}

void ExperiencePage::importExperiencesFromCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导入失败"),
                              file.errorString());
        return;
    }
    QTextStream stream(&file);
    if (stream.atEnd()) {
        QMessageBox::warning(this, QStringLiteral("导入失败"),
                             QStringLiteral("CSV 文件为空"));
        return;
    }
    QString headerLine = stream.readLine();
    if (headerLine.startsWith(QChar(0xFEFF)))
        headerLine = headerLine.mid(1);
    const QStringList header = CsvUtils::parseCsvLine(headerLine);
    if (header.size() < 4 || header.at(0) != QStringLiteral("标题") ||
        header.at(1) != QStringLiteral("类型") ||
        header.at(2) != QStringLiteral("时间") ||
        header.at(3) != QStringLiteral("描述")) {
        QMessageBox::warning(this, QStringLiteral("格式错误"),
                             QStringLiteral("CSV 表头不匹配。"));
        return;
    }

    const QStringList validTypes = {
        QStringLiteral("实习"), QStringLiteral("竞赛"),
        QStringLiteral("项目"), QStringLiteral("其他")};
    int successCount = 0;
    int failCount = 0;
    QSqlQuery query;
    query.prepare(
        QStringLiteral("INSERT INTO experiences "
                       "(user_id, title, type, date, content) VALUES "
                       "(:uid, :title, :type, :date, :content)"));
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;
        const QStringList fields = CsvUtils::parseCsvLine(line);
        if (fields.size() < 4 || fields.at(0).isEmpty() ||
            !validTypes.contains(fields.at(1))) {
            ++failCount;
            continue;
        }
        query.bindValue(QStringLiteral(":uid"),
                        User::getInstance().getId());
        query.bindValue(QStringLiteral(":title"), fields.at(0));
        query.bindValue(QStringLiteral(":type"), fields.at(1));
        query.bindValue(QStringLiteral(":date"), fields.at(2));
        query.bindValue(QStringLiteral(":content"), fields.at(3));
        query.exec() ? ++successCount : ++failCount;
    }
    refresh();
    emit dataChanged();
    QMessageBox::information(
        this, QStringLiteral("导入完成"),
        QStringLiteral("成功导入 %1 条，失败 %2 条")
            .arg(successCount)
            .arg(failCount));
}

void ExperiencePage::exportExperiencesToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              file.errorString());
        return;
    }
    QTextStream stream(&file);
    stream << QChar(0xFEFF);
    CsvUtils::writeCsvRow(
        stream, {QStringLiteral("标题"), QStringLiteral("类型"),
                 QStringLiteral("时间"), QStringLiteral("描述")});
    const int rowCount = m_experienceModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        CsvUtils::writeCsvRow(
            stream,
            {m_experienceModel->data(m_experienceModel->index(row, 2))
                 .toString(),
             m_experienceModel->data(m_experienceModel->index(row, 3))
                 .toString(),
             m_experienceModel->data(m_experienceModel->index(row, 4))
                 .toString(),
             m_experienceModel->data(m_experienceModel->index(row, 5))
                 .toString()});
    }
    QMessageBox::information(
        this, QStringLiteral("导出成功"),
        QStringLiteral("已导出 %1 条经历数据").arg(rowCount));
}

void ExperiencePage::importAwardsFromCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导入失败"),
                              file.errorString());
        return;
    }
    QTextStream stream(&file);
    if (stream.atEnd()) {
        QMessageBox::warning(this, QStringLiteral("导入失败"),
                             QStringLiteral("CSV 文件为空"));
        return;
    }
    QString headerLine = stream.readLine();
    if (headerLine.startsWith(QChar(0xFEFF)))
        headerLine = headerLine.mid(1);
    const QStringList header = CsvUtils::parseCsvLine(headerLine);
    if (header.size() < 4 ||
        header.at(0) != QStringLiteral("奖项名称") ||
        header.at(1) != QStringLiteral("荣誉级别") ||
        header.at(2) != QStringLiteral("获奖时间") ||
        header.at(3) != QStringLiteral("奖金金额")) {
        QMessageBox::warning(this, QStringLiteral("格式错误"),
                             QStringLiteral("CSV 表头不匹配。"));
        return;
    }

    const QStringList validLevels = {
        QStringLiteral("国家级"), QStringLiteral("省级"),
        QStringLiteral("校级"), QStringLiteral("院级")};
    int successCount = 0;
    int failCount = 0;
    QSqlQuery query;
    query.prepare(
        QStringLiteral("INSERT INTO awards "
                       "(user_id, name, level, date, amount) VALUES "
                       "(:uid, :name, :level, :date, :amount)"));
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;
        const QStringList fields = CsvUtils::parseCsvLine(line);
        if (fields.size() < 4 || fields.at(0).isEmpty() ||
            !validLevels.contains(fields.at(1))) {
            ++failCount;
            continue;
        }
        bool amountOk = false;
        double amount = fields.at(3).toDouble(&amountOk);
        if (!amountOk)
            amount = 0.0;
        query.bindValue(QStringLiteral(":uid"),
                        User::getInstance().getId());
        query.bindValue(QStringLiteral(":name"), fields.at(0));
        query.bindValue(QStringLiteral(":level"), fields.at(1));
        query.bindValue(QStringLiteral(":date"), fields.at(2));
        query.bindValue(QStringLiteral(":amount"), amount);
        query.exec() ? ++successCount : ++failCount;
    }
    refresh();
    emit dataChanged();
    QMessageBox::information(
        this, QStringLiteral("导入完成"),
        QStringLiteral("成功导入 %1 条，失败 %2 条")
            .arg(successCount)
            .arg(failCount));
}

void ExperiencePage::exportAwardsToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              file.errorString());
        return;
    }
    QTextStream stream(&file);
    stream << QChar(0xFEFF);
    CsvUtils::writeCsvRow(
        stream, {QStringLiteral("奖项名称"), QStringLiteral("荣誉级别"),
                 QStringLiteral("获奖时间"), QStringLiteral("奖金金额")});
    const int rowCount = m_awardModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        CsvUtils::writeCsvRow(
            stream,
            {m_awardModel->data(m_awardModel->index(row, 2)).toString(),
             m_awardModel->data(m_awardModel->index(row, 3)).toString(),
             m_awardModel->data(m_awardModel->index(row, 4)).toString(),
             m_awardModel->data(m_awardModel->index(row, 5)).toString()});
    }
    QMessageBox::information(
        this, QStringLiteral("导出成功"),
        QStringLiteral("已导出 %1 条荣誉数据").arg(rowCount));
}

void ExperiencePage::showCsvHelp() {
    if (m_experienceTabActive) {
        QMessageBox::information(
            this, QStringLiteral("CSV 格式说明 — 经历"),
            QStringLiteral("<pre>标题,类型,时间,描述</pre>"
                           "<p>类型支持：实习、竞赛、项目、其他。</p>"));
    } else {
        QMessageBox::information(
            this, QStringLiteral("CSV 格式说明 — 荣誉"),
            QStringLiteral(
                "<pre>奖项名称,荣誉级别,获奖时间,奖金金额</pre>"
                "<p>级别支持：国家级、省级、校级、院级。</p>"));
    }
}

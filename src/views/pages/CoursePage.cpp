#include "CoursePage.h"

#include "AddCourseDialog.h"
#include "CsvUtils.h"
#include "DatabaseMannager.h"
#include "User.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

class CoreCourseDelegate final : public QStyledItemDelegate {
public:
    explicit CoreCourseDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QString displayText(const QVariant &value,
                        const QLocale &locale) const override {
        Q_UNUSED(locale);
        return value.toBool() ? QStringLiteral("是") : QStringLiteral("否");
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &) const override {
        auto *editor = new QComboBox(parent);
        editor->addItems({QStringLiteral("否"), QStringLiteral("是")});
        return editor;
    }

    void setEditorData(QWidget *editor,
                       const QModelIndex &index) const override {
        auto *combo = qobject_cast<QComboBox *>(editor);
        if (combo)
            combo->setCurrentIndex(index.data(Qt::EditRole).toBool() ? 1 : 0);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override {
        auto *combo = qobject_cast<QComboBox *>(editor);
        if (combo)
            model->setData(index, combo->currentIndex() == 1 ? 1 : 0,
                           Qt::EditRole);
    }
};

void setupTableView(QTableView *tableView) {
    tableView->setAlternatingRowColors(true);
    tableView->setShowGrid(false);
    tableView->verticalHeader()->setVisible(false);
    tableView->verticalHeader()->setDefaultSectionSize(42);
    tableView->horizontalHeader()->setMinimumHeight(44);
    tableView->horizontalHeader()->setStretchLastSection(true);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setEditTriggers(QAbstractItemView::DoubleClicked);
}

} // namespace

CoursePage::CoursePage(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("coursePage"));
    buildUi();
    setupModel();
    updateStats();
}

void CoursePage::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(14);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    m_importButton = new QPushButton(QStringLiteral("导入 CSV"), this);
    m_exportButton = new QPushButton(QStringLiteral("导出 CSV"), this);
    m_helpButton = new QPushButton(QStringLiteral("?"), this);
    m_resetButton = new QPushButton(QStringLiteral("清空课程"), this);
    m_deleteButton = new QPushButton(QStringLiteral("删除所选"), this);
    m_addButton = new QPushButton(QStringLiteral("＋ 添加课程"), this);

    m_importButton->setObjectName(QStringLiteral("importCourseCsvBtn"));
    m_exportButton->setObjectName(QStringLiteral("exportCourseCsvBtn"));
    m_helpButton->setObjectName(QStringLiteral("csvHelpCourseBtn"));
    m_resetButton->setObjectName(QStringLiteral("resetCourseBtn"));
    m_deleteButton->setObjectName(QStringLiteral("deleteCourseBtn"));
    m_addButton->setObjectName(QStringLiteral("addCourseBtn"));

    m_importButton->setProperty("variant", "secondary");
    m_exportButton->setProperty("variant", "secondary");
    m_helpButton->setProperty("variant", "tool");
    m_helpButton->setFixedSize(40, 40);
    m_helpButton->setToolTip(QStringLiteral("查看课程 CSV 格式说明"));
    m_resetButton->setProperty("variant", "quietDanger");
    m_resetButton->setToolTip(QStringLiteral("清空当前用户的全部课程记录"));
    m_deleteButton->setProperty("variant", "danger");
    m_addButton->setProperty("variant", "primary");

    toolbar->addWidget(m_importButton);
    toolbar->addWidget(m_exportButton);
    toolbar->addWidget(m_helpButton);
    toolbar->addWidget(m_resetButton);
    toolbar->addStretch();
    toolbar->addWidget(m_deleteButton);
    toolbar->addWidget(m_addButton);
    mainLayout->addLayout(toolbar);

    m_tableView = new QTableView(this);
    m_tableView->setObjectName(QStringLiteral("courseTableView"));
    m_tableView->setToolTip(
        QStringLiteral("按住 Cmd（macOS）或 Ctrl（Windows/Linux）可选择多行课程"));
    setupTableView(m_tableView);
    mainLayout->addWidget(m_tableView, 1);

    m_statsLabel = new QLabel(QStringLiteral("暂无课程数据"), this);
    m_statsLabel->setObjectName(QStringLiteral("labelStats"));
    m_statsLabel->setMinimumHeight(54);
    m_statsLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statsLabel);

    connect(m_addButton, &QPushButton::clicked, this, &CoursePage::addCourse);
    connect(m_deleteButton, &QPushButton::clicked, this,
            &CoursePage::deleteSelectedCourses);
    connect(m_resetButton, &QPushButton::clicked, this,
            &CoursePage::resetAllCourses);
    connect(m_helpButton, &QPushButton::clicked, this,
            &CoursePage::showCsvHelp);
    connect(m_importButton, &QPushButton::clicked, this, [this]() {
        const QString filePath = QFileDialog::getOpenFileName(
            this, QStringLiteral("导入课程 CSV"), QString(),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));
        if (!filePath.isEmpty())
            importFromCsv(filePath);
    });
    connect(m_exportButton, &QPushButton::clicked, this, [this]() {
        const QString filePath = QFileDialog::getSaveFileName(
            this, QStringLiteral("导出课程 CSV"),
            QStringLiteral("courses_%1.csv")
                .arg(QDate::currentDate().toString(QStringLiteral("yyyyMMdd"))),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));
        if (!filePath.isEmpty())
            exportToCsv(filePath);
    });
}

void CoursePage::setupModel() {
    const int userId = User::getInstance().getId();
    m_model = new QSqlTableModel(this);
    m_model->setTable(QStringLiteral("courses"));
    m_model->setEditStrategy(QSqlTableModel::OnRowChange);
    m_model->setFilter(QStringLiteral("user_id = %1").arg(userId));
    m_model->setHeaderData(2, Qt::Horizontal, QStringLiteral("课程名称"));
    m_model->setHeaderData(3, Qt::Horizontal, QStringLiteral("学分"));
    m_model->setHeaderData(4, Qt::Horizontal, QStringLiteral("成绩"));
    m_model->setHeaderData(5, Qt::Horizontal, QStringLiteral("学期"));
    m_model->setHeaderData(6, Qt::Horizontal, QStringLiteral("绩点"));
    m_model->setHeaderData(8, Qt::Horizontal, QStringLiteral("核心课程"));
    m_model->setSort(7, Qt::AscendingOrder);

    m_tableView->setModel(m_model);
    m_tableView->setColumnHidden(0, true);
    m_tableView->setColumnHidden(1, true);
    m_tableView->setColumnHidden(7, true);
    m_tableView->setItemDelegateForColumn(
        8, new CoreCourseDelegate(m_tableView));
    m_model->select();

    connect(m_model, &QSqlTableModel::dataChanged, this,
            [this](const QModelIndex &topLeft,
                   const QModelIndex &bottomRight) {
                const QStringList semesters = {
                    QStringLiteral("大一上"), QStringLiteral("大一下"),
                    QStringLiteral("大二上"), QStringLiteral("大二下"),
                    QStringLiteral("大三上"), QStringLiteral("大三下"),
                    QStringLiteral("大四上"), QStringLiteral("大四下")};
                for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                    if (topLeft.column() <= 4 && 4 <= bottomRight.column()) {
                        const double score =
                            m_model->data(m_model->index(row, 4)).toDouble();
                        m_model->setData(
                            m_model->index(row, 6),
                            DatabaseManager::scoreToGpa(score));
                    }
                    if (topLeft.column() <= 5 && 5 <= bottomRight.column()) {
                        const QString semester =
                            m_model->data(m_model->index(row, 5)).toString();
                        const int order = semesters.indexOf(semester);
                        if (order >= 0)
                            m_model->setData(m_model->index(row, 7), order);
                    }
                }
                updateStats();
                emit dataChanged();
            });
}

void CoursePage::refresh() {
    if (m_model)
        m_model->select();
    updateStats();
}

void CoursePage::addCourse() {
    AddCourseDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = dialog.getName();
    if (name.isEmpty())
        return;

    const int row = m_model->rowCount();
    if (!m_model->insertRow(row))
        return;

    m_model->setData(m_model->index(row, 1), User::getInstance().getId());
    m_model->setData(m_model->index(row, 2), name);
    m_model->setData(m_model->index(row, 3), dialog.getCredit());
    m_model->setData(m_model->index(row, 4), dialog.getScore());
    m_model->setData(m_model->index(row, 5), dialog.getSemester());
    m_model->setData(m_model->index(row, 6),
                     DatabaseManager::scoreToGpa(dialog.getScore()));
    m_model->setData(m_model->index(row, 7), dialog.getSemesterOrder());
    m_model->setData(m_model->index(row, 8),
                     dialog.isCoreCourse() ? 1 : 0);

    if (!m_model->submitAll()) {
        m_model->revertAll();
        QMessageBox::critical(this, QStringLiteral("添加失败"),
                              m_model->lastError().text());
        return;
    }

    m_tableView->selectRow(row);
    updateStats();
    emit dataChanged();
}

void CoursePage::deleteSelectedCourses() {
    const QModelIndexList selectedRows =
        m_tableView->selectionModel()->selectedRows(0);
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(
            this, QStringLiteral("提示"),
            QStringLiteral("请先选择课程；按住 Cmd（macOS）或 "
                           "Ctrl（Windows/Linux）可多选"));
        return;
    }

    const int selectedCount = selectedRows.size();
    const auto result = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        selectedCount == 1
            ? QStringLiteral("确定要删除选中的课程吗？")
            : QStringLiteral("确定要删除选中的 %1 门课程吗？")
                  .arg(selectedCount));
    if (result != QMessageBox::Yes)
        return;

    QList<int> courseIds;
    for (const QModelIndex &index : selectedRows) {
        const int id = m_model->data(m_model->index(index.row(), 0)).toInt();
        if (id > 0)
            courseIds.append(id);
    }
    if (courseIds.isEmpty())
        return;

    QSqlDatabase database = QSqlDatabase::database();
    if (!database.transaction()) {
        QMessageBox::critical(this, QStringLiteral("删除失败"),
                              QStringLiteral("无法开始数据库事务。"));
        return;
    }

    QSqlQuery query(database);
    query.prepare(
        QStringLiteral("DELETE FROM courses WHERE id = :id AND user_id = "
                       ":user_id"));
    const int userId = User::getInstance().getId();
    for (const int id : courseIds) {
        query.bindValue(QStringLiteral(":id"), id);
        query.bindValue(QStringLiteral(":user_id"), userId);
        if (!query.exec()) {
            const QString error = query.lastError().text();
            database.rollback();
            refresh();
            QMessageBox::critical(this, QStringLiteral("删除失败"), error);
            return;
        }
    }

    if (!database.commit()) {
        database.rollback();
        refresh();
        QMessageBox::critical(this, QStringLiteral("删除失败"),
                              database.lastError().text());
        return;
    }

    refresh();
    m_tableView->clearSelection();
    emit dataChanged();
}

void CoursePage::resetAllCourses() {
    const int courseCount = m_model->rowCount();
    if (courseCount == 0) {
        QMessageBox::information(this, QStringLiteral("清空课程"),
                                 QStringLiteral("当前没有课程数据。"));
        return;
    }

    const auto result = QMessageBox::warning(
        this, QStringLiteral("清空全部课程"),
        QStringLiteral("此操作将清空当前用户的全部 %1 门课程，且无法撤销。\n\n"
                       "确定要继续吗？")
            .arg(courseCount),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (result != QMessageBox::Yes)
        return;

    QSqlQuery query;
    query.prepare(
        QStringLiteral("DELETE FROM courses WHERE user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"),
                    User::getInstance().getId());
    if (!query.exec()) {
        QMessageBox::critical(this, QStringLiteral("清空失败"),
                              query.lastError().text());
        return;
    }

    refresh();
    emit dataChanged();
}

void CoursePage::updateStats() {
    const QVariantMap stats = DatabaseManager::getInstance().getTotalStats(
        User::getInstance().getId());
    if (stats.value(QStringLiteral("count")).toInt() == 0) {
        m_statsLabel->setText(QStringLiteral("暂无数据"));
        return;
    }

    m_statsLabel->setText(
        QStringLiteral("总课程: %1 | 算术平均: %2 | 加权 GPA: %3 | 总学分: %4")
            .arg(stats.value(QStringLiteral("count")).toInt())
            .arg(stats.value(QStringLiteral("avg")).toDouble(), 0, 'f', 2)
            .arg(stats.value(QStringLiteral("gpa")).toDouble(), 0, 'f', 2)
            .arg(stats.value(QStringLiteral("totalCredits")).toDouble(), 0,
                 'f', 1));
}

void CoursePage::importFromCsv(const QString &filePath) {
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
    const bool hasCoreCourse =
        header.size() >= 5 && header.at(4) == QStringLiteral("核心课程");
    if (header.size() < 4 || header.at(0) != QStringLiteral("课程名称") ||
        header.at(1) != QStringLiteral("学分") ||
        header.at(2) != QStringLiteral("成绩") ||
        header.at(3) != QStringLiteral("学期") ||
        (header.size() >= 5 && !hasCoreCourse)) {
        QMessageBox::warning(
            this, QStringLiteral("格式错误"),
            QStringLiteral("CSV 表头不匹配。期望：\n"
                           "课程名称,学分,成绩,学期,核心课程"));
        return;
    }

    const QStringList validSemesters = {
        QStringLiteral("大一上"), QStringLiteral("大一下"),
        QStringLiteral("大二上"), QStringLiteral("大二下"),
        QStringLiteral("大三上"), QStringLiteral("大三下"),
        QStringLiteral("大四上"), QStringLiteral("大四下")};
    int successCount = 0;
    int failCount = 0;
    QSqlQuery query;
    query.prepare(
        QStringLiteral("INSERT INTO courses "
                       "(user_id, name, credit, score, semester, gpa, "
                       "semester_order, is_core) VALUES "
                       "(:uid, :name, :credit, :score, :semester, :gpa, "
                       ":semester_order, :is_core)"));

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;
        const QStringList fields = CsvUtils::parseCsvLine(line);
        if (fields.size() < (hasCoreCourse ? 5 : 4)) {
            ++failCount;
            continue;
        }

        bool creditOk = false;
        bool scoreOk = false;
        const double credit = fields.at(1).toDouble(&creditOk);
        const double score = fields.at(2).toDouble(&scoreOk);
        const int semesterOrder = validSemesters.indexOf(fields.at(3));
        bool isCoreCourse = false;
        const bool coreOk =
            !hasCoreCourse ||
            CsvUtils::parseCoreCourseValue(fields.at(4), isCoreCourse);
        if (fields.at(0).isEmpty() || !creditOk || credit <= 0 || !scoreOk ||
            score < 0 || score > 100 || semesterOrder < 0 || !coreOk) {
            ++failCount;
            continue;
        }

        query.bindValue(QStringLiteral(":uid"),
                        User::getInstance().getId());
        query.bindValue(QStringLiteral(":name"), fields.at(0));
        query.bindValue(QStringLiteral(":credit"), credit);
        query.bindValue(QStringLiteral(":score"), score);
        query.bindValue(QStringLiteral(":semester"), fields.at(3));
        query.bindValue(QStringLiteral(":gpa"),
                        DatabaseManager::scoreToGpa(score));
        query.bindValue(QStringLiteral(":semester_order"), semesterOrder);
        query.bindValue(QStringLiteral(":is_core"), isCoreCourse ? 1 : 0);
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

void CoursePage::exportToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              file.errorString());
        return;
    }

    QTextStream stream(&file);
    stream << QChar(0xFEFF);
    CsvUtils::writeCsvRow(
        stream, {QStringLiteral("课程名称"), QStringLiteral("学分"),
                 QStringLiteral("成绩"), QStringLiteral("学期"),
                 QStringLiteral("核心课程")});
    const int rowCount = m_model->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        CsvUtils::writeCsvRow(
            stream,
            {m_model->data(m_model->index(row, 2)).toString(),
             m_model->data(m_model->index(row, 3)).toString(),
             m_model->data(m_model->index(row, 4)).toString(),
             m_model->data(m_model->index(row, 5)).toString(),
             m_model->data(m_model->index(row, 8)).toBool()
                 ? QStringLiteral("是")
                 : QStringLiteral("否")});
    }
    QMessageBox::information(
        this, QStringLiteral("导出成功"),
        QStringLiteral("已导出 %1 条课程数据").arg(rowCount));
}

void CoursePage::showCsvHelp() {
    QMessageBox::information(
        this, QStringLiteral("CSV 格式说明 — 课程"),
        QStringLiteral(
            "<h3>表头</h3>"
            "<pre>课程名称,学分,成绩,学期,核心课程</pre>"
            "<p>课程名称、学分、成绩和学期必填；核心课程填写是或否。</p>"
            "<h3>示例</h3>"
            "<pre>高等数学,4,92,大一上,是</pre>"));
}

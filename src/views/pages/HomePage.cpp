#include "HomePage.h"

#include "CsvUtils.h"
#include "DatabaseMannager.h"
#include "User.h"

#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSqlQuery>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <QtMath>

namespace {

constexpr qreal kHiResScale = 4.0;

QFrame *makeCard(const QString &title, const QString &objectName) {
    auto *card = new QFrame;
    card->setObjectName(objectName);
    card->setFrameShape(QFrame::NoFrame);
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(13, 148, 136, 14));
    card->setGraphicsEffect(shadow);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 16, 22, 18);
    layout->setSpacing(8);
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("homeCardTitle"));
    layout->addWidget(titleLabel);
    return card;
}

QFrame *makeStatCard(const QString &title, const QString &subTitle,
                     QLabel **valueLabel, const QString &tone) {
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("homeStatCard"));
    card->setProperty("tone", tone);
    card->setFrameShape(QFrame::NoFrame);
    card->setFixedHeight(104);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(16);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(13, 148, 136, 20));
    card->setGraphicsEffect(shadow);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 12);
    layout->setSpacing(3);
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("homeStatTitle"));
    *valueLabel = new QLabel(QStringLiteral("0"));
    (*valueLabel)->setObjectName(QStringLiteral("homeStatValue"));
    auto *subLabel = new QLabel(subTitle);
    subLabel->setObjectName(QStringLiteral("homeStatSub"));
    layout->addWidget(titleLabel);
    layout->addWidget(*valueLabel);
    layout->addWidget(subLabel);
    return card;
}

} // namespace

HomePage::HomePage(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("homePage"));
    buildUi();
    refresh();
}

void HomePage::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(14);

    QFrame *chartCard =
        makeCard(QStringLiteral("学期平均 GPA 走势"),
                 QStringLiteral("homeChartCard"));
    chartCard->setMinimumHeight(292);
    chartCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chartLabel = new QLabel;
    m_chartLabel->setObjectName(QStringLiteral("homeChartLabel"));
    m_chartLabel->setMinimumSize(520, 220);
    m_chartLabel->setAlignment(Qt::AlignCenter);
    m_chartLabel->setSizePolicy(QSizePolicy::Expanding,
                                QSizePolicy::Expanding);
    chartCard->layout()->addWidget(m_chartLabel);

    auto *metricsPanel = new QFrame;
    metricsPanel->setObjectName(QStringLiteral("homeMetricsPanel"));
    metricsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *metricsLayout = new QVBoxLayout(metricsPanel);
    metricsLayout->setContentsMargins(4, 6, 4, 0);
    metricsLayout->setSpacing(10);

    auto *metricsHeader = new QHBoxLayout;
    metricsHeader->setContentsMargins(2, 0, 2, 0);
    metricsHeader->setSpacing(10);
    auto *metricsTitle = new QLabel(QStringLiteral("学习档案快照"));
    metricsTitle->setObjectName(QStringLiteral("homeMetricsTitle"));
    metricsTitle->setStyleSheet(QStringLiteral(
        "font-size:18px;font-weight:800;color:#25332F;background:transparent;"));
    auto *metricsSub = new QLabel(QStringLiteral("课程、实践与成果概览"));
    metricsSub->setObjectName(QStringLiteral("homeMetricsSub"));
    metricsSub->setStyleSheet(QStringLiteral(
        "font-size:12px;font-weight:600;color:#7A827E;background:transparent;"));
    metricsHeader->addWidget(metricsTitle);
    metricsHeader->addWidget(metricsSub);
    metricsHeader->addStretch();
    metricsLayout->addLayout(metricsHeader);

    auto *statsLayout = new QGridLayout;
    statsLayout->setContentsMargins(0, 0, 0, 4);
    statsLayout->setHorizontalSpacing(10);
    statsLayout->setVerticalSpacing(0);
    statsLayout->addWidget(
        makeStatCard(QStringLiteral("已修课程"), QStringLiteral("修读课程总数"),
                     &m_courseCountLabel, QStringLiteral("teal")),
        0, 0);
    statsLayout->addWidget(
        makeStatCard(QStringLiteral("GPA"), QStringLiteral("加权 GPA"),
                     &m_gpaLabel, QStringLiteral("cyan")),
        0, 1);
    statsLayout->addWidget(
        makeStatCard(QStringLiteral("竞赛"), QStringLiteral("课外活动"),
                     &m_competitionCountLabel, QStringLiteral("amber")),
        0, 2);
    statsLayout->addWidget(
        makeStatCard(QStringLiteral("实习"), QStringLiteral("实践经历"),
                     &m_internshipCountLabel, QStringLiteral("blue")),
        0, 3);
    statsLayout->addWidget(
        makeStatCard(QStringLiteral("项目"), QStringLiteral("项目 / 科研"),
                     &m_projectCountLabel, QStringLiteral("violet")),
        0, 4);
    statsLayout->addWidget(
        makeStatCard(QStringLiteral("荣誉"), QStringLiteral("奖项成果"),
                     &m_awardCountLabel, QStringLiteral("rose")),
        0, 5);
    for (int column = 0; column < 6; ++column)
        statsLayout->setColumnStretch(column, 1);
    metricsLayout->addLayout(statsLayout);

    mainLayout->addWidget(chartCard, 1);
    mainLayout->addWidget(metricsPanel);

    auto *actionBar = new QFrame;
    actionBar->setObjectName(QStringLiteral("homeActionBar"));
    actionBar->setFrameShape(QFrame::NoFrame);
    actionBar->setFixedHeight(58);
    auto *actionShadow = new QGraphicsDropShadowEffect(actionBar);
    actionShadow->setBlurRadius(16);
    actionShadow->setOffset(0, 4);
    actionShadow->setColor(QColor(13, 148, 136, 20));
    actionBar->setGraphicsEffect(actionShadow);

    auto *actionLayout = new QHBoxLayout(actionBar);
    actionLayout->setContentsMargins(16, 8, 10, 8);
    actionLayout->setSpacing(10);
    auto *actionTextLayout = new QVBoxLayout;
    actionTextLayout->setContentsMargins(0, 0, 0, 0);
    actionTextLayout->setSpacing(0);
    auto *actionTitle = new QLabel(QStringLiteral("数据管理"));
    actionTitle->setObjectName(QStringLiteral("homeActionTitle"));
    auto *actionSub = new QLabel(QStringLiteral("快速备份或迁移全部档案"));
    actionSub->setObjectName(QStringLiteral("homeActionSub"));
    actionTextLayout->addWidget(actionTitle);
    actionTextLayout->addWidget(actionSub);

    auto *importButton =
        new QPushButton(QStringLiteral("一键导入全部数据"), actionBar);
    auto *exportButton =
        new QPushButton(QStringLiteral("一键导出全部数据"), actionBar);
    auto *helpButton = new QPushButton(QStringLiteral("?"), actionBar);
    importButton->setObjectName(QStringLiteral("homeImportAllBtn"));
    exportButton->setObjectName(QStringLiteral("homeExportAllBtn"));
    helpButton->setObjectName(QStringLiteral("homeCsvHelpBtn"));
    importButton->setFixedHeight(40);
    exportButton->setFixedHeight(40);
    helpButton->setFixedSize(40, 40);
    helpButton->setToolTip(QStringLiteral("查看一键导入导出 CSV 格式说明"));

    actionLayout->addLayout(actionTextLayout);
    actionLayout->addStretch();
    actionLayout->addWidget(importButton);
    actionLayout->addWidget(exportButton);
    actionLayout->addWidget(helpButton);
    mainLayout->addWidget(actionBar);

    connect(importButton, &QPushButton::clicked, this, [this]() {
        const QString filePath = QFileDialog::getOpenFileName(
            this, QStringLiteral("一键导入全部数据"), QString(),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));
        if (!filePath.isEmpty())
            importAllFromCsv(filePath);
    });
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        const QString filePath = QFileDialog::getSaveFileName(
            this, QStringLiteral("一键导出全部数据"),
            QStringLiteral("college_data_%1.csv")
                .arg(QDate::currentDate().toString(QStringLiteral("yyyyMMdd"))),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));
        if (!filePath.isEmpty())
            exportAllToCsv(filePath);
    });
    connect(helpButton, &QPushButton::clicked, this,
            &HomePage::showCsvHelp);
}

void HomePage::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &HomePage::refresh);
}

void HomePage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (isVisible())
        QTimer::singleShot(0, this, &HomePage::refresh);
}

void HomePage::refresh() {
    if (!m_chartLabel)
        return;

    const int userId = User::getInstance().getId();
    const QStringList semesters = {
        QStringLiteral("大一上"), QStringLiteral("大一下"),
        QStringLiteral("大二上"), QStringLiteral("大二下"),
        QStringLiteral("大三上"), QStringLiteral("大三下"),
        QStringLiteral("大四上"), QStringLiteral("大四下")};
    QVector<double> gpas(semesters.size(), -1.0);
    QVector<double> gpaCreditSums(semesters.size(), 0.0);
    QVector<double> creditSums(semesters.size(), 0.0);

    QSqlQuery courseQuery;
    courseQuery.prepare(QStringLiteral(
        "SELECT semester, score, credit FROM courses WHERE user_id = :uid"));
    courseQuery.bindValue(QStringLiteral(":uid"), userId);
    int totalCourses = 0;
    double totalGpaCreditSum = 0.0;
    double totalCredits = 0.0;
    if (courseQuery.exec()) {
        while (courseQuery.next()) {
            const QString semester = courseQuery.value(0).toString().trimmed();
            const double score = courseQuery.value(1).toDouble();
            const double credit = courseQuery.value(2).toDouble();
            const double gpa = DatabaseManager::scoreToGpa(score);
            ++totalCourses;
            if (credit <= 0.0)
                continue;
            totalGpaCreditSum += gpa * credit;
            totalCredits += credit;
            int index = semesters.indexOf(semester);
            if (index < 0) {
                for (int i = 0; i < semesters.size(); ++i) {
                    if (semester.contains(semesters.at(i))) {
                        index = i;
                        break;
                    }
                }
            }
            if (index >= 0) {
                gpaCreditSums[index] += gpa * credit;
                creditSums[index] += credit;
            }
        }
    }
    for (int i = 0; i < semesters.size(); ++i) {
        if (creditSums.at(i) > 0.0)
            gpas[i] = gpaCreditSums.at(i) / creditSums.at(i);
    }

    int internshipCount = 0;
    int competitionCount = 0;
    int projectCount = 0;
    QSqlQuery experienceQuery;
    experienceQuery.prepare(QStringLiteral(
        "SELECT type, COUNT(*) FROM experiences WHERE user_id = :uid "
        "GROUP BY type"));
    experienceQuery.bindValue(QStringLiteral(":uid"), userId);
    if (experienceQuery.exec()) {
        while (experienceQuery.next()) {
            const QString type = experienceQuery.value(0).toString();
            const int count = experienceQuery.value(1).toInt();
            if (type.contains(QStringLiteral("实习")))
                internshipCount += count;
            else if (type.contains(QStringLiteral("竞赛")))
                competitionCount += count;
            else if (type.contains(QStringLiteral("项目")))
                projectCount += count;
        }
    }

    int awardCount = 0;
    QSqlQuery awardQuery;
    awardQuery.prepare(
        QStringLiteral("SELECT COUNT(*) FROM awards WHERE user_id = :uid"));
    awardQuery.bindValue(QStringLiteral(":uid"), userId);
    if (awardQuery.exec() && awardQuery.next())
        awardCount = awardQuery.value(0).toInt();

    const double averageGpa =
        totalCredits > 0.0 ? totalGpaCreditSum / totalCredits : 0.0;
    m_courseCountLabel->setText(QString::number(totalCourses));
    m_gpaLabel->setText(QString::number(averageGpa, 'f', 2));
    m_competitionCountLabel->setText(QString::number(competitionCount));
    m_internshipCountLabel->setText(QString::number(internshipCount));
    m_projectCountLabel->setText(QString::number(projectCount));
    m_awardCountLabel->setText(QString::number(awardCount));

    const int width = qMax(360, m_chartLabel->width());
    const int height = qMax(220, m_chartLabel->height());
    QPixmap pixmap(qCeil(width * kHiResScale),
                   qCeil(height * kHiResScale));
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(kHiResScale);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    constexpr qreal leftMargin = 68.0;
    constexpr qreal rightMargin = 30.0;
    constexpr qreal topMargin = 44.0;
    constexpr qreal bottomMargin = 62.0;
    const QRectF plot(leftMargin, topMargin,
                      width - leftMargin - rightMargin,
                      height - topMargin - bottomMargin);
    const QPen gridPen(QColor(226, 221, 211));
    painter.setFont(QFont(QStringLiteral("PingFang SC"), 9,
                          QFont::DemiBold));
    for (int i = 0; i <= 4; ++i) {
        const double y = plot.bottom() - plot.height() * i / 4.0;
        painter.setPen(gridPen);
        painter.drawLine(QPointF(plot.left(), y),
                         QPointF(plot.right(), y));
        painter.setPen(QColor(101, 112, 107));
        painter.drawText(QRectF(8, y - 10, leftMargin - 22, 20),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(i, 'f', 1));
    }
    painter.setPen(QPen(QColor(207, 201, 190), 1));
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());

    QVector<QPointF> points;
    const double dataLeft = plot.left() + 10.0;
    const double dataRight = plot.right() - 10.0;
    const double step = (dataRight - dataLeft) / (semesters.size() - 1);
    painter.setPen(QColor(70, 82, 78));
    for (int i = 0; i < semesters.size(); ++i) {
        const double x = dataLeft + i * step;
        painter.drawText(QRectF(x - 28, plot.bottom() + 10, 56, 28),
                         Qt::AlignCenter, semesters.at(i));
        if (gpas.at(i) >= 0.0) {
            const double y =
                plot.bottom() -
                plot.height() * qBound(0.0, gpas.at(i), 4.0) / 4.0;
            points.append(QPointF(x, y));
        }
    }

    if (points.size() >= 2) {
        QPainterPath path(points.first());
        for (int i = 1; i < points.size(); ++i)
            path.lineTo(points.at(i));
        QPainterPath areaPath = path;
        areaPath.lineTo(points.last().x(), plot.bottom());
        areaPath.lineTo(points.first().x(), plot.bottom());
        areaPath.closeSubpath();
        QLinearGradient gradient(0, plot.top(), 0, plot.bottom());
        gradient.setColorAt(0, QColor(31, 107, 91, 72));
        gradient.setColorAt(1, QColor(31, 107, 91, 4));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawPath(areaPath);
        painter.setPen(QPen(QColor(31, 107, 91), 4, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
    for (const QPointF &point : points) {
        painter.setPen(QPen(Qt::white, 4));
        painter.setBrush(QColor(31, 107, 91));
        painter.drawEllipse(point, 6, 6);
    }
    if (points.isEmpty()) {
        painter.setPen(QColor(140, 147, 143));
        painter.setFont(QFont(QStringLiteral("PingFang SC"), 15,
                              QFont::DemiBold));
        painter.drawText(
            plot, Qt::AlignCenter,
            QStringLiteral("暂无课程 GPA 数据\n录入课程后这里会自动生成折线图"));
    }
    painter.setPen(QColor(101, 112, 107));
    painter.setFont(QFont(QStringLiteral("PingFang SC"), 10,
                          QFont::DemiBold));
    painter.drawText(QRectF(plot.left(), 4, plot.width(), 22),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("横轴：学期    纵轴：平均 GPA"));
    m_chartLabel->setPixmap(pixmap);
}

void HomePage::exportAllToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              file.errorString());
        return;
    }

    QTextStream stream(&file);
    stream << QChar(0xFEFF);
    const int userId = User::getInstance().getId();
    int courseCount = 0;
    int experienceCount = 0;
    int awardCount = 0;

    CsvUtils::writeCsvRow(stream, {QStringLiteral("#SECTION: 课程")});
    CsvUtils::writeCsvRow(
        stream, {QStringLiteral("课程名称"), QStringLiteral("学分"),
                 QStringLiteral("成绩"), QStringLiteral("学期"),
                 QStringLiteral("核心课程")});
    QSqlQuery courseQuery;
    courseQuery.prepare(QStringLiteral(
        "SELECT name, credit, score, semester, is_core FROM courses "
        "WHERE user_id = :uid ORDER BY semester_order"));
    courseQuery.bindValue(QStringLiteral(":uid"), userId);
    if (courseQuery.exec()) {
        while (courseQuery.next()) {
            CsvUtils::writeCsvRow(
                stream,
                {courseQuery.value(0).toString(),
                 courseQuery.value(1).toString(),
                 courseQuery.value(2).toString(),
                 courseQuery.value(3).toString(),
                 courseQuery.value(4).toBool() ? QStringLiteral("是")
                                               : QStringLiteral("否")});
            ++courseCount;
        }
    }

    stream << "\n";
    CsvUtils::writeCsvRow(stream, {QStringLiteral("#SECTION: 经历")});
    CsvUtils::writeCsvRow(
        stream, {QStringLiteral("标题"), QStringLiteral("类型"),
                 QStringLiteral("时间"), QStringLiteral("描述")});
    QSqlQuery experienceQuery;
    experienceQuery.prepare(QStringLiteral(
        "SELECT title, type, date, content FROM experiences "
        "WHERE user_id = :uid ORDER BY date"));
    experienceQuery.bindValue(QStringLiteral(":uid"), userId);
    if (experienceQuery.exec()) {
        while (experienceQuery.next()) {
            CsvUtils::writeCsvRow(
                stream, {experienceQuery.value(0).toString(),
                         experienceQuery.value(1).toString(),
                         experienceQuery.value(2).toString(),
                         experienceQuery.value(3).toString()});
            ++experienceCount;
        }
    }

    stream << "\n";
    CsvUtils::writeCsvRow(stream, {QStringLiteral("#SECTION: 荣誉")});
    CsvUtils::writeCsvRow(
        stream, {QStringLiteral("奖项名称"), QStringLiteral("荣誉级别"),
                 QStringLiteral("获奖时间"), QStringLiteral("奖金金额")});
    QSqlQuery awardQuery;
    awardQuery.prepare(QStringLiteral(
        "SELECT name, level, date, amount FROM awards "
        "WHERE user_id = :uid ORDER BY date"));
    awardQuery.bindValue(QStringLiteral(":uid"), userId);
    if (awardQuery.exec()) {
        while (awardQuery.next()) {
            CsvUtils::writeCsvRow(
                stream, {awardQuery.value(0).toString(),
                         awardQuery.value(1).toString(),
                         awardQuery.value(2).toString(),
                         awardQuery.value(3).toString()});
            ++awardCount;
        }
    }

    QMessageBox::information(
        this, QStringLiteral("导出成功"),
        QStringLiteral("已导出全部数据：\n课程 %1 条 | 经历 %2 条 | 荣誉 %3 条")
            .arg(courseCount)
            .arg(experienceCount)
            .arg(awardCount));
}

void HomePage::importAllFromCsv(const QString &filePath) {
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

    enum Section { None, Courses, Experiences, Awards };
    Section section = None;
    bool headerValidated = false;
    bool headerBad = false;
    bool courseHeaderHasCore = false;
    int courseOk = 0;
    int courseFail = 0;
    int experienceOk = 0;
    int experienceFail = 0;
    int awardOk = 0;
    int awardFail = 0;
    const int userId = User::getInstance().getId();
    const QStringList semesters = {
        QStringLiteral("大一上"), QStringLiteral("大一下"),
        QStringLiteral("大二上"), QStringLiteral("大二下"),
        QStringLiteral("大三上"), QStringLiteral("大三下"),
        QStringLiteral("大四上"), QStringLiteral("大四下")};
    const QStringList types = {
        QStringLiteral("实习"), QStringLiteral("竞赛"),
        QStringLiteral("项目"), QStringLiteral("其他")};
    const QStringList levels = {
        QStringLiteral("国家级"), QStringLiteral("省级"),
        QStringLiteral("校级"), QStringLiteral("院级")};

    QSqlQuery courseQuery;
    courseQuery.prepare(QStringLiteral(
        "INSERT INTO courses "
        "(user_id, name, credit, score, semester, gpa, semester_order, "
        "is_core) VALUES "
        "(:uid, :name, :credit, :score, :semester, :gpa, :order, :is_core)"));
    QSqlQuery experienceQuery;
    experienceQuery.prepare(QStringLiteral(
        "INSERT INTO experiences (user_id, title, type, date, content) "
        "VALUES (:uid, :title, :type, :date, :content)"));
    QSqlQuery awardQuery;
    awardQuery.prepare(QStringLiteral(
        "INSERT INTO awards (user_id, name, level, date, amount) "
        "VALUES (:uid, :name, :level, :date, :amount)"));

    bool firstLine = true;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (firstLine) {
            if (line.startsWith(QChar(0xFEFF)))
                line = line.mid(1);
            firstLine = false;
        }
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        if (line.startsWith(QStringLiteral("#SECTION:"))) {
            const QString name = line.mid(9).trimmed();
            section = name == QStringLiteral("课程")
                          ? Courses
                          : name == QStringLiteral("经历")
                                ? Experiences
                                : name == QStringLiteral("荣誉") ? Awards
                                                                 : None;
            headerValidated = false;
            headerBad = false;
            courseHeaderHasCore = false;
            continue;
        }

        if (!headerValidated) {
            const QStringList header = CsvUtils::parseCsvLine(line);
            if (section == Courses) {
                courseHeaderHasCore =
                    header.size() >= 5 &&
                    header.at(4) == QStringLiteral("核心课程");
                headerBad =
                    header.size() < 4 ||
                    header.at(0) != QStringLiteral("课程名称") ||
                    header.at(1) != QStringLiteral("学分") ||
                    header.at(2) != QStringLiteral("成绩") ||
                    header.at(3) != QStringLiteral("学期") ||
                    (header.size() >= 5 && !courseHeaderHasCore);
            } else if (section == Experiences) {
                headerBad =
                    header.size() < 4 ||
                    header.at(0) != QStringLiteral("标题") ||
                    header.at(1) != QStringLiteral("类型") ||
                    header.at(2) != QStringLiteral("时间") ||
                    header.at(3) != QStringLiteral("描述");
            } else if (section == Awards) {
                headerBad =
                    header.size() < 4 ||
                    header.at(0) != QStringLiteral("奖项名称") ||
                    header.at(1) != QStringLiteral("荣誉级别") ||
                    header.at(2) != QStringLiteral("获奖时间") ||
                    header.at(3) != QStringLiteral("奖金金额");
            } else {
                headerBad = true;
            }
            headerValidated = true;
            continue;
        }
        if (headerBad)
            continue;

        const QStringList fields = CsvUtils::parseCsvLine(line);
        if (section == Courses) {
            if (fields.size() < (courseHeaderHasCore ? 5 : 4)) {
                ++courseFail;
                continue;
            }
            bool creditOk = false;
            bool scoreOk = false;
            const double credit = fields.at(1).toDouble(&creditOk);
            const double score = fields.at(2).toDouble(&scoreOk);
            const int order = semesters.indexOf(fields.at(3));
            bool isCore = false;
            const bool coreOk =
                !courseHeaderHasCore ||
                CsvUtils::parseCoreCourseValue(fields.at(4), isCore);
            if (fields.at(0).isEmpty() || !creditOk || credit <= 0 ||
                !scoreOk || score < 0 || score > 100 || order < 0 ||
                !coreOk) {
                ++courseFail;
                continue;
            }
            courseQuery.bindValue(QStringLiteral(":uid"), userId);
            courseQuery.bindValue(QStringLiteral(":name"), fields.at(0));
            courseQuery.bindValue(QStringLiteral(":credit"), credit);
            courseQuery.bindValue(QStringLiteral(":score"), score);
            courseQuery.bindValue(QStringLiteral(":semester"), fields.at(3));
            courseQuery.bindValue(
                QStringLiteral(":gpa"),
                DatabaseManager::scoreToGpa(score));
            courseQuery.bindValue(QStringLiteral(":order"), order);
            courseQuery.bindValue(QStringLiteral(":is_core"),
                                  isCore ? 1 : 0);
            courseQuery.exec() ? ++courseOk : ++courseFail;
        } else if (section == Experiences) {
            if (fields.size() < 4 || fields.at(0).isEmpty() ||
                !types.contains(fields.at(1))) {
                ++experienceFail;
                continue;
            }
            experienceQuery.bindValue(QStringLiteral(":uid"), userId);
            experienceQuery.bindValue(QStringLiteral(":title"), fields.at(0));
            experienceQuery.bindValue(QStringLiteral(":type"), fields.at(1));
            experienceQuery.bindValue(QStringLiteral(":date"), fields.at(2));
            experienceQuery.bindValue(QStringLiteral(":content"),
                                      fields.at(3));
            experienceQuery.exec() ? ++experienceOk : ++experienceFail;
        } else if (section == Awards) {
            if (fields.size() < 4 || fields.at(0).isEmpty() ||
                !levels.contains(fields.at(1))) {
                ++awardFail;
                continue;
            }
            bool amountOk = false;
            double amount = fields.at(3).toDouble(&amountOk);
            if (!amountOk)
                amount = 0.0;
            awardQuery.bindValue(QStringLiteral(":uid"), userId);
            awardQuery.bindValue(QStringLiteral(":name"), fields.at(0));
            awardQuery.bindValue(QStringLiteral(":level"), fields.at(1));
            awardQuery.bindValue(QStringLiteral(":date"), fields.at(2));
            awardQuery.bindValue(QStringLiteral(":amount"), amount);
            awardQuery.exec() ? ++awardOk : ++awardFail;
        }
    }

    refresh();
    emit allDataChanged();
    QMessageBox::information(
        this, QStringLiteral("导入完成"),
        QStringLiteral("导入结果：\n"
                       "课程：成功 %1 条，失败 %2 条\n"
                       "经历：成功 %3 条，失败 %4 条\n"
                       "荣誉：成功 %5 条，失败 %6 条")
            .arg(courseOk)
            .arg(courseFail)
            .arg(experienceOk)
            .arg(experienceFail)
            .arg(awardOk)
            .arg(awardFail));
}

void HomePage::showCsvHelp() {
    QMessageBox::information(
        this, QStringLiteral("CSV 格式说明 — 一键导入导出"),
        QStringLiteral(
            "<p>文件使用 <b>#SECTION: 课程</b>、<b>#SECTION: 经历</b> 和 "
            "<b>#SECTION: 荣誉</b> 分隔三类数据。</p>"
            "<pre>#SECTION: 课程\n"
            "课程名称,学分,成绩,学期,核心课程\n"
            "高等数学,4,92,大一上,是\n\n"
            "#SECTION: 经历\n"
            "标题,类型,时间,描述\n\n"
            "#SECTION: 荣誉\n"
            "奖项名称,荣誉级别,获奖时间,奖金金额</pre>"));
}

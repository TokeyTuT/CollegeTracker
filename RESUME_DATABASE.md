# 简历数据库接口说明

简历数据由 `DatabaseManager` 提供，不依赖任何前端控件。所有接口都要求传入
当前登录用户的 `userId`。

## 数据表

- `resume_profiles`：一名用户一份简历基本资料。
- `education_records`：一名用户可有多条教育经历。
- `experiences`：复用原有经历，新增单位、角色、排序和可见性字段。
- `awards`：复用原有荣誉，新增描述、排序和可见性字段。
- `courses`：复用原有课程，新增核心课程标记。

程序启动时会自动迁移旧数据库。旧用户会获得默认简历资料；已有
`users.school` 的用户会获得一条初始教育经历。

`phone`、`email`、`job_target` 和个人网站属于简历资料。历史版本若曾将这些
字段写入 `users`，启动时会自动迁移到 `resume_profiles`。迁移只填补简历表中的
空值；简历表已有内容不会被旧数据覆盖。迁移成功后，旧数据库中的 `phone`、
`email`、`job_target`、`website` 四个重复字段会从 `users` 表中删除。

编辑个人资料中的学校和专业时，`users` 与主 `education_records` 会在同一个事务
中同步更新；如果用户还没有教育经历，则自动创建一条。简历预览因此会立即使用
最新的学校和专业。旧数据库中若用户只有一条教育经历，启动时也会自动修复此前
已经产生的学校或专业不一致。

编辑个人资料还可以填写四位数的入学年份和毕业年份，分别保存到主教育经历的
`start_date` 与 `end_date`。简历中显示为 `2022～2026`。

## 简历资料

读取：

```cpp
QVariantMap profile = db.getResumeProfile(userId);
```

保存（完整覆盖）：

```cpp
QVariantMap profile;
profile["full_name"] = "张三";
profile["phone"] = "13800000000";
profile["email"] = "name@example.com";
profile["job_target"] = "Qt/C++ 开发工程师";
profile["github_url"] = "https://github.com/example";
profile["website_url"] = "";
profile["summary"] = "个人总结";
profile["skills"] = "C++\nQt\nSQLite";
profile["photo_path"] = "photos/user_1_profile.jpg";
db.updateResumeProfile(userId, profile);
```

`updateResumeProfile()` 是 upsert 接口，但采用完整覆盖语义。未提供的字段会被
保存为空字符串。

## 教育经历

教育经历使用以下键：

```text
id, user_id, school, major, degree, start_date, end_date,
description, sort_order, is_visible
```

主要接口：

```cpp
QVariantList rows = db.getEducationRecords(userId);
QVariantList visibleRows = db.getEducationRecords(userId, true);
int id = db.addEducationRecord(userId, education);
db.updateEducationRecord(userId, id, education);
db.deleteEducationRecord(userId, id);
```

新增和修改时 `school` 必填。`is_visible` 使用布尔值，`sort_order` 数字越小越靠前。

## 经历、荣誉和课程

```cpp
QVariantList experiences = db.getResumeExperiences(userId);
QVariantList awards = db.getResumeAwards(userId);
QVariantList courses = db.getResumeCoreCourses(userId);
```

用于配置简历展示的接口：

```cpp
db.updateExperienceResumeFields(
    userId, experienceId, organization, role, sortOrder, isVisible);

db.updateAwardResumeFields(
    userId, awardId, description, sortOrder, isVisible);

db.setCourseCore(userId, courseId, true);
```

经历的 `content` 建议保存纯文本，并用换行分隔多个要点。生成 HTML 时再将每一行
转换为 `<li>`，不要在数据库中保存 HTML。

## 导出聚合接口

HTML 生成层可以一次性获取全部可见数据：

```cpp
QVariantMap resume = db.getResumeData(userId);
```

返回结构：

```text
user          QVariantMap   账户基础资料
profile       QVariantMap   简历基本资料
education     QVariantList  可见教育经历
experiences   QVariantList  可见经历
awards        QVariantList  可见荣誉
core_courses  QVariantList  核心课程
course_stats  QVariantMap   GPA、平均分、总学分等统计
```

照片建议复制到应用数据目录后保存相对路径。生成 HTML/PDF 时再读取文件并转换为
Base64，避免导出结果依赖原始图片位置。

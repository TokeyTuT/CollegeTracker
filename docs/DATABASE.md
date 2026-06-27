# CollegeTracker 数据库说明

本文档描述项目当前使用的 SQLite 数据库，包括存储位置、表结构、数据关系、
迁移策略和主要访问接口。简历模板的新增与维护请参阅
[`RESUME_EXPORT.md`](RESUME_EXPORT.md)。

## 基本信息

- 数据库类型：SQLite
- Qt 驱动：`QSQLITE`
- 管理入口：`src/models/DatabaseMannager.h`
- 实现文件：`src/models/DatabaseMannager.cpp`
- 数据库文件名：`college_tracker.db`
- 外键约束：连接成功后执行 `PRAGMA foreign_keys = ON`

数据库由 `DatabaseManager` **单例**统一初始化。实际路径通过
`QStandardPaths::AppDataLocation` 取得；若系统无法提供该目录，则回退到用户主目录
下的 `.CollegeTracker`。

旧版本曾把数据库和照片保存在可执行文件旁。程序启动时，
`AppDataPaths::migrateLegacyData()` 会在新位置尚无对应数据时复制旧数据库和照片。

## 数据关系

每张业务表都通过 `user_id` 隔离不同用户的数据：

```text
users
├── resume_profiles      1 : 0..1
├── education_records    1 : N
├── courses              1 : N
├── experiences          1 : N
└── awards               1 : N
```

所有子表的 `user_id` 都引用 `users.id`，并使用 `ON DELETE CASCADE`。删除用户时，
该用户的简历资料、教育经历、课程、实践经历和荣誉会一并删除。

业务查询和修改必须携带当前用户的 `userId`。更新或删除单条数据时，应同时使用
记录 `id` 和 `user_id` 作为条件，避免跨用户访问。

## 表结构

### `users`

保存账户信息和侧边栏使用的主教育信息。

| 字段 | 类型与约束 | 说明 |
|---|---|---|
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 用户 ID |
| `username` | TEXT UNIQUE NOT NULL | 登录用户名 |
| `password` | TEXT NOT NULL | 加盐后的 SHA-256 哈希 |
| `password_salt` | TEXT DEFAULT `''` | 随机密码盐 |
| `grade` | TEXT DEFAULT `''` | 年级 |
| `gender` | TEXT DEFAULT `''` | 性别 |
| `major` | TEXT DEFAULT `''` | 主专业 |
| `school` | TEXT DEFAULT `''` | 主学校 |

注册时会生成 16 字节随机盐，并保存
`SHA-256(password_salt + password)`。历史明文密码会在启动迁移中自动转换。

### `resume_profiles`

每名用户最多一条简历基本资料，`user_id` 具有唯一约束。

| 字段 | 类型与约束 | 说明 |
|---|---|---|
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 资料 ID |
| `user_id` | INTEGER UNIQUE NOT NULL | 所属用户 |
| `full_name` | TEXT DEFAULT `''` | 简历姓名 |
| `phone` | TEXT DEFAULT `''` | 电话 |
| `email` | TEXT DEFAULT `''` | 邮箱 |
| `job_target` | TEXT DEFAULT `''` | 求职方向 |
| `github_url` | TEXT DEFAULT `''` | GitHub 地址 |
| `website_url` | TEXT DEFAULT `''` | 个人网站 |
| `summary` | TEXT DEFAULT `''` | 个人总结 |
| `skills` | TEXT DEFAULT `''` | 技术能力，每行一项 |
| `photo_path` | TEXT DEFAULT `''` | 应用数据目录内的照片相对路径 |
| `template_id` | TEXT NOT NULL DEFAULT `classic` | 当前模板 ID |
| `created_at` | TEXT DEFAULT CURRENT_TIMESTAMP | 创建时间 |
| `updated_at` | TEXT DEFAULT CURRENT_TIMESTAMP | 更新时间 |

`template_id` 只负责保存用户选择。模板定义及未知 ID 的回退规则由
`ResumeTemplateRegistry` 管理，详见 [`RESUME_EXPORT.md`](RESUME_EXPORT.md)。

### `education_records`

保存一名用户的多条教育经历。

| 字段 | 类型与约束 | 说明 |
|---|---|---|
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 教育经历 ID |
| `user_id` | INTEGER NOT NULL | 所属用户 |
| `school` | TEXT NOT NULL | 学校，新增和修改时必填 |
| `major` | TEXT DEFAULT `''` | 专业 |
| `degree` | TEXT DEFAULT `''` | 学位 |
| `start_date` | TEXT DEFAULT `''` | 开始日期或四位年份 |
| `end_date` | TEXT DEFAULT `''` | 结束日期或四位年份 |
| `description` | TEXT DEFAULT `''` | 补充说明 |
| `sort_order` | INTEGER NOT NULL DEFAULT 0 | 展示顺序，越小越靠前 |
| `is_visible` | INTEGER NOT NULL DEFAULT 1 | 是否出现在简历中，只允许 0/1 |

`users.school`、`users.major` 代表主教育信息。编辑个人资料时，程序会在同一事务中
同步最匹配的一条 `education_records`；若不存在教育经历，则自动创建。

### `courses`

| 字段 | 类型与约束 | 说明 |
|---|---|---|
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 课程 ID |
| `user_id` | INTEGER NOT NULL | 所属用户 |
| `name` | TEXT NOT NULL | 课程名称 |
| `credit` | REAL | 学分 |
| `score` | REAL | 百分制成绩 |
| `semester` | TEXT | 学期名称 |
| `gpa` | REAL DEFAULT 0 | 单科 GPA |
| `semester_order` | INTEGER DEFAULT 0 | 学期排序值 |
| `is_core` | INTEGER NOT NULL DEFAULT 0 | 是否为简历核心课程，只允许 0/1 |

总 GPA 在查询统计时按学分加权计算。当前分数转换规则为：

| 成绩 | GPA |
|---|---:|
| 90–100 | 4.0 |
| 85–89 | 3.7 |
| 82–84 | 3.3 |
| 78–81 | 3.0 |
| 75–77 | 2.7 |
| 72–74 | 2.3 |
| 68–71 | 2.0 |
| 64–67 | 1.5 |
| 60–63 | 1.0 |
| 低于 60 | 0.0 |

### `experiences`

| 字段 | 类型与约束 | 说明 |
|---|---|---|
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 经历 ID |
| `user_id` | INTEGER NOT NULL | 所属用户 |
| `title` | TEXT NOT NULL | 经历标题 |
| `type` | TEXT NOT NULL | 经历类型 |
| `date` | TEXT | 时间 |
| `content` | TEXT | 经历内容，建议每行一个要点 |
| `organization` | TEXT DEFAULT `''` | 单位或组织 |
| `role` | TEXT DEFAULT `''` | 职务或角色 |
| `sort_order` | INTEGER NOT NULL DEFAULT 0 | 简历展示顺序 |
| `is_visible` | INTEGER NOT NULL DEFAULT 1 | 是否出现在简历中，只允许 0/1 |

导出时，类型为“实习”的记录进入实习模块；“项目”或“竞赛”进入项目与获奖模块；
其他类型进入社区参与或其他实践模块。

### `awards`

| 字段 | 类型与约束 | 说明 |
|---|---|---|
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 荣誉 ID |
| `user_id` | INTEGER NOT NULL | 所属用户 |
| `name` | TEXT NOT NULL | 荣誉名称 |
| `level` | TEXT | 级别 |
| `date` | TEXT | 日期 |
| `amount` | REAL DEFAULT 0 | 奖金或金额 |
| `description` | TEXT DEFAULT `''` | 简历补充说明 |
| `sort_order` | INTEGER NOT NULL DEFAULT 0 | 简历展示顺序 |
| `is_visible` | INTEGER NOT NULL DEFAULT 1 | 是否出现在简历中，只允许 0/1 |

## 索引

项目为简历常用筛选和排序建立了以下索引：

```sql
idx_education_user_sort
    ON education_records(user_id, sort_order, id)

idx_experiences_user_resume
    ON experiences(user_id, is_visible, sort_order, id)

idx_awards_user_resume
    ON awards(user_id, is_visible, sort_order, id)

idx_courses_user_core
    ON courses(user_id, is_core, semester_order, id)
```

## 主要接口

### 用户与统计

```cpp
db.registerUser(username, password, grade, gender, major, school);
int userId = db.loginUser(username, password);
QVariantMap user = db.getUserInfo(userId);
QVariantMap stats = db.getTotalStats(userId);
```

`updateUserInfo()` 会在事务中同时更新：

- `users` 中的年级、性别、专业和学校；
- 主 `education_records` 的学校、专业和起止年份；
- `resume_profiles` 中的电话、邮箱、求职方向和个人网站。

任意一步失败都会回滚。

### 简历基本资料

```cpp
QVariantMap profile = db.getResumeProfile(userId);
profile["summary"] = "个人总结";
profile["skills"] = "C++\nQt\nSQLite";
profile["template_id"] = "classic";
db.updateResumeProfile(userId, profile);
```

`updateResumeProfile()` 使用 upsert，但采用完整覆盖语义。调用者通常应先读取原资料，
修改需要的字段后再保存；缺失字段会被写为空字符串。

### 教育经历

```cpp
QVariantList allRows = db.getEducationRecords(userId);
QVariantList visibleRows = db.getEducationRecords(userId, true);
int id = db.addEducationRecord(userId, education);
db.updateEducationRecord(userId, id, education);
db.deleteEducationRecord(userId, id);
```

### 简历展示控制

```cpp
db.updateExperienceResumeFields(
    userId, experienceId, organization, role, sortOrder, isVisible);

db.updateAwardResumeFields(
    userId, awardId, description, sortOrder, isVisible);

db.setCourseCore(userId, courseId, true);
```

### 简历数据聚合

```cpp
QVariantMap resume = db.getResumeData(userId);
```

返回结构：

| 键 | 类型 | 内容 |
|---|---|---|
| `user` | QVariantMap | 用户基础资料及联系方式 |
| `profile` | QVariantMap | 简历基本资料 |
| `education` | QVariantList | 可见教育经历 |
| `experiences` | QVariantList | 可见实践经历 |
| `awards` | QVariantList | 可见荣誉 |
| `core_courses` | QVariantList | 核心课程 |
| `course_stats` | QVariantMap | 课程数、平均分、加权 GPA、总学分 |

## 启动迁移

`initDatabase()` 的顺序为：

1. 打开 SQLite 数据库；
2. 启用外键；
3. 创建尚不存在的表；
4. 执行 `migrateTables()`。

当前迁移会：

- 为旧表补齐用户、GPA、排序、简历可见性等字段；
- 根据成绩重新计算课程 GPA；
- 根据学期名称补齐 `semester_order`；
- 为旧用户建立默认 `resume_profiles`；
- 将旧 `users.phone`、`email`、`job_target`、`website` 迁入
  `resume_profiles`，随后在事务中删除旧字段；
- 根据 `users.school` 初始化或修复主教育经历；
- 将没有盐值的历史明文密码转换为加盐哈希；
- 创建缺失的业务索引。

新增字段时，应继续通过 `ensureColumn()` 做向后兼容，不能只修改建表 SQL，否则已有
用户的数据库不会自动获得新字段。涉及多表一致性的迁移或更新应使用事务。

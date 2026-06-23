# 简历导出与模板扩展说明

本文档重点说明简历导出的模板机制，以及后续如何新增和维护模板。数据库表结构与
数据接口请参阅 [`DATABASE.md`](DATABASE.md)。

## 导出流程

```text
ResumePage
    │ 保存 template_id
    ▼
DatabaseManager::getResumeData()
    │ 聚合用户、教育、课程、经历和荣誉
    ▼
ResumeExporter::generateHtml()
    │ 生成各内容区块并替换 {{...}} 占位符
    ▼
HTML 模板
    ├── 浏览器预览
    └── Chromium 无头打印为 PDF
```

相关文件：

| 文件 | 职责 |
|---|---|
| `src/services/ResumeTemplateRegistry.h/.cpp` | 统一登记模板 ID、名称、说明、HTML 和预览图 |
| `src/services/ResumeExporter.h/.cpp` | 聚合数据、生成 HTML、浏览器预览和 PDF 导出 |
| `src/views/pages/ResumePage.cpp` | 根据注册表生成模板卡片和预览界面 |
| `templates/` | HTML 模板 |
| `assets/resume-preview-*.png` | 模板缩略图和放大预览图 |
| `resources.qrc` | 将 HTML 与图片编译进 Qt 资源系统 |

页面、模板描述、预览图、HTML 选择和未知模板回退都使用同一份
`ResumeTemplateRegistry`，不需要在多个调用方维护模板白名单。

## 当前模板

| ID | 显示名称 | HTML |
|---|---|---|
| `classic` | 经典学术 | `templates/resume_template.html` |
| `navy` | 深海蓝双栏 | `templates/resume_template_navy.html` |
| `editorial` | 暖色编辑风 | `templates/resume_template_editorial.html` |

注册表第一项是默认模板。数据库中 `template_id` 为空、大小写不一致或无法识别时，
`ResumeTemplateRegistry::findById()` 会回退到第一项。

模板 ID 发布后应保持稳定，因为用户选择会保存到
`resume_profiles.template_id`。修改显示名称或说明不会影响历史数据。

## HTML 占位符协议

所有模板必须保留以下占位符：

| 占位符 | 内容 |
|---|---|
| `{{DOCUMENT_TITLE}}` | HTML 文档标题 |
| `{{FULL_NAME}}` | 简历姓名 |
| `{{CONTACT_INFO}}` | 电话、邮箱、求职方向、GitHub 和个人网站 |
| `{{PHOTO_BLOCK}}` | 照片区域；无照片时为空 |
| `{{SUMMARY_SECTION}}` | 个人总结完整区块 |
| `{{EDUCATION_SECTION}}` | 教育背景和课程统计完整区块 |
| `{{SKILLS_SECTION}}` | 技术能力完整区块 |
| `{{INTERNSHIP_SECTION}}` | 实习经历完整区块 |
| `{{PROJECT_AWARD_SECTION}}` | 项目、竞赛和荣誉完整区块 |
| `{{OTHER_SECTION}}` | 其他实践经历完整区块 |
| `{{EMPTY_MESSAGE}}` | 没有可展示内容时的提示 |

除姓名和联系方式外，多数占位符已经包含完整的 `<section>`。模板只负责决定这些
区块放在哪里以及如何排版，不要再次给它们包一层同名内容。

`ResumeExporter` 当前生成并依赖以下常用 CSS 类：

```text
contact-line
header-right
profile-photo
section-title
item
item-header
item-title
item-date
item-desc
academic-summary
separator
empty-resume
```

新模板应为实际使用到的类提供样式。可以重新布局和配色，但不要仅在某个模板中
发明新的数据占位符；未在 `ResumeExporter::generateHtml()` 中替换的占位符会原样
出现在导出结果中。

## 新增模板

以下示例新增一个 ID 为 `minimal`、名称为“极简黑白”的模板。

### 1. 创建 HTML

在 `templates/` 中新建：

```text
templates/resume_template_minimal.html
```

最快的方式是复制最接近目标布局的现有模板，然后修改 HTML 和 CSS。完成后确认
上述 11 个占位符均存在，且每个只承担约定的内容。

### 2. 创建预览图

在 `assets/` 中添加：

```text
assets/resume-preview-minimal.png
```

预览图应使用与最终简历相同的纵向纸张比例，内容和配色要与 HTML 模板保持一致。
模板卡片和按空格显示的放大预览都会使用这张图片。

### 3. 注册 Qt 资源

在 `resources.qrc` 对应分组中各增加一项：

```xml
<qresource prefix="/templates">
    <file alias="resume_template_minimal.html">
        templates/resume_template_minimal.html
    </file>
</qresource>

<qresource prefix="/previews">
    <file alias="resume-minimal.png">
        assets/resume-preview-minimal.png
    </file>
</qresource>
```

实际文件可以保持单行书写；这里换行只是为了便于阅读。注册后的资源路径分别为：

```text
:/templates/resume_template_minimal.html
:/previews/resume-minimal.png
```

### 4. 登记模板

在 `src/services/ResumeTemplateRegistry.cpp` 的
`registeredTemplates` 中增加：

```cpp
{
    QStringLiteral("minimal"),
    QStringLiteral("极简黑白"),
    QStringLiteral("克制的单栏排版，适合正式申请和打印。"),
    QStringLiteral(":/templates/resume_template_minimal.html"),
    QStringLiteral(":/previews/resume-minimal.png"),
},
```

完成这一步后，`ResumePage` 会自动生成模板卡片、名称和说明；
`ResumeExporter` 会自动使用对应 HTML；预览文件名也会自动包含 `minimal`。
不需要修改 `ResumePage.cpp` 或 `ResumeExporter.cpp`。

### 5. 构建与验证

```bash
cmake --build build -j4
```

至少检查以下项目：

1. 模板卡片显示名称、说明和预览图正确；
2. 点击模板后，离开页面再返回仍保持选择；
3. 浏览器预览使用新增模板；
4. PDF 导出与浏览器预览布局一致；
5. 有照片和无照片时都没有空白错位；
6. 内容为空、内容较长以及多条经历时没有溢出；
7. 中文、英文、链接和特殊字符能够正常显示；
8. 打印结果没有被截断到第二页之外，或分页行为符合设计预期。

## 修改已有模板

只调整颜色、字体、间距或版式时，通常只需修改对应 HTML 和预览图。

若要修改模板名称或说明，只改注册表即可。不要随意修改模板 ID；确实需要更换 ID
时，应同时为数据库中的历史 `template_id` 设计迁移或兼容映射。

若要增加新的数据内容或占位符，则不再是单纯的模板新增，需要同步修改：

1. `DatabaseManager::getResumeData()`，确保数据可用；
2. `ResumeExporter::generateHtml()`，生成安全的 HTML 并替换占位符；
3. 所有现有 HTML 模板，决定新内容的位置和样式；
4. 本文档中的占位符协议；
5. 必要时更新数据库结构和 [`DATABASE.md`](DATABASE.md)。

用户输入必须在导出器中进行 HTML 转义，不要把数据库中的原始文本直接拼接进模板。

## 常见问题

### 页面出现新模板，但导出仍是默认模板

检查注册表中的模板 ID 是否与数据库保存值一致，以及 HTML 资源路径是否填写正确。
未知 ID 会正常回退到第一项，因此不会直接报错。

### 预览图为空

检查图片是否已加入 `resources.qrc`，并确认注册表使用的是 `:/previews/...` 资源路径，
而不是磁盘相对路径。

### 提示“无法读取内置简历模板”

通常是 HTML 没有加入 `resources.qrc`、资源别名与注册表不一致，或文件名大小写不匹配。

### 页面正常但 PDF 样式不同

PDF 由 Chrome、Edge 或 Chromium 的无头打印功能生成。优先使用适合打印的标准 CSS，
避免依赖浏览器扩展、外部网络字体、远程图片或交互脚本。模板资源应全部随程序提供。

### 新模板需要额外 C++ 文件吗

不需要。普通模板只包含 HTML、CSS 和预览图，不必修改 `CMakeLists.txt`。这些文件由
`resources.qrc` 纳入构建。

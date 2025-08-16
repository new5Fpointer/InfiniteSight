# InfiniteSight
## 🌟 核心特性

### 🚀 极致性能
- **大图处理**：使用 libvips 引擎处理超过 256MB 的超大图像
- **智能缓存**：LRU 缓存策略优化内存使用
- **后台加载**：非阻塞线程处理，保持 UI 流畅

### 🎨 基础功能
- **EXIF 元数据解析**：完整显示相机参数、GPS 等专业信息
- **多格式支持**：PNG, JPG, BMP, GIF, TIFF, WEBP 等主流格式
- **图像分析**：技术参数（尺寸、DPI、色彩模式）解析
- **最近文件历史**：智能记录访问历史，支持快速回溯

### 🖌️ 个性化
- **主题系统**：深色/浅色主题一键切换
- **多语言界面**：支持中英文动态切换
## 📂文件树图
```
InfiniteSight/
├── i18n/                   # 国际化文件
│   ├── en_us.json          # 英文翻译
│   └── zh_cn.json          # 中文翻译
├─ themes/                  # 样式文件
│  ├─ dark.qss              # 深色样式
│  └─ light.qss             # 浅色样式
├── vips/                   # libvips 预编译二进制文件
│   └── bin/                # libvips 的 DLL 文件
├── image_cache.py          # 图像缓存工具
├── image_loader.py         # 图像加载器
├── image_viewer.py         # 图像查看器主窗口
├── language_manager.py     # 语言管理器
├── main.py                 # 入口文件
├── requirements.txt        # 依赖列表
└── settings.py             # 设置管理器和对话框
```

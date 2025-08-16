import os
from PyQt6.QtWidgets import (QMainWindow, QLabel, QFileDialog, QVBoxLayout, QWidget, 
                             QScrollArea, QMenuBar, QDockWidget, QSplitter, QTreeWidget,
                             QTreeWidgetItem, QHeaderView, QProgressBar, QApplication, QDialog)
from PyQt6.QtGui import QFont, QMovie, QColor, QAction, QActionGroup
from PyQt6.QtCore import Qt, QThread, QSize
from settings import SettingsManager, SettingsDialog
from image_loader import ImageLoader
from language_manager import LanguageManager

class ImageViewer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("InfiniteSight - Modern Image Viewer")
        self.setGeometry(100, 100, 1400, 900)
        self.current_image_path = None
        self.setAcceptDrops(True)
        
        # 初始化设置管理器
        self.settings_manager = SettingsManager()
        self.settings = self.settings_manager.current_settings
        
        # 创建主布局
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QVBoxLayout(main_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        
        # 使用分割器让信息面板可调整
        self.splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # 图片显示区域
        self.image_scroll = QScrollArea()
        self.image_scroll.setWidgetResizable(True)
        
        # 图片容器
        self.image_container = QWidget()
        self.image_layout = QVBoxLayout(self.image_container)
        self.image_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        # 图片标签
        self.image_label = QLabel()
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.image_layout.addWidget(self.image_label)
        
        # 加载指示器
        self.loading_label = QLabel()
        self.loading_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.loading_label.setVisible(False)
        
        # 创建加载动画
        self.loading_movie = QMovie()
        self.loading_movie.setFileName("loading.gif")
        self.loading_movie.setScaledSize(QSize(50, 50))
        self.loading_label.setMovie(self.loading_movie)
        
        self.image_layout.addWidget(self.loading_label)
        self.image_scroll.setWidget(self.image_container)
        
        # 进度条
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        self.progress_bar.setRange(0, 100)
        self.image_layout.addWidget(self.progress_bar)
        
        # 信息面板
        self.info_dock = QDockWidget("Image Information", self)
        self.info_dock.setFeatures(QDockWidget.DockWidgetFeature.DockWidgetMovable | 
                                  QDockWidget.DockWidgetFeature.DockWidgetFloatable)
        self.info_dock.setAllowedAreas(Qt.DockWidgetArea.RightDockWidgetArea | 
                                      Qt.DockWidgetArea.LeftDockWidgetArea)
        
        self.info_tree = QTreeWidget()
        self.info_tree.setHeaderHidden(True)
        self.info_tree.setColumnWidth(0, 200)
        self.info_tree.header().setSectionResizeMode(0, QHeaderView.ResizeMode.ResizeToContents)
        self.info_tree.setFont(QFont("Segoe UI", 10))
        
        self.info_dock.setWidget(self.info_tree)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, self.info_dock)
        
        self.splitter.addWidget(self.image_scroll)
        self.splitter.addWidget(self.info_dock)
        self.splitter.setSizes([1000, 200])
        self.info_dock.setMinimumWidth(150)
        
        main_layout.addWidget(self.splitter)
        
        # 初始化语言管理器
        self.language_manager = LanguageManager(self.settings_manager)
        
        # 创建菜单
        self._create_menu()
        
        # 状态栏
        self.statusBar().showMessage("Ready")
        
        # 后台加载线程
        self.loader_thread = None
        self.image_loader = None
        
        # 应用初始设置
        self.apply_initial_settings()
    
    def tr(self, key, **kwargs):
        """翻译文本的便捷方法"""
        return self.language_manager.tr(key, **kwargs)

    def retranslate_ui(self):
        """重新翻译所有UI文本"""
        # 窗口标题
        self.setWindowTitle(self.tr("app_title"))
        
        # 信息面板标题
        self.info_dock.setWindowTitle(self.tr("dock_info_title"))
        
        # 菜单项
        self.file_menu.setTitle(self.tr("menu_file"))
        self.view_menu.setTitle(self.tr("menu_view"))
        self.settings_menu.setTitle(self.tr("menu_settings"))
        
        # 菜单动作
        self.open_action.setText(self.tr("menu_open"))
        self.exit_action.setText(self.tr("menu_exit"))
        self.settings_action.setText(self.tr("menu_settings_app"))
        self.info_toggle.setText(self.tr("menu_info_panel"))
        
        # 最近文件菜单
        self.recent_menu.setTitle(self.tr("menu_recent"))
        if hasattr(self, 'clear_action'):
            self.clear_action.setText(self.tr("clear_recent"))
        
        # 状态栏
        self.statusBar().showMessage(self.tr("status_ready"))
        
        # 更新最近文件菜单
        self.update_recent_files_menu()

    def apply_initial_settings(self):
        """应用初始设置（窗口状态、语言、主题等）"""
        # 应用窗口状态
        self.apply_window_state()
        
        # 应用语言设置
        self.language_manager.load_language(self.settings["general"]["language"])
        self.retranslate_ui()
        
        # 应用性能设置（包括简约样式）
        self.apply_performance_settings()
        
        # 应用主题（性能设置后应用，确保简约样式优先）
        self.apply_theme()
        
        # 应用外观设置（字体）
        self.apply_appearance_settings()
        
        # 应用信息面板设置
        self.info_dock.setVisible(self.settings["general"]["show_info_panel"])
        if hasattr(self, 'info_toggle'):
            self.info_toggle.setChecked(self.settings["general"]["show_info_panel"])

    def apply_window_state(self):
        """应用保存的窗口状态"""
        state = self.settings["general"]["default_window_state"]
        if state == "maximized":
            self.showMaximized()
        elif state == "fullscreen":
            self.showFullScreen()

    def apply_runtime_settings(self):
        """应用运行时设置（当设置变更时调用）"""
        # 重新加载设置
        self.settings = self.settings_manager.current_settings

        # 应用性能设置（包括简约样式）
        self.apply_performance_settings()

        # 应用主题（性能设置后应用，确保简约样式优先）
        self.apply_theme()
        
        # 应用外观设置（字体）
        self.apply_appearance_settings()

        # 应用信息面板设置
        self.info_dock.setVisible(self.settings["general"]["show_info_panel"])
        if hasattr(self, 'info_toggle'):
            self.info_toggle.setChecked(self.settings["general"]["show_info_panel"])

        # 重新加载语言并更新界面
        self.language_manager.load_language(self.settings["general"]["language"])
        self.retranslate_ui()

    def apply_performance_settings(self):
        """应用性能优化设置"""
        # 保存简约样式标志
        self.simple_style = self.settings["performance"]["simple_style"]
        
        # 应用其他性能设置（如果有）
        self.setProperty("quick_render", self.settings["performance"]["quick_render"])

    def apply_appearance_settings(self):
        """应用外观设置（字体、颜色等）"""
        # 获取设置
        font_family = self.settings["appearance"]["ui_font"]
        font_size = self.settings["appearance"]["ui_font_size"]
        
        # 更新状态栏和菜单栏字体
        app_font = QFont(font_family, font_size)
        QApplication.instance().setFont(app_font)
        self.statusBar().setFont(app_font)
        self.menuBar().setFont(app_font)

    def apply_theme(self):
        """应用当前主题设置，考虑简约样式选项"""
        if self.simple_style:
            # 简约样式：使用内置样式表
            self.apply_simple_style()
        else:
            # 完整样式：加载主题文件
            self.apply_theme_file()

    def apply_simple_style(self):
        bg_color = self.settings["appearance"]["background_color"]
        accent_color = self.settings["appearance"]["accent_color"]
        font_family = self.settings["appearance"]["ui_font"]
        font_size = self.settings["appearance"]["ui_font_size"]
        theme = self.settings["appearance"]["theme"]
        text_color = "#E0E0E0"
        if theme == "light":
            bg_color = "#FFFFFF"
            text_color = "#000000"
            accent_color = "#007ACC"

        self.setStyleSheet(f"""
            QMainWindow, QDockWidget, QTreeWidget, QScrollArea, QWidget {{
                background-color: {bg_color};
                color: {text_color};
                font-family: '{font_family}';
                font-size: {font_size}pt;
            }}
            QMenuBar {{
                background-color: {bg_color};
                color: #CCCCCC;
            }}
            QTreeWidget::item:selected {{
                background-color: {accent_color};
                color: #FFFFFF;
            }}
            QProgressBar {{
                border: 1px solid {accent_color};
                border-radius: 3px;
                text-align: center;
                background-color: {bg_color};
            }}
            QProgressBar::chunk {{
                background-color: {accent_color};
            }}
        """)

    def apply_theme_file(self):
        """从文件加载主题样式表"""
        theme = self.settings["appearance"]["theme"]
        qss_path = os.path.join(os.path.dirname(__file__), "themes", f"{theme}.qss")
        try:
            if os.path.exists(qss_path):
                with open(qss_path, encoding="utf-8") as f:
                    print("Applying simple style now!")
                    self.setStyleSheet(f.read())
            else:
                # 回退到简约样式
                self.apply_simple_style()
        except Exception as e:
            print(f"加载主题失败: {e}")
            # 回退到简约样式
            self.apply_simple_style()

    def _create_menu(self):
        """创建菜单系统"""
        menu_bar = QMenuBar(self)

        # 文件菜单
        self.file_menu = menu_bar.addMenu(self.tr("menu_file"))

        self.open_action = QAction(self.tr("menu_open"), self)
        self.open_action.setShortcut("Ctrl+O")
        self.open_action.triggered.connect(self._open_image)
        self.file_menu.addAction(self.open_action)

        # 最近文件
        self.recent_menu = self.file_menu.addMenu(self.tr("menu_recent"))
        self.update_recent_files_menu()

        self.file_menu.addSeparator()

        self.exit_action = QAction(self.tr("menu_exit"), self)
        self.exit_action.setShortcut("Ctrl+Q")
        self.exit_action.triggered.connect(self.close)
        self.file_menu.addAction(self.exit_action)

        # 视图菜单
        self.view_menu = menu_bar.addMenu(self.tr("menu_view"))

        # 信息面板开关
        self.info_toggle = QAction(self.tr("menu_info_panel"), self)
        self.info_toggle.setShortcut("Ctrl+I")
        self.info_toggle.setCheckable(True)
        self.info_toggle.setChecked(self.settings["general"]["show_info_panel"])
        self.info_toggle.toggled.connect(self._toggle_info_panel)
        self.view_menu.addAction(self.info_toggle)

        # 主题子菜单
        theme_menu = self.view_menu.addMenu(self.tr("settings_theme"))
        theme_ag = QActionGroup(self)  # 互斥组
        theme_ag.setExclusive(True)

        self.dark_action = QAction(self.tr("settings_theme_dark"), self, checkable=True)
        self.light_action = QAction(self.tr("settings_theme_light"), self, checkable=True)
        for a in (self.dark_action, self.light_action):
            a.setActionGroup(theme_ag)
            theme_menu.addAction(a)

        self.dark_action.triggered.connect(lambda: self.switch_theme("dark"))
        self.light_action.triggered.connect(lambda: self.switch_theme("light"))

        # 初始化勾选
        current_theme = self.settings["appearance"]["theme"]
        self.dark_action.setChecked(current_theme == "dark")
        self.light_action.setChecked(current_theme == "light")

        # 设置菜单
        self.settings_menu = menu_bar.addMenu(self.tr("menu_settings"))
        self.settings_action = QAction(self.tr("menu_settings_app"), self)
        self.settings_action.triggered.connect(self._open_settings)
        self.settings_menu.addAction(self.settings_action)

        self.setMenuBar(menu_bar)

    def update_recent_files_menu(self):
        """更新最近文件菜单"""
        self.recent_menu.clear()
        recent_files = self.settings["general"]["recent_files"]
        
        if not recent_files:
            no_files_action = QAction(self.tr("no_recent_files"), self)
            no_files_action.setEnabled(False)
            self.recent_menu.addAction(no_files_action)
            return
        
        for file_path in recent_files:
            if os.path.exists(file_path):
                action = QAction(os.path.basename(file_path), self)
                action.setData(file_path)
                action.triggered.connect(lambda checked, path=file_path: self.open_recent_file(path))
                self.recent_menu.addAction(action)
        
        self.recent_menu.addSeparator()
        
        # 清除最近文件操作
        self.clear_action = QAction(self.tr("clear_recent"), self)
        self.clear_action.triggered.connect(self.clear_recent_files)
        self.recent_menu.addAction(self.clear_action)

    def open_recent_file(self, file_path):
        """打开最近文件"""
        if os.path.exists(file_path):
            # 添加到最近文件
            self.settings_manager.add_recent_file(file_path)
            self.update_recent_files_menu()
            
            # 显示加载状态
            self.current_image_path = file_path
            self.statusBar().showMessage(
                self.tr("status_loading", file=os.path.basename(file_path)))
            self.image_label.clear()
            self.loading_label.setVisible(True)
            self.loading_movie.start()
            self.progress_bar.setVisible(True)
            self.progress_bar.setValue(0)
            
            # 在后台线程中加载图片
            self.start_image_loading(file_path)
        else:
            self.statusBar().showMessage(f"File not found: {file_path}")

    def clear_recent_files(self):
        """清除最近文件列表"""
        self.settings_manager.current_settings["general"]["recent_files"] = []
        self.settings_manager.save_settings()
        self.update_recent_files_menu()

    def _open_settings(self):
        """打开设置对话框"""
        dialog = SettingsDialog(self.settings_manager, self)
        if dialog.exec() == QDialog.DialogCode.Accepted:
            self.statusBar().showMessage(self.tr("status_settings_applied"))

    def _toggle_info_panel(self, visible):
        """切换信息面板可见性"""
        self.info_dock.setVisible(visible)
        self.settings_manager.update_setting("general", "show_info_panel", visible)
        self.settings_manager.save_settings()

    def _open_image(self):
        """打开图像文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, 
            "Open Image", 
            "", 
            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp)"
        )
        if file_path:
            # 添加到最近文件
            self.settings_manager.add_recent_file(file_path)
            self.update_recent_files_menu()
            
            # 取消任何正在进行的加载
            if self.image_loader:
                self.image_loader.cancel()
            
            # 显示加载状态
            self.current_image_path = file_path
            self.statusBar().showMessage(f"Loading: {os.path.basename(file_path)}...")
            self.image_label.clear()
            self.loading_label.setVisible(True)
            self.loading_movie.start()
            self.progress_bar.setVisible(True)
            self.progress_bar.setValue(0)
            
            # 在后台线程中加载图片
            self.start_image_loading(file_path)
    
    def start_image_loading(self, file_path):
        """启动后台线程加载图片"""
        # 创建加载器
        self.image_loader = ImageLoader(file_path, self.settings["performance"])
        
        # 创建线程
        self.loader_thread = QThread()
        self.image_loader.moveToThread(self.loader_thread)
        
        # 连接信号
        self.image_loader.finished.connect(self.on_image_loaded)
        self.image_loader.info_ready.connect(self.on_info_ready)
        self.image_loader.progress.connect(self.progress_bar.setValue)
        self.loader_thread.started.connect(self.image_loader.run)
        
        # 启动线程
        self.loader_thread.start()

    def on_image_loaded(self, pixmap, file_path):
        """图片加载完成时的处理"""
        # 确保加载的是当前请求的图片
        if file_path != self.current_image_path:
            return
            
        if pixmap is None:
            self.statusBar().showMessage(self.tr("error_load_image"))
            self.image_label.setText(self.tr("error_failed_load"))
            return
        
        # 根据性能设置选择渲染模式
        if self.settings["performance"]["quick_render"]:
            # 快速渲染模式 - 降低质量但更快
            scaled_pixmap = pixmap.scaled(
                self.image_scroll.width() - 50,
                self.image_scroll.height() - 50,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.FastTransformation
            )
        else:
            # 高质量渲染模式
            scaled_pixmap = pixmap.scaled(
                self.image_scroll.width() - 50,
                self.image_scroll.height() - 50,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation
            )
            
        self.image_label.setPixmap(scaled_pixmap)
        
        # 隐藏加载指示器
        self.loading_label.setVisible(False)
        self.loading_movie.stop()
        self.progress_bar.setVisible(False)
        
        self.statusBar().showMessage(f"Loaded: {os.path.basename(file_path)}")
        
        # 清理线程
        if self.loader_thread:
            self.loader_thread.quit()
            self.loader_thread.wait()
            self.loader_thread = None
            self.image_loader = None

    def on_info_ready(self, image_info):
        """图片信息加载完成时的处理（单列嵌套显示）"""
        self.info_tree.clear()

        def add_section(title, data):
            """把一段字典信息挂到树上"""
            if not data:
                return
            root = QTreeWidgetItem([title])
            self.info_tree.addTopLevelItem(root)
            for k, v in data.items():
                child = QTreeWidgetItem([f"{k}: {v}"])
                root.addChild(child)
            root.setExpanded(True)

        # 1) 文件信息
        add_section(self.tr("file_info_title"), image_info.get("file_info", {}))

        # 2) 图像技术信息
        add_section(self.tr("image_info_title"), image_info.get("image_info", {}))

        # 3) EXIF 元数据
        add_section(self.tr("exif_info_title"), image_info.get("exif_info", {}))

        # 4) 错误信息（如果有）
        if image_info.get("error"):
            err_root = QTreeWidgetItem(["Error"])
            self.info_tree.addTopLevelItem(err_root)
            err_root.addChild(QTreeWidgetItem([image_info["error"]]))
            err_root.setExpanded(True)

    def switch_theme(self, theme_name: str):
        """切换主题"""
        self.settings_manager.update_setting("appearance", "theme", theme_name)
        self.settings_manager.save_settings() 
        self.settings = self.settings_manager.current_settings

        # 清空旧样式，防止冲突
        self.setStyleSheet("")

        # 重新加载新主题
        self.apply_theme()

        # 更新菜单勾选状态
        self.dark_action.setChecked(theme_name == "dark")
        self.light_action.setChecked(theme_name == "light")

    # ----------------------------  拖放支持  ----------------------------
    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
        else:
            event.ignore()

    def dropEvent(self, event):
        files = [u.toLocalFile() for u in event.mimeData().urls()]
        for path in files:
            if os.path.isfile(path):
                ext = os.path.splitext(path)[1].lower()
                if ext in {'.png', '.jpg', '.jpeg', '.bmp', '.gif', '.tiff', '.webp'}:
                    self.open_recent_file(path)   # 复用现有加载逻辑
                    break   # 只取第一张
                else:
                    self.statusBar().showMessage("Unsupported file format", 3000)
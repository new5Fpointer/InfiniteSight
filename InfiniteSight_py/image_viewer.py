import os
import uuid

from PySide6.QtWidgets import (QMainWindow, QLabel, QFileDialog, QVBoxLayout, QWidget, 
                             QScrollArea, QMenuBar, QDockWidget, QSplitter, QTreeWidget,
                             QTreeWidgetItem, QHeaderView, QProgressBar, QApplication, 
                             QDialog, QGraphicsView, QGraphicsScene, QGraphicsPixmapItem)
from PySide6.QtGui import QFont, QMovie, QAction, QActionGroup, QWheelEvent, QIcon, QTransform
from PySide6.QtCore import Qt, QThread, QSize, QFile
from settings import SettingsManager, SettingsDialog
from image_loader import ImageLoader
from language_manager import LanguageManager
from PySide6 import QtGui

class ZoomableGraphicsView(QGraphicsView):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setDragMode(QGraphicsView.DragMode.ScrollHandDrag)
        self.setRenderHint(QtGui.QPainter.RenderHint.SmoothPixmapTransform)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        # 启用抗锯齿
        self.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        self.setRenderHint(QtGui.QPainter.RenderHint.TextAntialiasing)
        
        # 设置视图port更新模式
        self.setViewportUpdateMode(QGraphicsView.ViewportUpdateMode.FullViewportUpdate)
        
        # 设置缓存背景
        self.setCacheMode(QGraphicsView.CacheModeFlag.CacheBackground)
    def wheelEvent(self, event: QWheelEvent):
        if event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            zoom_in_factor = 1.15
            zoom_out_factor = 1 / zoom_in_factor

            if event.angleDelta().y() > 0:
                scale_factor = zoom_in_factor
            else:
                scale_factor = zoom_out_factor

            mouse_pos = event.position()
            scene_pos = self.mapToScene(mouse_pos.toPoint())

            self.scale(scale_factor, scale_factor)

            new_scene_pos = self.mapToScene(mouse_pos.toPoint())
            delta = new_scene_pos - scene_pos
            self.setTransformationAnchor(QGraphicsView.ViewportAnchor.NoAnchor)
            self.translate(delta.x(), delta.y())

            event.accept()
        else:
            # 平滑的普通滚动
            delta = event.angleDelta().y() * 0.5
            self.verticalScrollBar().setValue(
                self.verticalScrollBar().value() - int(delta)
            )
            event.accept()

    def dragEnterEvent(self, event):
        self.parent().dragEnterEvent(event)

    def dropEvent(self, event):
        self.parent().dropEvent(event)

class ImageViewer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("InfiniteSight - Modern Image Viewer")
        self.setGeometry(100, 100, 1400, 900)
        self.current_image_path = None
        self.setAcceptDrops(True)
        self.pixmap_item = None
        self.scale_factor = 1.0
        self.current_folder_images = []   # 同级目录图片列表
        self.current_folder_index = -1    # 当前图片在列表中的索引
        
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
        self.graphics_view = ZoomableGraphicsView()
        self.graphics_scene = QGraphicsScene()
        self.graphics_view.setScene(self.graphics_scene)
        self.graphics_view.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.graphics_view.setRenderHint(QtGui.QPainter.RenderHint.SmoothPixmapTransform)
        self.graphics_view.setDragMode(QGraphicsView.DragMode.ScrollHandDrag)  # 支持拖动
        self.image_layout.addWidget(self.graphics_view)
        
        # 加载指示器
        self.loading_label = QLabel()
        self.loading_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.loading_label.setVisible(False)

        # 漫游信息标签
        self.roam_label = QLabel()
        self.roam_label.setObjectName("roam_label")   # 方便样式表
        self.statusBar().addPermanentWidget(self.roam_label)   # 永久靠右
        self.roam_label.setVisible(False)            # 默认隐藏
        
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
        
        self.splitter.addWidget(self.graphics_view)
        self.splitter.addWidget(self.info_dock)
        self.splitter.setSizes([1000, 200])
        self.info_dock.setMinimumWidth(150)
        
        main_layout.addWidget(self.splitter)
        
        # 初始化语言管理器
        self.language_manager = LanguageManager(self.settings_manager)
        
        # 创建菜单
        self._create_menu()

        self._create_toolbar()
        
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
        
        # 主题菜单
        if hasattr(self, 'dark_action'):
            self.dark_action.setText(self.tr("settings_theme_dark"))
            self.light_action.setText(self.tr("settings_theme_light"))
        
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
        
        # 应用性能设置
        self.apply_performance_settings()
        
        # 应用外观设置（字体和样式）
        self.apply_appearance_settings()

        # 切换图标
        self.refresh_toolbar_icons()
        
        # 应用信息面板设置
        self.info_dock.setVisible(self.settings["general"]["show_info_panel"])
        if hasattr(self, 'info_toggle'):
            self.info_toggle.setChecked(self.settings["general"]["show_info_panel"])
        
        # 初始化主题菜单勾选状态
        if hasattr(self, 'dark_action') and hasattr(self, 'light_action'):
            current_theme = self.settings["appearance"]["theme"]
            self.dark_action.setChecked(current_theme == "dark")
            self.light_action.setChecked(current_theme == "light")

    def apply_runtime_settings(self):
        """应用运行时设置（当设置变更时调用）"""
        # 重新加载设置
        self.settings = self.settings_manager.current_settings

        # 应用性能设置
        self.apply_performance_settings()
        
        # 应用外观设置（字体和样式）
        self.apply_appearance_settings()

        # 应用信息面板设置
        self.info_dock.setVisible(self.settings["general"]["show_info_panel"])
        if hasattr(self, 'info_toggle'):
            self.info_toggle.setChecked(self.settings["general"]["show_info_panel"])

        # 更新主题菜单勾选状态
        if hasattr(self, 'dark_action') and hasattr(self, 'light_action'):
            current_theme = self.settings["appearance"]["theme"]
            self.dark_action.setChecked(current_theme == "dark")
            self.light_action.setChecked(current_theme == "light")

        # 重新加载语言并更新界面
        self.language_manager.load_language(self.settings["general"]["language"])
        self.retranslate_ui()

    def apply_window_state(self):
        """应用保存的窗口状态"""
        state = self.settings["general"]["default_window_state"]
        if state == "maximized":
            self.showMaximized()
        elif state == "fullscreen":
            self.showFullScreen()

    def apply_performance_settings(self):
        """应用性能优化设置"""
        # 应用性能设置
        self.setProperty("quick_render", self.settings["performance"]["quick_render"])

    def apply_appearance_settings(self):
        """应用外观设置（字体、样式等）"""
        # 获取设置
        font_family = self.settings["appearance"]["ui_font"]
        font_size = self.settings["appearance"]["ui_font_size"]
        theme = self.settings["appearance"]["theme"]
        
        # 更新状态栏和菜单栏字体
        app_font = QFont(font_family, font_size)
        QApplication.instance().setFont(app_font)
        self.statusBar().setFont(app_font)
        self.menuBar().setFont(app_font)
        
        # 应用简约样式
        self.apply_simple_style(theme, font_family, font_size)

    def apply_simple_style(self, theme, font_family, font_size):
        """应用简约样式（优化版）"""
        bg_color = "#2D2D30" if theme == "dark" else "#FFFFFF"
        accent_color = "#007ACC"
        text_color = "#E0E0E0" if theme == "dark" else "#000000"
        menu_text_color = "#CCCCCC" if theme == "dark" else "#000000"
        border_color = "#1E1E1E" if theme == "dark" else "#CCCCCC"
        selected_bg = "#3F3F46" if theme == "dark" else "#E0E0E0"
        progress_bg = "#1E1E1E" if theme == "dark" else "#FFFFFF"
        scrollbar_bg = "#404040" if theme == "dark" else "#F0F0F0"
        scrollbar_handle = "#606060" if theme == "dark" else "#C0C0C0"
        scrollbar_handle_hover = "#808080" if theme == "dark" else "#A0A0A0"

        # 使用更简洁的样式表，避免重复选择器
        style_sheet = f"""
            QMainWindow, QDockWidget, QTreeWidget, QScrollArea, QWidget {{
                background-color: {bg_color};
                color: {text_color};
                font-family: '{font_family}';
                font-size: {font_size}pt;
            }}
            QMenuBar {{
                background-color: {bg_color};
                color: {menu_text_color};
                border-bottom: 1px solid {border_color};
            }}
            QMenuBar::item:selected {{
                background-color: {selected_bg};
            }}
            QTreeWidget::item:selected {{
                background-color: {accent_color};
                color: #FFFFFF;
            }}
            QProgressBar {{
                border: 1px solid {border_color};
                border-radius: 3px;
                text-align: center;
                background-color: {progress_bg};
                color: {text_color};
            }}
            QProgressBar::chunk {{
                background-color: {accent_color};
            }}
            QScrollBar:vertical {{
                border: none;
                background: {scrollbar_bg};
                width: 12px;
                margin: 0px;
            }}
            QScrollBar::handle:vertical {{
                background: {scrollbar_handle};
                border-radius: 6px;
                min-height: 30px;
            }}
            QScrollBar::handle:vertical:hover {{
                background: {scrollbar_handle_hover};
            }}
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
                border: none;
                background: none;
                height: 0px;
            }}
            QScrollBar:horizontal {{
                border: none;
                background: {scrollbar_bg};
                height: 12px;
                margin: 0px;
            }}
            QScrollBar::handle:horizontal {{
                background: {scrollbar_handle};
                border-radius: 6px;
                min-width: 30px;
            }}
            QScrollBar::handle:horizontal:hover {{
                background: {scrollbar_handle_hover};
            }}
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {{
                border: none;
                background: none;
                width: 0px;
            }}
            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
            QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {{
                background: none;
            }}
        """
        
        # 使用 setStyleSheet 而不是重新设置样式
        self.setStyleSheet(style_sheet)

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
            # 先停止任何正在进行的加载
            self.stop_current_loading()
            
            # 重置画布状态
            self.reset_canvas()

            # 添加到最近文件
            self.settings_manager.add_recent_file(file_path)
            self.update_recent_files_menu()
            
            # 显示加载状态
            self.current_image_path = file_path
            self.statusBar().showMessage(
                self.tr("status_loading", file=os.path.basename(file_path)))
            self.graphics_scene.clear()
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
            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.tif *.webp)"
        )
        if file_path:
            # 先停止任何正在进行的加载
            self.stop_current_loading()
            
            # 重置画布状态
            self.reset_canvas()

            # 添加到最近文件
            self.settings_manager.add_recent_file(file_path)
            self.update_recent_files_menu()
            
            # 显示加载状态
            self.current_image_path = file_path
            self.statusBar().showMessage(f"Loading: {os.path.basename(file_path)}...")
            self.graphics_scene.clear()
            self.loading_label.setVisible(True)
            self.loading_movie.start()
            self.progress_bar.setVisible(True)
            self.progress_bar.setValue(0)
            
            # 在后台线程中加载图片
            self.start_image_loading(file_path)
    
    def start_image_loading(self, file_path: str) -> None:
        # 1. 生成新版本号
        ImageViewer.current_job_id = uuid.uuid4().hex

        # 2. 停旧线程（协作式）
        self.stop_current_loading()

        # 3. 创建加载器/线程
        self.image_loader = ImageLoader(
            file_path,
            self.settings["performance"],
            ImageViewer.current_job_id
        )
        self.loader_thread = QThread()
        self.image_loader.moveToThread(self.loader_thread)

        # 4. 连接信号
        self.image_loader.finished.connect(self.on_image_loaded)
        self.image_loader.info_ready.connect(self.on_info_ready)
        self.image_loader.progress.connect(self.progress_bar.setValue)
        self.loader_thread.started.connect(self.image_loader.run)
        self.loader_thread.finished.connect(self.loader_thread.deleteLater)

        # 5. 启动
        self.loader_thread.start()

    def on_image_loaded(self, pixmap, file_path, job_id: str) -> None:
        # 5. 主线程里比对版本号，过期直接丢弃
        if job_id != ImageViewer.current_job_id:
            return
        if pixmap is None:
            self.statusBar().showMessage(self.tr("error_load_image"))
            return

        # 重置画布 - 先重置所有状态
        self.reset_canvas()
        
        # 清空旧图像
        self.graphics_scene.clear()
        
        # 创建新的图像项
        self.pixmap_item = QGraphicsPixmapItem(pixmap)
        self.graphics_scene.addItem(self.pixmap_item)

        # 更新场景矩形以适应新图片
        self.graphics_view.setSceneRect(self.graphics_scene.itemsBoundingRect())
        
        # 自适应窗口大小
        self.graphics_view.fitInView(self.pixmap_item, Qt.AspectRatioMode.KeepAspectRatio)
        
        # 重置滚动条位置（再次确保）
        self.graphics_view.horizontalScrollBar().setValue(0)
        self.graphics_view.verticalScrollBar().setValue(0)

        # 隐藏加载动画
        self.loading_label.setVisible(False)
        self.loading_movie.stop()
        self.progress_bar.setVisible(False)
        self.statusBar().showMessage(f"Loaded: {os.path.basename(file_path)}")

        # 清理线程（使用新的安全方法）
        self.stop_current_loading()

        self.init_folder_roaming(file_path)
        
        self.update_roam_status()

    def on_info_ready(self, image_info, job_id: str) -> None:
        if job_id != ImageViewer.current_job_id:
            return
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

        # 重新应用简约样式
        font_family = self.settings["appearance"]["ui_font"]
        font_size = self.settings["appearance"]["ui_font_size"]
        self.apply_simple_style(theme_name, font_family, font_size)
        self.refresh_toolbar_icons()

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
                if ext in {'.png', '.jpg', '.jpeg', '.bmp', '.gif', '.tiff', '.tif', '.webp'}:
                    self.open_recent_file(path)   # 复用现有加载逻辑
                    break   # 只取第一张
                else:
                    self.statusBar().showMessage("Unsupported file format", 3000)
    
    def _create_toolbar(self):
        """创建带有自定义图标的工具栏"""
        self.toolbar = self.addToolBar("Tools")
        self.toolbar.setMovable(False)
        self.toolbar.setIconSize(QSize(20, 20))

        # 缩放+ 按钮
        zoom_in_icon = self.themed_icon("zoom-in")
        self.zoom_in_action = QAction(zoom_in_icon, "", self)
        self.zoom_in_action.setToolTip(self.tr("toolbar_zoom_in") + " (Ctrl++)")
        self.zoom_in_action.setShortcut("Ctrl++")
        self.zoom_in_action.triggered.connect(self.zoom_in)
        self.toolbar.addAction(self.zoom_in_action)
        
        # 缩放- 按钮
        zoom_out_icon = self.themed_icon("zoom-out")
        self.zoom_out_action = QAction(zoom_out_icon, "", self)
        self.zoom_out_action.setToolTip(self.tr("toolbar_zoom_out") + " (Ctrl+-)")
        self.zoom_out_action.setShortcut("Ctrl+-")
        self.zoom_out_action.triggered.connect(self.zoom_out)
        self.toolbar.addAction(self.zoom_out_action)
        
        # 1:1 实际大小按钮
        actual_size_icon = self.themed_icon("actual-size")
        self.actual_size_action = QAction(actual_size_icon, "", self)
        self.actual_size_action.setToolTip(self.tr("toolbar_actual_size") + " (Ctrl+0)")
        self.actual_size_action.setShortcut("Ctrl+0")
        self.actual_size_action.triggered.connect(self.actual_size)
        self.toolbar.addAction(self.actual_size_action)
        
        # 适应窗口按钮
        fit_window_icon = self.themed_icon("fit-screen")
        self.fit_window_action = QAction(fit_window_icon, "", self)
        self.fit_window_action.setToolTip(self.tr("toolbar_fit_window") + " (Ctrl+1)")
        self.fit_window_action.setShortcut("Ctrl+1")
        self.fit_window_action.triggered.connect(self.fit_to_window)
        self.toolbar.addAction(self.fit_window_action)

        # 旋转按钮 - 向左旋转
        rotate_left_icon = self.themed_icon("rotate-left")
        self.rotate_left_action = QAction(rotate_left_icon, "", self)
        self.rotate_left_action.setToolTip(self.tr("toolbar_rotate_left") + " (Ctrl+L)")
        self.rotate_left_action.setShortcut("Ctrl+L")
        self.rotate_left_action.triggered.connect(lambda: self.rotate_image(-90))
        self.toolbar.addAction(self.rotate_left_action)
        
        # 旋转按钮 - 向右旋转
        rotate_right_icon = self.themed_icon("rotate-right")
        self.rotate_right_action = QAction(rotate_right_icon, "", self)
        self.rotate_right_action.setToolTip(self.tr("toolbar_rotate_right") + " (Ctrl+R)")
        self.rotate_right_action.setShortcut("Ctrl+R")
        self.rotate_right_action.triggered.connect(lambda: self.rotate_image(90))
        self.toolbar.addAction(self.rotate_right_action)
        
        # 镜像按钮
        mirror_icon = self.themed_icon("mirror-horizontal")
        self.mirror_action = QAction(mirror_icon, "", self)
        self.mirror_action.setToolTip(self.tr("toolbar_mirror") + " (Ctrl+M)")
        self.mirror_action.setShortcut("Ctrl+M")
        self.mirror_action.triggered.connect(self.mirror_image)
        self.toolbar.addAction(self.mirror_action)

        # 上一张图像按钮
        prev_icon = self.themed_icon("chevron-left")
        self.prev_image_action = QAction(prev_icon, "", self)
        self.prev_image_action.setToolTip(self.tr("toolbar_prev_image") + " (Left)")
        self.prev_image_action.setShortcut("Left")
        self.prev_image_action.triggered.connect(lambda: self.navigate_folder_image(-1))
        self.toolbar.addAction(self.prev_image_action)

        # 下一张图像按钮
        next_icon = self.themed_icon("chevron-right")
        self.next_image_action = QAction(next_icon, "", self)
        self.next_image_action.setToolTip(self.tr("toolbar_next_image") + " (Right)")
        self.next_image_action.setShortcut("Right")
        self.next_image_action.triggered.connect(lambda: self.navigate_folder_image(1))
        self.toolbar.addAction(self.next_image_action)
    
    def zoom_in(self):
        """放大图像"""
        if self.pixmap_item:
            self.scale_factor *= 1.2
            self.graphics_view.scale(1.2, 1.2)

    def zoom_out(self):
        """缩小图像"""
        if self.pixmap_item:
            self.scale_factor *= 0.8
            self.graphics_view.scale(0.8, 0.8)

    def actual_size(self):
        """实际大小 (1:1)"""
        if self.pixmap_item:
            # 重置缩放
            self.graphics_view.resetTransform()
            self.scale_factor = 1.0

    def fit_to_window(self):
        """适应窗口大小"""
        if self.pixmap_item:
            self.graphics_view.fitInView(self.pixmap_item, Qt.AspectRatioMode.KeepAspectRatio)
            # 获取当前缩放比例
            view_rect = self.graphics_view.viewport().rect()
            scene_rect = self.graphics_view.mapToScene(view_rect).boundingRect()
            if scene_rect.width() > 0:
                self.scale_factor = view_rect.width() / scene_rect.width()
    
    def rotate_image(self, angle):
        """
        旋转图像
        
        Args:
            angle: 旋转角度（90：向右，-90：向左）
        """
        if not self.pixmap_item or not self.current_image_path:
            return
        
        try:
            # 获取当前 pixmap
            current_pixmap = self.pixmap_item.pixmap()
            if current_pixmap.isNull():
                return
            
            # 创建变换矩阵并旋转
            transform = QTransform()
            transform.rotate(angle)
            
            # 应用旋转变换
            rotated_pixmap = current_pixmap.transformed(
                transform, 
                Qt.TransformationMode.SmoothTransformation
            )
            
            # 更新图像
            self.pixmap_item.setPixmap(rotated_pixmap)
            
            # 更新缩放以适应新方向
            self.graphics_view.fitInView(self.pixmap_item, Qt.AspectRatioMode.KeepAspectRatio)
            
            # 显示状态信息
            direction = "向左" if angle < 0 else "向右"
            self.statusBar().showMessage(f"已旋转 {abs(angle)}度 {direction}")
            
            # 记录旋转状态（可选：用于保存功能）
            if not hasattr(self, 'rotation_history'):
                self.rotation_history = {}
            self.rotation_history[self.current_image_path] = (
                self.rotation_history.get(self.current_image_path, 0) + angle
            ) % 360  # 保持在 0-360 度范围内
            
        except Exception as e:
            self.statusBar().showMessage(f"旋转失败: {str(e)}")

    def reset_canvas(self):
        """重置画布到初始状态"""
        # 重置变换矩阵
        self.graphics_view.resetTransform()
        self.scale_factor = 1.0
        
        # 重置滚动条位置
        self.graphics_view.horizontalScrollBar().setValue(0)
        self.graphics_view.verticalScrollBar().setValue(0)
        
        # 清除选择状态等
        self.graphics_scene.clearSelection()
        
        # 重置视图中心点
        self.graphics_view.centerOn(0, 0)
        
        # 强制更新场景矩形
        self.graphics_view.setSceneRect(self.graphics_scene.itemsBoundingRect())
    
    def mirror_image(self):
        """水平镜像图像"""
        if not self.pixmap_item or not self.current_image_path:
            return
        
        try:
            # 获取当前 pixmap
            current_pixmap = self.pixmap_item.pixmap()
            if current_pixmap.isNull():
                return
            
            # 创建水平镜像变换
            transform = QTransform()
            transform.scale(-1, 1)  # 水平翻转
            transform.translate(-current_pixmap.width(), 0)  # 平移回正确位置
            
            # 应用变换
            mirrored_pixmap = current_pixmap.transformed(
                transform, 
                Qt.TransformationMode.SmoothTransformation
            )
            
            # 更新图像
            self.pixmap_item.setPixmap(mirrored_pixmap)
            
            # 显示状态信息
            self.statusBar().showMessage("已应用水平镜像")
            
            # 记录镜像状态（可选）
            if not hasattr(self, 'mirror_state'):
                self.mirror_state = {}
            
            # 切换镜像状态
            current_mirrored = self.mirror_state.get(self.current_image_path, False)
            self.mirror_state[self.current_image_path] = not current_mirrored
            
        except Exception as e:
            self.statusBar().showMessage(f"镜像失败: {str(e)}")
        if hasattr(self, 'mirror_state') and self.current_image_path in self.mirror_state:
            del self.mirror_state[self.current_image_path]

    def stop_current_loading(self) -> None:
        """安全停止当前加载线程"""
        if self.image_loader:
            self.image_loader.cancel()

        # 2. 如果线程对象还在且确实在运行，才退出/等待
        if self.loader_thread and self.loader_thread.isRunning():
            self.loader_thread.quit()
            if not self.loader_thread.wait(2000):
                self.loader_thread.terminate()
                self.loader_thread.wait()

        # 3. 确定线程已结束后，再清引用
        self.loader_thread = None
        self.image_loader = None

    def closeEvent(self, event):
        """窗口关闭时清理资源"""
        self.stop_current_loading()
        event.accept()

    def themed_icon(self, name: str) -> QIcon:
        """根据当前主题返回不同图标文件"""
        theme = self.settings["appearance"]["theme"]
        path = f"icons/{theme}/{name}.svg"
        return QIcon(path) if QFile.exists(path) else QIcon()
    
    def refresh_toolbar_icons(self):
        """主题一变，重新刷一遍图标"""
        self.zoom_in_action.setIcon(self.themed_icon("zoom-in"))
        self.zoom_out_action.setIcon(self.themed_icon("zoom-out"))
        self.actual_size_action.setIcon(self.themed_icon("actual-size"))
        self.fit_window_action.setIcon(self.themed_icon("fit-screen"))
        self.rotate_left_action.setIcon(self.themed_icon("rotate-left"))
        self.rotate_right_action.setIcon(self.themed_icon("rotate-right"))
        self.mirror_action.setIcon(self.themed_icon("mirror-horizontal"))
        self.prev_image_action.setIcon(self.themed_icon("chevron-left"))
        self.next_image_action.setIcon(self.themed_icon("chevron-right"))

    def init_folder_roaming(self, image_path: str):
        """根据已打开的图片，自动获取同级目录所有图片并初始化漫游列表"""
        folder = os.path.dirname(image_path)
        if not os.path.isdir(folder):
            return

        # 支持的扩展名
        exts = {'.png', '.jpg', '.jpeg', '.bmp', '.gif', '.tiff', '.tif', '.webp'}
        files = sorted(
            [os.path.join(folder, f) for f in os.listdir(folder)
            if os.path.isfile(os.path.join(folder, f)) and
                os.path.splitext(f)[1].lower() in exts],
            key=str.lower
        )

        if not files:
            self.current_folder_images = []
            self.current_folder_index = -1
            return

        self.current_folder_images = files
        try:
            self.current_folder_index = files.index(image_path)
        except ValueError:
            self.current_folder_index = 0
        self.update_roam_status()

    def navigate_folder_image(self, direction: int):
        """方向：+1 下一张，-1 上一张"""
        if not self.current_folder_images or self.current_folder_index < 0:
            return

        new_index = (self.current_folder_index + direction) % len(self.current_folder_images)
        if new_index == self.current_folder_index:
            return

        # 直接复用“打开最近文件”逻辑，无需再建加载器
        self.open_recent_file(self.current_folder_images[new_index])
        self.current_folder_index = new_index

    def update_roam_status(self):
        """刷新右侧漫游信息"""
        if not self.current_folder_images or self.current_folder_index < 0:
            self.roam_label.setVisible(False)
            return

        folder = os.path.basename(os.path.dirname(self.current_image_path))
        curr = self.current_folder_index + 1
        total = len(self.current_folder_images)
        text = self.tr("roam_status",
                    folder=folder,
                    current=curr,
                    total=total)
        self.roam_label.setText(text)
        self.roam_label.setVisible(True)
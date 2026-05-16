from PySide6.QtCore import QSettings
from PySide6.QtWidgets import (QDialog, QDialogButtonBox, QVBoxLayout, QTabWidget, 
                             QWidget, QFormLayout, QGroupBox, QComboBox, QCheckBox, 
                             QSpinBox, QHBoxLayout, QPushButton, QLabel, QSizePolicy)
from PySide6.QtGui import QFontDatabase
from PySide6.QtCore import Qt

class SettingsManager:
    # 默认应用设置
    DEFAULT_SETTINGS = {
        "general": {
            "default_window_state": "normal",
            "show_info_panel": True,
            "recent_files": [],
            "max_recent_files": 5,
            "language": "en_us" 
        },
        "performance": {
            "lazy_loading": True,
            "quick_render": False,
            "skip_exif": False,
            "cache_size": 100,  # MB
        },
        "appearance": {
            "ui_font": "Segoe UI",
            "ui_font_size": 10,
            "theme": "dark" 
        }
    }

    def __init__(self, app_name="InfiniteSight"):
        self.settings = QSettings(app_name, "Settings")
        self.current_settings = self.load_settings()

    def load_settings(self):
        """加载保存的设置，如果没有则使用默认值"""
        settings = {}
        
        # 加载常规设置
        settings["general"] = {
            "default_window_state": self.settings.value("general/default_window_state", 
                                                    self.DEFAULT_SETTINGS["general"]["default_window_state"]),
            "show_info_panel": self.settings.value("general/show_info_panel", 
                                                self.DEFAULT_SETTINGS["general"]["show_info_panel"], type=bool),
            "recent_files": self.settings.value("general/recent_files", 
                                            self.DEFAULT_SETTINGS["general"]["recent_files"], type=list),
            "max_recent_files": self.settings.value("general/max_recent_files", 
                                                self.DEFAULT_SETTINGS["general"]["max_recent_files"], type=int),
            "language": self.settings.value("general/language", 
                                        self.DEFAULT_SETTINGS["general"]["language"], type=str)
        }
        
        # 加载性能设置
        settings["performance"] = {
            "lazy_loading": self.settings.value("performance/lazy_loading", 
                                              self.DEFAULT_SETTINGS["performance"]["lazy_loading"], type=bool),
            "quick_render": self.settings.value("performance/quick_render", 
                                              self.DEFAULT_SETTINGS["performance"]["quick_render"], type=bool),
            "skip_exif": self.settings.value("performance/skip_exif", 
                                           self.DEFAULT_SETTINGS["performance"]["skip_exif"], type=bool),
            "cache_size": self.settings.value("performance/cache_size", 
                                            self.DEFAULT_SETTINGS["performance"]["cache_size"], type=int)
        }
        
        # 加载外观设置
        settings["appearance"] = {
            "ui_font": self.settings.value("appearance/ui_font", 
                                          self.DEFAULT_SETTINGS["appearance"]["ui_font"]),
            "ui_font_size": self.settings.value("appearance/ui_font_size", 
                                               self.DEFAULT_SETTINGS["appearance"]["ui_font_size"], type=int),
            "theme": self.settings.value("appearance/theme",
                                         self.DEFAULT_SETTINGS["appearance"]["theme"], type=str)
        }
        
        return settings

    def save_settings(self):
        """保存当前设置到注册表/配置文件"""
        # 保存常规设置
        self.settings.setValue("general/default_window_state", self.current_settings["general"]["default_window_state"])
        self.settings.setValue("general/show_info_panel", self.current_settings["general"]["show_info_panel"])
        self.settings.setValue("general/recent_files", self.current_settings["general"]["recent_files"])
        self.settings.setValue("general/max_recent_files", self.current_settings["general"]["max_recent_files"])
        self.settings.setValue("general/language", self.current_settings["general"]["language"])
        
        # 保存性能设置
        self.settings.setValue("performance/lazy_loading", self.current_settings["performance"]["lazy_loading"])
        self.settings.setValue("performance/quick_render", self.current_settings["performance"]["quick_render"])
        self.settings.setValue("performance/skip_exif", self.current_settings["performance"]["skip_exif"])
        self.settings.setValue("performance/cache_size", self.current_settings["performance"]["cache_size"])
        
        # 保存外观设置
        self.settings.setValue("appearance/ui_font", self.current_settings["appearance"]["ui_font"])
        self.settings.setValue("appearance/ui_font_size", self.current_settings["appearance"]["ui_font_size"])
        self.settings.setValue("appearance/theme", self.current_settings["appearance"]["theme"])
    
    def update_setting(self, category, key, value):
        """更新单个设置值"""
        if category in self.current_settings and key in self.current_settings[category]:
            self.current_settings[category][key] = value
    
    def add_recent_file(self, file_path):
        """添加最近打开的文件"""
        recent_files = self.current_settings["general"]["recent_files"]
        max_files = self.current_settings["general"]["max_recent_files"]
        
        # 如果文件已经在列表中，先移除
        if file_path in recent_files:
            recent_files.remove(file_path)
        
        # 添加到列表开头
        recent_files.insert(0, file_path)
        
        # 如果超过最大数量，移除最旧的文件
        if len(recent_files) > max_files:
            recent_files = recent_files[:max_files]
        
        self.current_settings["general"]["recent_files"] = recent_files
        self.save_settings()

class SettingsDialog(QDialog):
    def __init__(self, settings_manager, parent=None):
        super().__init__(parent)
        self.settings_manager = settings_manager
        self.parent_window = parent
        self.setWindowTitle("Application Settings")
        self.setMinimumSize(700, 500)
        
        # 创建标签页
        self.tab_widget = QTabWidget()
        
        # 常规设置标签页
        self.general_tab = QWidget()
        self.setup_general_tab()
        self.tab_widget.addTab(self.general_tab, self.tr("settings_general_tab"))
        
        # 性能设置标签页
        self.performance_tab = QWidget()
        self.setup_performance_tab()
        self.tab_widget.addTab(self.performance_tab, self.tr("settings_performance_tab"))
        
        # 外观设置标签页
        self.appearance_tab = QWidget()
        self.setup_appearance_tab()
        self.tab_widget.addTab(self.appearance_tab, self.tr("settings_appearance_tab"))
        
        # 按钮
        self.button_box = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | 
            QDialogButtonBox.StandardButton.Cancel |
            QDialogButtonBox.StandardButton.Apply |
            QDialogButtonBox.StandardButton.RestoreDefaults
        )
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)
        self.button_box.button(QDialogButtonBox.StandardButton.Apply).clicked.connect(self.save_and_apply)
        self.button_box.button(QDialogButtonBox.StandardButton.RestoreDefaults).clicked.connect(self.restore_defaults)
        
        # 主布局
        layout = QVBoxLayout()
        layout.addWidget(self.tab_widget)
        layout.addWidget(self.button_box)
        self.setLayout(layout)
        
        # 加载当前设置
        self.load_current_settings()

    def tr(self, key, **kwargs):
        """使用父窗口的翻译方法"""
        if self.parent_window:
            return self.parent_window.tr(key, **kwargs)
        return key

    def setup_general_tab(self):
        """设置常规选项卡"""
        layout = QVBoxLayout()
        
        # 窗口状态
        window_group = QGroupBox(self.tr("settings_window_behavior"))
        window_layout = QFormLayout()
        
        self.window_state_combo = QComboBox()
        self.window_state_combo.addItems([
            self.tr("window_state_normal"),
            self.tr("window_state_maximized"),
            self.tr("window_state_fullscreen")
        ])
        window_layout.addRow(self.tr("settings_default_window_state"), self.window_state_combo)
        
        self.show_info_check = QCheckBox(self.tr("settings_show_info_panel"))
        window_layout.addRow(self.show_info_check)
        
        # 最近文件
        self.max_recent_spin = QSpinBox()
        self.max_recent_spin.setRange(0, 20)
        window_layout.addRow(self.tr("settings_max_recent_files"), self.max_recent_spin)
        
        window_group.setLayout(window_layout)
        layout.addWidget(window_group)

        # 语言设置
        self.language_combo = QComboBox()
        self.language_combo.addItems([
            "English (en_us)",
            "简体中文 (zh_cn)",
            "繁體中文 (zh_tw)"
        ])
        self.language_combo.setItemData(0, "en_us")
        self.language_combo.setItemData(1, "zh_cn")
        self.language_combo.setItemData(2, "zh_tw")
        window_layout.addRow(self.tr("settings_language"), self.language_combo)

        # 其他设置
        other_group = QGroupBox(self.tr("settings_other_settings"))
        other_layout = QFormLayout()
        
        self.auto_update_check = QCheckBox(self.tr("settings_check_updates"))
        other_layout.addRow(self.auto_update_check)
        
        self.confirm_exit_check = QCheckBox(self.tr("settings_confirm_exit"))
        other_layout.addRow(self.confirm_exit_check)
        
        other_group.setLayout(other_layout)
        layout.addWidget(other_group)
        
        layout.addStretch()
        self.general_tab.setLayout(layout)

    def setup_performance_tab(self):
        """设置性能选项卡"""
        layout = QVBoxLayout()
        
        # 图片处理
        image_group = QGroupBox(self.tr("settings_image_processing"))
        image_layout = QFormLayout()
        
        self.lazy_loading_check = QCheckBox(self.tr("settings_lazy_exif"))
        image_layout.addRow(self.lazy_loading_check)
        
        self.quick_render_check = QCheckBox(self.tr("settings_quick_render"))
        image_layout.addRow(self.quick_render_check)
        
        self.skip_exif_check = QCheckBox(self.tr("settings_skip_exif"))
        image_layout.addRow(self.skip_exif_check)
        
        image_group.setLayout(image_layout)
        layout.addWidget(image_group)
        
        # 缓存设置
        cache_group = QGroupBox(self.tr("settings_caching"))
        cache_layout = QFormLayout()
        
        self.cache_size_spin = QSpinBox()
        self.cache_size_spin.setRange(0, 1000)
        self.cache_size_spin.setSuffix(" MB")
        cache_layout.addRow(self.tr("settings_cache_size"), self.cache_size_spin)
        
        cache_group.setLayout(cache_layout)
        layout.addWidget(cache_group)
        
        layout.addStretch()
        self.performance_tab.setLayout(layout)

    def setup_appearance_tab(self):
        """设置外观选项卡"""
        layout = QVBoxLayout()
        
        # 字体设置
        font_group = QGroupBox(self.tr("settings_font"))
        font_layout = QFormLayout()
        
        self.font_combo = QComboBox()
        self.font_combo.addItems(QFontDatabase.families())
        font_layout.addRow(self.tr("settings_ui_font"), self.font_combo)
        
        self.font_size_spin = QSpinBox()
        self.font_size_spin.setRange(8, 20)
        font_layout.addRow(self.tr("settings_font_size"), self.font_size_spin)
        
        # 主题选择
        self.theme_combo = QComboBox()
        self.theme_combo.addItems([self.tr("settings_theme_dark"),
                                self.tr("settings_theme_light")])
        font_layout.addRow(self.tr("settings_theme"), self.theme_combo)
        
        font_group.setLayout(font_layout)
        layout.addWidget(font_group)
        
        # 预览
        preview_group = QGroupBox(self.tr("settings_preview"))
        preview_layout = QVBoxLayout()
        
        self.preview_label = QLabel(self.tr("settings_preview_text"))
        self.preview_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.preview_label.setMinimumHeight(80)
        self.preview_label.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        preview_layout.addWidget(self.preview_label)
        
        preview_group.setLayout(preview_layout)
        layout.addWidget(preview_group)
        
        self.appearance_tab.setLayout(layout)

    def update_preview(self):
        """更新外观预览"""
        # 获取当前外观设置
        theme = self.settings_manager.current_settings["appearance"]["theme"]
        bg_color = "#2D2D30" if theme == "dark" else "#FFFFFF"
        accent_color = "#007ACC"
        font_family = self.settings_manager.current_settings["appearance"]["ui_font"]
        font_size = self.settings_manager.current_settings["appearance"]["ui_font_size"]
        
        # 应用样式到预览标签
        self.preview_label.setStyleSheet(f"""
            background-color: {bg_color};
            color: {'#E0E0E0' if theme == 'dark' else '#000000'};
            border: 2px solid {accent_color};
            border-radius: 5px;
            font-family: '{font_family}';
            font-size: {font_size}pt;
            padding: 10px;
        """)

    def load_current_settings(self):
        """加载当前设置到UI控件"""
        settings = self.settings_manager.current_settings
        
        # 常规设置
        window_state_map = {"normal": 0, "maximized": 1, "fullscreen": 2}
        self.window_state_combo.setCurrentIndex(
            window_state_map.get(settings["general"]["default_window_state"], 0)
        )
        self.show_info_check.setChecked(settings["general"]["show_info_panel"])
        self.max_recent_spin.setValue(settings["general"]["max_recent_files"])

        # 加载语言设置
        current_lang = settings["general"]["language"]
        if current_lang == "en_us":
            self.language_combo.setCurrentIndex(0)
        elif current_lang == "zh_cn":
            self.language_combo.setCurrentIndex(1)
        elif current_lang == "zh_tw":
            self.language_combo.setCurrentIndex(2)
        
        # 性能设置
        self.lazy_loading_check.setChecked(settings["performance"]["lazy_loading"])
        self.quick_render_check.setChecked(settings["performance"]["quick_render"])
        self.skip_exif_check.setChecked(settings["performance"]["skip_exif"])
        self.cache_size_spin.setValue(settings["performance"]["cache_size"])
        
        # 外观设置
        self.font_combo.setCurrentText(settings["appearance"]["ui_font"])
        self.font_size_spin.setValue(settings["appearance"]["ui_font_size"])
        self.theme_combo.setCurrentIndex(0 if settings["appearance"]["theme"] == "dark" else 1)
        
        # 更新预览
        self.update_preview()

    def get_settings_from_ui(self):
        """从UI控件获取设置值"""
        # 常规设置
        window_state_map = {0: "normal", 1: "maximized", 2: "fullscreen"}
        self.settings_manager.update_setting("general", "default_window_state", 
                                           window_state_map[self.window_state_combo.currentIndex()])
        self.settings_manager.update_setting("general", "show_info_panel", 
                                           self.show_info_check.isChecked())
        self.settings_manager.update_setting("general", "max_recent_files", 
                                           self.max_recent_spin.value())
        
        # 获取语言设置
        lang_index = self.language_combo.currentIndex()
        lang_code = self.language_combo.itemData(lang_index)
        self.settings_manager.update_setting("general", "language", lang_code)
        
        # 性能设置
        self.settings_manager.update_setting("performance", "lazy_loading", 
                                           self.lazy_loading_check.isChecked())
        self.settings_manager.update_setting("performance", "quick_render", 
                                           self.quick_render_check.isChecked())
        self.settings_manager.update_setting("performance", "skip_exif", 
                                           self.skip_exif_check.isChecked())
        self.settings_manager.update_setting("performance", "cache_size", 
                                           self.cache_size_spin.value())
        
        # 外观设置
        self.settings_manager.update_setting("appearance", "ui_font", 
                                           self.font_combo.currentText())
        self.settings_manager.update_setting("appearance", "ui_font_size", 
                                           self.font_size_spin.value())
        self.settings_manager.update_setting("appearance", "theme",
                                            "dark" if self.theme_combo.currentIndex() == 0 else "light")

    def save_and_apply(self):
        """保存设置并应用到主窗口"""
        self.get_settings_from_ui()
        self.settings_manager.save_settings()
        
        # 通知主窗口应用新设置
        if self.parent_window and hasattr(self.parent_window, 'apply_runtime_settings'):
            self.parent_window.apply_runtime_settings()

    def restore_defaults(self):
        """恢复默认设置"""
        self.settings_manager.current_settings = SettingsManager.DEFAULT_SETTINGS.copy()
        self.load_current_settings()
        self.settings_manager.save_settings()
        
        if self.parent_window:
            self.parent_window.apply_runtime_settings()

    def accept(self):
        """点击OK时应用设置并关闭对话框"""
        self.save_and_apply()
        super().accept()
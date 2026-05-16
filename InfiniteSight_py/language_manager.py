import json
import os
from PySide6.QtCore import QLocale

class LanguageManager:
    """多语言管理器"""
    
    def __init__(self, settings_manager):
        """
        初始化语言管理器
        
        Args:
            settings_manager: 设置管理器实例
        """
        self.settings_manager = settings_manager
        self.language = self.settings_manager.current_settings["general"].get("language", "en_us")
        self.translations = {}
        self.load_language(self.language)
    
    def load_language(self, lang_code):
        """
        加载语言文件
        
        Args:
            lang_code: 语言代码 (如'en_us', 'zh_cn')
        """
        lang_dir = "i18n"
        lang_file = os.path.join(lang_dir, f"{lang_code}.json")
        
        try:
            if os.path.exists(lang_file):
                with open(lang_file, 'r', encoding='utf-8') as f:
                    self.translations = json.load(f)
            else:
                # 尝试系统语言
                system_lang = QLocale.system().name().lower().replace("-", "_")
                fallback_file = os.path.join(lang_dir, f"{system_lang}.json")
                
                if os.path.exists(fallback_file):
                    with open(fallback_file, 'r', encoding='utf-8') as f:
                        self.translations = json.load(f)
                else:
                    # 回退到英语
                    en_file = os.path.join(lang_dir, "en_us.json")
                    if os.path.exists(en_file):
                        with open(en_file, 'r', encoding='utf-8') as f:
                            self.translations = json.load(f)
                    else:
                        # 如果连英语文件都没有，使用空字典
                        self.translations = {}
        except Exception as e:
            # 语言文件加载失败
            self.translations = {}
        
    def tr(self, key, **kwargs):
        """
        获取翻译文本，支持格式化参数
        
        Args:
            key: 翻译键
            kwargs: 格式化参数
            
        Returns:
            str: 翻译后的文本
        """
        text = self.translations.get(key, key)
        if kwargs:
            try:
                return text.format(**kwargs)
            except KeyError:
                return text
        return text
    
    def set_language(self, lang_code):
        """设置当前语言并保存到设置"""
        self.language = lang_code
        self.load_language(lang_code)
        self.settings_manager.update_setting("general", "language", lang_code)
        self.settings_manager.save_settings()
import json
import os
from PyQt6.QtCore import QLocale

class LanguageManager:
    def __init__(self, settings_manager):
        self.settings_manager = settings_manager
        self.language = self.settings_manager.current_settings["general"].get("language", "en_us")
        self.translations = {}
        self.load_language(self.language)
    
    def load_language(self, lang_code):
        lang_dir = "i18n"
        # 确保 lang_code 是字符串
        lang_code = str(lang_code)
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
                    # 最终回退到英语
                    en_file = os.path.join(lang_dir, "en_us.json")
                    if os.path.exists(en_file):
                        with open(en_file, 'r', encoding='utf-8') as f:
                            self.translations = json.load(f)
                    else:
                        # 如果连英语文件都没有，使用空字典
                        self.translations = {}
        except Exception as e:
            print(f"Error loading language file: {e}")
            self.translations = {}
        
    def tr(self, key, **kwargs):
        """获取翻译文本，支持格式化参数"""
        text = self.translations.get(key, key)
        if kwargs:
            try:
                return text.format(**kwargs)
            except KeyError:
                return text
        return text
    
    def set_language(self, lang_code):
        self.language = lang_code
        self.load_language(lang_code)
        self.settings_manager.update_setting("general", "language", lang_code)
        self.settings_manager.save_settings()
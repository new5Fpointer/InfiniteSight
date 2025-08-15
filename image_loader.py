import os
from datetime import datetime
from PyQt6.QtCore import QObject, pyqtSignal
from PyQt6.QtGui import QPixmap
from image_cache import is_very_large, load_thumbnail

class ImageLoader(QObject):
    """
    后台图像加载器
    
    信号:
        finished: 加载完成信号 (pixmap, 文件路径)
        info_ready: 图像信息准备好信号 (图像信息字典)
        progress: 进度更新信号 (0-100)
    """
    finished = pyqtSignal(object, str)
    info_ready = pyqtSignal(object)
    progress = pyqtSignal(int)

    def __init__(self, file_path, performance_settings):
        """
        初始化图像加载器
        
        Args:
            file_path: 图像文件路径
            performance_settings: 性能设置字典
        """
        super().__init__()
        self.file_path = file_path
        self.performance_settings = performance_settings
        self.canceled = False

    def run(self):
        """执行图像加载任务"""
        try:
            # 1) 大图检测 - 使用缩略图优化加载
            if is_very_large(self.file_path):
                pixmap = load_thumbnail(self.file_path, max_edge=4096)
                if pixmap.isNull():
                    raise Exception("Thumbnail generation failed")
            else:
                # 2) 小图正常加载
                pixmap = QPixmap(self.file_path)
                if pixmap.isNull():
                    raise Exception("Failed to load image")

            self.progress.emit(30)

            # 收集图像信息
            image_info = self.collect_image_info(self.file_path)
            self.progress.emit(70)

            if self.canceled:
                return

            # 发出加载完成信号
            self.finished.emit(pixmap, self.file_path)
            self.info_ready.emit(image_info)
            self.progress.emit(100)

        except Exception as e:
            # 加载失败时发送空图像
            self.finished.emit(None, f"Error: {str(e)}")

    def collect_image_info(self, file_path):
        """
        收集图像信息
        
        Args:
            file_path: 图像文件路径
            
        Returns:
            dict: 包含文件信息、图像信息和EXIF数据的字典
        """
        info = {
            "file_info": {},
            "image_info": {},
            "exif_info": {}
        }
        
        try:
            # 基础文件信息
            info["file_info"] = {
                "File Name": os.path.basename(file_path),
                "Path": file_path,
                "Size": f"{os.path.getsize(file_path)/1024:.2f} KB",
                "Modified": datetime.fromtimestamp(os.path.getmtime(file_path)).strftime("%Y-%m-%d %H:%M:%S")
            }
            
            # 如果跳过EXIF解析，直接返回
            if self.performance_settings["skip_exif"]:
                return info
                
            # 按需加载PIL模块
            if not self.performance_settings["lazy_loading"] or self.load_pil():
                from PIL import Image
                
                with Image.open(file_path) as img:
                    # 图片技术信息
                    info["image_info"] = {
                        "Format": img.format if img.format else "Unknown",
                        "Color Mode": img.mode,
                        "Dimensions": f"{img.width} x {img.height} pixels",
                        "DPI": f"{img.info.get('dpi', (72, 72))[0]} x {img.info.get('dpi', (72, 72))[1]}"
                    }
                    
                    # EXIF元数据
                    if not self.performance_settings["skip_exif"]:
                        exif_info = self.get_exif_data(img)
                        if exif_info:
                            info["exif_info"] = exif_info
        except Exception as e:
            info["error"] = f"Could not read image info: {str(e)}"
        
        return info

    def load_pil(self):
        """按需加载PIL模块"""
        try:
            # 延迟导入PIL模块
            from PIL import Image
            from PIL.ExifTags import TAGS, GPSTAGS
            return True
        except ImportError:
            return False

    def get_exif_data(self, image):
        """
        提取EXIF数据
        
        Args:
            image: PIL图像对象
            
        Returns:
            dict: EXIF数据字典
        """
        try:
            exif_data = {}
            info = image.getexif()
            if not info:
                return None
                
            for tag_id, value in info.items():
                tag = TAGS.get(tag_id, tag_id)
                
                # 处理特殊值类型
                if isinstance(value, bytes):
                    try:
                        value = value.decode('utf-8', errors='replace')
                    except:
                        value = "Binary data"
                
                # 处理GPS信息
                if tag == "GPSInfo":
                    gps_data = {}
                    for gps_tag in value:
                        sub_tag = GPSTAGS.get(gps_tag, gps_tag)
                        gps_data[sub_tag] = value[gps_tag]
                    exif_data[tag] = gps_data
                else:
                    exif_data[tag] = value
                    
            return exif_data
        except:
            return None

    def cancel(self):
        """取消加载任务"""
        self.canceled = True
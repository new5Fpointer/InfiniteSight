import os
os.add_dll_directory(os.path.abspath('vips/bin'))
import pyvips
from PyQt6.QtGui import QPixmap, QImage
from PyQt6.QtCore import QDir

CACHE_DIR = QDir.tempPath() + "/InfiniteSight_cache"
os.makedirs(CACHE_DIR, exist_ok=True)

def is_very_large(file_path, threshold_bytes=256 * 1024 * 1024):
    """简单规则：> 256 MB 就认为是大图"""
    return os.path.getsize(file_path) > threshold_bytes

def load_thumbnail(file_path, max_edge=4096):
    """返回 QPixmap 缩略图（内存友好）"""
    cache_key = f"{abs(hash(file_path))}_{max_edge}.jpg"
    cache_file = os.path.join(CACHE_DIR, cache_key)

    if os.path.exists(cache_file):
        return QPixmap(cache_file)

    # 用 vips 生成缩略图
    img = pyvips.Image.thumbnail(file_path, max_edge)
    img.write_to_file(cache_file)
    return QPixmap(cache_file)

def load_tile(file_path, x=0, y=0, w=2048, h=2048):
    """返回指定区域的 QPixmap（后续放大查看用）"""
    img = pyvips.Image.new_from_file(file_path)
    tile = img.crop(x, y, w, h)
    buf = tile.write_to_buffer(".png")
    return QPixmap.fromImage(QImage.fromData(buf))
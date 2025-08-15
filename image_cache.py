import os
os.add_dll_directory(os.path.abspath('vips/bin'))
import pyvips
from PyQt6.QtGui import QPixmap, QImage
from PyQt6.QtCore import QDir

# 设置缓存目录
CACHE_DIR = QDir.tempPath() + "/InfiniteSight_cache"
os.makedirs(CACHE_DIR, exist_ok=True)

def is_very_large(file_path, threshold_bytes=256 * 1024 * 1024):
    """
    检查文件是否为大图（默认阈值256MB）
    
    Args:
        file_path: 文件路径
        threshold_bytes: 大小阈值（字节）
        
    Returns:
        bool: 文件大小是否超过阈值
    """
    return os.path.getsize(file_path) > threshold_bytes

def load_thumbnail(file_path, max_edge=4096):
    """
    加载缩略图（内存友好）
    
    Args:
        file_path: 文件路径
        max_edge: 最大边长
        
    Returns:
        QPixmap: 缩略图对象
    """
    # 生成缓存键名（文件哈希 + 最大边长）
    cache_key = f"{abs(hash(file_path))}_{max_edge}.jpg"
    cache_file = os.path.join(CACHE_DIR, cache_key)

    # 如果缓存存在，直接返回
    if os.path.exists(cache_file):
        return QPixmap(cache_file)

    # 使用vips生成缩略图并保存到缓存
    img = pyvips.Image.thumbnail(file_path, max_edge)
    img.write_to_file(cache_file)
    return QPixmap(cache_file)

def load_tile(file_path, x=0, y=0, w=2048, h=2048):
    """
    加载指定区域的图像块（用于放大查看）
    
    Args:
        file_path: 文件路径
        x: 起始X坐标
        y: 起始Y坐标
        w: 宽度
        h: 高度
        
    Returns:
        QPixmap: 图像块对象
    """
    img = pyvips.Image.new_from_file(file_path)
    tile = img.crop(x, y, w, h)
    buf = tile.write_to_buffer(".png")
    return QPixmap.fromImage(QImage.fromData(buf))
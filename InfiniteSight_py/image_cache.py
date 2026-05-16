"""图像缓存与分块读取工具（基于 libvips）"""
from __future__ import annotations

import os

from PySide6.QtCore import QDir
from PySide6.QtGui import QImage, QPixmap
import pyvips

CACHE_DIR = os.path.join(QDir.tempPath(), "InfiniteSight_cache")
os.makedirs(CACHE_DIR, exist_ok=True)


def is_very_large(file_path: str, threshold_bytes: int = 256 << 20) -> bool:
    """文件大小是否超过阈值"""
    return os.path.getsize(file_path) > threshold_bytes


def load_thumbnail(file_path: str, max_edge: int = 4096) -> QPixmap:
    """生成或读取缓存缩略图"""
    cache_key = f"{abs(hash(file_path))}_{max_edge}.jpg"
    cache_file = os.path.join(CACHE_DIR, cache_key)

    if not os.path.exists(cache_file):
        img = pyvips.Image.thumbnail(file_path, max_edge)
        img.write_to_file(cache_file)

    return QPixmap(cache_file)


def load_tile(
    file_path: str,
    x: int = 0,
    y: int = 0,
    w: int = 2048,
    h: int = 2048,
) -> QPixmap:
    """读取图像指定区域"""
    img = pyvips.Image.new_from_file(file_path)
    tile = img.crop(x, y, w, h)
    buf = tile.write_to_buffer(".png")
    return QPixmap.fromImage(QImage.fromData(buf))
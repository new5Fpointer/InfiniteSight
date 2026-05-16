import os
import sys
os.environ['PATH'] += os.pathsep + os.path.abspath('vips/bin')
from PySide6.QtWidgets import QApplication
from image_viewer import ImageViewer
os.environ["QT_IMAGEIO_MAXALLOC"] = "4096"

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    app.setApplicationName("InfiniteSight")

    window = ImageViewer()
    window.show()
    sys.exit(app.exec())
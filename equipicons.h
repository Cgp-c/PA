#ifndef EQUIPICONS_H
#define EQUIPICONS_H

#include <QPixmap>
#include <QImage>
#include <QHash>
#include <QString>

// 装备图标：从 Qt 资源 :/equip/<file> 加载并按尺寸缓存。
// 棋盘装备框 / 回收槽 / 掉落区 / 拖拽幽灵 / 合成树窗口共用。
//
// 深色界面下的可见性处理（缩放后逐像素处理）：
//   1. 增亮：非透明像素 RGB ×1.28（原图整体偏浅、对比度低）
//   2. 描边：透明像素若邻接不透明像素则填深色轮廓，
//      让浅色图标在深色背景上有清晰边界
// 资源缺失时返回空 QPixmap，绘制为空不崩溃。
inline const QPixmap& equipIcon(const char* file, int size)
{
    static QHash<QString, QPixmap> cache;
    QString key = QString("%1_%2").arg(QString::fromLatin1(file)).arg(size);
    auto it = cache.find(key);
    if (it != cache.end()) return *it;

    QPixmap pm(QStringLiteral(":/equip/") + QString::fromLatin1(file));
    if (pm.isNull()) {
        static const QPixmap nullPm;
        return *cache.insert(key, nullPm);
    }

    // 先缩放到目标尺寸再处理：描边保持 1px 清晰
    QImage img = pm.scaled(size, size, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation)
                     .toImage()
                     .convertToFormat(QImage::Format_ARGB32);
    const int w = img.width(), h = img.height();

    // 1) 增亮非透明像素
    for (int y = 0; y < h; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            const QRgb c = line[x];
            const int a = qAlpha(c);
            if (a == 0) continue;
            line[x] = qRgba(qMin(255, qRed(c) * 5 / 4),
                            qMin(255, qGreen(c) * 5 / 4),
                            qMin(255, qBlue(c) * 5 / 4),
                            a);
        }
    }

    // 2) 深色描边（检测 4 邻域的不透明像素）
    QImage marked = img;
    const QRgb outline = qRgba(38, 28, 18, 255);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (qAlpha(img.pixel(x, y)) > 40) continue;
            bool edge = false;
            if (x > 0     && qAlpha(img.pixel(x - 1, y)) > 100) edge = true;
            if (x < w - 1 && qAlpha(img.pixel(x + 1, y)) > 100) edge = true;
            if (y > 0     && qAlpha(img.pixel(x, y - 1)) > 100) edge = true;
            if (y < h - 1 && qAlpha(img.pixel(x, y + 1)) > 100) edge = true;
            if (edge) marked.setPixel(x, y, outline);
        }
    }

    return *cache.insert(key, QPixmap::fromImage(marked));
}

#endif // EQUIPICONS_H

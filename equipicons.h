#ifndef EQUIPICONS_H
#define EQUIPICONS_H

#include <QPixmap>
#include <QHash>
#include <QString>

// 装备图标：从 Qt 资源 :/equip/<file> 加载并按尺寸缓存。
// 棋盘装备框 / 回收槽 / 掉落区 / 拖拽幽灵 / 合成树窗口共用。
// 资源缺失时返回空 QPixmap，绘制为空不崩溃。
inline const QPixmap& equipIcon(const char* file, int size)
{
    static QHash<QString, QPixmap> cache;
    QString key = QString("%1_%2").arg(QString::fromLatin1(file)).arg(size);
    auto it = cache.find(key);
    if (it != cache.end()) return *it;
    QPixmap pm(QStringLiteral(":/equip/") + QString::fromLatin1(file));
    return *cache.insert(key, pm.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

#endif // EQUIPICONS_H

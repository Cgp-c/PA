#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成装备图标：在 32x32 逻辑画布上用多边形/线段作画，统一深色描边，
再 4x 最近邻放大到 128x128，透明背景。与 src/unit 现有素材尺寸、像素密度一致。"""

from PIL import Image, ImageDraw
import os

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "src", "unit")
SCALE = 4
W = H = 32


class Canvas:
    def __init__(self):
        self.im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)

    def poly(self, pts, color):
        self.d.polygon(pts, fill=color)

    def line(self, pts, color, width=1):
        self.d.line(pts, fill=color, width=width)

    def px(self, x, y, color):
        self.d.point((x, y), fill=color)

    def rect(self, x0, y0, x1, y1, color):
        self.d.rectangle([x0, y0, x1, y1], fill=color)

    def outline(self, color, alpha_threshold=10):
        """按 alpha 膨胀 1px 生成描边，不吞掉相邻色块。"""
        a = self.im.getchannel("A")
        solid = Image.new("RGBA", (W, H), color + (255,))
        mask = a.filter(__import__("PIL.ImageFilter", fromlist=["MinFilter"]).MinFilter(3))
        # MinFilter 后：原透明区域中“周围有实体”的像素变亮，取反作为描边掩码
        from PIL import ImageOps
        edge = ImageOps.invert(mask.point(lambda v: 255 if v > alpha_threshold else 0))
        # 只描外圈：原图 alpha 低的才上描边色
        edge = Image.composite(Image.new("L", (W, H), 0), edge, a.point(lambda v: 255 if v > 40 else 0))
        self.im = Image.composite(solid, self.im, edge)
        self.d = ImageDraw.Draw(self.im)

    def save(self, name):
        out = self.im.resize((W * SCALE, H * SCALE), Image.NEAREST)
        os.makedirs(OUT_DIR, exist_ok=True)
        path = os.path.join(OUT_DIR, name)
        out.save(path)
        print("saved", path)


def mix(c1, c2, t):
    return tuple(int(a + (b - a) * t) for a, b in zip(c1, c2))


# ═══════════════════════════════════════════════════════════════
# 通用部件
# ═══════════════════════════════════════════════════════════════

def draw_handle(cv, base_xy, up=True):
    """剑柄：位于 (x,y)，向上或向下伸出的握把 + 圆头。"""
    x, y = base_xy
    B, Db = (109, 76, 47), (70, 46, 26)
    if up:
        cv.rect(x - 1, y - 7, x + 1, y, B)
        cv.rect(x, y - 7, x + 1, y, mix(B, (255, 255, 255), 0.25))
        cv.d.ellipse([x - 2, y - 10, x + 2, y - 6], fill=Db)
    else:
        cv.rect(x - 1, y, x + 1, y + 7, B)
        cv.rect(x, y, x + 1, y + 7, mix(B, (255, 255, 255), 0.25))
        cv.d.ellipse([x - 2, y + 6, x + 2, y + 10], fill=Db)


def blade(cv, pts, dark, mid, light, white):
    """四边形刃身 + 左侧亮边 + 脊线高光。pts = [tip, base1, base2]（左刃缘在左）。"""
    tip, b1, b2 = pts
    # 主体
    cv.poly([tip, b1, b2], mid)
    # 右半暗面
    cv.poly([tip, ((tip[0]+b2[0])//2, (tip[1]+b2[1])//2), b2], dark)
    # 左缘亮线
    cv.line([tip, b1], light)
    # 脊线白
    cv.line([((tip[0]*3+b1[0])//4, (tip[1]*3+b1[1])//4),
             ((tip[0]+b1[0]*3)//4, (tip[1]+b1[1]*3)//4)], white)


# ═══════════════════════════════════════════════════════════════
# 1. 极速战刀 swift_blade — 青色弯刀 + 残影
# ═══════════════════════════════════════════════════════════════
def gen_swift_blade():
    cv = Canvas()
    Dk, D, M, L, Wt = (24, 96, 116), (31, 127, 150), (60, 195, 217), (143, 240, 248), (242, 251, 253)
    S = (170, 235, 245, 170)
    # 弯刀刃身（三段折线逼近弧线），尖朝右上
    seg = [ (26, 4), (20, 10), (13, 15), (8, 21) ]      # 刃脊路径
    for i in range(len(seg) - 1):
        x0, y0 = seg[i]; x1, y1 = seg[i+1]
        thick = 1 + i  # 越靠柄越宽
        cv.line([(x0, y0), (x1, y1)], M, thick + 1)
        cv.line([(x0 - 1, y0), (x1 - 1, y1)], L, 1)      # 左亮缘
        cv.line([(x0 + 1, y0 + 1), (x1 + 1, y1 + 1)], D, 1)
    cv.line([(27, 4), (28, 3)], Wt, 1)                    # 尖端白
    cv.line([(25, 6), (18, 12)], Wt, 1)                   # 脊高光
    # 护手（金）
    G, g = (217, 164, 65), (154, 111, 36)
    cv.poly([(5, 20), (9, 24), (7, 26), (3, 22)], G)
    cv.poly([(5, 20), (9, 24), (8, 25), (4, 21)], g)
    # 柄向左下
    cv.line([(4, 23), (1, 26)], (109, 76, 47), 2)
    cv.d.ellipse([0, 25, 3, 28], fill=(70, 46, 26))
    # 残影：三条与刃平行的淡青短线
    for (sx, sy, ex, ey) in [(20, 2, 26, 0), (14, 7, 21, 4), (8, 12, 15, 8)]:
        cv.line([(sx, sy), (ex, ey)], S, 1)
    cv.outline((16, 40, 50))
    cv.save("swift_blade.png")


# ═══════════════════════════════════════════════════════════════
# 2/3. 手套（红=攻速手套，青=疾风手套）
# ═══════════════════════════════════════════════════════════════
def gen_glove(name, dark, mid, light, white, streak, is_gale):
    cv = Canvas()
    # 正面视角手套：4 根立着的手指 + 手掌 + 右侧拇指 + 下方宽腕带
    # 四指（每根 3px 宽，顶部圆角）
    for i in range(4):
        x0 = 11 + i * 3
        shade = mid if i < 3 else mix(mid, dark, 0.4)
        cv.rect(x0, 8, x0 + 2, 14, shade)
        cv.d.ellipse([x0, 6, x0 + 2, 9], fill=shade)
        cv.line([(x0, 8), (x0, 13)], light)                       # 每指左受光
        cv.px(x0 + 1, 7, white)
    # 指缝
    for i in range(3):
        x0 = 10 + i * 3
        cv.line([(x0 + 3, 8), (x0 + 3, 14)], mix(dark, (0, 0, 0), 0.3))
    # 手掌
    cv.poly([(10, 14), (23, 14), (24, 22), (9, 22)], mid)
    cv.poly([(10, 14), (16, 14), (16, 22), (9, 22)], light)
    cv.poly([(20, 14), (24, 17), (24, 22), (19, 22)], dark)
    # 拇指（右侧立起）
    cv.d.ellipse([22, 10, 27, 17], fill=mix(mid, dark, 0.35))
    cv.d.ellipse([23, 11, 25, 13], fill=light)
    # 掌心高光
    cv.px(12, 17, white); cv.px(12, 18, white)
    # 宽腕带 + 金扣
    cv.rect(8, 22, 25, 27, mix(mid, (30, 20, 10), 0.5))
    cv.rect(8, 22, 25, 23, mix(mid, (255, 255, 255), 0.35))
    cv.rect(15, 23, 18, 26, (217, 164, 65))
    cv.px(16, 24, (255, 224, 130))
    if is_gale:
        # 白色旋风弧（绕手套断续一圈）
        for (a, b) in [((6, 4), (14, 1)), ((26, 3), (30, 10)),
                       ((30, 20), (26, 28)), ((10, 30), (3, 27)),
                       ((16, 1), (22, 0)), ((1, 12), (2, 19))]:
            cv.line([a, b], streak, 1)
    else:
        # 橙色速度线（左侧三条水平）
        for y in (8, 14, 20):
            cv.line([(1, y), (6, y)], streak, 1)
    cv.outline(mix(dark, (0, 0, 0), 0.5))
    cv.save(name)


# ═══════════════════════════════════════════════════════════════
# 4. 战马 warhorse — 骑士棋子马头（朝左）+ 缰绳
# ═══════════════════════════════════════════════════════════════
def gen_warhorse():
    cv = Canvas()
    Dk, D, M, L = (56, 34, 16), (107, 74, 42), (143, 106, 61), (181, 138, 85)
    # ── 骑士棋子式马头（朝左），自上而下：耳-头-口鼻-颈-底座 ──
    # 耳朵（两支，尖端朝上）
    cv.poly([(15, 5), (14, 0), (17, 4)], M)
    cv.poly([(19, 5), (20, 0), (22, 4)], D)
    # 头顶与额头
    cv.poly([(14, 5), (23, 5), (24, 12), (14, 12)], M)
    cv.poly([(14, 5), (18, 5), (18, 12), (14, 12)], L)
    # 口鼻（向左大幅突出，骑士轮廓的关键）
    cv.poly([(14, 9), (3, 11), (2, 15), (5, 17), (15, 15)], M)
    cv.poly([(14, 9), (3, 11), (2, 15), (4, 16), (14, 13)], L)
    cv.px(3, 13, mix(L, (255, 255, 255), 0.35))                # 鼻尖亮
    cv.px(2, 14, Dk)                                           # 鼻孔
    # 下颚与颈（右缘竖直，左缘内收——棋子剪影）
    cv.poly([(15, 15), (23, 13), (25, 20), (24, 27), (9, 27), (7, 20), (10, 16)], M)
    cv.poly([(10, 16), (7, 20), (9, 27), (13, 27), (13, 17)], L)
    cv.poly([(19, 14), (24, 16), (24, 27), (19, 27)], D)
    # 鬃毛：沿后颈右缘的深色锯齿
    for i in range(4):
        y = 10 + i * 4
        cv.poly([(24, y), (28, y + 1), (24, y + 4)], Dk if i % 2 else D)
    # 眼睛（大而清晰）
    cv.rect(17, 9, 18, 10, (20, 12, 6))
    cv.px(17, 9, (255, 255, 255))
    # 金缰绳：口鼻 → 眼下 → 颈
    cv.line([(4, 14), (10, 16), (17, 14), (22, 18)], (217, 164, 65))
    cv.px(10, 16, (255, 224, 130))
    # 底座（棋子 pedestal）
    cv.rect(6, 27, 26, 30, D)
    cv.rect(6, 27, 26, 28, L)
    cv.outline(Dk)
    cv.save("warhorse.png")


# ═══════════════════════════════════════════════════════════════
# 5. 反伤铠甲 thorns_armor — 深钢胸甲 + 肩刺 + 红芯
# ═══════════════════════════════════════════════════════════════
def gen_thorns_armor():
    cv = Canvas()
    Dk, D, M, L = (24, 24, 34), (47, 47, 58), (86, 86, 100), (139, 139, 156)
    R, r = (196, 60, 52), (120, 24, 20)
    # 左右肩刺（向外上方的三角）
    for sx, dire in ((4, -1), (27, 1)):
        cv.poly([(sx, 12), (sx + dire * 5, 5), (sx + dire, 15)], M)
        cv.line([(sx, 12), (sx + dire * 4, 6)], L)
    # 肩甲球
    cv.d.ellipse([3, 10, 10, 17], fill=M)
    cv.d.ellipse([21, 10, 28, 17], fill=D)
    cv.px(5, 12, L); cv.px(6, 12, L)
    # 胸甲主体
    cv.poly([(7, 13), (24, 13), (26, 20), (16, 29), (6, 20)], M)
    cv.poly([(7, 13), (15, 14), (15, 28), (6, 20)], L)                   # 左亮
    cv.poly([(20, 13), (26, 20), (16, 29), (16, 20)], D)                 # 右暗
    # 中缝
    cv.line([(15, 14), (15, 28)], Dk)
    # 红芯宝石（倒三角）
    cv.poly([(13, 17), (19, 17), (16, 23)], R)
    cv.poly([(14, 18), (18, 18), (16, 22)], mix(R, (255, 255, 255), 0.35))
    cv.px(16, 26, r)
    # 下缘刺（小三角朝下）
    for x in (9, 13, 17, 21):
        cv.poly([(x - 1, 24 if x != 16 else 27), (x, 28 if x != 16 else 31), (x + 1, 24 if x != 16 else 27)], D)
    cv.outline(Dk)
    cv.save("thorns_armor.png")


# ═══════════════════════════════════════════════════════════════
# 6. 生命重甲 vitality_armor — 金色重甲 + 红心
# ═══════════════════════════════════════════════════════════════
def gen_vitality_armor():
    cv = Canvas()
    Dk, D, M, L, Wt = (66, 46, 12), (138, 106, 28), (201, 152, 46), (236, 198, 85), (255, 240, 168)
    R, r, p = (212, 48, 48), (130, 20, 24), (255, 130, 120)
    # 肩甲（大圆）
    cv.d.ellipse([2, 9, 11, 18], fill=M)
    cv.d.ellipse([20, 9, 29, 18], fill=D)
    cv.px(4, 11, Wt); cv.px(5, 11, Wt)
    # 胸甲主体（宽梯形）
    cv.poly([(6, 12), (25, 12), (27, 22), (16, 30), (4, 22)], M)
    cv.poly([(6, 12), (15, 13), (15, 29), (4, 22)], L)                   # 左亮
    cv.poly([(21, 12), (27, 22), (16, 30), (16, 21)], D)                 # 右暗
    # 领口弧
    cv.line([(10, 12), (16, 15), (22, 12)], Dk)
    # 红心（像素心形，中心 16,21 半径 4）
    heart = [(16,18),(15,19),(17,19),(14,20),(18,20),
             (13,21),(19,21),(14,22),(18,22),(15,23),(17,23),(16,24)]
    for (x, y) in heart:
        cv.px(x, y, R); cv.px(x + 1, y, R)
    cv.px(15, 20, p)                                                     # 心高光
    cv.px(16, 19, p)
    cv.px(15, 18, r); cv.px(18, 18, r)
    # 铆钉
    for (x, y) in [(8, 15), (23, 15), (8, 20), (24, 20), (16, 27)]:
        cv.px(x, y, Wt)
    # 下摆甲片
    cv.poly([(8, 26), (12, 31), (8, 31)], D)
    cv.poly([(20, 26), (24, 31), (20, 31)], D)
    cv.outline(Dk)
    cv.save("vitality_armor.png")


# ═══════════════════════════════════════════════════════════════
# 7. 复活石 revive_stone — 立起的圆形符文石 + 十字复活符文
# ═══════════════════════════════════════════════════════════════
def gen_revive_stone():
    cv = Canvas()
    Dk, D, M, L, Wt = (24, 16, 40), (58, 47, 82), (90, 72, 128), (129, 104, 176), (178, 152, 224)
    C, c = (103, 233, 246), (51, 150, 209)
    # 石体（竖椭圆）
    cv.d.ellipse([6, 6, 25, 27], fill=M)
    cv.d.ellipse([8, 7, 20, 20], fill=L)                                 # 左上受光
    cv.d.ellipse([18, 14, 24, 26], fill=D)                               # 右下暗
    cv.d.ellipse([10, 8, 14, 12], fill=Wt)                               # 高光斑
    # 石面刻痕（青色十字复活符文，带深青描边感）
    cross = [(15, 10), (16, 10), (15, 11), (16, 11),                     # 竖上部
             (15, 21), (16, 21), (15, 22), (16, 22),                     # 竖下部
             (12, 15), (13, 15), (12, 16), (13, 16),                     # 横左
             (18, 15), (19, 15), (18, 16), (19, 16)]                     # 横右
    for (x, y) in cross:
        cv.px(x, y, C)
    cv.rect(15, 12, 16, 20, C)                                           # 竖梁
    cv.rect(14, 15, 17, 16, C)                                           # 横梁
    for (x, y) in [(15, 19), (16, 19), (13, 16), (17, 15)]:
        cv.px(x, y, c)
    # 符文外圈微光点
    for (x, y) in [(10, 13), (22, 19), (16, 8), (16, 25)]:
        cv.px(x, y, mix(C, (255, 255, 255), 0.4))
    # 底部绿辉（浮空感）
    G, g = (110, 230, 160, 130), (170, 255, 210, 170)
    cv.line([(10, 30), (14, 29)], G)
    cv.line([(18, 29), (22, 30)], G)
    cv.px(16, 29, g)
    cv.outline(Dk)
    cv.save("revive_stone.png")


if __name__ == "__main__":
    gen_swift_blade()
    gen_glove("speed_gloves.png", (120, 26, 22), (196, 44, 34), (240, 96, 72), (255, 190, 170),
              (255, 150, 60, 190), is_gale=False)
    gen_glove("gale_gloves.png", (18, 96, 120), (36, 160, 186), (92, 214, 232), (220, 250, 255),
              (200, 245, 252, 200), is_gale=True)
    gen_warhorse()
    gen_thorns_armor()
    gen_vitality_armor()
    gen_revive_stone()
    print("all done")

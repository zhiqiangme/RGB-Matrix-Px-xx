# HUB75 SM5368 Notes

## 目标

这个仓库里新增了一套针对 `96x48 1/24 scan` 面板的特殊适配，用来解决两类问题：

1. `SM5368` 行译码
2. `R/B` 颜色数据线对调

对应的 profile 名称是：

```cmake
96X48_1_24_SM5368
```

## 这次改动的核心思路

在实现上，把问题拆成两个互相独立的层：

1. 行选择怎么做
2. 颜色数据怎么映射

这样做的好处是：

1. `SM5368` 只负责“扫哪一行”
2. `R/B` 对调只负责“颜色数据送到哪一根线上”
3. 普通 `HUB75` 的像素排布逻辑可以继续复用

这也是迁移到其他工程时最推荐的做法。

## SM5368 做了什么

普通 `HUB75` 面板一般把 `A/B/C/D/E` 当成二进制地址线使用。

`SM5368` 不是这个模式，而是使用三根线：

1. `A = row clk`
2. `B = BK`
3. `C = row data`

也就是说，它不是“直接输出当前行地址”，而是“通过移位时序把行选择推进到下一行”。

所以实现时不能只改一个普通的地址值，而要单独做一套行控制逻辑。

## 在本工程中的实现方式

### 1. 新增 profile

在 `CMakeLists.txt` 里新增：

```cmake
96X48_1_24_SM5368
```

这个 profile 的参数是：

1. `MATRIX_PANEL_WIDTH = 96`
2. `MATRIX_PANEL_HEIGHT = 48`
3. `ROWSEL_N_PINS = 3`
4. `PANEL_SCAN_DEPTH = 24`
5. `PANEL_DEFINE_VALUE = HUB75_SM5368_ABC`
6. `SWAP_RB_PINS_VALUE = true`

含义是：

1. 面板尺寸是 `96x48`
2. 扫描方式是 `1/24`
3. 行选择只使用 `A/B/C` 三根线
4. 自动开启 `SM5368` 行译码
5. 自动开启 `R/B` 对调

### 2. 新增面板宏

在 `src/hub75.hpp` 里新增了编译期开关：

```cpp
HUB75_SM5368_ABC
```

它表示：

1. `A = row clk`
2. `B = BK`
3. `C = row data`

另外增加了：

```cpp
SWAP_RB_PINS
```

它表示：

1. 不改物理 GPIO 顺序
2. 只在软件里交换 `R/B` 的数据位映射

### 3. 行选择逻辑改成两拍输出

在 `src/hub75.cpp` 的 `encode_row_address()` 中，`SM5368` 分支不再返回普通二进制地址，而是返回两拍控制字：

1. 第一拍：`A=0, B=1, C=data`
2. 第二拍：`A=1, B=1, C=data`

处理逻辑是：

1. 第 0 行时向 `C` 注入一个 `1`
2. 之后每一行只继续时钟推进
3. `B` 在两拍期间保持有效

这个逻辑对应 ESP32 版本里：

```cpp
if (m_cfg.line_decoder == HUB75_I2S_CFG::SM5368)
{
    uint16_t c = (row_idx == 0) ? BIT_C : 0x0000;
    row[...] |= c | BIT_B;
    row[...] |= c | BIT_A | BIT_B;
}
```

### 4. PIO 单独增加 SM5368 行程序

在 `src/hub75.pio` 中单独增加了：

1. `hub75_row_sm5368_abc`
2. `hub75_row_sm5368_abc_inverted`

这两个程序和普通 `hub75_row` 的差别在于：

1. 普通模式是一次输出并行行地址
2. `SM5368` 模式是输出两拍 `A/B/C` 控制波形

### 5. PIO 初始化按面板模式切换

在 `src/hub75.cpp` 的 `configure_pio()` 中：

1. 普通面板使用原来的 row PIO 程序
2. `SM5368` 面板使用新的 `hub75_row_sm5368_abc` 程序

这样可以保证：

1. 普通面板逻辑不受影响
2. 特殊面板逻辑单独维护

### 6. 像素映射继续复用普通 HUB75

`SM5368` 只改变“行选择方式”，不改变像素在 framebuffer 中的普通排布。

所以最终处理方式是：

1. `update()` 和 `update_bgr()` 继续走普通 `HUB75` 像素排布分支
2. 只在行控制部分切到 `SM5368`

这个点很重要。

如果只接了 `SM5368` 行控制，但没有让像素映射仍然复用普通 `HUB75` 路径，就会出现能编译但屏幕没有示例画面的情况。

## R/B 对调是怎么做的

这个问题没有通过改物理引脚顺序解决，而是通过软件打包阶段解决。

原因是：

1. 现有 PIO 数据流默认假设颜色位顺序固定
2. 直接改 `DATA_BASE_PIN` 或者打乱 6 根颜色线顺序，会影响现有整条数据路径
3. 对当前工程来说，在颜色打包阶段交换 `R/B` 最稳

### 具体实现

在 `src/hub75.cpp` 的两个颜色打包函数里：

1. `pack_lut_rgb()`
2. `pack_lut_rgb_()`

增加了：

```cpp
#if SWAP_RB_PINS
    return (rv << 20u) | (gv << 10u) | bv;
#else
    return (bv << 20u) | (gv << 10u) | rv;
#endif
```

含义是：

1. 正常模式下，仍然按原有位顺序打包
2. `SWAP_RB_PINS=true` 时，把 `R` 和 `B` 的输出位位置对调

这样做以后：

1. 物理引脚定义不需要改
2. PIO 程序不需要改
3. 只用编译期开关就能切换颜色方向

## 迁移到其他工程时的建议

如果你要在其他工程里复用类似功能，推荐按下面步骤做。

### 第一步：先把问题拆开

不要把所有特殊情况塞进一个面板类型里，至少拆成：

1. 面板尺寸和扫描深度
2. 行译码模式
3. 颜色数据位映射
4. 时钟相位或 latch 时序

### 第二步：优先复用普通像素映射

如果只是 `SM5368` 行译码不一样，优先保持：

1. framebuffer 到像素流的映射不变
2. 只改行控制逻辑

只有确认屏本身像素排布也不同，才改像素映射分支。

### 第三步：特殊行译码单独开模式

推荐做成下面这种编译期开关：

```cpp
#if defined(PANEL_SM5368)
    // 特殊行译码
#else
    // 普通 ABCDE 地址
#endif
```

### 第四步：颜色反了优先做软件映射

推荐加这种开关：

```cpp
#if SWAP_RB_PINS
    // 交换 R/B 输出位
#else
    // 正常输出
#endif
```

这通常比直接改整套 pinmap 更稳。

### 第五步：统一收敛到 profile

推荐在构建系统里加 profile，例如：

```cmake
if(PANEL_PROFILE STREQUAL "96X48_1_24_SM5368")
    set(PANEL_DEFINE_VALUE PANEL_SM5368)
    set(SWAP_RB_PINS_VALUE true)
    set(PANEL_SCAN_DEPTH_VALUE 24)
endif()

target_compile_definitions(app PRIVATE
    ${PANEL_DEFINE_VALUE}
    SWAP_RB_PINS=${SWAP_RB_PINS_VALUE}
    PANEL_SCAN_DEPTH=${PANEL_SCAN_DEPTH_VALUE}
)
```

这样做的优点是：

1. 配置清晰
2. 切换方便
3. 不会污染普通面板逻辑

## 这一套方案的经验总结

可以直接记成这几句话：

1. `SM5368` 解决“扫哪一行”
2. `SWAP_RB_PINS` 解决“颜色送到哪根线”
3. 行译码和像素映射不要混在一起
4. 颜色通道反了优先做软件交换
5. 所有差异尽量做成编译期开关和 profile

## 当前工程中相关文件

关键改动位置如下：

1. `CMakeLists.txt`
2. `src/hub75.hpp`
3. `src/hub75.cpp`
4. `src/hub75.pio`

## 当前 profile 的使用方法

配置命令示例：

```powershell
cmake -S . -B build-sm5368 -G Ninja -DPANEL_PROFILE=96X48_1_24_SM5368
cmake --build build-sm5368
```

## 一句话总结

这次的处理方式就是：

1. 用单独 profile 管理特殊面板
2. 用单独的 row decoder 处理 `SM5368`
3. 用单独的颜色位映射处理 `R/B` 对调
4. 普通 `HUB75` 像素映射尽量复用不动

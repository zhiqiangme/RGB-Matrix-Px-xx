STM32F103C8T6（Blue Pill，LQFP48）移植说明，基于 STM32F103RB 例程修改。

引脚重映射（RB 板上的 PB10、PC7 在 LQFP48 封装上不存在）：

| 信号 | RB 原引脚 | C8 新引脚 |
| --- | --- | --- |
| G2 | PB10 | PB7 |
| B | PC7 | PB12 |

其余引脚与 RB 例程一致（PA2/PA3 为 USART2，PA13/PA14 为 SWD）。

HUB75 信号与 Blue Pill 引脚对照：

| HUB75 信号 | 引脚 | HUB75 信号 | 引脚 |
| --- | --- | --- | --- |
| R1 | PA10 | R2 | PB4 |
| G1 | PB3 | G2 | PB7 |
| B1 | PB5 | B2 | PA8 |
| A | PA9 | D | PA7 |
| B | PB12 | E | PA6 |
| C | PB6 | CLK | PA5 |
| LAT | PB9 | OE | PB8 |

## 硬件连接

面板 HUB75 插座（2x8，从接线侧看，缺口/红线为 1 脚）与 Blue Pill 引脚一一对应：

| 1 脚侧（奇数脚） | 信号 | 接 Blue Pill | 2 脚侧（偶数脚） | 信号 | 接 Blue Pill |
| --- | --- | --- | --- | --- | --- |
| P1 | R1 | PA10 | P2 | G1 | PB3 |
| P3 | B1 | PB5 | P4 | GND | GND |
| P5 | R2 | PB4 | P6 | G2 | PB7 |
| P7 | B2 | PA8 | P8 | GND | GND |
| P9 | A | PA9 | P10 | B | PB12 |
| P11 | C | PB6 | P12 | D | PA7 |
| P13 | CLK | PA5 | P14 | LAT | PB9 |
| P15 | OE | PB8 | P16 | GND | GND |

### 供电

- **面板**：由独立 5V 电源直接接面板电源端子/DC 座。64x32 全白显示约 4A，建议 5V/3A 以上电源；一般显示内容约 1~2A。**不要用 Blue Pill 给面板供电**。
- **Blue Pill**：5V 引脚可搭接在同一 5V 电源取电（板载稳压器降到 3.3V）。
- **共地**：Blue Pill 的 GND 必须与面板 GND 相连（P4/P8/P16 任接一个），否则无信号参考必然花屏。

### 接线注意事项

1. **插头方向**：排线红线/插座缺口对准 P1（R1 一侧）；若面板有 IN/OUT 两个 HUB 座，信号线接 **IN**。
2. **杜邦线接法**：16 根母对母杜邦线按上表逐脚连接即可（HUB75 座针距 2.54mm）。
3. **电平**：Blue Pill 3.3V 直推 5V 面板为本例程方案，通常可正常工作；若出现花屏/闪烁，可在信号线上加 74AHCT245 电平转换。
4. **烧录口不受影响**：PA13/PA14 为 SWD（ST-Link 接 3V3/GND/SWCLK/SWDIO），PA2/PA3 为 USART2 串口，均不与面板信号冲突。
5. **JTAG 释放**：PB3/PB4 默认为 JTAG 引脚，工程已通过 `__HAL_AFIO_REMAP_SWJ_NOJTAG()` 释放为普通 IO，无需额外处理。

默认面板配置为 64x32（1/16 扫描），如需其他尺寸，修改 `User/Driver_HUB75/Driver_RGBMatrix.h` 中的 `HUB75_PANEL_PROFILE` 后重新编译。

如果有移植驱动的需求,可以在\User\Driver_HUB75下找到对应驱动。

并且需要打开MCU定时器并且在定时中断中添加如下内容:
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM1)
    display_tick = 1;
}

这里项目是默认开启的TIM1，如果使用其他定时器作为HUB75的显示tick的话,需要将相关配置也改为对应定时器的句柄
void HUB75_SetRefreshRate(uint8_t level)
{
  uint32_t arr = 0;
  switch(level)
  {
    case 1:
      arr = 8U;
      break;
    case 2:
      arr = 12U;
      break;
    case 3:
      arr = 16U;
      break;
    case 4:
    default:
      arr = 20U;
      break;
  }
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
  HAL_TIM_Base_Start_IT(&htim1);
}

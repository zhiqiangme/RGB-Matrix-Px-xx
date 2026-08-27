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

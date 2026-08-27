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

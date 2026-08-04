#include "Driver_RGBMatrix.h"

#if (HUB75_SCAN_ROWS == 0)
#error "HUB75_SCAN_ROWS must be greater than 0"
#endif
#if (HUB75_PANEL_CHAIN_COUNT == 0)
#error "HUB75_PANEL_CHAIN_COUNT must be greater than 0"
#endif
#if ((HUB75_PANEL_UNIT_WIDTH % 8u) != 0u)
#error "HUB75_PANEL_UNIT_WIDTH must be a multiple of 8"
#endif
#if ((HUB75_PANEL_HEIGHT % 2u) != 0u)
#error "HUB75_PANEL_HEIGHT must be even"
#endif
#if (HUB75_SCAN_ROWS > (HUB75_PANEL_HEIGHT / 2u))
#error "HUB75_SCAN_ROWS must not exceed HUB75_PANEL_HEIGHT / 2"
#endif
#if !HUB75_PANEL_IS_SM5368 && (HUB75_SCAN_ROWS > 32u)
#error "HUB75_SCAN_ROWS must not exceed 32 with A/B/C/D/E address lines"
#endif

volatile uint16_t display_tick = 0;
HUB75_port RGB_Matrix;
uint8_t hub75_blink = 0;
uint8_t hub75_color = HUB75_Color_Pink;
static uint8_t hub75_brightness;
static uint32_t hub75_slot_cycles;
#define HUB75_OE_GUARD_CYCLES  (0u)
uint8_t hub75_buff[HUB75_PANEL_WIDTH/8 * HUB75_PANEL_HEIGHT];
uint8_t hub75_panel_buff[HUB75_PANEL_WIDTH * HUB75_PANEL_HEIGHT / 2 * 3];

#define HUB75_DATA_PORTA_MASK    (R1_Pin | B2_Pin)
#define HUB75_DATA_PORTB_MASK    (G2_Pin | G1_Pin | R2_Pin | B1_Pin)
#define HUB75_ADDR_PORTA_MASK    (A_Pin | D_Pin | E_Pin)
#define HUB75_ADDR_PORTB_MASK    (C_Pin)
#define HUB75_ADDR_PORTC_MASK    (B_Pin)

#if HUB75_PANEL_IS_SM5368
#define HUB75_PACK_TOP_R_BIT       (7u)
#define HUB75_PACK_TOP_G_BIT       (6u)
#define HUB75_PACK_TOP_B_BIT       (5u)
#define HUB75_PACK_BOTTOM_R_BIT    (4u)
#define HUB75_PACK_BOTTOM_G_BIT    (3u)
#define HUB75_PACK_BOTTOM_B_BIT    (2u)
#else
#define HUB75_PACK_TOP_R_BIT       (5u)
#define HUB75_PACK_TOP_G_BIT       (6u)
#define HUB75_PACK_TOP_B_BIT       (7u)
#define HUB75_PACK_BOTTOM_R_BIT    (2u)
#define HUB75_PACK_BOTTOM_G_BIT    (3u)
#define HUB75_PACK_BOTTOM_B_BIT    (4u)
#endif

static inline void HUB75_AssignPackedBit(uint8_t *plane, uint8_t bit, uint8_t enabled)
{
  uint8_t mask = (uint8_t)(1u << bit);
  *plane = (enabled != 0u) ? (uint8_t)(*plane | mask) : (uint8_t)(*plane & (uint8_t)~mask);
}

static inline uint16_t HUB75_GetShiftColumn(uint16_t column)
{
#if (HUB75_PANEL_CHAIN_COUNT > 1u) && HUB75_CHAIN_SHIFT_REVERSE
  uint16_t panel = (uint16_t)(column / HUB75_PANEL_UNIT_WIDTH);
  uint16_t panel_column = (uint16_t)(column % HUB75_PANEL_UNIT_WIDTH);
  uint16_t mapped_panel = (uint16_t)((HUB75_PANEL_CHAIN_COUNT - 1u) - panel);

  return (uint16_t)(mapped_panel * HUB75_PANEL_UNIT_WIDTH + panel_column);
#else
  return column;
#endif
}

static inline uint16_t HUB75_GetShiftByteIndex(uint16_t byte_index)
{
#if (HUB75_PANEL_CHAIN_COUNT > 1u) && HUB75_CHAIN_SHIFT_REVERSE
  uint16_t panel_width_bytes = (uint16_t)(HUB75_PANEL_UNIT_WIDTH / 8u);
  uint16_t panel = (uint16_t)(byte_index / panel_width_bytes);
  uint16_t panel_byte = (uint16_t)(byte_index % panel_width_bytes);
  uint16_t mapped_panel = (uint16_t)((HUB75_PANEL_CHAIN_COUNT - 1u) - panel);

  return (uint16_t)(mapped_panel * panel_width_bytes + panel_byte);
#else
  return byte_index;
#endif
}

static inline void HUB75_SetAddressBinaryFast(uint16_t line)
{
  uint32_t bsrr_a = (uint32_t)HUB75_ADDR_PORTA_MASK << 16u;
  uint32_t bsrr_b = (uint32_t)HUB75_ADDR_PORTB_MASK << 16u;
  uint32_t bsrr_c = (uint32_t)HUB75_ADDR_PORTC_MASK << 16u;

  if((line & 0x0001u) != 0u)
    bsrr_a |= A_Pin;
  if((line & 0x0002u) != 0u)
    bsrr_c |= B_Pin;
  if((line & 0x0004u) != 0u)
    bsrr_b |= C_Pin;
  if((line & 0x0008u) != 0u)
    bsrr_a |= D_Pin;
  if((line & 0x0010u) != 0u)
    bsrr_a |= E_Pin;

  GPIOA->BSRR = bsrr_a;
  GPIOB->BSRR = bsrr_b;
  GPIOC->BSRR = bsrr_c;
}

static inline void HUB75_SetAddressSm5368Phase(uint8_t row_data, uint8_t row_clk)
{
  uint32_t bsrr_a = (uint32_t)HUB75_ADDR_PORTA_MASK << 16u;
  uint32_t bsrr_b = (uint32_t)HUB75_ADDR_PORTB_MASK << 16u;
  uint32_t bsrr_c = ((uint32_t)HUB75_ADDR_PORTC_MASK << 16u) | B_Pin;

  if(row_clk != 0u)
    bsrr_a |= A_Pin;
  if(row_data != 0u)
    bsrr_b |= C_Pin;

  GPIOA->BSRR = bsrr_a;
  GPIOB->BSRR = bsrr_b;
  GPIOC->BSRR = bsrr_c;
}

static inline void HUB75_SetScanLineFast(uint16_t line)
{
#if HUB75_PANEL_IS_SM5368
  uint8_t row_data = (line == 0u) ? 1u : 0u;

  HUB75_SetAddressSm5368Phase(row_data, 0u);
  Delay_us(HUB75_SM5368_PHASE_DELAY_US);
  HUB75_SetAddressSm5368Phase(row_data, 1u);
  Delay_us(HUB75_SM5368_PHASE_DELAY_US);
  HUB75_SetAddressSm5368Phase(row_data, 0u);
  Delay_us(HUB75_SM5368_PHASE_DELAY_US);
#else
  HUB75_SetAddressBinaryFast(line);
#endif
}

static inline void HUB75_LatchRowFast(uint16_t scan_row)
{
  HUB75_OE_H();
  HUB75_LAT_H();
  HUB75_SetScanLineFast(scan_row);
  HUB75_LAT_L();
  HUB75_OE_L();
}

static uint32_t HUB75_GetTim1ClockHz(void)
{
  RCC_ClkInitTypeDef clk_config;
  uint32_t flash_latency = 0U;
  uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

  HAL_RCC_GetClockConfig(&clk_config, &flash_latency);

  if(clk_config.APB2CLKDivider == RCC_HCLK_DIV1)
    return pclk2;

  return (uint32_t)(pclk2 * 2u);
}
/**
 * Initialization routine.
 * You might need to enable access to DWT registers on Cortex-M7
 *   DWT->LAR = 0xC5ACCE55
 */
void DWT_Init(void)
{
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

/**
 * Delay routine itself.
 * Time is in microseconds (1/1000000th of a second), not to be
 * confused with millisecond (1/1000th).
 *
 * No need to check an overflow. Let it just tick :)
 *
 * @param uint32_t us  Number of microseconds to delay for
 */
void Delay_us(uint32_t us) // microseconds
{
    uint32_t startTick = DWT->CYCCNT,
             delayTicks = us * (SystemCoreClock/1000000);

    while (DWT->CYCCNT - startTick < delayTicks);
}


void HUB75_Init(void)
{
  HUB75_OE_H();
  HUB75_CLK_L();
  HUB75_LAT_L();

  HUB75_DR1_L();
  HUB75_DG1_L();
  HUB75_DB1_L();
  HUB75_DR2_L();
  HUB75_DG2_L();
  HUB75_DB2_L();

  HUB75_A_L();
  HUB75_B_L();
  HUB75_C_L();
  HUB75_D_L();
  HUB75_E_L();

  hub75_brightness = 0U;
  hub75_slot_cycles = 0U;
  display_tick = 0U;
}

// Set refresh rate of the matrix
// level: 1-4, 1: highest, 4: lowest
void HUB75_SetRefreshRate(uint8_t level)
{
  uint32_t arr = 0;
  uint32_t timer_clock_hz;
  uint32_t prescaler = (uint32_t)htim1.Init.Prescaler + 1u;
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
  timer_clock_hz = HUB75_GetTim1ClockHz();
  if(timer_clock_hz == 0u)
  {
    hub75_slot_cycles = 0u;
  }
  else
  {
    hub75_slot_cycles = (uint32_t)(((uint64_t)SystemCoreClock * prescaler * (arr + 1u)) / timer_clock_hz);
  }
  HAL_TIM_Base_Start_IT(&htim1);
}

/*
 * @brief  Set brightness of the matrix
 * @param brightness Brightness value (0-255)
 * @return None
 */
void HUB75_SetBrightness(uint8_t brightness)
{
  hub75_brightness = brightness;
}

// Keep OE enabled for a brightness-controlled fraction of the current scan slot.
static void HUB75_OE_Window(uint32_t row_start_cycles)
{
  uint32_t available_cycles;
  uint32_t target_cycles;
  uint32_t oe_start_cycles;
  uint32_t elapsed_cycles;

  if(hub75_brightness == 0u)
  {
    HUB75_OE_H();
    return;
  }

  elapsed_cycles = (uint32_t)(DWT->CYCCNT - row_start_cycles);
  if((hub75_slot_cycles == 0u) || (elapsed_cycles + HUB75_OE_GUARD_CYCLES >= hub75_slot_cycles))
  {
    HUB75_OE_H();
    return;
  }

  available_cycles = hub75_slot_cycles - elapsed_cycles - HUB75_OE_GUARD_CYCLES;
  target_cycles = (uint32_t)((((uint64_t)available_cycles * hub75_brightness) + 254u) / 255u);
  oe_start_cycles = DWT->CYCCNT;

  while((uint32_t)(DWT->CYCCNT - oe_start_cycles) < target_cycles)
    __NOP();

  HUB75_OE_H();
}

void send_scan_line(unsigned char line)
{
  HUB75_SetScanLineFast((uint16_t)line);
}

void HUB75_WriteByte(uint8_t p_buff[], uint8_t color)
{
  static uint16_t row = 0;
  uint32_t row_start_cycles = DWT->CYCCNT;
  uint16_t scan_row = HUB75_MAP_SCAN_ROW(row);
  uint16_t panel_width_bytes = (uint16_t)(HUB75_PANEL_WIDTH / 8u);
  uint16_t half_plane_offset = (uint16_t)(panel_width_bytes * (HUB75_PANEL_HEIGHT / 2u));
  uint8_t data_r1, data_g1, data_b1, data_r2, data_g2, data_b2 = 0;
  HUB75_OE_H();

  /* write column data */
  for(uint16_t i = 0; i < panel_width_bytes; i++)
  {
    uint16_t shift_byte_index = HUB75_GetShiftByteIndex(i);

    data_r1 = data_g1 = data_b1 = 0;
    data_r2 = data_g2 = data_b2 = 0;

    /* color red */
    if(color & 0x01)
    {
      data_b1 = p_buff[row * panel_width_bytes + shift_byte_index];
      data_b2 = p_buff[row * panel_width_bytes + half_plane_offset + shift_byte_index];
    }

    /* color green */
    if(color & 0x02)
    {
      data_r1 = p_buff[row * panel_width_bytes + shift_byte_index];
      data_r2 = p_buff[row * panel_width_bytes + half_plane_offset + shift_byte_index];
    }

    /* color blue */
    if(color & 0x04)
    {
      data_g1 = p_buff[row * panel_width_bytes + shift_byte_index];
      data_g2 = p_buff[row * panel_width_bytes + half_plane_offset + shift_byte_index];
    }

    /* horizontal 8 bits data */
    for(uint8_t k=0; k<8; k++)
    {
      ((data_r1>>(7-k)) & 0x01) ? HUB75_DR1_H() : HUB75_DR1_L();
      ((data_g1>>(7-k)) & 0x01) ? HUB75_DG1_H() : HUB75_DG1_L();
      ((data_b1>>(7-k)) & 0x01) ? HUB75_DB1_H() : HUB75_DB1_L();
      ((data_r2>>(7-k)) & 0x01) ? HUB75_DR2_H() : HUB75_DR2_L();
      ((data_g2>>(7-k)) & 0x01) ? HUB75_DG2_H() : HUB75_DG2_L();
      ((data_b2>>(7-k)) & 0x01) ? HUB75_DB2_H() : HUB75_DB2_L();

      HUB75_CLK_L();
      HUB75_CLK_H();
    }
  }

  HUB75_OE_H();
  HUB75_LatchRowFast(scan_row);
  HUB75_OE_Window(row_start_cycles);

  if(++row >= HUB75_SCAN_ROWS){
    row = 0;
  }
}

void HUB75_WritePixel(uint8_t p_buff[])
{
  static const uint8_t plane_repeat[3] = {4U, 2U, 1U};
  static uint16_t plane = 0;
  static uint16_t row = 0;
  static uint8_t repeat_left = 4U;
  uint32_t row_start_cycles = DWT->CYCCNT;
  uint16_t scan_row = HUB75_MAP_SCAN_ROW(row);
  uint16_t row_offset = (uint16_t)(row * HUB75_PANEL_WIDTH);
  uint16_t plane_stride = (uint16_t)(HUB75_PANEL_WIDTH * (HUB75_PANEL_HEIGHT / 2u));
  const uint8_t *block0 = &p_buff[row_offset];
  const uint8_t *block1 = &p_buff[row_offset + plane_stride];
  const uint8_t *block2 = &p_buff[row_offset + (uint16_t)(plane_stride * 2u)];
  const uint8_t *plane_data = (plane == 0U) ? block0 : ((plane == 1U) ? block1 : block2);

  HUB75_OE_H();

  for(uint16_t i = 0; i < HUB75_PANEL_WIDTH; i++)
  {
    uint8_t byte = plane_data[HUB75_GetShiftColumn(i)];
    uint32_t bsrr_a = (uint32_t)HUB75_DATA_PORTA_MASK << 16u;
    uint32_t bsrr_b = (uint32_t)HUB75_DATA_PORTB_MASK << 16u;

    if((byte & (1u << 5)) != 0u)
      bsrr_a |= R1_Pin;
    if((byte & (1u << 4)) != 0u)
      bsrr_a |= B2_Pin;

    if((byte & (1u << 3)) != 0u)
      bsrr_b |= G2_Pin;
    if((byte & (1u << 6)) != 0u)
      bsrr_b |= G1_Pin;
    if((byte & (1u << 2)) != 0u)
      bsrr_b |= R2_Pin;
    if((byte & (1u << 7)) != 0u)
      bsrr_b |= B1_Pin;

    GPIOA->BSRR = bsrr_a;
    GPIOB->BSRR = bsrr_b;
    HUB75_CLK_L();
    HUB75_CLK_H();
  }

  HUB75_LatchRowFast(scan_row);
  HUB75_OE_Window(row_start_cycles);

  if(++row == HUB75_SCAN_ROWS)
  {
    row = 0;

    if(repeat_left > 1U)
    {
      repeat_left--;
    }
    else
    {
      plane++;
      if(plane >= 3U)
        plane = 0U;
      repeat_left = plane_repeat[plane];
    }
  }
}

void HUB75_WritePanel(uint8_t R, uint8_t G, uint8_t B, uint16_t row, uint16_t column)
{
  uint8_t *p_plane1 = NULL, *p_plane2 = NULL, *p_plane3 = NULL;

  if((row >= HUB75_PANEL_HEIGHT) || (column >= HUB75_PANEL_WIDTH))
    return;

  p_plane1 = &hub75_panel_buff[row%(HUB75_PANEL_HEIGHT/2)*HUB75_PANEL_WIDTH+HUB75_PANEL_WIDTH*HUB75_PANEL_HEIGHT/2*0+column];
  p_plane2 = &hub75_panel_buff[row%(HUB75_PANEL_HEIGHT/2)*HUB75_PANEL_WIDTH+HUB75_PANEL_WIDTH*HUB75_PANEL_HEIGHT/2*1+column];
  p_plane3 = &hub75_panel_buff[row%(HUB75_PANEL_HEIGHT/2)*HUB75_PANEL_WIDTH+HUB75_PANEL_WIDTH*HUB75_PANEL_HEIGHT/2*2+column];

  if(row < (HUB75_PANEL_HEIGHT/2))      //Top
  {
    HUB75_AssignPackedBit(p_plane1, HUB75_PACK_TOP_R_BIT, (uint8_t)((R >> 7) & 0x01u));
    HUB75_AssignPackedBit(p_plane2, HUB75_PACK_TOP_R_BIT, (uint8_t)((R >> 6) & 0x01u));
    HUB75_AssignPackedBit(p_plane3, HUB75_PACK_TOP_R_BIT, (uint8_t)((R >> 5) & 0x01u));

    HUB75_AssignPackedBit(p_plane1, HUB75_PACK_TOP_G_BIT, (uint8_t)((G >> 7) & 0x01u));
    HUB75_AssignPackedBit(p_plane2, HUB75_PACK_TOP_G_BIT, (uint8_t)((G >> 6) & 0x01u));
    HUB75_AssignPackedBit(p_plane3, HUB75_PACK_TOP_G_BIT, (uint8_t)((G >> 5) & 0x01u));

    HUB75_AssignPackedBit(p_plane1, HUB75_PACK_TOP_B_BIT, (uint8_t)((B >> 7) & 0x01u));
    HUB75_AssignPackedBit(p_plane2, HUB75_PACK_TOP_B_BIT, (uint8_t)((B >> 6) & 0x01u));
    HUB75_AssignPackedBit(p_plane3, HUB75_PACK_TOP_B_BIT, (uint8_t)((B >> 5) & 0x01u));
  }
  else                                  //Bottom
  {
    HUB75_AssignPackedBit(p_plane1, HUB75_PACK_BOTTOM_R_BIT, (uint8_t)((R >> 7) & 0x01u));
    HUB75_AssignPackedBit(p_plane2, HUB75_PACK_BOTTOM_R_BIT, (uint8_t)((R >> 6) & 0x01u));
    HUB75_AssignPackedBit(p_plane3, HUB75_PACK_BOTTOM_R_BIT, (uint8_t)((R >> 5) & 0x01u));

    HUB75_AssignPackedBit(p_plane1, HUB75_PACK_BOTTOM_G_BIT, (uint8_t)((G >> 7) & 0x01u));
    HUB75_AssignPackedBit(p_plane2, HUB75_PACK_BOTTOM_G_BIT, (uint8_t)((G >> 6) & 0x01u));
    HUB75_AssignPackedBit(p_plane3, HUB75_PACK_BOTTOM_G_BIT, (uint8_t)((G >> 5) & 0x01u));

    HUB75_AssignPackedBit(p_plane1, HUB75_PACK_BOTTOM_B_BIT, (uint8_t)((B >> 7) & 0x01u));
    HUB75_AssignPackedBit(p_plane2, HUB75_PACK_BOTTOM_B_BIT, (uint8_t)((B >> 6) & 0x01u));
    HUB75_AssignPackedBit(p_plane3, HUB75_PACK_BOTTOM_B_BIT, (uint8_t)((B >> 5) & 0x01u));
  }
}

void HUB75_LoadRGB565Frame(const uint16_t *frame, uint16_t width, uint16_t height)
{
  uint16_t y_max = height;
  uint16_t x_max = width;

  if(frame == NULL)
    return;
  if((width == 0U) || (height == 0U))
    return;

  if(y_max > HUB75_PANEL_HEIGHT)
    y_max = HUB75_PANEL_HEIGHT;
  if(x_max > HUB75_PANEL_WIDTH)
    x_max = HUB75_PANEL_WIDTH;

  memset(hub75_panel_buff, 0, sizeof(hub75_panel_buff));

  for(uint16_t y = 0; y < y_max; y++)
  {
    for(uint16_t x = 0; x < x_max; x++)
    {
      uint16_t pixel = frame[(uint32_t)y * width + x];
      uint8_t r5 = (uint8_t)((pixel >> 11) & 0x1F);
      uint8_t g6 = (uint8_t)((pixel >> 5) & 0x3F);
      uint8_t b5 = (uint8_t)(pixel & 0x1F);
      uint8_t r8 = (uint8_t)((r5 << 3) | (r5 >> 2));
      uint8_t g8 = (uint8_t)((g6 << 2) | (g6 >> 4));
      uint8_t b8 = (uint8_t)((b5 << 3) | (b5 >> 2));
      HUB75_WritePanel(r8, g8, b8, y, x);
    }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM1)
  {
    display_tick = 1u;
  }
}

/*
 * @brief  Execute pending HUB75 display refresh steps gated by TIM1.
 * @note   TIM1 update interrupt acts as a single pending refresh flag.
 *         Process one scan step per call to avoid stalling the app loop.
 * @retval None
 */
void HUB75_Display(void)
{
  __disable_irq();
  if(display_tick == 0u)
  {
    __enable_irq();
    return;
  }
  display_tick = 0u;
  __enable_irq();

  HUB75_WritePixel(hub75_panel_buff);
}

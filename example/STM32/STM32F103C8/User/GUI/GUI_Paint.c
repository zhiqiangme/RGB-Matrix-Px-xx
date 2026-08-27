#include "GUI_Paint.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset()
#include <math.h>

PAINT Paint;

/******************************************************************************
功能：创建图像
参数：
    image   :   指向图像缓存的指针
    width   :   图像的宽度
    Height  :   图像的高度
    Color   :   图像是否反色
******************************************************************************/
void Paint_NewImage(UWORD *image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
    Paint.Image = NULL;
    Paint.Image = image;

    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;    

		
    Paint.WidthByte = Width;
    Paint.HeightByte = Height;    
//    printf("WidthByte = %d, HeightByte = %d\r\n", Paint.WidthByte, Paint.HeightByte);
//    printf(" LCD_WIDTH / 8 = %d\r\n",  122 / 8);
   
    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;
    
    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

/******************************************************************************
功能：选择图像
参数：
    image : 指向图像缓存的指针
******************************************************************************/
void Paint_SelectImage(UWORD *image)
{
    Paint.Image = image;
}

/******************************************************************************
功能：选择图像的旋转角度
参数：
    Rotate : 0、90、180、270
******************************************************************************/
void Paint_SetRotate(UWORD Rotate)
{
    if(Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270) {
        printf("Set image Rotate %d\r\n", Rotate);
        Paint.Rotate = Rotate;
    } else {
        printf("rotate = 0, 90, 180, 270\r\n");
    }

    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Paint.WidthMemory;
        Paint.Height = Paint.HeightMemory;
    } else {
        Paint.Width = Paint.HeightMemory;
        Paint.Height = Paint.WidthMemory;
    }
}

/******************************************************************************
功能：选择图像的镜像方式
参数：
    mirror   :不镜像、水平镜像、垂直镜像、原点镜像
******************************************************************************/
void Paint_SetMirroring(UBYTE mirror)
{
    if(mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL || 
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
        printf("mirror image x:%s, y:%s\r\n",(mirror & 0x01)? "mirror":"none", ((mirror >> 1) & 0x01)? "mirror":"none");
        Paint.Mirror = mirror;
    } else {
        printf("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, \
        MIRROR_VERTICAL or MIRROR_ORIGIN\r\n");
    }    
}

/******************************************************************************
功能：绘制像素点
参数：
    Xpoint : X 方向坐标
    Ypoint : Y 方向坐标
    Color  : 绘制的颜色
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    if(Xpoint > Paint.Width || Ypoint > Paint.Height){
        printf("Exceeding display boundaries\r\n");
        return;
    }      
    UWORD X, Y;

    switch(Paint.Rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    default:
        return;
    }
    
    switch(Paint.Mirror) {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X > Paint.WidthMemory || Y > Paint.HeightMemory){
        printf("Exceeding display boundaries\r\n");
        return;
    }
    
		UDOUBLE Addr = X + Y * Paint.WidthByte;
		Paint.Image[Addr] = Color;


}

/******************************************************************************
功能：清除整幅图像的颜色
参数：
    Color : 填充的颜色
******************************************************************************/
void Paint_Clear(UWORD Color)
{

		for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
				for (UWORD X = 0; X < Paint.WidthByte; X++ ) {//8 像素 = 1 字节
						UDOUBLE Addr = X + Y * Paint.WidthByte;
					 // printf("%d ",Addr);
						Paint.Image[Addr] = Color;
						
				}
		}
		//printf("xxx\r\n");
    
}

/******************************************************************************
功能：清除某个窗口区域的颜色
参数：
    Xstart : x 起点
    Ystart : y 起点
    Xend   : x 终点
    Yend   : y 终点
    Color  : 填充的颜色
******************************************************************************/
void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color)
{
    UWORD X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) {//8 像素 = 1 字节
            Paint_SetPixel(X, Y, Color);
        }
    }
}

/******************************************************************************
功能：绘制并填充点 (Xpoint, Ypoint)
参数：
    Xpoint		: 点的 X 坐标
    Ypoint		: 点的 Y 坐标
    Color		: 填充颜色
    Dot_Pixel	: 点的大小
    Dot_Style	: 点的样式
******************************************************************************/
void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style)
{
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        printf("Paint_DrawPoint Input exceeds the normal display range\r\n");
        printf("Xpoint = %d , Paint.Width = %d  \r\n ",Xpoint,Paint.Width);
        printf("Ypoint = %d , Paint.Height = %d  \r\n ",Ypoint,Paint.Height);
        return;
    }

    int16_t XDir_Num , YDir_Num;
    if (Dot_Style == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if(Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                // printf("x = %d, y = %d\r\n", Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel);
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num <  Dot_Pixel; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num <  Dot_Pixel; YDir_Num++) {
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
            }
        }
    }
}

/******************************************************************************
功能：绘制任意斜率的直线
参数：
    Xstart ：起点的 X 坐标
    Ystart ：起点的 Y 坐标
    Xend   ：终点的 X 坐标
    Yend   ：终点的 Y 坐标
    Color  ：线段的颜色
    Line_width : 线宽
    Line_Style: 实线与虚线
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        printf("Paint_DrawLine Input exceeds the normal display range\r\n");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // 递增方向，1 为正向，-1 为反向
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //累计误差
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        //绘制虚线，第 2 个点为虚拟点
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            //printf("LINE_DOTTED\r\n");
						if(Color)
							Paint_DrawPoint(Xpoint, Ypoint, BLACK, Line_width, DOT_STYLE_DFT);
            else
							Paint_DrawPoint(Xpoint, Ypoint, WHITE, Line_width, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
功能：绘制矩形
参数：
    Xstart ：矩形的起点 X 坐标
    Ystart ：矩形的起点 Y 坐标
    Xend   ：矩形的终点 X 坐标
    Yend   ：矩形的终点 Y 坐标
    Color  ：矩形的颜色
    Line_width: 线宽
    Draw_Fill : 是否填充矩形内部
******************************************************************************/
void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        printf("Input exceeds the normal display range\r\n");
        return;
    }

    if (Draw_Fill) {
        UWORD Ypoint;
        for(Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color , Line_width, LINE_STYLE_SOLID);
        }
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
    }
}

/******************************************************************************
功能：使用八点法在指定位置绘制指定大小的圆
参数：
    X_Center  ：圆心 X 坐标
    Y_Center  ：圆心 Y 坐标
    Radius    ：圆的半径
    Color     ：圆弧的颜色
    Line_width: 线宽
    Draw_Fill : 是否填充圆内部
******************************************************************************/
void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius,
                      UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
        printf("Paint_DrawCircle Input exceeds the normal display range\r\n");
        return;
    }

    //从 (0, R) 作为起点开始画圆
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = Radius;

    //累计误差，用于判断标志的下一个点
    int16_t Esp = 3 - (Radius << 1 );

    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent ) { //实心圆
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//1
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//2
                Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//3
                Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//4
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//5
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//6
                Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//7
                Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //绘制空心圆
        while (XCurrent <= YCurrent ) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//1
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//2
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//3
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//4
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//5
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//6
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//7
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//0

            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
功能：显示英文字符
参数：
    Xpoint           ：X 坐标
    Ypoint           ：Y 坐标
    Acsii_Char       ：要显示的英文字符
    Font             ：指向描述字符尺寸的字体结构体的指针
    Color_Foreground : 选择前景色
    Color_Background : 选择背景色
******************************************************************************/
void Paint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Page, Column;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        printf("Paint_DrawChar Input exceeds the normal display range\r\n");
        return;
    }

    uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &Font->table[Char_Offset];

    for (Page = 0; Page < Font->Height; Page ++ ) {
        for (Column = 0; Column < Font->Width; Column ++ ) {

            //判断字体背景色与屏幕背景色是否一致
            if (FONT_BACKGROUND == Color_Background) { //此流程用于加快扫描速度
                if (*ptr & (0x80 >> (Column % 8)))
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            } else {
                if (*ptr & (0x80 >> (Column % 8))) {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                } else {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                }
            }
            //一个像素占 8 位
            if (Column % 8 == 7)
                ptr++;
        }//写入一行
        if (Font->Width % 8 != 0)
            ptr++;
    }//写入全部
}

/******************************************************************************
功能：显示字符串
参数：
    Xstart           ：X 坐标
    Ystart           ：Y 坐标
    pString          ：要显示的英文字符串的首地址
    Font             ：指向描述字符尺寸的字体结构体的指针
    Color_Foreground : 选择前景色
    Color_Background : 选择背景色
******************************************************************************/
void Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        printf("Paint_DrawString_EN Input exceeds the normal display range\r\n");
        return;
    }

    while (* pString != '\0') {
        //若 X 方向写满，则换行到 (Xstart, Ypoint)，Ypoint 为 Y 方向加上一个字符高度
        if ((Xpoint + Font->Width ) > Paint.Width ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // 若 Y 方向也写满，则回到起点 (Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > Paint.Height ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        Paint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Background, Color_Foreground);

        //下一个字符的地址
        pString ++;

        //横坐标方向前进一个字体宽度
        Xpoint += Font->Width;
    }
}


/******************************************************************************
功能：显示字符串
参数：
    Xstart  ：X 坐标
    Ystart  ：Y 坐标
    pString ：要显示的中文字符串与英文字符串的首地址
    Font    ：指向描述字符尺寸的字体结构体的指针
    Color_Foreground : 选择前景色
    Color_Background : 选择背景色
******************************************************************************/
void Paint_DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString, cFONT* font,
                        UWORD Color_Foreground, UWORD Color_Background)
{
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j,Num;

    /* 在屏幕上逐字符显示字符串 */
    while (*p_text != 0) {
        if(*p_text <= 0x7F) {  //ASCII 码 < 126
            for(Num = 0; Num < font->size; Num++) {
                if(*p_text== font->table[Num].index[0]) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //此流程用于加快扫描速度
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                } else {
                                    Paint_SetPixel(x + i, y + j, Color_Background);
                                    // Paint_DrawPoint(x + i, y + j, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* 指向下一个字符 */
            p_text += 1;
            /* 将列位置移动 16 */
            x += font->ASCII_Width;
        } else {        //中文
            for(Num = 0; Num < font->size; Num++) {
                if((*p_text== font->table[Num].index[0]) && (*(p_text+1) == font->table[Num].index[1])) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //此流程用于加快扫描速度
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                } else {
                                    Paint_SetPixel(x + i, y + j, Color_Background);
                                    // Paint_DrawPoint(x + i, y + j, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* 指向下一个字符 */
            p_text += 2;
            /* 将列位置移动 16 */
            x += font->Width;
        }
    }
}

/******************************************************************************
功能：显示数字
参数：
    Xstart           ：X 坐标
    Ystart           : Y 坐标
    Nummber          : 要显示的数字
    Font             ：指向描述字符尺寸的字体结构体的指针
	Digit						 : 小数位宽
    Color_Foreground : 选择前景色
    Color_Background : 选择背景色
******************************************************************************/
#define  ARRAY_LEN 255
void Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, double Nummber,
                   sFONT* Font, UWORD Digit,UWORD Color_Foreground, UWORD Color_Background)
{
    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
    uint8_t *pStr = Str_Array;
	int temp = Nummber;
	float decimals;
	uint8_t i;
  if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        printf("Paint_DisNum Input exceeds the normal display range\r\n");
        //return;
   }

	if(Digit > 0) {		
		decimals = Nummber - temp;
		for(i=Digit; i > 0; i--) {
			decimals*=10;
		}
		temp = decimals;
		//将数字转换为字符串
		for(i=Digit; i>0; i--) {
			Num_Array[Num_Bit] = temp % 10 + '0';
			Num_Bit++;
			temp /= 10;						
		}	
		Num_Array[Num_Bit] = '.';
		Num_Bit++;
	}

	temp = Nummber;
    //将数字转换为字符串
    while (temp) {
        Num_Array[Num_Bit] = temp % 10 + '0';
        Num_Bit++;
        temp /= 10;
    }
		
    //将字符串反转
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit ++;
        Num_Bit --;
    }

    //显示
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Background, Color_Foreground);
}

/******************************************************************************
功能：显示时间
参数：
    Xstart           ：X 坐标
    Ystart           : Y 坐标
    pTime            : 与时间相关的结构体
    Font             ：指向描述字符尺寸的字体结构体的指针
    Color_Foreground : 选择前景色
    Color_Background : 选择背景色
******************************************************************************/
void Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font,
                    UWORD Color_Foreground, UWORD Color_Background)
{
    uint8_t value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    UWORD Dx = Font->Width;

    //将数据写入缓存
    Paint_DrawChar(Xstart                           , Ystart, value[pTime->Hour / 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx                      , Ystart, value[pTime->Hour % 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx  + Dx / 4 + Dx / 2   , Ystart, ':'                    , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 2 + Dx / 2         , Ystart, value[pTime->Min / 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 3 + Dx / 2         , Ystart, value[pTime->Min % 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 4 + Dx / 2 - Dx / 4, Ystart, ':'                    , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 5                  , Ystart, value[pTime->Sec / 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 6                  , Ystart, value[pTime->Sec % 10] , Font, Color_Background, Color_Foreground);
    
}


void Paint_DrawImage(const unsigned char *image, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image) 
{
    int i,j; 
		for(j = 0; j < H_Image; j++){
			for(i = 0; i < W_Image; i++){
				if(xStart+i < Paint.WidthMemory  &&  yStart+j < Paint.HeightMemory)//超出部分不显示
					Paint_SetPixel(xStart + i, yStart + j, (*(image + j*W_Image*2 + i*2+1))<<8 | (*(image + j*W_Image*2 + i*2)));
				//利用数组顺序存储的特性，通过算法访问原数组
				//j*W_Image*2 			   Y 偏移
				//i*2              	   X 偏移
			}
		} 
}

void Paint_DrawImage1(const unsigned char *image, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image) 
{
    int i,j; 
		for(j = 0; j < H_Image; j++){
			for(i = 0; i < W_Image; i++){
				if(xStart+i < Paint.HeightMemory  &&  yStart+j < Paint.WidthMemory)//超出部分不显示
					Paint_SetPixel(xStart + i, yStart + j, (*(image + j*W_Image*2 + i*2+1))<<8 | (*(image + j*W_Image*2 + i*2)));
				//利用数组顺序存储的特性，通过算法访问原数组
				//j*W_Image*2 			   Y 偏移
				//i*2              	   X 偏移
			}
		} 
}

void Paint_DrawGradientImage(UWORD xStart, UWORD yStart, UWORD width, UWORD height)
{
    UWORD x, y;
    UWORD max_x = (width > 1) ? (width - 1) : 1;

    if(width == 0 || height == 0)
        return;

    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            UWORD px = xStart + x;
            UWORD py = yStart + y;
            UWORD r = (UWORD)(((max_x - x) * 31u) / max_x);
            UWORD b = (UWORD)((x * 31u) / max_x);
            UWORD color = (UWORD)((r << 11) | b);

            if(px < Paint.WidthMemory && py < Paint.HeightMemory)
                Paint_SetPixel(px, py, color);
        }
    }
}


/******************************************************************************
功能：显示单色位图
参数：
    image_buffer ：转换到位图格式的图像数据
说明：
    先在电脑上把图像转换成对应的数组，
    再把该数组直接以内嵌 .c 文件的形式写入 Imagedata.cpp。
******************************************************************************/
void Paint_DrawBitMap(const unsigned char* image_buffer)
{
    UWORD x, y;
    UDOUBLE Addr = 0;

    for (y = 0; y < Paint.HeightByte; y++) {
        for (x = 0; x < Paint.WidthByte; x++) {//8 像素 = 1 字节
            Addr = x + y * Paint.WidthByte;
            Paint.Image[Addr] = (unsigned char)image_buffer[Addr];
        }
    }
}

void Paint_DrawBitMap_Block(const unsigned char* image_buffer, UBYTE Region)
{
    UWORD x, y;
    UDOUBLE Addr = 0;
		for (y = 0; y < Paint.HeightByte; y++) {
				for (x = 0; x < Paint.WidthByte; x++) {//8 像素 = 1 字节
						Addr = x + y * Paint.WidthByte ;
						Paint.Image[Addr] = \
						(unsigned char)image_buffer[Addr+ (Paint.HeightByte)*Paint.WidthByte*(Region - 1)];
				}
		}
}



 void Paint_BmpWindows(unsigned char x,unsigned char y,const unsigned char *pBmp,
					unsigned char chWidth,unsigned char chHeight)
{
	uint16_t i, j, byteWidth = (chWidth + 7)/8;
    for(j = 0; j < chHeight; j ++){
        for(i = 0; i < chWidth; i ++ ) {
            if(*(pBmp + j * byteWidth + i / 8) & (128 >> (i & 7))) {
                Paint_SetPixel(x+i, y+j, 0xffff);
            }
        }
    }
}
         


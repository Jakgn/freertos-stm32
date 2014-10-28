#include "game.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define numOfBox	0x5
#define rowOfBox	0x4
int16_t boxPosX[rowOfBox] = {0, 60, 120, 180}; 
int16_t boxPosY[numOfBox] = {0, 64, 128, 192, 256};
int16_t boxW = 60;
int16_t boxH = 60;
int8_t order[numOfBox] = {0, 1, 2, 3, 0};
int8_t tmpOrder[numOfBox] = {0, 1, 2, 3, 0};
int16_t colorOrder[numOfBox] = {0, 1, 2, 3, 4};
int8_t clickRight = 0;
int8_t swh = 1;

static uint16_t color_array[] = {LCD_COLOR_GREY, LCD_COLOR_BLUE, 
				 LCD_COLOR_BLUE2, LCD_COLOR_RED,
			 	 LCD_COLOR_MAGENTA, LCD_COLOR_GREEN,
			         LCD_COLOR_CYAN, LCD_COLOR_YELLOW};

void Usart1_EventHandler()
{
    while(1)
    {
        while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
        char t = USART_ReceiveData(USART1);
		if(t == 'v' && order[0] == 0)
			clickRight = 1;
		if(t == 'c' && order[0] == 1)
			clickRight = 1;
		if(t == 'x' && order[0] == 2)
			clickRight = 1;
		if(t == 'z' && order[0] == 3)
			clickRight = 1;
        if ((t == '\r')) {
            while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
            USART_SendData(USART1, t);
            t = '\n';
        }
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, t);
    }
}

void GAME_Update()
{
	int16_t posX_index;
	int16_t color_index;
	int8_t i = 0;

	LCD_SetTextColor( LCD_COLOR_BLACK );
	for(i = 0 ; i < numOfBox ; i++)
		LCD_DrawFullRect( boxPosX[ tmpOrder[i] ], boxPosY[i], boxW, boxH );
	/* Hold the temporarily state because of two layers using for being blacken */
	for(i=0 ; i < numOfBox ; i++)
		tmpOrder[i] = order[i];
	if(clickRight == 1){
		clickRight = 0;
		for(i=0 ; i < numOfBox-1 ; i++){
			order[i] = order[i+1];
			colorOrder[i] = colorOrder[i+1];
		}
		/* random for box appear*/
		while (RNG_GetFlagStatus(RNG_FLAG_DRDY) != SET);
		posX_index = RNG_GetRandomNumber() % rowOfBox;
		order[numOfBox - 1] = posX_index;
		/* random for box color */	
		color_index = RNG_GetRandomNumber() % 
		(sizeof(color_array) / sizeof(color_array[0]));
		colorOrder[numOfBox - 1] = color_index;
	}
}

void GAME_Render()
{
	int8_t i = 0;
	for(i = 0 ; i < numOfBox ; i++){
		LCD_SetTextColor( color_array[ colorOrder[i] ] );
		LCD_DrawFullRect( boxPosX[ order[i] ], boxPosY[i], boxW, boxH );
	}
	//char *test = "hello";
	//LCD_DisplayStringLine(50, test);
	/* Change the layer */
	if(swh == 1){
		swh = 2;
		LCD_SetTransparency(0xFF);
		LCD_SetLayer( LCD_BACKGROUND_LAYER );
		LCD_SetTransparency(0x00);
	}else{
		swh = 1;
		LCD_SetTransparency(0xFF);
		LCD_SetLayer( LCD_FOREGROUND_LAYER );
		LCD_SetTransparency(0x00);
	}

	//LCD_DrawLine( 10, LCD_PIXEL_HEIGHT / 2, LCD_PIXEL_WIDTH - 20, LCD_DIR_HORIZONTAL );
}

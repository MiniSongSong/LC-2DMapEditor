#ifndef __MAPBOX_H__
#define __MAPBOX_H__

#include "posbox.h"

/* 计算地图尺寸 */
LCUI_Size MapBox_CountSize( LCUI_Widget *widget );

/* 获取指定行指定列的地图块的像素坐标 */
LCUI_Pos MapBox_MapBlock_GetPixelPos( LCUI_Widget *widget, int row, int col );

/* 获取指定像素坐标上的地图块的坐标 */
LCUI_Pos MapBox_MapBlock_GetPos( LCUI_Widget *widget, LCUI_Pos pixel_pos );

/* 获取地图块的尺寸 */
LCUI_Size MapBox_GetMapBlockSize( LCUI_Widget *widget );

/* 创建地图 */
int MapBox_CreateMap( LCUI_Widget *widget, int rows, int cols );

/* 调整地图尺寸 */
int MapBox_ResizeMap( LCUI_Widget *widget, int rows, int cols, POSBOX_POS flag );

/* 获取地图尺寸 */
LCUI_Size MapBox_GetMapSize( LCUI_Widget *widget );

/* 高亮一个地图块 */
int MapBox_HiglightMapBlock( LCUI_Widget *widget, LCUI_Pos pos );

/* 选中一个地图块 */
int MapBox_SelectMapBlock( LCUI_Widget *widget, LCUI_Pos pos );

/* 获取已选中的地图块的坐标，没有选中的地图块则返回FALSE，否则返回TRUE */
LCUI_BOOL MapBox_GetSelected( LCUI_Widget *widget, LCUI_Pos *pos );

/* 设置地图块的尺寸 */
void MapBox_SetMapBlockSize( LCUI_Widget *widget, int width, int height );

/* 设定指定ID的地图块的图像 */
int MapBox_SetMapBlockIMG( LCUI_Widget *widget, int id, LCUI_Graph *mapblk_img );

/* 设定指定ID的地图对象的图像 */
int MapBox_SetMapObjIMG(	LCUI_Widget *widget, int id,
				LCUI_Graph *mapobj_img, LCUI_Pos offset );

/* 设定已选定的地图块所使用的ID */
int MapBox_SetMapBlock(	LCUI_Widget *widget, int mapblock_id );

/* 设定已选定的地图块上的地图对象所使用的ID */
int MapBox_SetMapObj( LCUI_Widget *widget, int mapobj_id );

/* 设定当前使用的地图块 */
void MapBox_SetCurrentMapBlock( LCUI_Widget *widget, int mapblock_id );

/* 设定当前使用的地图对象 */
void MapBox_SetCurrentMapObj( LCUI_Widget *widget, int mapobj_id );

/* 对已选中的地图块进行垂直翻转 */
int MapBox_MapBlock_VertiFlip( LCUI_Widget *widget );

/* 对已选中的地图块进行水平翻转 */
int MapBox_MapBlock_HorizFlip( LCUI_Widget *widget );

/* 对已选中的地图对象进行水平翻转 */
int MapBox_MapObj_HorizFlip( LCUI_Widget *widget );

/* 从文件中载入地图数据 */
int MapBox_LoadMapData( LCUI_Widget *widget, const char *mapdata_filepath );

/* 保存地图数据至文件 */
int MapBox_SaveMapData( LCUI_Widget *widget, const char *mapdata_filepath );

/* 连接地图对象的Clicked事件，在鼠标指针点击地图对象时，会调用已连接的函数 */
void MapBox_ConnectMapObjClicked( LCUI_Widget *widget, void (*func)(LCUI_Widget*) );

/* 连接地图块的Clicked事件，在鼠标指针点击地图块时，会调用已连接的函数 */
void MapBox_ConnectMapBlockClicked( LCUI_Widget *widget, void (*func)(LCUI_Widget*) );

/* 注册MapBox部件 */
void Register_MapBox(void);
#endif

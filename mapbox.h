#ifndef __MAPBOX_H__
#define __MAPBOX_H__

/* 计算地图尺寸 */
LCUI_Size MapBox_CountSize( LCUI_Widget *widget );

/* 获取指定行指定列的地图块的像素坐标 */
LCUI_Pos MapBox_MapBlock_GetPixelPos( LCUI_Widget *widget, int row, int col );

/* 获取指定像素坐标上的地图块的坐标 */
LCUI_Pos MapBox_MapBlock_GetPos( LCUI_Widget *widget, LCUI_Pos pixel_pos );

/* 创建地图 */
int MapBox_CreateMap( LCUI_Widget *widget, int rows, int cols );

/* 调整地图尺寸 */
int MapBox_ResizeMap( LCUI_Widget *widget, int rows, int cols );

/* 获取地图尺寸 */
LCUI_Size MapBox_GetMapSize( LCUI_Widget *widget );

/* 高亮一个地图块 */
int MapBox_HiglightMapBlock( LCUI_Widget *widget, LCUI_Pos pos );

/* 选中一个地图块 */
int MapBox_SelectMapBlock( LCUI_Widget *widget, LCUI_Pos pos );

/* 设定已选定的地图块所使用的ID */
int MapBox_SetMapBlock(	LCUI_Widget *widget, int mapblock_id );

/* 设定当前使用的地图块 */
void MapBox_SetCurrentMapBlock( LCUI_Widget *widget, int mapblock_id );

/* 对已选中的地图块进行垂直翻转 */
int MapBox_MapBlock_VertiFlip( LCUI_Widget *widget );

/* 对已选中的地图块进行水平翻转 */
int MapBox_MapBlock_HorizFlip( LCUI_Widget *widget );

/* 从文件中载入地图数据 */
int MapBox_LoadMapData( const char *mapdata_filepath );

/* 保存地图数据至文件 */
int MapBox_SaveMapData( const char *mapdata_filepath );

/* 注册MapBox部件 */
void Register_MapBox(void);
#endif
#ifndef __MAPBOX_H__
#define __MAPBOX_H__
// 地图块样式
typedef enum _MAP_STYLE {
	MAP_STYLE_NORMAL,	// 正常样式
	MAP_STYLE_LEFT_90,	// 左旋转90度
	MAP_STYLE_LEFT_180,	// 左旋转180度
	MAP_STYLE_RIGHT_90,	// 右旋转90度
	MAP_STYLE_RIGHT_180,	// 右旋转180度
	MAP_STYLE_HORIZ,	// 水平翻转
	MAP_STYLE_VERTI		// 垂直翻转
} MAP_STYLE;

/* 创建地图 */
int MapBox_CreateMap( LCUI_Widget *widget, int rows, int cols );

/* 调整地图尺寸 */
int MapBox_ResizeMap( LCUI_Widget *widget, int rows, int cols );

/* 设定地图块 */
int MapBox_SetMapBlock(	LCUI_Widget	*widget,
			LCUI_Pos	pos,
			int		mapblock_id,
			MAP_STYLE	style_id );

/* 从文件中载入地图数据 */
int MapBox_LoadMapData( const char *mapdata_filepath );

/* 保存地图数据至文件 */
int MapBox_SaveMapData( const char *mapdata_filepath );


void Register_MapBox(void);

#endif
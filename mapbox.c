#include <LCUI_Build.h>
#include LC_LCUI_H
#include LC_WIDGET_H
#include LC_GRAPH_H
#include "res_map.h"
#include "mapbox.h"

// 地图块
typedef struct _map_blocks_data {
	int id;			// 地图块ID
	MAP_STYLE style;	// 地图块的样式
} map_blocks_data;

typedef struct _MapBox_Data {
	int rows, cols;			// 记录地图行数与列数
	LCUI_Pos selected;		// 被选中的地图块所在的坐标
	map_blocks_data **block;	// 地图块数据
	LCUI_Graph map_res;		// 地图资源
} MapBox_Data ;

/* 计算地图尺寸 */
LCUI_Size MapBox_CountSize( LCUI_Widget *widget )
{
	LCUI_Size size;
	MapBox_Data *mapbox;
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	size.w = (int)(MAP_BLOCK_WIDTH*mapbox->cols/2.0
		+ MAP_BLOCK_WIDTH*mapbox->rows/2.0 +0.5);
	size.h = (int)(MAP_BLOCK_HEIGHT*mapbox->cols/2.0
		+ MAP_BLOCK_HEIGHT*mapbox->rows/2.0 +0.5);
	return size;
}

/* 获取指定行指定列的地图块的像素坐标 */
LCUI_Pos MapBox_MapBlock_GetPixelPos( LCUI_Widget *widget, int row, int col )
{
	double x, y;
	LCUI_Pos pixel_pos;
	LCUI_Size map_size;
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	map_size = MapBox_CountSize( widget );
	/* 计算像素坐标，和MapBox_ExecDraw函数中的计算方法基本一样 */
	x = map_size.w/2.0 - MAP_BLOCK_WIDTH/2.0;
	x -= (MAP_BLOCK_WIDTH*row/2.0);
	y = (MAP_BLOCK_HEIGHT*row/2.0);
	x += (MAP_BLOCK_WIDTH*col/2.0);
	y += (MAP_BLOCK_HEIGHT*col/2.0);
	pixel_pos.x = (int)(x+0.5);
	pixel_pos.y = (int)(y+0.5);
	return pixel_pos;
}

/* 获取指定像素坐标上的地图块的坐标 */
LCUI_Pos MapBox_MapBlock_GetPos( LCUI_Widget *widget, LCUI_Pos pixel_pos )
{
	int i, j;
	LCUI_Rect rect;
	LCUI_Size map_size;
	MapBox_Data *mapbox;
	double start_x, start_y, x, y;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	map_size = MapBox_CountSize( widget );
	start_x = map_size.w/2.0 - MAP_BLOCK_WIDTH/2.0;
	start_y = 0;
	for( i=0; i<mapbox->rows; ++i ) {
		x = start_x;
		y = start_y;
		for( j=0; j<mapbox->cols; ++j ) {
			rect.x = (int)(x+0.5);
			rect.y = (int)(y+0.5);
			rect.width = MAP_BLOCK_WIDTH;
			rect.height = MAP_BLOCK_HEIGHT;
			/* 如果该像素点在当前地图块的范围内 */
			if( LCUIRect_IncludePoint( pixel_pos, rect ) ) {
				return Pos(j, i);
			}
			x += (MAP_BLOCK_WIDTH/2.0);
			y += (MAP_BLOCK_HEIGHT/2.0);
		}
		start_x -= (MAP_BLOCK_WIDTH/2.0);
		start_y += (MAP_BLOCK_HEIGHT/2.0);
	}
	return Pos(-1, -1);
}

static void MapBox_ExecDraw( LCUI_Widget *widget )
{
	int i, j, n;
	LCUI_Pos pos;
	LCUI_Rect rect;
	MapBox_Data *mapbox;
	LCUI_Size size, map_size;
	LCUI_Graph *graph, red_border, map_blocks[MAP_BLOCK_TOTAL];
	double start_x, start_y, x, y;

	size = Widget_GetSize( widget );
	map_size = MapBox_CountSize( widget );
	/* 如果部件尺寸需要调整 */
	if( size.w != map_size.w
	|| size.h != map_size.h ) {
		Widget_Resize( widget, map_size );
		return;
	}
	DEBUG_MSG("map size: %d,%d\n", map_size.w, map_size.h);
	graph = Widget_GetSelfGraph( widget );
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	rect.x = rect.y = 0;
	rect.width = MAP_BLOCK_WIDTH;
	rect.height = MAP_BLOCK_HEIGHT;
	/* 引用出每一个地图图块 */
	for(i=0; i<MAP_BLOCK_TOTAL; ++i, rect.x+=MAP_BLOCK_WIDTH) {
		Graph_Quote( &map_blocks[i], &mapbox->map_res, rect );
	}
	start_x = size.w/2.0 - MAP_BLOCK_WIDTH/2.0;
	start_y = 0;
	DEBUG_MSG("rows: %d, cols: %d\n", mapbox->rows, mapbox->cols);
	/* 根据地图数据进行绘图 */
	for( i=0; i<mapbox->rows; ++i ) {
		x = start_x;
		y = start_y;
		for( j=0; j<mapbox->cols; ++j ) {
			pos.x = (int)(x+0.5);
			pos.y = (int)(y+0.5);
			Graph_Mix( graph, &map_blocks[
				mapbox->block[i][j].id],
				pos );
			x += (MAP_BLOCK_WIDTH/2.0);
			y += (MAP_BLOCK_HEIGHT/2.0);
		}
		start_x -= (MAP_BLOCK_WIDTH/2.0);
		start_y += (MAP_BLOCK_HEIGHT/2.0);
	}
	/* 如果之前已经有地图块被选中，则重绘该地图块 */
	if( mapbox->selected.x != -1 && mapbox->selected.y != -1 ) {
		j = mapbox->selected.y;
		i = mapbox->selected.x;
		/* 获取地图块的像素坐标 */
		pos = MapBox_MapBlock_GetPixelPos( widget, j, i );
		n = mapbox->block[j][i].id;
		Graph_Init( &red_border );
		/* 载入红色边框 */
		load_red_border( &red_border );
		Graph_Mix( graph, &map_blocks[n], pos );
		Graph_Mix( graph, &red_border, pos );
		Graph_Free( &red_border );
		rect.x = pos.x;
		rect.y = pos.y;
		/* 标记无效区域，以进行刷新 */
		Widget_InvalidArea( widget, rect );
	}
}

/* 创建地图 */
int MapBox_CreateMap( LCUI_Widget *widget, int rows, int cols )
{
	int i, j;
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->rows = rows;
	mapbox->cols = cols;
	mapbox->block = (map_blocks_data**)malloc( 
					rows*sizeof(map_blocks_data*) );
	if( !mapbox->block ) {
		return -1;
	}
	for(i=0; i<rows; ++i) {
		mapbox->block[i] = (map_blocks_data*)malloc(
					cols*sizeof(map_blocks_data) );
		if( !mapbox->block[i] ) {
			return -1;
		}
		for(j=0; j<cols; ++j) {
			mapbox->block[i][j].id = 0;
			mapbox->block[i][j].style = MAP_STYLE_NORMAL;
		}
	}
	Widget_Draw(widget);
	return 0;
}

/* 调整地图尺寸 */
int MapBox_ResizeMap( LCUI_Widget *widget, int rows, int cols )
{
	int i, j;
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->block = (map_blocks_data**)realloc( 
	mapbox->block, rows*sizeof(map_blocks_data*) );
	if( !mapbox->block ) {
		return -1;
	}
	for(i=0; i<rows; ++i) {
		mapbox->block[i] = (map_blocks_data*)realloc(
		mapbox->block[i], cols*sizeof(map_blocks_data) );
		if( !mapbox->block[i] ) {
			return -1;
		}
		for(j=0; j<cols-mapbox->cols; ++j) {
			mapbox->block[i][j].id = 0;
			mapbox->block[i][j].style = MAP_STYLE_NORMAL;
		}
	}
	mapbox->rows = rows;
	mapbox->cols = cols;
	Widget_Draw( widget );
	return 0;
}

/* 选中一个地图块 */
int MapBox_SelectMapBlock( LCUI_Widget *widget, LCUI_Pos pos )
{
	int i, n, row, col;
	LCUI_Rect rect;
	LCUI_Pos pixel_pos;
	MapBox_Data *mapbox;
	LCUI_Graph *graph, red_border, map_blocks[MAP_BLOCK_TOTAL];
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	graph = Widget_GetSelfGraph( widget );
	DEBUG_MSG("widget: %p, graph: %p\n", widget, graph);
	Graph_Init( &red_border );
	row = mapbox->selected.y;
	col = mapbox->selected.x;
	DEBUG_MSG("old pos: %d, %d\n", row, col);
	rect.x = rect.y = 0;
	rect.width = MAP_BLOCK_WIDTH;
	rect.height = MAP_BLOCK_HEIGHT;
	for(i=0; i<MAP_BLOCK_TOTAL; ++i, rect.x+=MAP_BLOCK_WIDTH) {
		Graph_Quote( &map_blocks[i], &mapbox->map_res, rect );
	}
	if( pos.x == mapbox->selected.x
	&& pos.y == mapbox->selected.y ) {
		return 0;
	}
	/* 如果之前已经有地图块被选中，则重绘该地图块 */
	if( mapbox->selected.x != -1 && mapbox->selected.y != -1 ) {
		/* 获取地图块的像素坐标 */
		pixel_pos = MapBox_MapBlock_GetPixelPos( widget, row, col );
		n = mapbox->block[row][col].id;
		Graph_Mix( graph, &map_blocks[n], pixel_pos );
		rect.x = pixel_pos.x;
		rect.y = pixel_pos.y;
		/* 标记无效区域，以进行刷新 */
		Widget_InvalidArea( widget, rect );
	}
	mapbox->selected = pos;
	if( pos.x == -1 || pos.y  == -1 ) {
		return -1;
	}
	row = mapbox->selected.y;
	col = mapbox->selected.x;
	DEBUG_MSG("current pos: %d, %d\n", row, col);
	pixel_pos = MapBox_MapBlock_GetPixelPos( widget, row, col );
	n = mapbox->block[row][col].id;
	/* 载入红色边框 */
	load_red_border( &red_border );
	DEBUG_MSG("pixel_pos: %d,%d\n", pixel_pos.x, pixel_pos.y);
	Graph_Mix( graph, &map_blocks[n], pixel_pos );
	/* 将红色边框绘制上去 */
	Graph_Mix( graph, &red_border, pixel_pos );
	rect.x = pixel_pos.x;
	rect.y = pixel_pos.y;
	Widget_InvalidArea( widget, rect );
	Graph_Free( &red_border );
	return 0;
}

/* 设定地图块 */
int MapBox_SetMapBlock(	LCUI_Widget	*widget,
			LCUI_Pos	pos,
			int		mapblock_id,
			MAP_STYLE	style_id )
{
	return 0;
}

/* 从文件中载入地图数据 */
int MapBox_LoadMapData( const char *mapdata_filepath )
{
	return 0;
}

/* 保存地图数据至文件 */
int MapBox_SaveMapData( const char *mapdata_filepath )
{
	return 0;
}

/* 处理鼠标移动事件 */
static void MapBox_ProcMouseMotionEvent( LCUI_MouseMotionEvent *event, void *arg )
{
	LCUI_Pos pos, pixel_pos;
	LCUI_Widget *widget;

	widget = (LCUI_Widget*)arg;
	pixel_pos = Widget_GetGlobalPos( widget );
	/* 用鼠标游标的坐标减去部件的全局坐标 */
	pixel_pos.x = event->x - pixel_pos.x;
	pixel_pos.y = event->y - pixel_pos.y;
	DEBUG_MSG("cursor pixel pos: %d, %d\n", pixel_pos.x, pixel_pos.y);
	pos = MapBox_MapBlock_GetPos( widget, pixel_pos );
	DEBUG_MSG("mapblock pos: %d, %d\n", pos.x, pos.y);
	/* 选中地图块 */
	MapBox_SelectMapBlock( widget, pos );
}

static void MapBox_ProcClickedEvent( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{

}

static void MapBox_ProcDragEvent( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{

}

static void MapBox_ExecInit( LCUI_Widget *widget )
{
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)WidgetPrivData_New(widget, sizeof(MapBox_Data)); 
	mapbox->rows = mapbox->cols = 0;
	mapbox->selected.x = mapbox->selected.y = 0;
	mapbox->block = NULL;
	Graph_Init( &mapbox->map_res );
	load_res_map( &mapbox->map_res );
	/* 关联鼠标移动事件,点击事件，以及拖动事件 */
	LCUI_MouseMotionEvent_Connect( MapBox_ProcMouseMotionEvent, widget );
	Widget_Event_Connect( widget, EVENT_CLICKED, MapBox_ProcClickedEvent );
	Widget_Event_Connect( widget, EVENT_DRAG, MapBox_ProcDragEvent );
	Widget_SetBackgroundColor( widget, RGB(195,195,195) );
	Widget_SetBackgroundTransparent( widget, FALSE );
}

/* 注册MapBox部件 */
void Register_MapBox(void)
{
	WidgetType_Add("mapbox");
	WidgetFunc_Add("mapbox", MapBox_ExecInit, FUNC_TYPE_INIT);
	WidgetFunc_Add("mapbox", MapBox_ExecDraw, FUNC_TYPE_DRAW);
}

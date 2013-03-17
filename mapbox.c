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
	int rows, cols;				// 记录地图行数与列数j
	LCUI_Pos selected;			// 被选中的地图块所在的坐标
	LCUI_Pos higlight;			// 被鼠标游标覆盖的地图块所在的坐标
	int current_mapblock_id;		// 当前使用的地图块ID
	map_blocks_data **blocks;		// 地图块数据
	LCUI_Graph map_blocks[MAP_BLOCK_TOTAL];	// 已记录的地图块
	LCUI_Graph map_res;			// 地图资源
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

/* 重绘地图块 */
static int MapBox_RedrawMapBlock( LCUI_Widget *widget, int row, int col )
{
	int n;
	LCUI_Pos pos;
	LCUI_Rect rect;
	MapBox_Data *mapbox;
	LCUI_Graph *graph, border_img;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( row < 0 || row >= mapbox->rows
	|| col < 0 || col >= mapbox->cols ) {
		return -1;
	}
	graph = Widget_GetSelfGraph( widget );
	n = mapbox->blocks[row][col].id;
	pos = MapBox_MapBlock_GetPixelPos( widget, row, col );
	Graph_Mix( graph, &mapbox->map_blocks[n], pos );
	Graph_Init( &border_img );
	load_red_border( &border_img );
	/* 如果之前已经有地图块被选中，则重绘该地图块 */
	if( mapbox->selected.x == col && mapbox->selected.y == row ) {
		Graph_FillColor( &border_img, RGB(255,255,255) );
		Graph_Mix( graph, &border_img, pos );
	}
	/* 如果有需要高亮地的图块 */
	if( mapbox->higlight.x == col && mapbox->higlight.y == row ) {
		Graph_Mix( graph, &border_img, pos );
	}
	Graph_Free( &border_img );
	rect.x = pos.x;
	rect.y = pos.y;
	rect.width = MAP_BLOCK_WIDTH;
	rect.height = MAP_BLOCK_HEIGHT;
	/* 标记无效区域，以进行刷新 */
	Widget_InvalidArea( widget, rect );
	return 0;
}

static void MapBox_ExecDraw( LCUI_Widget *widget )
{
	int i, j;
	LCUI_Pos pos;
	LCUI_Graph *graph;
	MapBox_Data *mapbox;
	LCUI_Size size, map_size;
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
			Graph_Mix( graph, &mapbox->map_blocks[
				mapbox->blocks[i][j].id],
				pos );
			x += (MAP_BLOCK_WIDTH/2.0);
			y += (MAP_BLOCK_HEIGHT/2.0);
		}
		start_x -= (MAP_BLOCK_WIDTH/2.0);
		start_y += (MAP_BLOCK_HEIGHT/2.0);
	}
	MapBox_RedrawMapBlock( widget, mapbox->selected.y, mapbox->selected.x );
	MapBox_RedrawMapBlock( widget, mapbox->higlight.y, mapbox->higlight.x );
}

/* 创建地图 */
int MapBox_CreateMap( LCUI_Widget *widget, int rows, int cols )
{
	int i, j;
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->rows = rows;
	mapbox->cols = cols;
	mapbox->blocks = (map_blocks_data**)malloc( 
					rows*sizeof(map_blocks_data*) );
	if( !mapbox->blocks ) {
		return -1;
	}
	for(i=0; i<rows; ++i) {
		mapbox->blocks[i] = (map_blocks_data*)malloc(
					cols*sizeof(map_blocks_data) );
		if( !mapbox->blocks[i] ) {
			return -1;
		}
		for(j=0; j<cols; ++j) {
			mapbox->blocks[i][j].id = 0;
			mapbox->blocks[i][j].style = MAP_STYLE_NORMAL;
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
	mapbox->blocks = (map_blocks_data**)realloc( 
	mapbox->blocks, rows*sizeof(map_blocks_data*) );
	if( !mapbox->blocks ) {
		return -1;
	}
	for(i=0; i<rows; ++i) {
		mapbox->blocks[i] = (map_blocks_data*)realloc(
		mapbox->blocks[i], cols*sizeof(map_blocks_data) );
		if( !mapbox->blocks[i] ) {
			return -1;
		}
		for(j=0; j<cols-mapbox->cols; ++j) {
			mapbox->blocks[i][j].id = 0;
			mapbox->blocks[i][j].style = MAP_STYLE_NORMAL;
		}
	}
	mapbox->rows = rows;
	mapbox->cols = cols;
	Widget_Draw( widget );
	return 0;
}

/* 高亮一个地图块 */
int MapBox_HiglightMapBlock( LCUI_Widget *widget, LCUI_Pos pos )
{
	LCUI_Pos tmp_pos;
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( pos.x == mapbox->higlight.x
	&& pos.y == mapbox->higlight.y ) {
		return 0;
	}
	tmp_pos = mapbox->higlight;
	mapbox->higlight = pos;
	/* 如果之前已经有地图块被选中，则重绘该地图块 */
	if( tmp_pos.x != -1 && tmp_pos.y != -1 ) {
		MapBox_RedrawMapBlock( widget, tmp_pos.y, tmp_pos.x );
	}
	if( pos.x == -1 || pos.y  == -1 ) {
		return -1;
	}
	MapBox_RedrawMapBlock( widget, pos.y, pos.x );
	return 0;
}

/* 选中一个地图块 */
int MapBox_SelectMapBlock( LCUI_Widget *widget, LCUI_Pos pos )
{
	LCUI_Pos tmp_pos;
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( pos.x == mapbox->selected.x
	&& pos.y == mapbox->selected.y ) {
		return 0;
	}
	tmp_pos = mapbox->selected;
	mapbox->selected = pos;
	/* 如果之前已经有地图块被选中，则重绘该地图块 */
	if( tmp_pos.x != -1 && tmp_pos.y != -1 ) {
		MapBox_RedrawMapBlock( widget, tmp_pos.y, tmp_pos.x );
	}
	if( pos.x == -1 || pos.y  == -1 ) {
		return -1;
	}
	MapBox_RedrawMapBlock( widget, pos.y, pos.x );
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
	MapBox_HiglightMapBlock( widget, pos );
}

static void MapBox_ProcClickedEvent( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	MapBox_SelectMapBlock( widget, mapbox->higlight );
}

static void MapBox_ProcDragEvent( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{

}

static void MapBox_ExecInit( LCUI_Widget *widget )
{
	int i;
	LCUI_Rect rect;
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)WidgetPrivData_New(widget, sizeof(MapBox_Data)); 
	mapbox->rows = mapbox->cols = 0;
	mapbox->selected.x = mapbox->selected.y = -1;
	mapbox->higlight.x = mapbox->higlight.y = -1;
	mapbox->blocks = NULL;
	mapbox->current_mapblock_id = 0;
	Graph_Init( &mapbox->map_res );
	load_res_map( &mapbox->map_res );
	
	rect.x = rect.y = 0;
	rect.width = MAP_BLOCK_WIDTH;
	rect.height = MAP_BLOCK_HEIGHT;
	/* 引用出每一个地图图块 */
	for(i=0; i<MAP_BLOCK_TOTAL; ++i, rect.x+=MAP_BLOCK_WIDTH) {
		Graph_Quote( &mapbox->map_blocks[i], &mapbox->map_res, rect );
	}

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
#include <LCUI_Build.h>
#include LC_LCUI_H
#include LC_WIDGET_H
#include LC_GRAPH_H
#include "res_map.h"
#include "mapbox.h"

// ��ͼ��
typedef struct _map_blocks_data {
	int id;			// ��ͼ��ID
	MAP_STYLE style;	// ��ͼ�����ʽ
} map_blocks_data;

typedef struct _MapBox_Data {
	int rows, cols;		// ��¼��ͼ����������
	map_blocks_data **block;	// ��ͼ������
	LCUI_Graph map_res;	// ��ͼ��Դ
} MapBox_Data ;

static LCUI_Size MapBox_CountSize( LCUI_Widget *widget )
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

static void MapBox_ExecDraw( LCUI_Widget *widget )
{
	int i, j;
	LCUI_Pos pos;
	LCUI_Rect rect;
	MapBox_Data *mapbox;
	LCUI_Size size, map_size;
	LCUI_Graph *graph, map_blocks[MAP_BLOCK_TOTAL];
	double start_x, start_y, x, y;

	size = Widget_GetSize( widget );
	map_size = MapBox_CountSize( widget );
	/* ��������ߴ���Ҫ���� */
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
	/* ���ó�ÿһ����ͼͼ�� */
	for(i=0; i<MAP_BLOCK_TOTAL; ++i, rect.x+=MAP_BLOCK_WIDTH) {
		Graph_Quote( &map_blocks[i], &mapbox->map_res, rect );
	}
	start_x = size.w/2.0 - MAP_BLOCK_WIDTH/2.0;
	start_y = 0;
	DEBUG_MSG("rows: %d, cols: %d\n", mapbox->rows, mapbox->cols);
	/* ���ݵ�ͼ���ݽ��л�ͼ */
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
}

/* ������ͼ */
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

/* ������ͼ�ߴ� */
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

/* �趨��ͼ�� */
int MapBox_SetMapBlock(	LCUI_Widget	*widget,
			LCUI_Pos	pos,
			int		mapblock_id,
			MAP_STYLE	style_id )
{
	return 0;
}

/* ���ļ��������ͼ���� */
int MapBox_LoadMapData( const char *mapdata_filepath )
{
	return 0;
}

/* �����ͼ�������ļ� */
int MapBox_SaveMapData( const char *mapdata_filepath )
{
	return 0;
}

/* ��������ƶ��¼� */
static void MapBox_ProcMouseMotionEvent( LCUI_MouseMotionEvent *event, void *arg )
{
	LCUI_Widget *widget;

	widget = (LCUI_Widget*)arg;
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
	mapbox->block = NULL;
	Graph_Init( &mapbox->map_res );
	load_res_map( &mapbox->map_res );
	/* ��������ƶ��¼�,����¼����Լ��϶��¼� */
	LCUI_MouseMotionEvent_Connect( MapBox_ProcMouseMotionEvent, widget );
	Widget_Event_Connect( widget, EVENT_CLICKED, MapBox_ProcClickedEvent );
	Widget_Event_Connect( widget, EVENT_DRAG, MapBox_ProcDragEvent );
	Widget_SetBackgroundColor( widget, RGB(195,195,195) );
	Widget_SetBackgroundTransparent( widget, FALSE );
}


void Register_MapBox(void)
{
	WidgetType_Add("mapbox");
	WidgetFunc_Add("mapbox", MapBox_ExecInit, FUNC_TYPE_INIT);
	WidgetFunc_Add("mapbox", MapBox_ExecDraw, FUNC_TYPE_DRAW);
}

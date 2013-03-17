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
	int rows, cols;			// ��¼��ͼ����������
	LCUI_Pos selected;		// ��ѡ�еĵ�ͼ�����ڵ�����
	map_blocks_data **block;	// ��ͼ������
	LCUI_Graph map_res;		// ��ͼ��Դ
} MapBox_Data ;

/* �����ͼ�ߴ� */
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

/* ��ȡָ����ָ���еĵ�ͼ����������� */
LCUI_Pos MapBox_MapBlock_GetPixelPos( LCUI_Widget *widget, int row, int col )
{
	double x, y;
	LCUI_Pos pixel_pos;
	LCUI_Size map_size;
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	map_size = MapBox_CountSize( widget );
	/* �����������꣬��MapBox_ExecDraw�����еļ��㷽������һ�� */
	x = map_size.w/2.0 - MAP_BLOCK_WIDTH/2.0;
	x -= (MAP_BLOCK_WIDTH*row/2.0);
	y = (MAP_BLOCK_HEIGHT*row/2.0);
	x += (MAP_BLOCK_WIDTH*col/2.0);
	y += (MAP_BLOCK_HEIGHT*col/2.0);
	pixel_pos.x = (int)(x+0.5);
	pixel_pos.y = (int)(y+0.5);
	return pixel_pos;
}

/* ��ȡָ�����������ϵĵ�ͼ������� */
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
			/* ��������ص��ڵ�ǰ��ͼ��ķ�Χ�� */
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
	/* ���֮ǰ�Ѿ��е�ͼ�鱻ѡ�У����ػ�õ�ͼ�� */
	if( mapbox->selected.x != -1 && mapbox->selected.y != -1 ) {
		j = mapbox->selected.y;
		i = mapbox->selected.x;
		/* ��ȡ��ͼ����������� */
		pos = MapBox_MapBlock_GetPixelPos( widget, j, i );
		n = mapbox->block[j][i].id;
		Graph_Init( &red_border );
		/* �����ɫ�߿� */
		load_red_border( &red_border );
		Graph_Mix( graph, &map_blocks[n], pos );
		Graph_Mix( graph, &red_border, pos );
		Graph_Free( &red_border );
		rect.x = pos.x;
		rect.y = pos.y;
		/* �����Ч�����Խ���ˢ�� */
		Widget_InvalidArea( widget, rect );
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

/* ѡ��һ����ͼ�� */
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
	/* ���֮ǰ�Ѿ��е�ͼ�鱻ѡ�У����ػ�õ�ͼ�� */
	if( mapbox->selected.x != -1 && mapbox->selected.y != -1 ) {
		/* ��ȡ��ͼ����������� */
		pixel_pos = MapBox_MapBlock_GetPixelPos( widget, row, col );
		n = mapbox->block[row][col].id;
		Graph_Mix( graph, &map_blocks[n], pixel_pos );
		rect.x = pixel_pos.x;
		rect.y = pixel_pos.y;
		/* �����Ч�����Խ���ˢ�� */
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
	/* �����ɫ�߿� */
	load_red_border( &red_border );
	DEBUG_MSG("pixel_pos: %d,%d\n", pixel_pos.x, pixel_pos.y);
	Graph_Mix( graph, &map_blocks[n], pixel_pos );
	/* ����ɫ�߿������ȥ */
	Graph_Mix( graph, &red_border, pixel_pos );
	rect.x = pixel_pos.x;
	rect.y = pixel_pos.y;
	Widget_InvalidArea( widget, rect );
	Graph_Free( &red_border );
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
	LCUI_Pos pos, pixel_pos;
	LCUI_Widget *widget;

	widget = (LCUI_Widget*)arg;
	pixel_pos = Widget_GetGlobalPos( widget );
	/* ������α�������ȥ������ȫ������ */
	pixel_pos.x = event->x - pixel_pos.x;
	pixel_pos.y = event->y - pixel_pos.y;
	DEBUG_MSG("cursor pixel pos: %d, %d\n", pixel_pos.x, pixel_pos.y);
	pos = MapBox_MapBlock_GetPos( widget, pixel_pos );
	DEBUG_MSG("mapblock pos: %d, %d\n", pos.x, pos.y);
	/* ѡ�е�ͼ�� */
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
	/* ��������ƶ��¼�,����¼����Լ��϶��¼� */
	LCUI_MouseMotionEvent_Connect( MapBox_ProcMouseMotionEvent, widget );
	Widget_Event_Connect( widget, EVENT_CLICKED, MapBox_ProcClickedEvent );
	Widget_Event_Connect( widget, EVENT_DRAG, MapBox_ProcDragEvent );
	Widget_SetBackgroundColor( widget, RGB(195,195,195) );
	Widget_SetBackgroundTransparent( widget, FALSE );
}

/* ע��MapBox���� */
void Register_MapBox(void)
{
	WidgetType_Add("mapbox");
	WidgetFunc_Add("mapbox", MapBox_ExecInit, FUNC_TYPE_INIT);
	WidgetFunc_Add("mapbox", MapBox_ExecDraw, FUNC_TYPE_DRAW);
}

#include <LCUI_Build.h>
#include LC_LCUI_H
#include LC_WIDGET_H
#include LC_GRAPH_H
#include "res_map.h"
#include "mapbox.h"

#define MAPFILE_VERSION	1
// 文件头信息
typedef struct _mapfile_header {
	char type[8];
	int version;
	int rows, cols;
} mapfile_header;

// 地图块
typedef struct _map_blocks_data {
	int blk_id;		// 地图块ID
	int obj_id;		// 地图对象ID
	LCUI_BOOL verti_flip;	// 是否垂直翻转
	LCUI_BOOL horiz_flip;	// 是否水平翻转
} map_blocks_data;

typedef struct _MapBlockIMG {
	int id;			// 地图块ID
	LCUI_Graph img;		// 地图块图像数据
} MapBlockIMG;

typedef struct _MapObjIMG {
	int id;				// 对象的ID
	LCUI_Graph img;			// 对象的图像数据
	LCUI_Size occupied_size;	// 占用的地图空间尺寸
} MapObjIMG;

typedef struct _MapBox_Data {
	int rows, cols;			// 记录地图行数与列数j
	LCUI_Pos selected;		// 被选中的地图块所在的坐标
	LCUI_Pos higlight;		// 被鼠标游标覆盖的地图块所在的坐标
	int current_mapblock_id;	// 当前使用的地图块ID
	map_blocks_data **blocks;	// 地图中各个地图块数据
	LCUI_Size mapblk_size;		// 地图块的尺寸
	LCUI_Queue mapblk_img;		// 已记录的地图块图像
	LCUI_Queue obj_img;		// 已记录的对象图像
	LCUI_Widget **objs_widget;	// 已记录的用于显示地图对象的部件
} MapBox_Data;

/* 获取指定ID的地图块图像数据 */
static MapBlockIMG *
MapBox_GetMapBlockIMG( LCUI_Widget *widget, int id )
{
	int i, n;
	MapBox_Data *mapbox;
	MapBlockIMG *ptr;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	n = Queue_GetTotal( &mapbox->mapblk_img );
	for(i=0; i<n; ++i) {
		ptr = (MapBlockIMG *)Queue_Get( &mapbox->mapblk_img, i );
		if( ptr == NULL ) {
			continue;
		}
		if( ptr->id == id ) {
			return ptr;
		}
	}
	return NULL;
}

/* 计算地图尺寸 */
LCUI_Size MapBox_CountSize( LCUI_Widget *widget )
{
	LCUI_Size size;
	MapBox_Data *mapbox;
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	size.w = (int)(mapbox->mapblk_size.w*mapbox->cols/2.0
		+ mapbox->mapblk_size.w*mapbox->rows/2.0 +0.5);
	size.h = (int)(mapbox->mapblk_size.h*mapbox->cols/2.0
		+ mapbox->mapblk_size.h*mapbox->rows/2.0 +0.5);
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
	x = (mapbox->rows-1)*mapbox->mapblk_size.w/2.0;
	x -= (mapbox->mapblk_size.w*row/2.0);
	y = (mapbox->mapblk_size.h*row/2.0);
	x += (mapbox->mapblk_size.w*col/2.0);
	y += (mapbox->mapblk_size.h*col/2.0);
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
	start_x = (mapbox->rows-1)*mapbox->mapblk_size.w/2.0;
	start_y = 0;
	for( i=0; i<mapbox->rows; ++i ) {
		x = start_x;
		y = start_y;
		for( j=0; j<mapbox->cols; ++j ) {
			rect.x = (int)(x+0.5);
			rect.y = (int)(y+0.5);
			rect.width = mapbox->mapblk_size.w;
			rect.height = mapbox->mapblk_size.h;
			/* 如果该像素点在当前地图块的范围内 */
			if( LCUIRect_IncludePoint( pixel_pos, rect ) ) {
				return Pos(j, i);
			}
			x += (mapbox->mapblk_size.w/2.0);
			y += (mapbox->mapblk_size.h/2.0);
		}
		start_x -= (mapbox->mapblk_size.w/2.0);
		start_y += (mapbox->mapblk_size.h/2.0);
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
	MapBlockIMG *mapblk_img;
	LCUI_Graph *graph, *img_mapblock, buff, border_img;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( row < 0 || row >= mapbox->rows
	|| col < 0 || col >= mapbox->cols ) {
		return -1;
	}
	Graph_Init( &buff );
	Graph_Init( &border_img );
	graph = Widget_GetSelfGraph( widget );
	n = mapbox->blocks[row][col].blk_id;
	pos = MapBox_MapBlock_GetPixelPos( widget, row, col );
	mapblk_img = MapBox_GetMapBlockIMG( widget, n );
	img_mapblock = &mapblk_img->img;
	if( mapbox->blocks[row][col].horiz_flip ) {
		Graph_HorizFlip( img_mapblock, &buff );
		img_mapblock = &buff;
	}
	if( mapbox->blocks[row][col].verti_flip ) {
		Graph_VertiFlip( img_mapblock, &buff );
		img_mapblock = &buff;
	}

	Graph_Mix( graph, img_mapblock, pos );
	load_red_border( &border_img );
	/* 如果之前已经有地图块被选中，则重绘该地图块 */
	if( mapbox->selected.x == col && mapbox->selected.y == row ) {
		Graph_FillColor( &border_img, RGB(255,255,255) );
		Graph_Mix( graph, &border_img, pos );
	}
	/* 如果有需要高亮地的图块 */
	if( mapbox->higlight.x == col && mapbox->higlight.y == row ) {
		if( mapbox->current_mapblock_id >= 0 ) {
			n = mapbox->current_mapblock_id;
			mapblk_img = MapBox_GetMapBlockIMG( widget, n );
			Graph_Mix( graph, &mapblk_img->img, pos );
		}
		Graph_Mix( graph, &border_img, pos );
	}
	Graph_Free( &border_img );
	Graph_Free( &buff );
	rect.x = pos.x;
	rect.y = pos.y;
	rect.width = mapbox->mapblk_size.w;
	rect.height = mapbox->mapblk_size.h;
	/* 标记无效区域，以进行刷新 */
	Widget_InvalidArea( widget, rect );
	return 0;
}

static void MapBox_ExecDraw( LCUI_Widget *widget )
{
	int i, j, n;
	LCUI_Pos pos;
	LCUI_Graph *img_mapblock, buff, *graph;
	MapBlockIMG *mapblk_img;
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
	Graph_Init( &buff );
	graph = Widget_GetSelfGraph( widget );
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	start_x = (mapbox->rows-1)*mapbox->mapblk_size.w/2.0;
	start_y = 0;
	/* 根据地图数据进行绘图 */
	for( i=0; i<mapbox->rows; ++i ) {
		x = start_x;
		y = start_y;
		for( j=0; j<mapbox->cols; ++j ) {
			pos.x = (int)(x+0.5);
			pos.y = (int)(y+0.5);
			n = mapbox->blocks[i][j].blk_id;
			/* 根据ID号来引用相应地图块的图像数据 */
			mapblk_img = MapBox_GetMapBlockIMG( widget, n );
			if( mapblk_img == NULL ) {
				_DEBUG_MSG("mapblock img of ID %d is not found!\n", n);
				return;
			}
			img_mapblock = &mapblk_img->img;
			/* 如果需要进行翻转，则再对地图块进行翻转处理 */
			if( mapbox->blocks[i][j].horiz_flip ) {
				Graph_HorizFlip( img_mapblock, &buff );
				img_mapblock = &buff;
			}
			if( mapbox->blocks[i][j].verti_flip ) {
				Graph_VertiFlip( img_mapblock, &buff );
				img_mapblock = &buff;
			}
			/* 粘贴至部件图层上 */
			Graph_Mix( graph, img_mapblock, pos );
			x += (mapbox->mapblk_size.w/2.0);
			y += (mapbox->mapblk_size.h/2.0);
		}
		start_x -= (mapbox->mapblk_size.w/2.0);
		start_y += (mapbox->mapblk_size.h/2.0);
	}
	/* 重绘被选中和高亮状态的地图块 */
	MapBox_RedrawMapBlock( widget, mapbox->selected.y, mapbox->selected.x );
	MapBox_RedrawMapBlock( widget, mapbox->higlight.y, mapbox->higlight.x );
	Graph_Free( &buff );
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
			for(j=i-1; j>=0; --j) {
				free( mapbox->blocks[i] );
			}
			free( mapbox->blocks );
			return -1;
		}
		for(j=0; j<cols; ++j) {
			mapbox->blocks[i][j].blk_id = 0;
			mapbox->blocks[i][j].verti_flip = FALSE;
			mapbox->blocks[i][j].horiz_flip = FALSE;
		}
	}
	Widget_Draw(widget);
	return 0;
}

/* 调整地图尺寸 */
int MapBox_ResizeMap( LCUI_Widget *widget, int rows, int cols, POSBOX_POS flag )
{
	int i, j, x, y;
	LCUI_Rect rect, cut_rect;
	MapBox_Data *mapbox;
	map_blocks_data **new_mapblocks;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	new_mapblocks = (map_blocks_data**)
			malloc( rows*sizeof(map_blocks_data*) );
	if( !mapbox->blocks ) {
		return -1;
	}

	for(i=0; i<rows; ++i) {
		new_mapblocks[i] = (map_blocks_data*)malloc( cols
					*sizeof(map_blocks_data) );
		if( !new_mapblocks[i] ) {
			for(j=i-1; j>=0; --j) {
				free( new_mapblocks[i] );
			}
			free( new_mapblocks );
			return -1;
		}
		for(j=0; j<cols; ++j) {
			new_mapblocks[i][j].blk_id = 0;
			new_mapblocks[i][j].verti_flip = FALSE;
			new_mapblocks[i][j].horiz_flip = FALSE;
		}
	}

	switch(flag) {
	case POS_TOPLEFT:
		rect.x = rect.y = 0;
		break;
	case POS_TOPCENTER:
		rect.x = (cols - mapbox->cols)/2;
		rect.y = 0;
		break;
	case POS_TOPRIGHT: 
		rect.x = cols - mapbox->cols;
		rect.y = 0;
		break;
	case POS_MIDDLELEFT:
		rect.x = 0;
		rect.y = (rows - mapbox->rows)/2;
		break;
	case POS_MIDDLECENTER:
		rect.x = (cols - mapbox->cols)/2;
		rect.y = (rows - mapbox->rows)/2;
		break;
	case POS_MIDDLERIGHT:
		rect.x = cols - mapbox->cols;
		rect.y = (rows - mapbox->rows)/2;
		break;
	case POS_BOTTOMLEFT:
		rect.x = 0;
		rect.y = rows - mapbox->rows;
		break;
	case POS_BOTTOMCENTER:
		rect.x = (cols - mapbox->cols)/2;
		rect.y = rows - mapbox->rows;
		break;
	case POS_BOTTOMRIGHT:
		rect.x = cols - mapbox->cols;
		rect.y = rows - mapbox->rows;
		break;
	}
	rect.width = mapbox->cols;
	rect.height = mapbox->rows;
	
	if( LCUIRect_GetCutArea( Size(cols,rows), rect, &cut_rect )) {
		rect.x += cut_rect.x;
		rect.y += cut_rect.y;
	}
	
	for( i=rect.y,y=0; y<cut_rect.height; ++y,++i ) {
		for( j=rect.x,x=0; x<cut_rect.width; ++x,++j ) { 
			new_mapblocks[i][j] = mapbox->blocks[y+cut_rect.y][cut_rect.x+x];
		}
	}
	/* 释放之前的地图块信息 */
	for(i=0; i<mapbox->rows; ++i) {
		free( mapbox->blocks[i] );
	}
	free( mapbox->blocks );
	/* 记录新的地图块信息 */
	mapbox->blocks = new_mapblocks;
	mapbox->rows = rows;
	mapbox->cols = cols;
	Widget_Draw( widget );
	return 0;
}

/* 获取地图尺寸 */
LCUI_Size MapBox_GetMapSize( LCUI_Widget *widget )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	return Size( mapbox->cols, mapbox->rows );
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

/* 设置地图块的尺寸 */
void MapBox_SetMapBlockSize( LCUI_Widget *widget, int width, int height )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->mapblk_size = Size( width, height );
}

/* 设定指定ID的地图块的图像 */
int MapBox_SetMapBlockIMG( LCUI_Widget *widget, int id, LCUI_Graph *mapblk_img )
{
	MapBlockIMG *img;
	MapBox_Data *mapbox;
	img = MapBox_GetMapBlockIMG( widget, id );
	mapbox = (MapBox_Data*)Widget_GetPrivData( widget );
	if( img == NULL ) {
		img = (MapBlockIMG*)malloc( sizeof(MapBlockIMG) );
		if( img == NULL ) {
			return -1;
		}
		img->id = id;
		Queue_AddPointer( &mapbox->mapblk_img, img );
	}
	if( mapblk_img == NULL ) {
		Graph_Init( &img->img );
	} else {
		img->img = *mapblk_img;
	}
	return 0;
}

/* 设定已选定的地图块所使用的ID */
int MapBox_SetMapBlock(	LCUI_Widget *widget, int mapblock_id )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( mapbox->selected.x == -1 || mapbox->selected.x >= mapbox->cols
	|| mapbox->selected.y == -1 || mapbox->selected.y >= mapbox->rows ) {
		return -1;
	}
	if( mapblock_id == -1 ) {
		return -2;
	}
	mapbox->blocks[mapbox->selected.y][mapbox->selected.x].blk_id = mapblock_id;
	MapBox_RedrawMapBlock( widget, mapbox->selected.y, mapbox->selected.x );
	return 0;
}

/* 设定当前使用的地图块 */
void MapBox_SetCurrentMapBlock( LCUI_Widget *widget, int mapblock_id )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->current_mapblock_id = mapblock_id;
}

/* 对已选中的地图块进行垂直翻转 */
int MapBox_MapBlock_VertiFlip( LCUI_Widget *widget )
{
	int row, col;
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	row = mapbox->selected.y;
	col = mapbox->selected.x;
	if( col == -1 || col >= mapbox->cols
	|| row == -1 || row >= mapbox->rows ) {
		return -1;
	}
	mapbox->blocks[row][col].verti_flip = (!mapbox->blocks[row][col].verti_flip);
	MapBox_RedrawMapBlock( widget, row, col );
	return 0;
}

/* 对已选中的地图块进行水平翻转 */
int MapBox_MapBlock_HorizFlip( LCUI_Widget *widget )
{
	int row, col;
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	row = mapbox->selected.y;
	col = mapbox->selected.x;
	if( col == -1 || col >= mapbox->cols
	|| row == -1 || row >= mapbox->rows ) {
		return -1;
	}
	mapbox->blocks[row][col].horiz_flip = (!mapbox->blocks[row][col].horiz_flip);
	MapBox_RedrawMapBlock( widget, row, col );
	return 0;
}

/* 从文件中载入地图数据 */
int MapBox_LoadMapData( LCUI_Widget *widget, const char *mapdata_filepath )
{
	FILE *fp;
	int row, col;
	MapBox_Data *mapbox;
	mapfile_header map_info;
	map_blocks_data **tmp, **new_mapblocks;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	fp = fopen( mapdata_filepath, "rb" );
	if(fp == NULL) {
		return -1;
	}
	fread( &map_info, sizeof(mapfile_header), 1, fp );
	/* 如果文件头中没有“LCUIMAP”这个字符串就退出 */
	if( strcmp( map_info.type, "LCUIMAP") != 0 ) {
		fclose( fp );
		return -1;
	}
	/* 如果文件版本不是当前支持的版本，也退出 */
	if( map_info.version != MAPFILE_VERSION ) {
		fclose( fp );
		return -2;
	}
	new_mapblocks = (map_blocks_data**)malloc(map_info.rows
					*sizeof(map_blocks_data*) );
	if( !new_mapblocks ) {
		return -2;
	}

	for( row=0; row<map_info.rows; ++row ) {
		new_mapblocks[row] = (map_blocks_data*)malloc(
				map_info.cols*sizeof(map_blocks_data) );
		if( !new_mapblocks[row] ) {
			for(row=row-1; row>=0; --row) {
				free( new_mapblocks[row] );
			}
			free( new_mapblocks );
			return -2;
		}
		for( col=0; col<map_info.cols; ++col ) {
			fread( &new_mapblocks[row][col],
			 sizeof(map_blocks_data), 1, fp );
		}
	}
	fclose( fp );

	tmp = mapbox->blocks;
	if( tmp != NULL )  {
		/* 释放现有的地图块信息 */
		for(row=0; row<mapbox->rows; ++row) {
			free( tmp[row] );
		}
		free( tmp );
	}
	mapbox->blocks = new_mapblocks;
	mapbox->rows = map_info.rows;
	mapbox->cols = map_info.cols;
	Widget_Draw(widget);
	return 0;
}

/* 保存地图数据至文件 */
int MapBox_SaveMapData( LCUI_Widget *widget, const char *mapdata_filepath )
{
	FILE *fp;
	int row, col;
	MapBox_Data *mapbox;
	mapfile_header map_info;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	map_info.type[0] = 'L';
	map_info.type[1] = 'C';
	map_info.type[2] = 'U';
	map_info.type[3] = 'I';
	map_info.type[4] = 'M';
	map_info.type[5] = 'A';
	map_info.type[6] = 'P';
	map_info.type[7] = '\0';
	map_info.version = MAPFILE_VERSION;
	map_info.rows = mapbox->rows;
	map_info.cols = mapbox->cols;
	fp = fopen( mapdata_filepath, "wb" );
	if(fp == NULL) {
		return -1;
	}
	fwrite( &map_info, sizeof(mapfile_header), 1, fp );
	for( row=0; row<mapbox->rows; ++row ) {
		for( col=0; col<mapbox->cols; ++col ) {
			fwrite( &mapbox->blocks[row][col],
			 sizeof(map_blocks_data), 1, fp );
		}
	}
	fclose( fp );
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
	MapBox_SetMapBlock( widget, mapbox->current_mapblock_id );
}


static void MapBox_ExecInit( LCUI_Widget *widget )
{
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)WidgetPrivData_New(widget, sizeof(MapBox_Data)); 
	mapbox->rows = mapbox->cols = 0;
	mapbox->selected.x = mapbox->selected.y = -1;
	mapbox->higlight.x = mapbox->higlight.y = -1;
	mapbox->blocks = NULL;
	mapbox->current_mapblock_id = -1;
	mapbox->mapblk_size = Size(0,0);
	Queue_Init( &mapbox->mapblk_img, sizeof(MapBlockIMG), NULL );

	/* 关联鼠标移动事件,点击事件，以及拖动事件 */
	LCUI_MouseMotionEvent_Connect( MapBox_ProcMouseMotionEvent, widget );
	Widget_Event_Connect( widget, EVENT_CLICKED, MapBox_ProcClickedEvent );
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

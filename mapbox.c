#include <LCUI_Build.h>
#include LC_LCUI_H
#include LC_WIDGET_H
#include LC_GRAPH_H
#include "res_map.h"
#include "mapbox.h"
#include <math.h>

#define MAPFILE_VERSION	2
// 文件头信息
typedef struct _mapfile_header {
	char type[8];
	int version;
	int rows, cols;
} mapfile_header;

// 地图块
typedef struct _map_blocks_data {
	int blk_id;			// 地图块ID
	int obj_id;			// 地图对象ID
	LCUI_BOOL blk_verti_flip;	// 地图块是否垂直翻转
	LCUI_BOOL blk_horiz_flip;	// 地图块是否水平翻转
	LCUI_BOOL obj_horiz_flip;	// 地图对象是否水平翻转
} map_blocks_data;

typedef struct _MapBlockIMG {
	int id;			// 地图块ID
	LCUI_Graph img;		// 地图块图像数据
} MapBlockIMG;

typedef struct _MapObjIMG {
	int id;				// 对象的ID
	LCUI_Graph img;			// 对象的图像数据
	LCUI_Size occupied_size;	// 占用的地图空间尺寸
	LCUI_Pos offset;		// xy坐标偏移量
} MapObjIMG;

typedef struct _MapBox_Data {
	int rows, cols;			// 记录地图行数与列数j
	LCUI_Pos selected;		// 被选中的地图块所在的坐标
	LCUI_Pos higlight;		// 被鼠标游标覆盖的地图块所在的坐标
	int current_mapblk_id;		// 当前使用的地图块ID
	int current_mapobj_id;		// 当前使用的地图对象ID
	map_blocks_data **blocks;	// 地图中各个地图块数据
	LCUI_Size mapblk_size;		// 地图块的尺寸
	LCUI_Queue mapblk_img;		// 已记录的地图块图像
	LCUI_Queue mapobj_img;		// 已记录的对象图像
	LCUI_Widget ***objs_widget;	// 已记录的用于显示地图对象的部件
	// 记录与地图块、地图对象连接的回调函数
	void (*mapobj_clicked)(LCUI_Widget*);
	void (*mapblk_clicked)(LCUI_Widget*);
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

/* 获取指定ID的地图对象图像数据 */
static MapObjIMG *
MapBox_GetMapObjIMG( LCUI_Widget *widget, int id )
{
	int i, n;
	MapBox_Data *mapbox;
	MapObjIMG *ptr;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	n = Queue_GetTotal( &mapbox->mapobj_img );
	for(i=0; i<n; ++i) {
		ptr = (MapObjIMG *)Queue_Get( &mapbox->mapobj_img, i );
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
	size.w = (int)(mapbox->mapblk_size.w*mapbox->cols*0.5
		+ mapbox->mapblk_size.w*mapbox->rows*0.5 +0.5);
	size.h = (int)(mapbox->mapblk_size.h*mapbox->cols*0.5
		+ mapbox->mapblk_size.h*mapbox->rows*0.5 +0.5);
	return size;
}

/* 获取指定行指定列的地图块的像素坐标 */
LCUI_Pos MapBox_MapBlock_GetPixelPos( LCUI_Widget *widget, int row, int col )
{
	double x, y;
	LCUI_Pos pixel_pos;
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if(row == -1 && col == -1) {
		row = mapbox->selected.y;
		col = mapbox->selected.x;
	}
	/* 计算像素坐标，和MapBox_ExecDraw函数中的计算方法基本一样 */
	x = (mapbox->rows-1)*mapbox->mapblk_size.w*0.5;
	x -= (mapbox->mapblk_size.w*row*0.5);
	y = (mapbox->mapblk_size.h*row*0.5);
	x += (mapbox->mapblk_size.w*col*0.5);
	y += (mapbox->mapblk_size.h*col*0.5);
	pixel_pos.x = (int)(x+0.5);
	pixel_pos.y = (int)(y+0.5);
	return pixel_pos;
}

/* 计算两点之间的距离 */
static double 
get_point_distance( LCUI_Pos pt1, LCUI_Pos pt2 )
{
	return sqrt(pow((double)(pt1.x-pt2.x), 2) + pow((double)(pt1.y-pt2.y), 2));
}


/* 获取指定像素坐标上的地图块的坐标 */
LCUI_Pos MapBox_MapBlock_GetPos( LCUI_Widget *widget, LCUI_Pos pixel_pos )
{
	int y_axis_x;
	double k, x, y, d, px, py;
	LCUI_Pos pos;
	LCUI_Size map_size;
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	map_size = MapBox_CountSize( widget );
	if(pixel_pos.x < 0 || pixel_pos.y < 0
	 || pixel_pos.x >= map_size.w || pixel_pos.y >= map_size.h ) {
		return Pos(-1, -1);
	}
	/* **************************************************
	 * 坐标系的x轴平分矩形，y轴平分地图左上角第一个地图块，
	 * 传入的坐标是以MapBox部件的左上角为原点，需要转换坐标
	 * **************************************************/
	/* 计算出Y轴的X轴坐标 */
	y_axis_x = (int)(mapbox->rows*mapbox->mapblk_size.w*0.5);
	pixel_pos.x = pixel_pos.x - y_axis_x;
	/* X轴的Y轴坐标就是map_size.h*0.5 */
	pixel_pos.y = (int)(map_size.h*0.5 - pixel_pos.y);
	/* 先计算出平行四边形的“左”边线所在直线的斜率k */
	k = -map_size.h * 1.0 / map_size.w;
	/* 计算出该点所在直线与“左”边线的交点坐标 */
	d = pixel_pos.y - k * pixel_pos.x;
	x = (d - map_size.h*0.5)/(-2*k);
	y = k*x + d;
	pos.y = (int)y;
	pos.x = (int)x;
	/* 如果该点不在左边线的右下方 */
	if( pixel_pos.x < pos.x || pixel_pos.y > pos.y ) {
		pos.x = pos.y = -1;
		return pos;
	}
	/* 该点与交点的距离就是地图坐标系中的X轴坐标 */
	px = get_point_distance( pos, pixel_pos );
	
	/* 计算出该点所在直线与“右”边的交点坐标 */
	d = pixel_pos.y + k * pixel_pos.x;
	x = (d - map_size.h*0.5)/(2*k);
	y = -k*x + d;
	pos.y = (int)y;
	pos.x = (int)x;
	/* 如果该点不在上边线的左下方 */
	if( pixel_pos.x > pos.x || pixel_pos.y > pos.y ) {
		pos.x = pos.y = -1;
		return pos;
	}
	/* 得出地图坐标系中的Y轴坐标 */
	py = get_point_distance( pos, pixel_pos );

	/* 计算出地图块在地图坐标系的xy轴跨距 */
	d = sqrt( pow(mapbox->mapblk_size.w*0.5, 2)
		 + pow(mapbox->mapblk_size.h*0.5, 2) );

	/* 得出以地图块为单位距离的坐标 */
	pos.y = (int)(py / d);
	pos.x = (int)(px / d);
	return pos;
}

/* 获取地图块的尺寸 */
LCUI_Size MapBox_GetMapBlockSize( LCUI_Widget *widget )
{
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	return mapbox->mapblk_size;
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
	if( mapbox->blocks[row][col].blk_horiz_flip ) {
		Graph_HorizFlip( img_mapblock, &buff );
		img_mapblock = &buff;
	}
	if( mapbox->blocks[row][col].blk_verti_flip ) {
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
		if( mapbox->current_mapblk_id >= 0 ) {
			n = mapbox->current_mapblk_id;
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

/* 重绘指定坐标的地图对象 */
static int MapBox_RedrawMapObj( LCUI_Widget *widget, int row, int col )
{
	int n;
	LCUI_Pos pos, offset;
	LCUI_Size size;
	MapBox_Data *mapbox;
	MapObjIMG *mapobj_data;
	LCUI_Widget *obj_widget;
	LCUI_Graph *img_mapobj, buff;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( row < 0 || row >= mapbox->rows
	|| col < 0 || col >= mapbox->cols ) {
		return -1;
	}
	
	Graph_Init( &buff );
	if( mapbox->higlight.x == col
	 && mapbox->higlight.y == row
	 && mapbox->current_mapobj_id >= 0 ) {
		n = mapbox->current_mapobj_id;
	} else {
		n = mapbox->blocks[row][col].obj_id;
	}
	pos = MapBox_MapBlock_GetPixelPos( widget, row, col );
	/* 如果该地图块上显示的对象ID有效 */
	if( n >= 0 ) {
		mapobj_data = MapBox_GetMapObjIMG( widget, n );
		img_mapobj = &mapobj_data->img;
		offset = mapobj_data->offset;
		/* 处理水平翻转 */
		if( mapbox->blocks[row][col].obj_horiz_flip ) {
			Graph_HorizFlip( img_mapobj, &buff );
		} else {
			Graph_Copy( &buff, img_mapobj );
		}
		img_mapobj = &buff;
		size = Graph_GetSize( img_mapobj );
	} else {
		img_mapobj = NULL;
		size.w = size.h = 0;
		offset.x = offset.y = 0;
	}
	obj_widget = mapbox->objs_widget[row][col];
	/* 如果该坐标上还没有部件用于显示地图对象，则创建一个 */
	if( obj_widget == NULL ) {
		/* 对象ID无效，那就不创建了 */
		if( n < 0 ) {
			return 1;
		}
		obj_widget = Widget_New(NULL);
		Widget_Container_Add( widget, obj_widget );
		mapbox->objs_widget[row][col] = obj_widget;
	}
	Widget_Resize( obj_widget, size );
	/* 如果该部件之前有背景图，则释放它 */
	if( Graph_IsValid( &obj_widget->background.image ) ) {
		Graph_Free( &obj_widget->background.image );
	}
	Widget_SetBackgroundImage( obj_widget, img_mapobj );
	Widget_SetBackgroundLayout( obj_widget, LAYOUT_CENTER );
	/* 计算部件的位置 */
	pos.x += ((MAP_BLOCK_WIDTH-size.w)/2);
	pos.y += (MAP_BLOCK_HEIGHT-size.h);
	pos.x += offset.x;
	pos.y += offset.y;
	Widget_Move( obj_widget, pos );
	/* 根据坐标来计算部件的z-index值 */
	n = mapbox->rows * mapbox->cols + col;
	/* 设置部件的堆叠顺序 */
	Widget_SetZIndex( obj_widget, n );
	Widget_Show( obj_widget );
	/* 刷新部件所在区域 */
	Widget_Refresh( obj_widget );
	return 0;
}

static void MapBox_RefreshObjIMG( LCUI_Widget *widget )
{
	int i, j;
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	for(i=0; i<mapbox->rows; ++i) {
		for(j=0; j<mapbox->cols; ++j) {
			MapBox_RedrawMapObj( widget, i, j );
		}
	}
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
	start_x = (mapbox->rows-1)*mapbox->mapblk_size.w*0.5;
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
			if( mapbox->blocks[i][j].blk_horiz_flip ) {
				Graph_HorizFlip( img_mapblock, &buff );
				img_mapblock = &buff;
			}
			if( mapbox->blocks[i][j].blk_verti_flip ) {
				Graph_VertiFlip( img_mapblock, &buff );
				img_mapblock = &buff;
			}
			/* 粘贴至部件图层上 */
			Graph_Mix( graph, img_mapblock, pos );
			x += (mapbox->mapblk_size.w*0.5);
			y += (mapbox->mapblk_size.h*0.5);
		}
		start_x -= (mapbox->mapblk_size.w*0.5);
		start_y += (mapbox->mapblk_size.h*0.5);
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
	map_blocks_data **tmp_mapblocks, **new_mapblocks;
	LCUI_Widget ***tmp_objs_widget, ***new_objs_widget;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	new_mapblocks = (map_blocks_data**)malloc(
			 rows*sizeof(map_blocks_data*) );
	if( !new_mapblocks ) {
		return -1;
	}
	new_objs_widget = (LCUI_Widget***)malloc(
			 rows*sizeof(LCUI_Widget**) );
	if( !new_objs_widget ) {
		free( new_mapblocks );
		return -1;
	}
	for(i=0; i<rows; ++i) {
		new_mapblocks[i] = (map_blocks_data*)malloc( cols
					*sizeof(map_blocks_data) );
		new_objs_widget[i] = (LCUI_Widget**)malloc(
					cols*sizeof(LCUI_Widget*) );
		if( !new_mapblocks[i] || !new_objs_widget[i] ) {
			for(j=i-1; j>=0; --j) {
				free( new_mapblocks[i] );
				free( new_objs_widget[i] );
			}
			free( new_mapblocks );
			free( new_objs_widget );
			return -1;
		}
		for(j=0; j<cols; ++j) {
			new_objs_widget[i][j] = NULL;
			new_mapblocks[i][j].blk_id = 0;
			new_mapblocks[i][j].obj_id = -1;
			new_mapblocks[i][j].blk_verti_flip = FALSE;
			new_mapblocks[i][j].blk_horiz_flip = FALSE;
			new_mapblocks[i][j].obj_horiz_flip = FALSE;
		}
	}

	tmp_mapblocks = mapbox->blocks;
	tmp_objs_widget = mapbox->objs_widget;
	/* 记录新的地图块信息 */
	mapbox->blocks = new_mapblocks;
	mapbox->objs_widget = new_objs_widget;
	if( tmp_mapblocks != NULL )  {
		/* 释放现有的地图块信息 */
		for(i=0; i<mapbox->rows; ++i) {
			free( tmp_mapblocks[i] );
			free( tmp_objs_widget[i] );
		}
		free( tmp_mapblocks );
		free( tmp_objs_widget );
	}
	mapbox->rows = rows;
	mapbox->cols = cols;
	/* 标记需要重绘 */
	Widget_Draw( widget );
	return 0;
}

/* 调整地图尺寸 */
int MapBox_ResizeMap( LCUI_Widget *widget, int rows, int cols, POSBOX_POS flag )
{
	int i, j, x, y;
	LCUI_Rect rect, cut_rect;
	MapBox_Data *mapbox;
	map_blocks_data **tmp_mapblocks, **new_mapblocks;
	LCUI_Widget ***tmp_objs_widget, ***new_objs_widget;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	new_mapblocks = (map_blocks_data**)malloc(
			 rows*sizeof(map_blocks_data*) );
	if( !new_mapblocks ) {
		return -1;
	}
	new_objs_widget = (LCUI_Widget***)malloc(
			 rows*sizeof(LCUI_Widget**) );
	if( !new_objs_widget ) {
		free( new_mapblocks );
		return -1;
	}
	for(i=0; i<rows; ++i) {
		new_mapblocks[i] = (map_blocks_data*)malloc( cols
					*sizeof(map_blocks_data) );
		new_objs_widget[i] = (LCUI_Widget**)malloc(
					cols*sizeof(LCUI_Widget*) );
		if( !new_mapblocks[i] || !new_objs_widget[i] ) {
			for(j=i-1; j>=0; --j) {
				free( new_mapblocks[i] );
				free( new_objs_widget[i] );
			}
			free( new_mapblocks );
			free( new_objs_widget );
			return -1;
		}
		for(j=0; j<cols; ++j) {
			new_objs_widget[i][j] = NULL;
			new_mapblocks[i][j].blk_id = 0;
			new_mapblocks[i][j].obj_id = -1;
			new_mapblocks[i][j].blk_verti_flip = FALSE;
			new_mapblocks[i][j].blk_horiz_flip = FALSE;
			new_mapblocks[i][j].obj_horiz_flip = FALSE;
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
			new_objs_widget[i][j] = mapbox->objs_widget[y+cut_rect.y][cut_rect.x+x];
		}
	}
	tmp_mapblocks = mapbox->blocks;
	tmp_objs_widget = mapbox->objs_widget;
	/* 记录新的地图块信息 */
	mapbox->blocks = new_mapblocks;
	mapbox->objs_widget = new_objs_widget;
	if( tmp_mapblocks != NULL )  {
		/* 释放现有的地图块信息 */
		for(i=0; i<mapbox->rows; ++i) {
			free( tmp_mapblocks[i] );
			free( tmp_objs_widget[i] );
		}
		free( tmp_mapblocks );
		free( tmp_objs_widget );
	}
	mapbox->rows = rows;
	mapbox->cols = cols;
	/* 标记需要重绘 */
	Widget_Draw( widget );
	/* 刷新地图对象的坐标 */
	MapBox_RefreshObjIMG( widget );
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
		MapBox_RedrawMapObj( widget, tmp_pos.y, tmp_pos.x );
	}

	if( pos.x < 0 || pos.y < 0
	 || pos.x >= mapbox->cols || pos.y >= mapbox->rows ) {
		mapbox->higlight.x = -1;
		mapbox->higlight.y = -1;
		return -1;
	}
	MapBox_RedrawMapBlock( widget, pos.y, pos.x );
	MapBox_RedrawMapObj( widget, pos.y, pos.x );
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

/* 获取已选中的地图块的坐标，没有选中的地图块则返回FALSE，否则返回TRUE */
LCUI_BOOL MapBox_GetSelected( LCUI_Widget *widget, LCUI_Pos *pos )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( mapbox->selected.x == -1
	 || mapbox->selected.y == -1 ) {
		pos->x = pos->y = 0;
		return FALSE;
	}
	*pos = mapbox->selected;
	return TRUE;
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
	/* 查找是否有已经存在的 */
	img = MapBox_GetMapBlockIMG( widget, id );
	/* 如果没有，则新增 */
	if( img == NULL ) {
		img = (MapBlockIMG*)malloc( sizeof(MapBlockIMG) );
		if( img == NULL ) {
			return -1;
		}
		img->id = id;
		mapbox = (MapBox_Data*)Widget_GetPrivData( widget );
		Queue_AddPointer( &mapbox->mapblk_img, img );
	}
	/* 然后记录图像 */
	if( mapblk_img == NULL ) {
		Graph_Init( &img->img );
	} else {
		img->img = *mapblk_img;
	}
	return 0;
}

/* 设定指定ID的地图对象的图像 */
int MapBox_SetMapObjIMG(	LCUI_Widget *widget, int id,
				LCUI_Graph *mapobj_img, LCUI_Pos offset )
{
	MapObjIMG *img, buff;
	MapBox_Data *mapbox;
	
	img = MapBox_GetMapObjIMG( widget, id );
	mapbox = (MapBox_Data*)Widget_GetPrivData( widget );
	if( img == NULL ) {
		buff.id = id;
		buff.offset = offset;
		if( mapobj_img == NULL ) {
			Graph_Init( &buff.img );
		} else {
			buff.img = *mapobj_img;
		}
		Queue_Add( &mapbox->mapobj_img, &buff );
	} else {
		img->offset = offset;
		if( mapobj_img == NULL ) {
			Graph_Init( &img->img );
		} else {
			img->img = *mapobj_img;
		}
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

/* 设定已选定的地图块上的地图对象所使用的ID */
int MapBox_SetMapObj( LCUI_Widget *widget, int mapobj_id )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	if( mapbox->selected.x == -1 || mapbox->selected.x >= mapbox->cols
	|| mapbox->selected.y == -1 || mapbox->selected.y >= mapbox->rows ) {
		return -1;
	}
	mapbox->blocks[mapbox->selected.y][mapbox->selected.x].obj_id = mapobj_id;
	MapBox_RedrawMapObj( widget, mapbox->selected.y, mapbox->selected.x );
	return 0;
}

/* 设定当前使用的地图块 */
void MapBox_SetCurrentMapBlock( LCUI_Widget *widget, int mapblock_id )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->current_mapblk_id = mapblock_id;
	MapBox_RedrawMapBlock( widget, mapbox->higlight.y,  mapbox->higlight.x );
}

/* 设定当前使用的地图对象 */
void MapBox_SetCurrentMapObj( LCUI_Widget *widget, int mapobj_id )
{
	MapBox_Data *mapbox;
	
	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->current_mapobj_id = mapobj_id;
	MapBox_RedrawMapObj( widget, mapbox->higlight.y,  mapbox->higlight.x );
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
	mapbox->blocks[row][col].blk_verti_flip = (!mapbox->blocks[row][col].blk_verti_flip);
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
	mapbox->blocks[row][col].blk_horiz_flip = (!mapbox->blocks[row][col].blk_horiz_flip);
	MapBox_RedrawMapBlock( widget, row, col );
	return 0;
}

/* 对已选中的地图对象进行水平翻转 */
int MapBox_MapObj_HorizFlip( LCUI_Widget *widget )
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
	mapbox->blocks[row][col].obj_horiz_flip = (!mapbox->blocks[row][col].obj_horiz_flip);
	MapBox_RedrawMapObj( widget, row, col );
	return 0;
}

/* 从文件中载入地图数据 */
int MapBox_LoadMapData( LCUI_Widget *widget, const char *mapdata_filepath )
{
	FILE *fp;
	int row, col, n;
	MapBox_Data *mapbox;
	mapfile_header map_info;
	LCUI_Widget ***tmp_objs_widget, ***new_objs_widget;
	map_blocks_data **tmp_mapblocks, **new_mapblocks;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	fp = fopen( mapdata_filepath, "rb" );
	if(fp == NULL) {
		return -1;
	}
	n = fread( &map_info, sizeof(mapfile_header), 1, fp );
	if( n < 1) {
		return -2;
	}
	/* 如果文件头中没有“LCUIMAP”这个字符串就退出 */
	if( strcmp( map_info.type, "LCUIMAP") != 0 ) {
		fclose( fp );
		return -2;
	}
	/* 如果文件版本不是当前支持的版本，也退出 */
	if( map_info.version != MAPFILE_VERSION ) {
		fclose( fp );
		return -3;
	}
	new_mapblocks = (map_blocks_data**)malloc(map_info.rows
					*sizeof(map_blocks_data*) );
	if( !new_mapblocks ) {
		return -4;
	}
	
	new_objs_widget = (LCUI_Widget***)malloc(
			 map_info.rows*sizeof(LCUI_Widget**) );
	if( !new_objs_widget ) {
		return -4;
	}
	for( row=0; row<map_info.rows; ++row ) {
		new_mapblocks[row] = (map_blocks_data*)malloc(
				map_info.cols*sizeof(map_blocks_data) );
		new_objs_widget[row] = (LCUI_Widget**)malloc(
				map_info.cols*sizeof(LCUI_Widget*) );
		if( !new_mapblocks[row] || !new_objs_widget[row] ) {
			goto error_return;
		}
		for( col=0; col<map_info.cols; ++col ) {
			n = fread( &new_mapblocks[row][col],
			sizeof(map_blocks_data), 1, fp );
			if( n < 1 ) {
				++row;
				++col;
				goto error_return;
			}
			new_objs_widget[row][col] = NULL;
		}
	}
	fclose( fp );
	goto no_error;
error_return:;
	for(row=row-1; row>=0; --row) {
		free( new_objs_widget[row] );
		free( new_mapblocks[row] );
	}
	free( new_objs_widget );
	free( new_mapblocks );
	return -4;
no_error:;
	tmp_mapblocks = mapbox->blocks;
	tmp_objs_widget = mapbox->objs_widget;
	mapbox->blocks = new_mapblocks;
	mapbox->objs_widget = new_objs_widget;
	if( tmp_mapblocks != NULL )  {
		/* 释放现有的地图块信息 */
		for(row=0; row<mapbox->rows; ++row) {
			free( tmp_mapblocks[row] );
			free( tmp_objs_widget[row] );
		}
		free( tmp_mapblocks );
		free( tmp_objs_widget );
	}
	mapbox->rows = map_info.rows;
	mapbox->cols = map_info.cols;
	Widget_Draw( widget );
	MapBox_RefreshObjIMG( widget);
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
static void 
MapBox_ProcMouseMotionEvent( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	LCUI_Pos pos;

	DEBUG_MSG("relative pixel pos: %d, %d\n", pos.x, pos.y);
	pos = MapBox_MapBlock_GetPos( widget, event->mouse_motion.rel_pos );
	DEBUG_MSG("mapblock pos: %d, %d\n", pos.x, pos.y);
	/* 高亮指定地图块 */
	MapBox_HiglightMapBlock( widget, pos );
}

static void 
MapBox_ProcClickedEvent( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	MapBox_Data *mapbox;
	LCUI_Widget *mapobj_widget = NULL;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	MapBox_SelectMapBlock( widget, mapbox->higlight );
	MapBox_SetMapBlock( widget, mapbox->current_mapblk_id );
	if( mapbox->current_mapobj_id >= 0 ) {
		MapBox_SetMapObj( widget, mapbox->current_mapobj_id );
	}
	if( mapbox->current_mapblk_id == -1 ) {
		mapobj_widget = Widget_At( widget, event->clicked.rel_pos );
	} else {
		if( mapbox->mapblk_clicked != NULL ) {
			mapbox->mapblk_clicked( widget );
			return;
		}
	}
	if( mapobj_widget == NULL ) {
		if( mapbox->mapblk_clicked != NULL ) {
			mapbox->mapblk_clicked( widget );
		}
	} else {
		if( mapbox->mapobj_clicked != NULL ) {
			mapbox->mapobj_clicked( mapobj_widget );
		}
	}
}

/* 连接地图对象的Clicked事件，在鼠标指针点击地图对象时，会调用已连接的函数 */
void MapBox_ConnectMapObjClicked( LCUI_Widget *widget, void (*func)(LCUI_Widget*) )
{
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->mapobj_clicked = func;
}

/* 连接地图块的Clicked事件，在鼠标指针点击地图块时，会调用已连接的函数 */
void MapBox_ConnectMapBlockClicked( LCUI_Widget *widget, void (*func)(LCUI_Widget*) )
{
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData( widget );
	mapbox->mapblk_clicked = func;
}

/* MapBox部件的析构函数 */
static void MapBox_ExecDestroy( LCUI_Widget *widget )
{
	int i;
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)Widget_GetPrivData(widget); 
	for(i=0; i<mapbox->rows; ++i) {
		free( mapbox->blocks[i] );
		free( mapbox->objs_widget[i] );
	}
	free( mapbox->blocks );
	free( mapbox->objs_widget );
}

static void MapBox_ExecInit( LCUI_Widget *widget )
{
	MapBox_Data *mapbox;

	mapbox = (MapBox_Data *)WidgetPrivData_New(widget, sizeof(MapBox_Data)); 
	mapbox->rows = mapbox->cols = 0;
	mapbox->selected.x = mapbox->selected.y = -1;
	mapbox->higlight.x = mapbox->higlight.y = -1;
	mapbox->blocks = NULL;
	mapbox->objs_widget = NULL;
	mapbox->current_mapblk_id = -1;
	mapbox->current_mapobj_id = -1;
	mapbox->mapblk_size = Size(0,0);
	mapbox->mapblk_clicked = NULL;
	mapbox->mapobj_clicked = NULL;
	Queue_Init( &mapbox->mapblk_img, sizeof(MapBlockIMG), NULL );
	Queue_Init( &mapbox->mapobj_img, sizeof(MapObjIMG), NULL );
	/* 关联鼠标移动事件,点击事件，以及拖动事件 */
	Widget_Event_Connect( widget, EVENT_MOUSEMOTION, MapBox_ProcMouseMotionEvent );
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
	WidgetFunc_Add("mapbox", MapBox_ExecDestroy, FUNC_TYPE_DESTROY);
}

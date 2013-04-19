#include <LCUI_Build.h>
#include LC_LCUI_H 
#include LC_GRAPH_H
#include LC_DRAW_H
#include LC_WIDGET_H 
#include LC_WINDOW_H
#include LC_LABEL_H
#include LC_BUTTON_H
#include LC_TEXTBOX_H
#include LC_INPUT_H
#include LC_RES_H

#include "mapbox.h"
#include "res_map.h"

#define IMG_PATH_ICON		"drawable/icon.png"
#define IMG_PATH_MAP		"drawable/map.png"
#define IMG_PATH_OBJS		"drawable/objs.png"
#define IMG_PATH_BTN_VERTI	"drawable/btn_vertiflip.png"
#define IMG_PATH_BTN_HORIZ	"drawable/btn_horizflip.png"
#define IMG_PATH_BTN_SAVE	"drawable/btn_save.png"
#define IMG_PATH_BTN_RESIZE	"drawable/btn_resize.png"
#define MAPFILE_PATH		"map.dat"

#define MAP_OBJ_WIDTH	52
#define MAP_OBJ_HEIGHT	43
#define MAP_OBJ_TOTAL	6


static LCUI_Graph wnd_icon, map_res, obj_res;
static LCUI_Graph map_blocks[MAP_BLOCK_TOTAL], map_objs[MAP_OBJ_TOTAL];
static LCUI_Graph img_btn_vertiflip, img_btn_horizflip, img_btn_save, img_btn_resize;

static LCUI_Widget *window, *mapbox_window, *mapobj_window;
static LCUI_Widget *btn[MAP_BLOCK_TOTAL], *obj_btn[MAP_OBJ_TOTAL], *mapbox;
static LCUI_Widget *btn_vertiflip, *btn_horizflip, *btn_save, *btn_resize;

#ifdef LCUI_BUILD_IN_WIN32
#include <io.h>
#include <fcntl.h>

/* 在运行程序时会打开控制台，以查看打印的调试信息 */
static void InitConsoleWindow(void)
{
	int hCrt;
	FILE *hf;
	AllocConsole();
	hCrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT );
	hf=_fdopen( hCrt, "w" );
	*stdout=*hf;
	setvbuf (stdout, NULL, _IONBF, 0);
	// test code
	printf ("InitConsoleWindow OK!\n");
}
#endif

static void load_res(void)
{
	int i;
	LCUI_Rect rect;
	LCUI_Rect obj_rect[MAP_OBJ_TOTAL]={
		{0,0,42,28},{87,0,52,43},{167,0,22,35},
		{0,28,53,40},{128,35,51,33},{98,81,43,38}
	};

	Graph_Init( &wnd_icon );
	Graph_Init( &map_res );
	Graph_Init( &obj_res );
	Graph_Init( &img_btn_horizflip );
	Graph_Init( &img_btn_vertiflip );
	Graph_Init( &img_btn_resize );
	Graph_Init( &img_btn_save );
	Load_Image( IMG_PATH_BTN_HORIZ, &img_btn_horizflip );
	Load_Image( IMG_PATH_BTN_VERTI, &img_btn_vertiflip );
	Load_Image( IMG_PATH_ICON, &wnd_icon );
	Load_Image( IMG_PATH_BTN_RESIZE, &img_btn_resize );
	Load_Image( IMG_PATH_BTN_SAVE, &img_btn_save );
	/* 载入地图图块资源 */
	i = Load_Image( IMG_PATH_MAP, &map_res );
	if( i != 0 ) {
		LCUI_MessageBoxW( MB_ICON_ERROR, 
		L"地图块的图像资源载入失败！", L"资源载入出错", MB_BTN_OK );
		return;
	}
	rect.x = rect.y = 0;
	rect.width = MAP_BLOCK_WIDTH;
	rect.height = MAP_BLOCK_HEIGHT;
	/* 引用出每一个地图图块 */
	for(i=0; i<MAP_BLOCK_TOTAL; ++i, rect.x+=MAP_BLOCK_WIDTH) {
		Graph_Quote( &map_blocks[i], &map_res, rect );
	}
	/* 载入地图对象资源 */
	i = Load_Image( IMG_PATH_OBJS, &obj_res );
	if( i != 0 ) {
		LCUI_MessageBoxW( MB_ICON_ERROR, 
		L"地图对象的图像资源载入失败！", L"资源载入出错", MB_BTN_OK );
		return;
	}
	/* 引用出每一个地图对象 */
	for(i=0; i<MAP_OBJ_TOTAL; ++i) {
		Graph_Quote( &map_objs[i], &obj_res, obj_rect[i] );
	}
}

static void free_res(void)
{
	Graph_Free( &wnd_icon );
	Graph_Free( &map_res );
	Graph_Free( &obj_res );
	Graph_Free( &img_btn_horizflip );
	Graph_Free( &img_btn_vertiflip );
	Graph_Free( &img_btn_save );
	Graph_Free( &img_btn_resize );
}

static void
proc_mapbtn_clicked( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	int i;
	for( i=0; i<MAP_BLOCK_TOTAL; ++i ) {
		if( btn[i] == widget ) {
			MapBox_SetCurrentMapBlock( mapbox, i );
			MapBox_SetCurrentMapObj( mapbox, -1 );
			return;
		}
	}
	MapBox_SetCurrentMapBlock( mapbox, -1 );
}


static void
proc_objbtn_clicked( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	int i;
	for( i=0; i<MAP_OBJ_TOTAL; ++i ) {
		if( obj_btn[i] == widget ) {
			MapBox_SetCurrentMapObj( mapbox, i );
			MapBox_SetCurrentMapBlock( mapbox, -1 );
			return;
		}
	}
	MapBox_SetCurrentMapObj( mapbox, -1 );
}

/* 响应鼠标右键，取消地图块/地图对象摆放 */
static void 
mapbox_proc_mousebutton( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	if( event->mouse_button.button != LCUIKEY_RIGHTBUTTON ) {
		return;
	}
	MapBox_SetCurrentMapBlock( mapbox, -1 );
	MapBox_SetCurrentMapObj( mapbox, -1 );
}

/* 更新地图位置 */
static void 
update_mapbox_pos( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	LCUI_Pos pos;
	LCUI_Size mapbox_size, ctnr_size;

	ctnr_size = Widget_GetContainerSize( Window_GetClientArea(window) );
	mapbox_size = Widget_GetSize( mapbox );
	if( mapbox_size.w <= ctnr_size.w
	 && mapbox_size.h <= ctnr_size.h ) {
		Widget_SetAlign( mapbox, ALIGN_MIDDLE_CENTER, Pos(0,0) );
		return;
	}
	pos = Widget_GetPos( mapbox );
	/* 调整坐标 */
	if( pos.x > 0 ) {
		pos.x = 0;
	}
	if( pos.y > 0 ) {
		pos.y = 0;
	}
	if( pos.x + mapbox_size.w  < ctnr_size.w ) {
		pos.x = ctnr_size.w - mapbox_size.w;
	}
	if( pos.y + mapbox_size.h  < ctnr_size.h ) {
		pos.y = ctnr_size.h - mapbox_size.h;
	}
	/* 根据地图尺寸，进行不同调整 */
	if( mapbox_size.w > ctnr_size.w
	 && mapbox_size.h <= ctnr_size.h ) {
		pos.y = (ctnr_size.h-mapbox_size.h)/2;
	} else if( mapbox_size.w <= ctnr_size.w
	 && mapbox_size.h > ctnr_size.h ) {
		pos.x = (ctnr_size.w-mapbox_size.w)/2;
	}
	/* 解除之前设定的align */
	Widget_SetAlign( mapbox, ALIGN_NONE, Pos(0,0) );
	Widget_Move( mapbox, pos );
}

static int button_type = 0;

static void proc_btn_ok( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	button_type = 0;
	LCUI_MainLoop_Quit(NULL);
}

static void porc_btn_cancel( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	button_type = 1;
	LCUI_MainLoop_Quit(NULL);
}

static void proc_btn_save( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	int ret;
	ret = MapBox_SaveMapData( mapbox, MAPFILE_PATH );
	if( ret != 0 ) {
		LCUI_MessageBoxW( MB_ICON_ERROR, L"地图数据保存出错！", L"错误", MB_BTN_OK );
	} else {
		LCUI_MessageBoxW( MB_ICON_INFO, L"地图数据保存成功！", L"提示", MB_BTN_OK );
	}
}


/* 点击按钮后，显示地图尺寸调整窗口 */
static void proc_btn_resize_clicked( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	POSBOX_POS flag;
	LCUI_Widget *wnd;
	LCUI_Widget *label_oldsize, *label_x, *label_newsize;
	LCUI_Widget *tb_rows, *tb_cols;
	LCUI_Widget *posbox, *label_pos;
	LCUI_Widget *btn_ok, *btn_cancel;
	LCUI_MainLoop *loop;
	LCUI_Size map_size;
	wchar_t text[32], text_cols[10], text_rows[10];

	wnd = Widget_New("window");
	tb_cols = Widget_New("text_box");
	tb_rows = Widget_New("text_box");
	label_newsize = Widget_New("label");
	label_oldsize = Widget_New("label");
	label_pos = Widget_New("label");
	label_x = Widget_New("label");
	posbox = Widget_New("posbox");
	btn_ok = Widget_New("button");
	btn_cancel = Widget_New("button");

	Window_SetTitleTextW( wnd, L"地图大小" );
	/* 将部件添加至窗口客户区中 */
	Window_ClientArea_Add( wnd, tb_cols );
	Window_ClientArea_Add( wnd, tb_rows );
	Window_ClientArea_Add( wnd, label_newsize );
	Window_ClientArea_Add( wnd, label_oldsize );
	Window_ClientArea_Add( wnd, label_x );
	Window_ClientArea_Add( wnd, label_pos );
	Window_ClientArea_Add( wnd, btn_ok );
	Window_ClientArea_Add( wnd, btn_cancel );
	Window_ClientArea_Add( wnd, posbox );
	/* 设置按钮文本 */
	Button_TextW( btn_ok, L"确定" );
	Button_TextW( btn_cancel, L"取消" );
	/* 设置label部件显示的文本 */
	Label_TextW( label_newsize, L"新的大小：" );
	Label_TextW( label_x, L"x" );
	Label_TextW( label_pos, L"伸缩方向：" );

	Widget_SetAutoSize( btn_ok, FALSE, 0 );
	Widget_SetAutoSize( btn_cancel, FALSE, 0 );
	/* 调整部件的位置 */
	Widget_Move( label_oldsize, Pos(4,6) );
	Widget_Move( label_newsize, Pos(4,29) );
	Widget_Move( tb_cols, Pos(40+22,27) );
	Widget_Move( tb_rows, Pos(80+22,27) );
	Widget_Move( label_x, Pos(80+13,27) );
	Widget_Move( label_pos, Pos(4,52) );
	Widget_Move( posbox, Pos(35,73) );

	Widget_SetAlign( btn_ok, ALIGN_BOTTOM_CENTER, Pos(-26,-5) );
	Widget_SetAlign( btn_cancel, ALIGN_BOTTOM_CENTER, Pos(26,-5) );
	Widget_Hide( Window_GetCloseButton(wnd) );
	/* 调整按钮的尺寸 */
	Widget_Resize( tb_rows, Size(28,22) );
	Widget_Resize( tb_cols, Size(28,22) );
	Widget_Resize( btn_ok, Size(50,25) );
	Widget_Resize( btn_cancel, Size(50,25) );
	Widget_Resize( wnd, Size(155,215) );
	/* 设置当前窗口为模态部件 */
	Widget_SetModal( wnd, TRUE );
	/* 限制这两个文本框只能被输入数字 */
	TextBox_LimitInput( tb_rows, L"0123456789" );
	TextBox_LimitInput( tb_cols, L"0123456789" );
	/* 限制文本框最多输入3位数 */
	TextBox_Text_SetMaxLength( tb_rows, 3 );
	TextBox_Text_SetMaxLength( tb_cols, 3 );

	map_size = MapBox_GetMapSize( mapbox );
	
	swprintf( text_rows, 10, L"%d", map_size.h );
	swprintf( text_cols, 10, L"%d", map_size.w );
	swprintf( text, 32, L"当前大小：%d x %d", map_size.w,map_size.h );
	Label_TextW( label_oldsize, text );
	TextBox_TextW( tb_rows, text_rows );
	TextBox_TextW( tb_cols, text_cols );

	Widget_Show( tb_cols );
	Widget_Show( tb_rows );
	Widget_Show( label_x );
	Widget_Show( label_pos );
	Widget_Show( label_newsize );
	Widget_Show( label_oldsize );
	Widget_Show( btn_ok );
	Widget_Show( btn_cancel );
	Widget_Show( posbox );
	Widget_Show( wnd );

	Widget_Event_Connect( btn_ok, EVENT_CLICKED, proc_btn_ok );
	Widget_Event_Connect( btn_cancel, EVENT_CLICKED, porc_btn_cancel );
	while(1) {
		loop = LCUI_MainLoop_New();
		LCUI_MainLoop_Run( loop );
		if( button_type == 1 ) {
			break;
		}
		TextBox_GetText( tb_rows, text_rows, 10 );
		TextBox_GetText( tb_cols, text_cols, 10 );
		swscanf( text_rows, L"%d", &map_size.h );
		swscanf( text_cols, L"%d", &map_size.w );
		if( map_size.w > 0 && map_size.h > 0 ) {
			flag = PosBox_GetPos( posbox );
			MapBox_ResizeMap( mapbox, map_size.h, map_size.w, flag );
			break;
		}
		LCUI_MessageBoxW( MB_ICON_WARNING, 
		L"无效的地图尺寸！", L"错误", MB_BTN_OK );
	}
	Widget_Hide( wnd );
	Widget_Destroy( wnd );
}

static void 
proc_btn_vertiflip_clicked( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	MapBox_MapBlock_VertiFlip( mapbox );
}

static void 
proc_btn_horizflip_clicked( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	MapBox_MapBlock_HorizFlip( mapbox );
}

/* 初始化地图对象窗口 */
static void mapobjbox_init(void)
{
	int i;
	LCUI_Size size;
	LCUI_Pos pos;

	mapobj_window = Widget_New("window");
	Window_SetTitleTextW( mapobj_window, L"地图对象" );
	size.w = 2*(MAP_OBJ_WIDTH+4)+10;
	size.h = (int)(MAP_OBJ_TOTAL/2.0+0.5)*(MAP_OBJ_HEIGHT+2) + 32;
	Widget_Resize( mapobj_window, size );
	Widget_Hide( Window_GetCloseButton(mapobj_window) );

	pos.y = -(MAP_OBJ_HEIGHT+2);
	for(i=0; i<MAP_OBJ_TOTAL; ++i) {
		if( i%2==0 ) {
			pos.x = 0;
			pos.y += (MAP_OBJ_HEIGHT+2);
		} else {
			pos.x = (MAP_OBJ_WIDTH+4);
		}
		obj_btn[i] = Widget_New("button");
		Window_ClientArea_Add( mapobj_window, obj_btn[i] );
		Widget_SetBackgroundImage( obj_btn[i], &map_objs[i] );
		Widget_SetBackgroundLayout( obj_btn[i], LAYOUT_CENTER );
		Widget_SetAutoSize( obj_btn[i], FALSE, 0 );
		Widget_Resize( obj_btn[i], Size(MAP_OBJ_WIDTH+4,MAP_OBJ_HEIGHT+2) );
		Widget_Move( obj_btn[i], pos );
		Widget_Event_Connect( obj_btn[i], EVENT_CLICKED, proc_objbtn_clicked );
		Widget_Show( obj_btn[i] );
	}
	Widget_SetZIndex( mapobj_window, 10 );
	Widget_Show( mapobj_window );
}

/* 初始化地图块窗口 */
static void mapblkbox_init(void)
{
	int i;
	LCUI_Size size;
	LCUI_Pos pos;

	mapbox_window = Widget_New("window");
	Window_SetTitleTextW( mapbox_window, L"地图图块" );
	size.w = 2*MAP_BLOCK_WIDTH+10;
	size.h = (int)(MAP_BLOCK_TOTAL/2.0+0.5)*MAP_BLOCK_WIDTH + 32;
	Widget_Resize( mapbox_window, size );
	/* 隐藏窗口右上角的关闭按钮 */
	Widget_Hide( Window_GetCloseButton(mapbox_window) );
	
	/* 创建地图块按钮，并计算出相应的坐标 */
	pos.y = -MAP_BLOCK_WIDTH;
	for(i=0; i<MAP_BLOCK_TOTAL; ++i) {
		if( i%2==0 ) {
			pos.x = 0;
			pos.y += MAP_BLOCK_WIDTH;
		} else {
			pos.x = MAP_BLOCK_WIDTH;
		}
		btn[i] = Widget_New("button");
		Window_ClientArea_Add( mapbox_window, btn[i] );
		Widget_SetBackgroundImage( btn[i], &map_blocks[i] );
		Widget_SetBackgroundLayout( btn[i], LAYOUT_CENTER );
		Widget_SetAutoSize( btn[i], FALSE, 0 );
		Widget_Resize( btn[i], Size(MAP_BLOCK_WIDTH,MAP_BLOCK_WIDTH) );
		Widget_Move( btn[i], pos );
		Widget_Event_Connect( btn[i], EVENT_CLICKED, proc_mapbtn_clicked );
		Widget_Show( btn[i] );
	}
	Widget_SetZIndex( mapbox_window, 10 );
	Widget_Show(mapbox_window);
}

static void destroy( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	LCUI_MainLoop_Quit(NULL);
}

/* 初始化主窗口 */
static void window_init(void)
{
	window = Widget_New("window");
	Window_SetTitleTextW( window, L"地图编辑器" );
	Window_SetTitleIcon( window, &wnd_icon );
	Widget_Resize( window, Size(640, 480) );
	Widget_Event_Connect( Window_GetCloseButton(window), EVENT_CLICKED, destroy );
	Widget_SetZIndex( window, 0 );
	Widget_Show(window);
}

static void proc_mapbox_drag( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	LCUI_Pos offset, pos;
	LCUI_Size mapbox_size, ctnr_size;
	
	pos = Widget_GetGlobalPos( widget );
	offset = Pos_Sub( event->drag.new_pos, pos );
	pos = Widget_GetPos( widget );
	pos = Pos_Add( pos, offset );

	ctnr_size = Widget_GetContainerSize( Window_GetClientArea(window) );
	mapbox_size = Widget_GetSize( mapbox );
	if( mapbox_size.w <= ctnr_size.w
	 && mapbox_size.h <= ctnr_size.h ) {
		Widget_SetAlign( mapbox, ALIGN_MIDDLE_CENTER, Pos(0,0) );
		return;
	}
	/* 调整坐标 */
	if( pos.x > 0 ) {
		pos.x = 0;
	}
	if( pos.y > 0 ) {
		pos.y = 0;
	}
	if( pos.x + mapbox_size.w  < ctnr_size.w ) {
		pos.x = ctnr_size.w - mapbox_size.w;
	}
	if( pos.y + mapbox_size.h  < ctnr_size.h ) {
		pos.y = ctnr_size.h - mapbox_size.h;
	}
	/* 根据地图尺寸，进行不同调整 */
	if( mapbox_size.w > ctnr_size.w
	 && mapbox_size.h <= ctnr_size.h ) {
		pos.y = (ctnr_size.h-mapbox_size.h)/2;
	} else if( mapbox_size.w <= ctnr_size.w
	 && mapbox_size.h > ctnr_size.h ) {
		pos.x = (ctnr_size.w-mapbox_size.w)/2;
	}

	/* 解除之前设定的align */
	Widget_SetAlign( widget, ALIGN_NONE, Pos(0,0) );
	Widget_Move( widget, pos );
}

/* 初始化MapBox部件 */
static void mapbox_init(void)
{
	int i;
	Register_MapBox();
	mapbox = Widget_New("mapbox");
	Window_ClientArea_Add( window, mapbox );
	Widget_SetAlign( mapbox, ALIGN_MIDDLE_CENTER, Pos(0,0) );
	MapBox_SetMapBlockSize( mapbox, MAP_BLOCK_WIDTH, MAP_BLOCK_HEIGHT );
	/* 设置MapBox部件的地图图块 */
	for(i=0; i<MAP_BLOCK_TOTAL; ++i) {
		MapBox_SetMapBlockIMG( mapbox, i, &map_blocks[i] );
	}
	/* 设置MapBox部件的地图对象 */
	for(i=0; i<MAP_OBJ_TOTAL; ++i) {
		MapBox_SetMapObjIMG( mapbox, i, &map_objs[i] );
	}
	MapBox_CreateMap( mapbox, 4, 4 );
	Widget_Event_Connect( mapbox, EVENT_DRAG, proc_mapbox_drag );
	/* 在地图框改变尺寸时，更新它在窗口内的位置 */
	Widget_Event_Connect( mapbox, EVENT_RESIZE, update_mapbox_pos );
	/* 响应鼠标右键点击 */
	Widget_Event_Connect( mapbox, EVENT_MOUSEBUTTON, mapbox_proc_mousebutton );

	i = MapBox_LoadMapData( mapbox, MAPFILE_PATH );
	Widget_Show( mapbox );
	switch( i ) {
	case -3:
		LCUI_MessageBoxW( MB_ICON_ERROR,
		 L"该版本的地图文件不被支持！",
		 L"地图文件错误", MB_BTN_OK );
		break;
	case -2:
		LCUI_MessageBoxW( MB_ICON_ERROR,
		 L"所载入的文件不是有效的地图文件！",
		 L"地图文件错误", MB_BTN_OK );
	case -1: break;
	case 0:
		LCUI_MessageBoxW( MB_ICON_INFO,
		 L"已经成功载入地图文件！",
		 L"提示", MB_BTN_OK );
		break;
	default: break;
	}
}


/* 初始化主窗口标题栏内的按钮 */
static void titebar_btn_init(void)
{
	/* 创建按钮 */
	btn_vertiflip = Widget_New("button");
	btn_horizflip = Widget_New("button");
	btn_save = Widget_New("button");
	btn_resize = Widget_New("button");
	/* 添加按钮 */
	Window_TitleBar_Add( window, btn_vertiflip );
	Window_TitleBar_Add( window, btn_horizflip );
	Window_TitleBar_Add( window, btn_resize );
	Window_TitleBar_Add( window, btn_save );
	/* 设置按钮不自动改变尺寸 */
	Widget_SetAutoSize( btn_vertiflip, FALSE, 0 );
	Widget_SetAutoSize( btn_horizflip, FALSE, 0 );
	Widget_SetAutoSize( btn_resize, FALSE, 0 );
	Widget_SetAutoSize( btn_save, FALSE, 0 );
	/* 设置按钮尺寸 */
	Widget_Resize( btn_vertiflip, Size(27,27) );
	Widget_Resize( btn_horizflip, Size(27,27) );
	Widget_Resize( btn_save, Size(27,27) );
	Widget_Resize( btn_resize, Size(27,27) );
	/* 设置按钮的背景图以及布局 */
	Widget_SetBackgroundImage( btn_vertiflip, &img_btn_vertiflip );
	Widget_SetBackgroundImage( btn_horizflip, &img_btn_horizflip );
	Widget_SetBackgroundImage( btn_save, &img_btn_save );
	Widget_SetBackgroundImage( btn_resize, &img_btn_resize );
	Widget_SetBackgroundLayout( btn_vertiflip, LAYOUT_CENTER );
	Widget_SetBackgroundLayout( btn_horizflip, LAYOUT_CENTER );
	Widget_SetBackgroundLayout( btn_save, LAYOUT_CENTER );
	Widget_SetBackgroundLayout( btn_resize, LAYOUT_CENTER );
	/* 设置按钮的位置 */
	Widget_SetAlign( btn_vertiflip, ALIGN_BOTTOM_RIGHT, Pos(-50,0) );
	Widget_SetAlign( btn_horizflip, ALIGN_BOTTOM_RIGHT, Pos(-(50+27),0) );
	Widget_SetAlign( btn_resize, ALIGN_BOTTOM_RIGHT, Pos(-(50+2*27),0) );
	Widget_SetAlign( btn_save, ALIGN_BOTTOM_RIGHT, Pos(-(50+3*27),0) );
	/* 为按钮关联相应事件 */
	Widget_Event_Connect( btn_vertiflip, EVENT_CLICKED, proc_btn_vertiflip_clicked );
	Widget_Event_Connect( btn_horizflip, EVENT_CLICKED, proc_btn_horizflip_clicked );
	Widget_Event_Connect( btn_resize, EVENT_CLICKED, proc_btn_resize_clicked );
	Widget_Event_Connect( btn_save, EVENT_CLICKED, proc_btn_save );
	/* 显示它们 */
	Widget_Show( btn_vertiflip );
	Widget_Show( btn_horizflip );
	Widget_Show( btn_save );
	Widget_Show( btn_resize );
}

int LCUIMainFunc( LCUI_ARGLIST )
{
#ifdef LCUI_BUILD_IN_WIN32
	InitConsoleWindow();
#endif
	setenv( "LCUI_FONTFILE", "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-R.ttf", FALSE );
	LCUI_Init(LCUI_DEFAULT_CONFIG);
	load_res();
	window_init();
	Register_PosBox();
	mapblkbox_init();
	mapobjbox_init();
	titebar_btn_init();
	mapbox_init();
	LCUIApp_AtQuit( free_res );
	return LCUI_Main();
}


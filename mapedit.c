#include <LCUI_Build.h>
#include LC_LCUI_H 
#include LC_GRAPH_H
#include LC_DRAW_H
#include LC_WIDGET_H 
#include LC_WINDOW_H
#include LC_LABEL_H
#include LC_BUTTON_H
#include LC_RES_H

#include "mapbox.h"
#include "res_map.h"

#define IMG_PATH_ICON	"drawable/icon.png"
#define IMG_PATH_MAP	"drawable/map.png"
#define MAP_FILE_PATH	"map.dat"

static LCUI_Graph wnd_icon, cursor_img, map_res, map_blocks[MAP_BLOCK_TOTAL];
static LCUI_Widget *window, *mapbox_window;
static LCUI_Widget *btn[MAP_BLOCK_TOTAL+1], *mapbox;

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

	Graph_Init( &wnd_icon );
	Graph_Init( &map_res );
	Graph_Init( &cursor_img );
	Load_Image( IMG_PATH_ICON, &wnd_icon );
	Load_Graph_Default_Cursor( &cursor_img );
	/* 载入地图资源 */
	load_res_map( &map_res );
	rect.x = rect.y = 0;
	rect.width = 45;
	rect.height = 21;
	/* 引用出每一个地图图块 */
	for(i=0; i<7; ++i, rect.x+=44) {
		Graph_Quote( &map_blocks[i], &map_res, rect );
	}
}

static void free_res(void)
{
	Graph_Free( &wnd_icon );
	Graph_Free( &map_res );
	Graph_Free( &cursor_img );
}

static void proc_mapbtn_clicked( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	int i;
	for( i=1; i<MAP_BLOCK_TOTAL+1; ++i ) {
		if( btn[i] == widget ) {
			MapBox_SetCurrentMapBlock( mapbox, i-1 );
			return;
		}
	}
	MapBox_SetCurrentMapBlock( mapbox, -1 );
}

static void map_toolbox_init(void)
{
	int i;
	LCUI_Size size;
	LCUI_Pos pos;

	mapbox_window = Widget_New("window");
	Window_SetTitleTextW( mapbox_window, L"地图图块" );
	size.w = 2*MAP_BLOCK_WIDTH+10;
	size.h = (int)(MAP_BLOCK_TOTAL/2.0+0.5)*MAP_BLOCK_WIDTH + 30;
	Widget_Resize( mapbox_window, size );
	Widget_Hide( Window_GetCloseButton(mapbox_window) );
	
	pos.y = -(MAP_BLOCK_WIDTH+2);
	for(i=0; i<MAP_BLOCK_TOTAL+1; ++i) {
		if( i%2==0 ) {
			pos.x = 0;
			pos.y += MAP_BLOCK_WIDTH;
		} else {
			pos.x = MAP_BLOCK_WIDTH;
		}
		btn[i] = Widget_New("button");
		Window_ClientArea_Add( mapbox_window, btn[i] );
		if( i == 0 ) {
			Widget_SetBackgroundImage( btn[i], &cursor_img );
		} else {
			Widget_SetBackgroundImage( btn[i], &map_blocks[i-1] );
		}
		Widget_SetBackgroundLayout( btn[i], LAYOUT_CENTER );
		Widget_SetAutoSize( btn[i], FALSE, 0 );
		Widget_Resize( btn[i], Size(MAP_BLOCK_WIDTH,MAP_BLOCK_WIDTH) );
		Widget_Move( btn[i], pos );
		Widget_Event_Connect( btn[i], EVENT_CLICKED, proc_mapbtn_clicked );
		Widget_Show( btn[i] );
	}
	Widget_Show(mapbox_window);
}

static void destroy( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	LCUI_MainLoop_Quit(NULL);
}

static void window_init(void)
{
	window = Widget_New("window");
	Window_SetTitleTextW( window, L"地图编辑器" );
	Window_SetTitleIcon( window, &wnd_icon );
	Widget_Resize( window, Size(320, 240) );
	Widget_Event_Connect( Window_GetCloseButton(window), EVENT_CLICKED, destroy );
	Widget_Show(window);	
}

static void mapbox_init(void)
{
	Register_MapBox();
	mapbox = Widget_New("mapbox");
	Window_ClientArea_Add( window, mapbox );
	Widget_SetAlign( mapbox, ALIGN_MIDDLE_CENTER, Pos(0,0) );
	MapBox_CreateMap( mapbox, 4, 4 );
	Widget_Show( mapbox );
}

#ifdef LCUI_BUILD_IN_WIN32
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
#else
int main(void) 
#endif
{
#ifdef LCUI_BUILD_IN_WIN32
	InitConsoleWindow();
	Win32_LCUI_Init( hInstance );
#endif
	//setenv( "LCUI_FONTFILE", "../../fonts/msyh.ttf", FALSE );
	LCUI_Init();
	load_res();
	window_init();
	mapbox_init();
	map_toolbox_init();
	LCUIApp_AtQuit( free_res );
	return LCUI_Main();
}


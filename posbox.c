// position box
// 定位框

#include <LCUI_Build.h>
#include LC_LCUI_H
#include LC_WIDGET_H
#include LC_DRAW_H
#include LC_GRAPH_H

#include "posbox.h"

#define IMG_TOPLEFT		"drawable/btn_topleft.png"
#define IMG_TOPCENTER		"drawable/btn_topcenter.png"
#define IMG_TOPRIGHT		"drawable/btn_topright.png"
#define IMG_MIDDLELEFT		"drawable/btn_middleleft.png"
#define IMG_MIDDLECENTER	"drawable/btn_middlecenter.png"
#define IMG_MIDDLERIGHT		"drawable/btn_middleright.png"
#define IMG_BOTTOMLEFT		"drawable/btn_bottomleft.png"
#define IMG_BOTTOMCENTER	"drawable/btn_bottomcenter.png"
#define IMG_BOTTOMRIGHT		"drawable/btn_bottomright.png"


typedef struct _PosBox_Data {
	LCUI_Widget *(btn[3][3]);
	LCUI_Graph img[3][3];
	POSBOX_POS pos;
} PosBox_Data;

static void
proc_btn_topleft( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	/* 获取父部件 */
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_TOPLEFT;
	Widget_Update( widget );
}

static void
proc_btn_topcenter( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_TOPCENTER;
	Widget_Update( widget );
}

static void
proc_btn_topright( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_TOPRIGHT;
	Widget_Update( widget );
}

static void
proc_btn_middleleft( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_MIDDLELEFT;
	Widget_Update( widget );
}

static void
proc_btn_middlecenter( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_MIDDLECENTER;
	Widget_Update( widget );
}

static void
proc_btn_middleright( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_MIDDLERIGHT;
	Widget_Update( widget );
}

static void
proc_btn_bottomleft( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_BOTTOMLEFT;
	Widget_Update( widget );
}

static void
proc_btn_bottomcenter( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_BOTTOMCENTER;
	Widget_Update( widget );
}

static void
proc_btn_bottomright( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	PosBox_Data *posbox;
	widget = Widget_GetParent( widget, "posbox" );
	if( widget == NULL ) {
		return;
	}
	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	posbox->pos = POS_BOTTOMRIGHT;
	Widget_Update( widget );
}

static void PosBox_ExecInit( LCUI_Widget *widget )
{
	int i, j;
	PosBox_Data *posbox;
	LCUI_Pos pos;

	void (*func[3][3])(LCUI_Widget*,LCUI_WidgetEvent*) = {
		{proc_btn_topleft, proc_btn_topcenter, proc_btn_topright},
		{proc_btn_middleleft, proc_btn_middlecenter, proc_btn_middleright},
		{proc_btn_bottomleft, proc_btn_bottomcenter, proc_btn_bottomright}
	};
	const char *imgfile[3][3]={
		{IMG_TOPLEFT, IMG_TOPCENTER, IMG_TOPRIGHT},
		{IMG_MIDDLELEFT, IMG_MIDDLECENTER, IMG_MIDDLERIGHT},
		{IMG_BOTTOMLEFT, IMG_BOTTOMCENTER, IMG_BOTTOMRIGHT}
	};

	posbox = (PosBox_Data *)WidgetPrivData_New
			(widget, sizeof(PosBox_Data)); 
	
	posbox->pos = 4;
	pos.x = pos.y = 0;
	Widget_SetPadding( widget, Padding(1,1,1,1) );
	Widget_SetBorder( widget, Border(1,BORDER_STYLE_SOLID, RGB(170,170,170)) );

	for(i=0; i<3; ++i,pos.y+=24) {
		for(pos.x=0,j=0; j<3; ++j,pos.x+=24) {
			posbox->btn[i][j] = Widget_New("button");
			Widget_Container_Add( widget, posbox->btn[i][j] );
			Widget_Move( posbox->btn[i][j], pos );
			Graph_Init( &posbox->img[i][j] );
			Load_Image( imgfile[i][j], &posbox->img[i][j] );
			Widget_SetBackgroundImage( posbox->btn[i][j], &posbox->img[i][j] );
			Widget_SetBackgroundLayout( posbox->btn[i][j], LAYOUT_CENTER );
			Widget_Event_Connect( posbox->btn[i][j], EVENT_CLICKED, func[i][j] );
			Widget_Resize( posbox->btn[i][j], Size(24,24) );
			Widget_SetAutoSize( posbox->btn[i][j], FALSE, 0 );
			Widget_Show( posbox->btn[i][j] );
		}
	}
	Widget_Resize( widget, Size(74,74) );
}

static void PosBox_ExecUpdate( LCUI_Widget *widget )
{
	int col, row, i, j, w, h;
	PosBox_Data *posbox;
	LCUI_Pos start;
	LCUI_Size btn_size;

	posbox = (PosBox_Data *)Widget_GetPrivData( widget );
	switch(posbox->pos) {
	case POS_TOPLEFT: start.x = start.y = -1; break;
	case POS_TOPCENTER: start.x = 0; start.y = -1; break;
	case POS_TOPRIGHT: start.x = 1; start.y = -1; break;
	case POS_MIDDLELEFT: start.x = -1; start.y = 0; break;
	case POS_MIDDLECENTER: start.x = 0; start.y = 0; break;
	case POS_MIDDLERIGHT: start.x = 1; start.y = 0; break;
	case POS_BOTTOMLEFT: start.x = -1; start.y = 1; break;
	case POS_BOTTOMCENTER: start.x = 0; start.y = 1; break;
	case POS_BOTTOMRIGHT: start.x = 1; start.y = 1; break;
	default:return;
	}

	row = start.y<0 ? 0-start.y : 0;
	if( start.y < 0 ) {
		row = 0 - start.y;
		h = 3 + start.y;
	} else {
		row = 0;
		h = 3;
	}

	for(i=0; i<3; ++i) {
		if( start.x < 0 ) {
			col = 0 - start.x;
			w = 3 + start.x;
		} else {
			col = 0;
			w = 3;
		}
		for(j=0; j<3; ++j) {
			if(j>=start.x && j<w && i >= start.y && i<h) {
				Widget_SetBackgroundImage( 
				posbox->btn[i][j], &posbox->img[row][col] );
				++col;
				continue;
			}
			Widget_SetBackgroundImage( posbox->btn[i][j], NULL );
		}
		if(i>=start.y && i<h) {
			++row;
		}
	}
	/* 根据当前部件的尺寸，更新各个按钮的位置及尺寸 */
	btn_size.w = (widget->size.w - 2)/3;
	btn_size.h = (widget->size.h - 2)/3;
	start.y = start.x = 0;
	for(i=0; i<3; ++i,start.y+=btn_size.h) {
		for(start.x=0,j=0; j<3; ++j,start.x+=btn_size.w) {
			Widget_Move( posbox->btn[i][j], start );
			Widget_Resize( posbox->btn[i][j], Size(24,24) );
		}
	}
}

/* 注册PosBox部件 */
void Register_PosBox(void)
{
	WidgetType_Add("posbox");
	WidgetFunc_Add("posbox", PosBox_ExecInit, FUNC_TYPE_INIT);
	WidgetFunc_Add("posbox", PosBox_ExecUpdate, FUNC_TYPE_UPDATE);
}

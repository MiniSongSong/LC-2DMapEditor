#ifndef __MAPBOX_H__
#define __MAPBOX_H__
// ��ͼ����ʽ
typedef enum _MAP_STYLE {
	MAP_STYLE_NORMAL,	// ������ʽ
	MAP_STYLE_LEFT_90,	// ����ת90��
	MAP_STYLE_LEFT_180,	// ����ת180��
	MAP_STYLE_RIGHT_90,	// ����ת90��
	MAP_STYLE_RIGHT_180,	// ����ת180��
	MAP_STYLE_HORIZ,	// ˮƽ��ת
	MAP_STYLE_VERTI		// ��ֱ��ת
} MAP_STYLE;

/* �����ͼ�ߴ� */
LCUI_Size MapBox_CountSize( LCUI_Widget *widget );

/* ��ȡָ����ָ���еĵ�ͼ����������� */
LCUI_Pos MapBox_MapBlock_GetPixelPos( LCUI_Widget *widget, int row, int col );

/* ��ȡָ�����������ϵĵ�ͼ������� */
LCUI_Pos MapBox_MapBlock_GetPos( LCUI_Widget *widget, LCUI_Pos pixel_pos );

/* ������ͼ */
int MapBox_CreateMap( LCUI_Widget *widget, int rows, int cols );

/* ������ͼ�ߴ� */
int MapBox_ResizeMap( LCUI_Widget *widget, int rows, int cols );

/* ѡ��һ����ͼ�� */
int MapBox_SelectMapBlock( LCUI_Widget *widget, LCUI_Pos pos );

/* �趨��ͼ�� */
int MapBox_SetMapBlock(	LCUI_Widget	*widget,
			LCUI_Pos	pos,
			int		mapblock_id,
			MAP_STYLE	style_id );

/* ���ļ��������ͼ���� */
int MapBox_LoadMapData( const char *mapdata_filepath );

/* �����ͼ�������ļ� */
int MapBox_SaveMapData( const char *mapdata_filepath );

/* ע��MapBox���� */
void Register_MapBox(void);
#endif
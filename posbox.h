#ifndef __POSBOX__
#define __POSBOX__

typedef enum POSBOX_POS_ {
	POS_TOPLEFT,
	POS_TOPCENTER,
	POS_TOPRIGHT,
	POS_MIDDLELEFT,
	POS_MIDDLECENTER,
	POS_MIDDLERIGHT,
	POS_BOTTOMLEFT,
	POS_BOTTOMCENTER,
	POS_BOTTOMRIGHT
} POSBOX_POS;

/* ��ȡPosBox�����м�¼�Ķ�λ */
POSBOX_POS PosBox_GetPos( LCUI_Widget *widget );

/* ע��PosBox���� */
void Register_PosBox(void);

#endif
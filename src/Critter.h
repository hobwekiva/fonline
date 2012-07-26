#ifndef __CRITTER_H__
#define __CRITTER_H__
/********************************************************************
	created:	21:08:2004   20:06

	author:		Oleg Mareskin
	edit:		Denis Balikhin, Anton Tsvetinsky
	
	purpose:	
*********************************************************************/
#include "CSpriteManager.h"
#include "CFont.h"
#include "netproto.h"
#include "BufMngr.h"

const BYTE FIRST_FRAME	=0;
const BYTE LAST_FRAME	=200;

const BYTE ANIM1_STAY	=1;

const BYTE ANIM2_USE	=12;
const BYTE ANIM2_SIT	=11;

class CCritter
{
public:
	void SetDir(BYTE dir);
	void RotCW();
	void RotCCW();

	void Animate(BYTE action, BYTE num_frame); //!Cvet �������� ������ � �������� weapon
	void SetAnimation(); //!Cvet
	void Action(Byte action, DWORD action_tick); //!Cvet

	void Process(); //!Cvet CBufMngr ��� ��������� ���� ����� ��� �������� � �����

	DWORD text_color; //!Cvet
	void SetText(char* str, DWORD color); //!Cvet DWORD text_color
	void SetName(char* str) {strncpy(name,str,31);name[30]=0;};
	void DrawText(CFOFont* lpfnt);
	void SetVisible(bool av) {visible=av;};
    
    // �������� ���������� ������ � �������� �����������
	WORD cur_id,miniplayer;
	short cur_ox;
	short cur_oy;
    // ���������� ���� � ������ Den Baster
    HexTYPE hex_x;
	HexTYPE hex_y;
	BYTE cur_dir; // �����������

	CrID id;
// ������� �� �������
	BYTE weapon; // ��� ������ � ����� ��� �������� Den Baster !Cvet (������������� �����)

	char name[MAX_NAME+1];
	char cases[5][MAX_NAME+1];
	
	bool human; //!Cvet �������� �� ����� ������� 0-��� 1-��
	WORD st[ALL_STATS]; //!Cvet ����� 4-� ������� XXXX
	WORD sk[ALL_SKILLS]; //!Cvet ����� 3-� ������� XXX
	BYTE pe[ALL_PERKS]; //!Cvet ����� 1-� ������� X

	BYTE cond; //!Cvet
	BYTE cond_ext; //!Cvet
	WORD flags; //!Cvet

	CrTYPE base_type; //!Cvet
	CrTYPE type;
	CrTYPE type_ext; //!Cvet

//!Cvet ++++++++++++++++++++++++++++++++++  ���������
	dyn_map obj; //��� ������� � ������
	dyn_obj* a_obj; //�������� ������ � ����1
	dyn_obj* a_obj2; //�������� ������ � ����2
	dyn_obj* a_obj_arm; //�������� ������ � ����� �����
	dyn_obj* m_obj; //mouse object

	void AddObject(BYTE aslot,DWORD o_id,DWORD broken_info,DWORD time_wear,stat_obj* s_obj);
	int GetMaxDistance();

	void Initialization(); //������������� ��������� ���������

	void RefreshWeap();
	void RefreshType();

	int Move(BYTE dir); // �������� ��������

	BYTE next_step[4];
	BYTE cur_step;

	void SetCur_offs(short set_ox, short set_oy);
	void ChangeCur_offs(short change_ox, short change_oy);
	void AccamulateCur_offs();

	BYTE move_type; //0- 1-������

	BYTE rate_object; //��� ������������� �������

	dyn_obj def_obj1;
	dyn_obj def_obj2;

	BYTE alpha; //������������ ��������
//!Cvet ----------------------------------

	INTRECT drect;
	
	dtree_map::iterator rit; // ������ �� �������� ����� ����� ������ �����

	CCritter(CSpriteManager* alpSM):lpSM(alpSM),cur_anim(NULL),cur_dir(0),cur_id(0),stay_wait(0),stay_tkr(0),text_str(NULL),visible(0),weapon(0){strcpy(name,"none");};
	~CCritter(){SAFEDELA(text_str);};

//!Cvet ++++++++++++++++++++++++++++
	int Tick_count; //����������������� ��������
	TICK Tick_start; //����� ������ ��������

	void Tick_Start(TICK tick_count) { Tick_count=tick_count; Tick_start=GetTickCount(); };
	void Tick_Null(){ Tick_count=0; };

	int IsFree() { if(GetTickCount()-Tick_start>=Tick_count) return 1; return 0; };

	BYTE cnt_per_turn;
	WORD ticks_per_turn;
//!Cvet ----------------------------

	CritFrames* cur_anim; // ������� ��� �������� //!Cvet ����� � ������
	
private:

	CSpriteManager* lpSM;
	
	
	TICK anim_tkr;//����� ������ ��������
	WORD cur_afrm;
	TICK change_tkr;//����� �� ����� �����

	int stay_wait; // ��� ������� ��������

	TICK stay_tkr;

	char base_fname[256];

	char* text_str;
	TICK SetTime;
	int text_delay; //!Cvet

	bool visible;
};

//������ critters, ������� ������������
typedef map<CrID, CCritter*, less<CrID> > crit_map;

#endif//__CRITTER_H__
#pragma once
#include <string>
using namespace std;

const int MAX_PROVINCE_NUM = 26; 

const string  gProvincename[MAX_PROVINCE_NUM] = {
"�ӱ�ʡ",
"ɽ��ʡ",
"���ɹ�������",
"����ʡ",
"����ʡ",
"������ʡ",
"����ʡ",
"�㽭ʡ",
"����ʡ",
"����ʡ",
"����ʡ",
"ɽ��ʡ",
"����ʡ",
"����ʡ",
"����ʡ",
"�㶫ʡ",
"����׳��������",
"����ʡ",
"�Ĵ�ʡ",
"����ʡ",
"����ʡ",
"����������",
"����ʡ",
"����ʡ",
"�ຣʡ",
"�½�ά���������"
};

typedef enum
{
	e_HEBEI,
	e_SHANXI1,//ɽ��
	e_NEIMENG,
	e_LIAONING,
	e_JINLIN,
	e_HEILONGJIANG,
	e_JIANGSU,
	e_ZHEJIANG,
	e_ANHUI,
	e_FUJIAN,
	e_JIANGXI,
	e_SHANDONG,
	e_HENAN,
	e_HUBEI,
	e_HUNAN,
	e_GUANGDONG,
	e_GUANGXI,
	e_HAINAN,
	e_SICHUAN,
	e_GUIZHOU,
	e_YUNNAN,
	e_XIZANG,
	e_SHANXI,//����
	e_GANSU,
	e_QINGHAI,	
	e_XINJIANG
};

typedef struct _MGraph  
{  
	string verx[MAX_PROVINCE_NUM];  //���嶥��  
	bool visited[MAX_PROVINCE_NUM]; //����ʱ�����ʻ���δ����
	int arcs[MAX_PROVINCE_NUM][MAX_PROVINCE_NUM]; //���廡  
	//int vernum,arcsnum;//������󶥵��� �ͻ�  	
}MGraph;  
int creatgraph(MGraph* G);
void BFSTraverse(MGraph G,int serchvertex,int* verlist,int listnum);
int convertproindex(char* province);

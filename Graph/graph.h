#pragma once
#include <string>
using namespace std;

const int MAX_PROVINCE_NUM = 26; 

const string  gProvincename[MAX_PROVINCE_NUM] = {
"河北省",
"山西省",
"内蒙古自治区",
"辽宁省",
"吉林省",
"黑龙江省",
"江苏省",
"浙江省",
"安徽省",
"福建省",
"江西省",
"山东省",
"河南省",
"湖北省",
"湖南省",
"广东省",
"广西壮族自治区",
"海南省",
"四川省",
"贵州省",
"云南省",
"西藏自治区",
"陕西省",
"甘肃省",
"青海省",
"新疆维吾尔自治区"
};

typedef enum
{
	e_HEBEI,
	e_SHANXI1,//山西
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
	e_SHANXI,//陕西
	e_GANSU,
	e_QINGHAI,	
	e_XINJIANG
};

typedef struct _MGraph  
{  
	string verx[MAX_PROVINCE_NUM];  //定义顶点  
	bool visited[MAX_PROVINCE_NUM]; //遍历时被访问或者未访问
	int arcs[MAX_PROVINCE_NUM][MAX_PROVINCE_NUM]; //定义弧  
	//int vernum,arcsnum;//定义最大顶点数 和弧  	
}MGraph;  
int creatgraph(MGraph* G);
void BFSTraverse(MGraph G,int serchvertex,int* verlist,int listnum);
int convertproindex(char* province);

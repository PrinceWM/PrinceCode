

#include <stdio.h>  
#include <stdlib.h>  
#include <iostream>
#include <queue>
#include <string>
using namespace std;
#include "graph.h"
#define OK 1  
#define ERROR -1  
#define OVERFLOW 0  
#define MAXVER 20  //定义最大顶点数  

//MGraph aa;
int addgraphvertex(MGraph& G,int vertex,int* verlist,int listnum)
{
	int i = 0;
	int tmp = 0;
	for(i = 0;i<listnum;i++)
	{
		tmp = verlist[i] ;
		if(tmp == -1)
		{
			printf("list data error \n");
		}
		(G.arcs)[vertex][tmp] = 1;
		(G.arcs)[tmp][vertex] = (G.arcs)[vertex][tmp] ;
	}
	return 0;
}

int creatgraph(MGraph* G)
{
	int verlist[MAX_PROVINCE_NUM];
	int listnum = 0;
	
	memset(G->visited,0,sizeof(G->visited));
	
	for(int i = 0;i<MAX_PROVINCE_NUM;i++)
	{
		(G->verx[i]) = (gProvincename[i]);
	}
	//memcpy(G->verx,gProvincename,sizeof(gProvincename));

#if 1
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_NEIMENG;
	verlist[1] = e_LIAONING;
	verlist[2] = e_SHANDONG;
	verlist[3] = e_SHANXI1;
	verlist[4] = e_HENAN;
	listnum = 5;
	addgraphvertex(*G,e_HEBEI,verlist,listnum);

	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_NEIMENG;
	verlist[1] = e_HEBEI;
	verlist[2] = e_SHANDONG;
	verlist[3] = e_SHANXI;
	verlist[4] = e_HENAN;
	listnum = 5;
	addgraphvertex(*G,e_SHANXI1,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_HEILONGJIANG;
	verlist[1] = e_LIAONING;
	verlist[2] = e_JINLIN;
	verlist[3] = e_HEBEI;
	verlist[4] = e_SHANXI;
	verlist[5] = e_SHANXI1;
	verlist[6] = e_GANSU;
	listnum = 7;
	addgraphvertex(*G,e_NEIMENG,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));

	verlist[0] = e_JINLIN;
	verlist[1] = e_NEIMENG;
	verlist[2] = e_HEBEI;
	listnum = 3;

	addgraphvertex(*G,e_LIAONING,verlist,listnum);

	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_HEILONGJIANG;
	verlist[1] = e_NEIMENG;
	verlist[2] = e_LIAONING;
	listnum = 3;
	addgraphvertex(*G,e_JINLIN,verlist,listnum);

	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_JINLIN;
	verlist[1] = e_NEIMENG;
	listnum = 2;

	addgraphvertex(*G,e_HEILONGJIANG,verlist,listnum);

	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_SHANDONG;
	verlist[1] = e_HENAN;
	verlist[2] = e_ANHUI;
	verlist[3] = e_ZHEJIANG;
	listnum = 4;
	addgraphvertex(*G,e_JIANGSU,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_JIANGSU;
	verlist[1] = e_ANHUI;
	verlist[2] = e_JIANGXI;
	verlist[3] = e_FUJIAN;
	listnum = 4;
	addgraphvertex(*G,e_ZHEJIANG,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] =  e_JIANGSU;
	verlist[1] = 	e_HENAN;
	verlist[2] = e_HUBEI;
	verlist[3] = e_JIANGXI;
	verlist[4] = e_ZHEJIANG;
	listnum = 5;
	addgraphvertex(*G,e_ANHUI,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_ZHEJIANG;
	verlist[1] = e_JIANGXI;
	verlist[2] = e_GUANGDONG;
	listnum = 3;
	addgraphvertex(*G,e_FUJIAN,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_ANHUI;
	verlist[1] = e_HUBEI;
	verlist[2] = e_HUNAN;
	verlist[3] = e_GUANGDONG;
	verlist[4] = e_FUJIAN;
	verlist[5] = e_ZHEJIANG;
	listnum = 6;
	addgraphvertex(*G,e_JIANGXI,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_HEBEI;
	verlist[1] = e_HENAN;
	verlist[2] = e_JIANGSU;
	listnum =3;
	addgraphvertex(*G,e_SHANDONG,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_SHANDONG;
	verlist[1] = e_HEBEI;
	verlist[2] = e_SHANXI1;
	verlist[3] = e_SHANXI;
	verlist[4] = e_HUBEI;
	verlist[5] = e_ANHUI;
	verlist[6] = e_JIANGSU;
	listnum = 7;
	addgraphvertex(*G,e_HENAN,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_HENAN;
	verlist[1] = e_SHANXI;
	verlist[2] = e_SICHUAN;
	verlist[3] = e_HUNAN;
	verlist[4] = e_JIANGXI;
	verlist[5] = e_ANHUI;
	listnum = 6;
	addgraphvertex(*G,e_HUBEI,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_HUBEI;
	verlist[1] = e_SICHUAN;
	verlist[2] = e_GUIZHOU;
	verlist[3] = e_GUANGXI;
	verlist[4] = e_GUANGDONG;		
	verlist[5] = e_JIANGXI;
	listnum = 6;
	addgraphvertex(*G,e_HUNAN,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_GUANGXI;
	verlist[1] = e_HUNAN;
	verlist[2] = e_JIANGXI;
	verlist[3] = e_FUJIAN;
	listnum = 4;
	addgraphvertex(*G,e_GUANGDONG,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));

	verlist[0] = e_YUNNAN;
	verlist[1] = e_GUIZHOU;
	verlist[2] = e_HUNAN;
	verlist[3] = e_GUANGDONG;
	listnum = 4;
	addgraphvertex(*G,e_GUANGXI,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_GUANGXI;
	verlist[1] = e_GUANGDONG;
	listnum = 2;
	addgraphvertex(*G,e_HAINAN,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_QINGHAI;
	verlist[1] = e_XIZANG;
	verlist[2] = e_YUNNAN;
	verlist[3] = e_GUIZHOU;
	verlist[4] = e_HUNAN;
	verlist[5] = e_HUBEI;
	verlist[6] = e_SHANXI;
	verlist[7] = e_GANSU;
	listnum = 8;
	addgraphvertex(*G,e_SICHUAN,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_SICHUAN;
	verlist[1] = e_YUNNAN;
	verlist[2] = e_GUANGXI;
	verlist[3] = e_HUNAN;
	listnum = 4;
	addgraphvertex(*G,e_GUIZHOU,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_XIZANG;
	verlist[1] = e_SICHUAN;
	verlist[2] = e_GUIZHOU;
	verlist[3] = e_GUANGXI;
	listnum = 4;

	addgraphvertex(*G,e_YUNNAN,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_XINJIANG;
	verlist[1] = e_QINGHAI;
	verlist[2] = e_SICHUAN;
	verlist[3] = e_YUNNAN;
	listnum = 4;
	addgraphvertex(*G,e_XIZANG,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_NEIMENG;
	verlist[1] = e_GANSU;
	verlist[2] = e_SICHUAN;
	verlist[3] = e_HUBEI;
	verlist[4] = e_HENAN;
	verlist[5] = e_SHANXI1;
	listnum = 6;
	addgraphvertex(*G,e_SHANXI,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_NEIMENG;
	verlist[1] = e_XINJIANG;
	verlist[2] = e_QINGHAI;
	verlist[3] = e_SICHUAN;
	verlist[4] = e_SHANXI;
	listnum = 5;

	addgraphvertex(*G,e_GANSU,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_GANSU;
	verlist[1] = e_XINJIANG;
	verlist[2] = e_XIZANG;
	verlist[3] = e_SICHUAN;
	listnum = 4;
	addgraphvertex(*G,e_QINGHAI,verlist,listnum);
	memset(verlist,-1,sizeof(verlist));
	verlist[0] = e_GANSU;
	verlist[1] = e_QINGHAI;
	verlist[2] = e_XIZANG;
	listnum = 3;
	addgraphvertex(*G,e_XINJIANG,verlist,listnum);
#endif
	return 0;
}


typedef char VertexType; /* 顶点类型应由用户定义 */
typedef int EdgeType; /* 边上的权值类型应由用户定义 */

#define MAXSIZE 9 /* 存储空间初始分配量 */
#define MAXEDGE 15
#define MAXVEX 9

void setdepth(int& data,unsigned char dp)
{
	data = (data&0x00FFFFFF)|(dp<<24);
}

unsigned char getdepth(int data)
{
	return (unsigned char)((data&0xFF000000)>>24);
}

int getdata(int data)
{
	return ( int )((data&0x00FFFFFF));
}


void test()
{
	int data = 0xFE123B22;
	unsigned char dp = 0xE3;
	unsigned char getd = 0;
	setdepth(data,dp);
	getd = getdepth(data);
	printf("getd = 0x%x \n",getd);
}



/* 邻接矩阵的广度遍历算法 */
void BFSTraverse(MGraph G,int serchvertex,int* verlist,int listnum)
{
	int i, j;
	bool flag = false;
	unsigned char dp = 0;// 从查找顶点开始，找到顶点相对于查找顶点的深度
	unsigned char befdp = 0;
	queue<int> Q;  
	queue<int> re;  
	//设置所有顶点为未访问状态
	for (i = 0; i < MAX_PROVINCE_NUM; i++)
	{
		G.visited[i] = false;
	}	

	if (!G.visited[serchvertex])/* 若是未访问过就处理 */
	{
		G.visited[serchvertex] = true;/* 设置当前顶点访问过 */
		//设置深度 并把顶点压入队列
		setdepth(serchvertex,dp);
		Q.push(serchvertex);
		while (!Q.empty())/* 若当前队列不为空 */
		{
			/* 将队对元素出队列，赋值给i */
			i = Q.front();
			Q.pop();			
			//保存之前顶点深度，并取出新顶点并查看深度
			befdp = dp;
			dp = getdepth(i);
			i = getdata(i);
			if(befdp < dp)
			{//如果之前顶点跟当前顶点已经不在一个深度，就查看之前深度下有无有效路径
				while(!re.empty())
				{
					int itmp;
					itmp = re.front();
					re.pop();
					flag = true;
					cout<<gProvincename[itmp]<<endl;
					//printf("%s ",(gProvincename[itmp]).c_str);
				}

			}
			if(flag == true)
			{
				return ;
			}

			for (j = 0 ; j < MAX_PROVINCE_NUM; j++)
			{
				/* 判断其它顶点若与当前顶点存在边且未访问过  */
				if (G.arcs[i][j] == 1 && !G.visited[j])
				{
					G.visited[j] = true;/* 将找到的此顶点标记为已访问 */
					//cout << G.vexs[j] << ' '; /* 打印顶点 */
					//EnQueue(&Q, j);/* 将找到的此顶点入队列  */
					setdepth(j,dp+1);
					Q.push(j);
					j = getdata(j);
					int itemp;
					for(itemp=0;itemp<listnum;itemp++)
					{
						if(verlist[itemp] == j)
						{
							re.push(j);
						}
					}
				}
			}
		}

		/*最后一级深度后的补充*/
		while(!re.empty())
		{
			int itmp;
			itmp = re.front();
			re.pop();
			flag = true;
			cout<<gProvincename[itmp]<<endl;
			//printf("%s ",(gProvincename[itmp]).c_str);
		}

	}
}

int convertproindex(char* province)
{
	int i;
	for(i = 0;i<MAX_PROVINCE_NUM;i++)
	{
		if(memcmp(gProvincename[i].c_str(),province,2)==0)
		{
			return i;
		}
	}
	return -1;
}




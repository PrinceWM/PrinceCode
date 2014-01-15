//#include "memhook.h"
#include <map>
#include <string>
#include <utility>
#include <iostream>
using namespace std;
class aa
{
public:
	aa()
	{
		printf("aa creat \n");
	}
	~aa()
	{ 
		printf("aa release \n");
	}
};


class U_ptr
{
	friend class HasPtr;
	int *ip;
	int use;
	U_ptr(int *p ):ip(p),use(1){}
	~U_ptr(){delete ip;}
};


class HasPtr
{
public:
	HasPtr(int *p ,int i):ptr(new U_ptr(p)),val(i){}
	HasPtr(const HasPtr &orig):ptr(orig.ptr),val(orig.val){++ptr->use;}
	HasPtr& operator= (const HasPtr&);
	~HasPtr(){
		printf("release HasPtr ptr->use=%d\n",ptr->use);
		if(--ptr->use == 0)
		{
			printf("delete ptr\n");
			delete ptr;
		}
	}
private:
	U_ptr *ptr;
	int val;
};

class base
{
private :
	int data;
protected:
	void setdata(int idata){data = idata;}
public:
	base(){};
	virtual ~base(){cout<<"base"<<"\n";}
	int getdata(void){return data;}
	virtual void print(){cout<<"print basedebug"<<"\n";};
};

class externbase: public base
{
private:
	int externdata;
protected:
	void setexterndata(int idata){externdata = idata;/*base::*/setdata(3);}
public:
	externbase(){};
	~externbase(){cout<<"externbase"<<"\n";};
	int getexterndata(externbase &obj1){return externdata;setdata(3);obj1.setdata(3);}
	void print(){cout<<"print externbasedebug"<<"\n";};

};


class externsecond:public externbase
{
public :
	externsecond(){};
	~externsecond(){cout<<"externsecond"<<"\n";};
	
};




int main()
{
	char buffer[3];
	int i = 0;
	char data[1] = {0xF8};
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer, "%02x", data[i]);
	printf("%08x",buffer);
	system("pause");
#if 0
	int *ptrbase = new int[100];
	HasPtr obj1(ptrbase,0);
	HasPtr obj2(obj1);
	HasPtr obj3(obj1);
	obj1.~HasPtr();
	obj2.~HasPtr();
	obj3.~HasPtr();
#endif
	externbase obj;
	externbase obj1;
	externbase *obj2 = &obj1;
	base bobj;
	//obj.setdata(5);
	//obj2->setdata(2);
	obj.getexterndata(obj1);
	
	base* bobj2 = new externbase;
	bobj2->print();
	delete bobj2;
	cout<<"\n";
	obj2 = new externsecond;
	delete obj2;



	system("pause");
	return 0;
	printf("come in main\n");
	//std::map<unsigned int,int> map_test;
	//map_test.insert();
	//map_test.insert(pair<unsigned int, int>(/*(unsigned int)((char*)ptr + 4 + sizeof(size_t))*/0x100,(int)10));
	//map_test.insert(map<int, int>::value_type (( int)1, (int)100));
	//printf("test ok \n");
	char* ptr = new char[100];

	delete ptr;
	printf("malloc class\n");
	aa* tmpclass = new aa;
	delete tmpclass;
	printf("str malloc test\n");
	
	system("pause");
}
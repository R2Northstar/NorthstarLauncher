#include "pch.h"
using namespace std;

enum Type
{
	floatT,
	stringT,
	intT,
	arrayT,
	objectT
};

class RSONObject
{
	Type type;
	float fltVal;
	string strVal;
	int intVal;
	map<string, RSONObject> children;
	RSONObject arr[];


	RSONObject(float a)
	{
		type = Type::floatT;
		fltVal = a;
	}
	RSONObject(string a)
	{
		type = Type::stringT;
		strVal = a;
	}
	RSONObject(map<string, RSONObject> a) 
	{ 
		type = Type::objectT;
		children = a; 
	}
	RSONObject(RSONObject a[]) 
	{ 
		type = Type::arrayT;
		arr = a;
	}
};
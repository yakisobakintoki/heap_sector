/*
 * main.cpp
 *
 *  Created on: 2012/03/21
 *      Author: shirao
 */

#include "heap_sector.h"

#define M1	(1024*1024)
char Buf[256]={};
S_Heap Heap1;
#define maxof(a)	(sizeof(a)/sizeof(a[0]))
S_HeapSector Sectors[8];

int main()
{
	Heap_Init(&Heap1,Sectors,maxof(Sectors));
	int c = Heap_Alloc(&Heap1,2);
	Heap_Free(&Heap1,c);

	return 0 ;
}

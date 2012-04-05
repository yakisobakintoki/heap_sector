/*
	2012/3/21 author: shirao hiroki.
	
	ヒープからメモリを確保するプログラムです。
	割り当てるセクタ番号を返すので、実メモリは別途利用者側が用意しておき、
	Heap_Alloc の返すセクタ番号を使って、アドレスにします。
	
	ヒープメモリの実メモリと、管理領域を分離しています。
	これによりヒープの管理領域をシリアライズすることが可能です。
	厳密な中断、再開処理を作成する際等に良いです。
	
	また、リンクドリストを、未使用領域と使用済領域を分けることで、
	空き領域の検索を行う際に、使用済領域を探索対象から除外しています。

	メモリを解放する際にセクタの併合に失敗した場合に限り、 管理ノードへとリンクしますが、これは断片化への第一歩なので、断片化のための開始ノードを別途追加して、
	アルゴリズムを進化させることが多分可能だと思います。

*/

#define NULL 0
typedef short HeapSectorNo_t;
typedef struct  {
	HeapSectorNo_t p,n,s;
}S_HeapSector ;

typedef struct {
	S_HeapSector 		*sector;		//セクタバッファー	（管理用ノードが４つ必要）
	HeapSectorNo_t 	sectorMax;		// セクタのの最大数
}S_Heap;


#define UB	0	//Used  Begin Node 使用領域の開始ノード
#define EB	1	//Empty Begin Node 空き領域の開始ノード

//空きノードとして初期化する
//	 	サイズはマイナスで保持。空き領域セクタの最終セクタに同じ内容を書き込みます。
void Heap_InitEmpty(S_HeapSector *e,int p,int n,int s)
{
	e->p = p ;
	e->n = n ;
	e->s = -s ;//空き領域はマイナスでマーク
	(e + s - 1 )->s = -s ; //逆方向サーチ用
}
void Heap_Init(S_Heap *o,S_HeapSector *sector,int sectorMax)
{
	o->sector = sector;
	o->sectorMax = sectorMax;
	// 管理領域に４セクタ使用する
	/*
	 * |UB|EB|e|EE|UE|
	 */
	// (u => used , e => empty)(b => begin, e =>end)
	// ub => [U]sed [B]egin
	int last = sectorMax - 1 ;
	int ub = 0;	
	int ue = last;
	int eb = 1;
	int ee = last-1;	
	int e  = 2;

	//終端
	o->sector[ub].p =
	o->sector[eb].p = 
	o->sector[ue].n =
	o->sector[ee].n = -1;
	
	o->sector[ub].n = ue ;
	o->sector[ue].p = ub ;
	o->sector[eb].n = o->sector[ee].p = e ;
	
	//サイズ
	o->sector[ub].s = o->sector[ue].s =  1 ; 
	o->sector[eb].s = o->sector[ee].s =  1 ; //空き領域の終端は、プラスにしておき判定をしやすくする 
	//初期の空き領域
	Heap_InitEmpty(o->sector+e,eb,ee,ee-eb-1 );
}

//-------- 確保処理　
//要求サイズ以上のセクタ連続を探す。
int Heap_SearchEmpty1(S_Heap *o,int request_size)
{
	S_HeapSector *e = o->sector + EB;
	for (e = o->sector + e->n; e->n != -1 ; e = o->sector + e->n ){
		if ( -e->s >= request_size){
			return e - o->sector ;
		}
	}
	return -1 ;
}


void Heap_DivideEmpty(S_HeapSector *t,S_HeapSector *c,int size)
{
	S_HeapSector *d = c + size;
	//-- 分割した残りを空き領域化　
	Heap_InitEmpty(d,c->p,c->n,(-c->s)-size);
	//前後を新規空き領域につなげる。
	t[c->n].p = t[c->p].n = d - t ;
	
}
//分割して使用する部分
void Heap_JoinUsed(S_Heap *o,int c,int size)
{
	S_HeapSector *a = o->sector + UB   ;
	S_HeapSector *b = o->sector + a->n ;
	a->n = b->p = c ;
	
	o->sector[c].p = a - o->sector ;
	o->sector[c].n = b - o->sector ;
	o->sector[c].s = size ;
	
}
//-1    => 確保失敗
//> 0   => 確保したセクタインデックス
int Heap_Alloc(S_Heap *o,int request_size)
{	
	int c = Heap_SearchEmpty1(o,request_size);
	if (c != -1){
		//request_size で分割し、前半を使用済リンクに移動　
		Heap_DivideEmpty(o->sector,o->sector+c,request_size);
		Heap_JoinUsed(o,c,request_size);
	}
	return c; 
}
//-------- 解放処理
S_HeapSector* Heap_TailToHead(S_HeapSector *tail)
{
	if (tail->s < 0){
		return tail + 1 + tail->s;
	}
	return NULL;
}

void Heap_Unlink(S_HeapSector *t,S_HeapSector *c)
{
	S_HeapSector *p,*n ;
	p = t + c->p;
	n = t + c->n;
	p->n = c->n;
	n->p = c->p;
}
void Heap_Linkin(S_HeapSector *t,S_HeapSector *a,S_HeapSector *b,S_HeapSector *c)
{

}
void Heap_Free(S_Heap *o,int c)
{
	S_HeapSector *z = o->sector + c ;
	Heap_Unlink(o->sector,z);
	// 前後のセクタの塊が、空きである場合に連続領域のため併合する
	
	S_HeapSector *f = Heap_TailToHead( z - 1);
	S_HeapSector *b = z + z->s; //使用済セクタは常に＋なので符号反転はしない

	if (f){
		Heap_InitEmpty(f,z->p,z->n,-(f->s + z->s));			
		o->sector[f->p].n = o->sector[f->n].p = c;
	}
	if (b->s < 0){
		Heap_InitEmpty(z,b->p,b->n,-b->s + z->s);
		o->sector[b->p].n = o->sector[b->n].p = c;
	}
	//
	if (!f && b->s >= 0){
		f = o->sector + EB;
		b = o->sector + f->n ;
		f->n = b->p = c ;
	}
}

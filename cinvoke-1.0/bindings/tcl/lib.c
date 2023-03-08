/*
C/Invoke Source Code File

Copyright (c) 2006 Will Weisser

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
   3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define CDECL __cdecl
#define STDCALL _stdcall
#define DLLEXPORT __declspec(dllexport)
#else
#define CDECL
#define STDCALL
#define DLLEXPORT
#endif

#define NOPRINTF_DEBUG
#ifdef PRINTF_DEBUG
#define dbgprintf printf
#else
#define dbgprintf voidprintf
#endif

void voidprintf(char* fmt,...) {
    //
}

DLLEXPORT char *hello_data="12345";

DLLEXPORT void test_hellodata(char* newtext) {
    dbgprintf ("old text is %s\n",hello_data);
    if(hello_data!=NULL) free(hello_data);
    //hello_data=strdup(newtext);
    dbgprintf ("new text is %s\n",hello_data);
    dbgprintf ("adr is %p\n",hello_data);
    return;
}

DLLEXPORT void test1() {
	dbgprintf ("test1\n");
}

DLLEXPORT int test2(int p) {
	dbgprintf ("test2: %d (want 22)\n", p);
	return 33;
}
DLLEXPORT void * test3() {
	dbgprintf ("test3\n");
	return (void*)0xFFFFFF;
}
DLLEXPORT float test4() {
	dbgprintf ("test4\n");
	return 1.1000000000000124422f;
}
DLLEXPORT double test5() {
	dbgprintf ("test5\n");
	return 1.100000000000000000004093840394830948304983049230920124422;
}
DLLEXPORT long long int test6(int a0, long long int a1, float a2, double a3, char a4) {
#ifdef _WIN32
	char buf[20];
	_i64toa(a1, buf, 10);
	dbgprintf ("test6: %d %s %f %f %c (want 11 22 %f %f 5)\n", a0, buf, a2, a3, a4,
		33.5, 44.5);
#else
	dbgprintf ("test6: %d %lld %f %f %c (want 11 22 %f %f 5)\n", a0, a1, a2, a3, a4,
		33.5, 44.5);
#endif
	return -5;
}
DLLEXPORT void test9(float (CDECL *f)(int, char, int)) {
    float ff=f(22, 'A', 44);
    if(ff!=11.5) printf("callback with wrnog result...%f\n",ff);
	//dbgprintf ("test9: %f (want %f)\n", f(22, 'A', 44), 11.5);
}
DLLEXPORT float test10(int i1, short i2, char i3, long i4, long i5,
	int i6, int i7, long i8, short i9) {
	dbgprintf ("test10: %d %d %c %ld %ld %d %d %ld %d (want 111 222 3 444 555 666 777 888 999)\n", i1, i2, i3, i4, i5, i6, i7, i8, i9);
	return 0.7f;
}
DLLEXPORT void test11(int (CDECL *f)(int, int, long, long, int, long long,
int, long long)) {
	dbgprintf ("test11: %d (want 1)\n", f(1, 2, 3, 4, 5, 0x66666666666666LL, 0x77777777, 0x88888888888888LL));
}


typedef struct _cs {
    char cc;
    int i ;
    long il;
    long long ill;
    float tf;
    double td;
    void* pp;
} cs;
typedef struct cs2 {
    int i2;
    int ooo[4];
    char *o;
} cs2;

typedef struct _simple {
    int a;
    int b;
} simple;
typedef struct withsubs {
    int i2;
    struct {
        int sa;
        int sb;
        union {
            int i;
            char c;
        } ;
    } ;
    int a;
    int b;
    union {
        struct {
            int usi1;
            int usi2;
        };
        struct {
            int usi3;
            char usc1;
        };
    };
} withsubs;

typedef struct _withdata {
    char* mydata;
    int size;
} withdata;

DLLEXPORT int test12(cs* p) {
	dbgprintf ("test12: %d %d %li %lli %f %f %p\n", p->cc,p->i,p->il,p->ill,p->tf,p->td,p->pp);
	return 33;
}
DLLEXPORT cs* test13(int a, int b) {
    cs* pcs;
    pcs=malloc(sizeof(cs));
    pcs->cc=a;
    pcs->i=b;
    pcs->il=0;pcs->ill=0;pcs->tf=0;pcs->td=0;pcs->pp=0;
	return pcs;
}

DLLEXPORT int test14(int* p) {
	dbgprintf ("test14: %d (want 22)\n", *p);
	return 33;
}
DLLEXPORT int test15(cs2* scs2) {
    int i;
    dbgprintf ("Test15: Size of cs2: %lu\n", sizeof(cs2));
    for(i=0;i<4;i++) {
        dbgprintf ("test15: %d = %d\n", i,scs2->ooo[i]);
    }
    dbgprintf ("Setting back to 20+i\n");
    for(i=0;i<4;i++) {
        scs2->ooo[i]=20+i;
    }
    
	return 0;
}

DLLEXPORT char* test16(char* instring) {
    dbgprintf ("test 16: instring %s\n",instring);
    char* outstring="t16: return this as result";    
	return outstring;
}

DLLEXPORT char* test17(char** instring) {
    dbgprintf ("test 17: instring %s\n",*instring);
    //free(*instring);
    //*instring=strdup("t17: Return this as a value");
    char* outstring="t17: return this as result";    
	return outstring;
}

DLLEXPORT int test18(withsubs* wsb) {
    //
    int i;
    dbgprintf ("Test18: Size of withsubs: %lu\n", sizeof(withsubs));
    dbgprintf ("test18: struct i2  %d a %d\n", wsb->i2, wsb->a);
    dbgprintf ("test18: substruct a  %d b %d\n", wsb->sa, wsb->sb);
    dbgprintf ("test18: substruct union us.i3  %d == us.i1 %d\n", wsb->usi3, wsb->usi1);
    dbgprintf ("Setting back to 20\n");
    wsb->sa=20;
    wsb->sb=21;
    dbgprintf ("test18: union i  %d c %c\n", wsb->i, wsb->c);
	return 0;
}

DLLEXPORT int test19(withdata* wsb) {
    dbgprintf ("Test19: Size of withdata: %lu\n", sizeof(withsubs));
    dbgprintf ("test19: size of data %dn", wsb->size);
    char* data;
    char* dest=(char*)wsb->mydata;
    for (int i=0; i<wsb->size; i++) {
        *(dest+i)=(wsb->size-i)%255;
    }
    return 0;
    
//
}

DLLEXPORT int test20(void* wsb, int size) {
    dbgprintf ("test20: size of data %dn", size);
    char* dest=(char*)wsb;
    for (int i=0; i<size; i++) {
        *(dest+i)=(size-i)%255;
    }
    return 0;
//
}

DLLEXPORT int testimg(char* instring, int width, int height) {
    //dbgprintf ("testimg: instring %s\n",*instring);
    int pos;
    for (int x=0;x<width;x++) {
        for (int y=0;y<height;y++) {
            pos=(x+y*width)*4;
            instring[pos]=255-instring[pos];
            instring[pos+1]=255-instring[pos+1];
            instring[pos+2]=255-instring[pos+2];            
        }
    }
	return 0;
}

    

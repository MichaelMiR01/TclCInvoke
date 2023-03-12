/*
 *  tclcinvoke - Tcl binding to CInvoke
 * 
 *  2022 by Michael Richter
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdlib.h>
#include <time.h>
#ifndef USE_TCL_STUBS
#define USE_TCL_STUBS
#endif
#include <tcl.h>
#include <tclInt.h>


/* hack to get printf under win32 and tcl */
Tcl_Interp* globalinterp;
#define printf(...) tcl_printf(__VA_ARGS__)
void tcl_printf(const char *fmt, ...);
/*
 * <setjmp.h> is used for the optional error recovery mechanism 
 */

#include <setjmp.h>

// don't include unistd.h since this conflicts with tcl/compat/unistd.h
#define	_UNISTD_H	1
#include <cinvoke.h>
#include "../../lib/hashtable.h"
#ifdef MEMDEBUG
#include "memdbg.c"
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

/* taken from ffidl project and modified a bit to our needs */
/*
 * Tcl object type used for representing pointers within Tcl.
 *
 * We wrap an existing "expr"-compatible Tcl_ObjType, in order to easily support
 * pointer arithmetic and formatting withing Tcl.  The size of the Tcl_ObjType
 * needs to match the pointer size of the platform: long on LP64, Tcl_WideInt on
 * LLP64 (e.g. WIN64).
 */
#if SIZEOF_VOID_P == SIZEOF_LONG
#  define FFIDL_POINTER_IS_LONG 1
#elif SIZEOF_VOID_P == 8 && defined(HAVE_WIDE_INT)
#  define FFIDL_POINTER_IS_LONG 0
#else
#  error "pointer size not supported"
#endif
#include "cffi_pointer.c"
#if FFIDL_POINTER_IS_LONG
# define PTR_TYPE long
# define TCL_NEWPTROBJ Tcl_NewLongObj
static Tcl_Obj *Ffidl_NewPointerObj(PTR_TYPE *ptr,char* tag) {
    Tcl_Obj* o=Tcl_NewStringObj(tag,-1);
    Tcl_Obj*rv= Tclh_PointerWrap(ptr,o);
    return rv;
    
}
static int Ffidl_GetPointerFromObj(Tcl_Interp *interp, Tcl_Obj *obj, PTR_TYPE **ptr,char* tag) {
  int status;
  long l;
  void *pvP=NULL;
  Tcl_Obj* otag;
  if(tag!=NULL) {
      otag= Tcl_NewStringObj(tag,-1);
  } else {
      otag= Tcl_NewStringObj("ptr",-1);
  }
  status =Tclh_PointerUnwrap(interp, obj, &pvP, otag);
  Tcl_DecrRefCount(otag);
  if(status!=TCL_OK) {
      return TCL_ERROR;
  }
  l=(long) pvP;
  if(l==0) {
      *ptr=NULL;
      return TCL_OK;
  } else {
      *ptr = (PTR_TYPE *)l;
  }
  return status;
}
#  define FFIDL_GETPOINTER FFIDL_GETINT
#else
#  define PTR_TYPE Tcl_WideInt
# define TCL_NEWPTROBJ Tcl_NewWideIntObj
static Tcl_Obj *Ffidl_NewPointerObj(PTR_TYPE *ptr) {
    Tcl_Obj* o=Tcl_NewStringObj(tag,-1);
    Tcl_Obj*rv= Tclh_PointerWrap(ptr,o);
    return rv;
}
static int Ffidl_GetPointerFromObj(Tcl_Interp *interp, Tcl_Obj *obj, PTR_TYPE **ptr) {
  int status;
  Tcl_WideInt w;
  void *pvP=NULL;
  Tcl_Obj* otag;
  if(tag!=NULL) {
      otag= Tcl_NewStringObj(tag,-1);
  } else {
      otag= Tcl_NewStringObj("ptr",-1);
  }
  status =Tclh_PointerUnwrap(interp, obj, &pvP, otag);
  Tcl_DecrRefCount(otag);
  if(status!=TCL_OK) {
      return TCL_ERROR;
  }
  w=(Tcl_WideInt)pvP;
  if(w==0) {
      *ptr=NULL;
  } else {
      *ptr = (PTR_TYPE *)w;
  }
  return status;
}
#  define FFIDL_GETPOINTER FFIDL_GETWIDEINT
#endif
static Tcl_Obj *PointerCast(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* newtag) {
    void *pvP=NULL;
    Tclh_PointerUnwrap(interp, obj, &pvP, NULL);
    Tcl_Obj*rv= Tclh_PointerWrap(pvP,newtag);
    return rv;
}    

#include "ffidl_stubs.c"

enum memmngr {
    MEM_TCL=0,
    MEM_SYSTEM=1,
    MEM_DEBUG=999
};

void* cinv_alloc(size_t size, int fromsystem, char* dgbmsg, int linenr) {
    // fromsystem rules the memory regime
    // -1
    // 0
    // 1
    // 999
    if(size==0) return NULL;
    if(fromsystem<0) return NULL; // don'T alloc
    
#ifdef MEMDEBUG
    if(fromsystem==MEM_TCL) fromsystem=MEM_DEBUG;
#endif
//fromsystem=MEM_DEBUG;
    switch(fromsystem) {
        case MEM_TCL:
            return ckalloc(size);
        case MEM_SYSTEM:
            return malloc(size);
#ifdef MEMDEBUG
        case MEM_DEBUG:
            return tcc_malloc_debug(size, dgbmsg,linenr);
#endif
        default:
            printf("Error memory not alloced %li\n",size);
            return NULL;
    }
    return NULL;
    
}
void cinv_free(void* ptr, int fromsystem) {
    // fromsystem rules the memory regime
    // -1
    // 0
    // 1
    // 999
    if(ptr==NULL) return;
    if(fromsystem<0) return; // don'T free

#ifdef MEMDEBUG
    if(fromsystem==MEM_TCL) fromsystem=MEM_DEBUG;
#endif
//fromsystem=MEM_DEBUG;
    switch(fromsystem) {
        case MEM_TCL:
            ckfree(ptr);
            return;
        case MEM_SYSTEM:
            free(ptr);
            return;
#ifdef MEMDEBUG
        case MEM_DEBUG:
            tcc_free_debug(ptr);
            return;
#endif
        default:
            printf("Error memory not freed %p\n",ptr);
    }
    return;
}
char *tcl_strdup(const char *src) {
    char *dst = cinv_alloc(strlen (src) + 1,MEM_TCL,src,__LINE__);  // Space for length plus nul
    if (dst == NULL) return NULL;          // No memory
    strcpy(dst, src);                      // Copy the characters
    return dst;                            // Return the new string
}
char *sys_strdup(const char *src) {
    char *dst = cinv_alloc(strlen (src) + 1,MEM_SYSTEM,"sys_strdup",__LINE__);  // Space for length plus nul
    if (dst == NULL) return NULL;          // No memory
    strcpy(dst, src);                      // Copy the characters
    return dst;                            // Return the new string
}
/* Append SRC on the end of DEST.  */
char * tcl_strcat (char *dest, const char *src)
{
    size_t n = strlen( dest ) + strlen( src )+ 1;
    char *s = cinv_alloc( n,MEM_TCL,"tcl_strcat",__LINE__ );
    strcpy( s, dest );
    strcpy( s+strlen( dest ), src );
    return s;
}

void tcl_printf(const char *fmt, ...) {
    char buf[1024];
    char format[1024];
    
    va_list args;
    va_start(args, fmt);
    vsprintf(format, fmt, args);
    va_end(args);
    sprintf(buf, "puts -nonewline \"%s\"", format);
    Tcl_Eval(globalinterp,buf);
}

typedef struct _cinv_client {
  Tcl_HashTable types;
  Tcl_HashTable dyntypes;
  Tcl_HashTable structs;
  Tcl_HashTable commands;
} cinv_client;
static cinv_client *cinvclient;

typedef struct _CInv_Type {
    // struct for derived types
    size_t size;
    int cinv_basetype;
    int cinv_inttype;
    int alignment;
    int isSigned;
    int isPtr;
    char* typename;
    char* typetag;
} CInv_Type;

#define init_type(size,cinv_basetype,cinv_inttype,alignment,isSigned,isPtr) { size, cinv_basetype, cinv_inttype, alignment, isSigned, isPtr ,NULL,NULL}

/* just to remember the cc, delete this later!!!!*/
typedef enum _int__ {
	__CINV_CC_CDECL = 0, /**< The cdecl calling convention, the most common convention on x86. */
	__CINV_CC_STDCALL = 1, /**< The stdcall calling convention, the default convention for the Windows API. */
	__CINV_CC_FASTCALL = 2 /**< Yet another, rarely used, Windows calling convention */
} int__;



void def_type(CInv_Type* strt, size_t _size, int _cinv_basetype,int _cinv_inttype,int _alignment,int _isSigned,int _isPtr,char* _typename) {
strt->size=_size;
strt->cinv_basetype=_cinv_basetype;
strt->cinv_inttype=_cinv_inttype;
strt->alignment=_alignment;
strt->isSigned=_isSigned;
strt->isPtr=_isPtr;
if(strt->typename!=NULL) cinv_free(strt->typename,MEM_TCL);
strt->typename=tcl_strdup(_typename);
if(strt->typetag!=NULL) cinv_free(strt->typetag,MEM_TCL);
strt->typetag=tcl_strdup("");
}
void copytype (CInv_Type* dest, CInv_Type* src) {
    memcpy(dest,src,sizeof(CInv_Type));
}
static CInv_Type newtype = init_type(0,0,0,0,0,0);
int type_release(CInv_Type* ttype) {
    //
    if(ttype==NULL) return 0;
    //printf("Releasing %s %p %p %p\n",ttype->typename,ttype,ttype->typename,ttype->typetag);
    //return 0;
    if(ttype->typename!=NULL) cinv_free(ttype->typename,MEM_TCL);
    if(ttype->typetag!=NULL) cinv_free(ttype->typetag,MEM_TCL);
    cinv_free(ttype,MEM_TCL);
    return 0;
}
typedef struct _TclCInvokeState {
	Tcl_Interp *interp;
	Tcl_LoadHandle tcllib;
	CInvContext *ctx;
	CInvLibrary *lib;
	int callingconvention;
} TclCInvokeState;
typedef struct _TclCStructDef {
	Tcl_Interp *interp;
	Tcl_Obj *structname;
	Tcl_Obj *definition;
} TclCStructDef;
typedef struct _TclCStructState {
	Tcl_Interp *interp;
	char* typename;
	CInvContext *ctx;
	CInvStructure *cstruct;
	PTR_TYPE *instance;
	Tcl_Obj *deflist;
	int deflen;
	int binsize;
	int allocsystem;
} TclCStructState;
typedef struct _TclCTypeState {
	Tcl_Interp *interp;
	CInvContext *ctx;
	CInv_Type* xtype;
	int int_type;
	int cinv_type;
	PTR_TYPE *value;
	int deflen;
	int binsize;
	int isarray;
	int iscdata;
	int allocsystem;
} TclCTypeState;
typedef struct _TclCCallbackState {
	Tcl_Interp *interp;
	CInvContext *ctx;
	CInvFunction *prototype;
	CInvCallback* callback;
	int callingconvention;
	char* tcl_procname;
	void* entrypoint;

	int return_type;
	int int_type;
	CInv_Type* xtype;
	char* returninfo;
	
	Tcl_Obj *paramlist;
	int paramlength;
} TclCCallbackState;
typedef struct _TclCCalloutState {
	Tcl_Interp *interp;
	CInvContext *ctx;
	CInvFunction *cfunc;
	int callingconvention;
	void* entrypoint;
	char* returnstring;

	Tcl_Obj *paramlist;
	int paramlength;
	int return_type;
} TclCCalloutState;

/*
 * hash table management
 */
/* define a hashtable entry */
static void entry_define(Tcl_HashTable *table, char *name, void *datum)
{
  int dummy;
  Tcl_HashEntry *entry = Tcl_FindHashEntry(table,name);
  if(entry!=NULL) Tcl_DeleteHashEntry(entry);
  Tcl_SetHashValue(Tcl_CreateHashEntry(table,name,&dummy), datum);
}
/* lookup an existing entry */
static void *entry_lookup(Tcl_HashTable *table, char *name)
{
  Tcl_HashEntry *entry = Tcl_FindHashEntry(table,name);
  return entry ? Tcl_GetHashValue(entry) : NULL;
}
/* find an entry by it's hash value */
static Tcl_HashEntry *entry_find(Tcl_HashTable *table, void *datum)
{
  Tcl_HashSearch search;
  Tcl_HashEntry *entry = Tcl_FirstHashEntry(table, &search);
  while (entry != NULL) {
    if (Tcl_GetHashValue(entry) == datum)
      return entry;
    entry = Tcl_NextHashEntry(&search);
  }
  return NULL;
}
/*
 * type management
 */
/* define a new type */
static void type_define(cinv_client *client, char *tname, CInv_Type *atype)
{
    CInv_Type* ttype=cinv_alloc(sizeof(CInv_Type),MEM_TCL,"type_define",__LINE__);
    copytype(ttype,atype);
    if(ttype->typename!=NULL) cinv_free(ttype->typename,MEM_TCL);
    if(ttype->typetag!=NULL) cinv_free(ttype->typetag,MEM_TCL);
    ttype->typename=tcl_strdup(tname);
    ttype->typetag=tcl_strdup("");
    Tcl_HashEntry *entry = Tcl_FindHashEntry(&cinvclient->types,tname);
    if(entry!=NULL) {
      CInv_Type* tso=Tcl_GetHashValue(entry);
      type_release(tso);
      Tcl_DeleteHashEntry(entry);
    }      
    entry_define(&client->types,tname,(void*)ttype);
}
static void type_define_dyn(cinv_client *client, char *tname, CInv_Type *ttype)
{
    if(ttype->typename!=NULL) cinv_free(ttype->typename,MEM_TCL);
    if(ttype->typetag!=NULL) cinv_free(ttype->typetag,MEM_TCL);
    ttype->typename=tcl_strdup(tname);
    ttype->typetag=tcl_strdup("");
    Tcl_HashEntry *entry = Tcl_FindHashEntry(&cinvclient->dyntypes,tname);
    if(entry!=NULL) {
      CInv_Type* tso=Tcl_GetHashValue(entry);
      type_release(tso);
      Tcl_DeleteHashEntry(entry);
    }      
    
    entry_define(&client->dyntypes,tname,(void*)ttype);
}
/* lookup an existing type */
static CInv_Type *type_lookup(cinv_client *client, char *tname)
{
  void* entry=entry_lookup(&client->types,tname);
  if(entry==NULL) {
      entry=entry_lookup(&client->dyntypes,tname);
  }
  return entry;
}

static void client_delete(ClientData clientData, Tcl_Interp *interp)
{
  cinv_client *client = (cinv_client *)clientData;
  Tcl_HashSearch search;
  Tcl_HashEntry *entry;
  
  if(client==NULL) return;
  Tcl_ResetResult(interp);
  /* free all allocated commands */
  for (entry = Tcl_FirstHashEntry(&client->commands, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
      //printf("Freeing %s\n",Tcl_GetHashKey(&client->commands,entry));
      Tcl_DeleteCommand(interp,Tcl_GetHashKey(&client->commands,entry));
  }
  Tcl_DeleteHashTable(&client->commands);
  for (entry = Tcl_FirstHashEntry(&client->structs, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
      //printf("Freeing %s\n",Tcl_GetHashKey(&client->structs,entry));
      TclCStructDef* ts=Tcl_GetHashValue(entry);
      Tcl_DecrRefCount(ts->structname);
      Tcl_DecrRefCount(ts->definition);
      cinv_free(ts,MEM_TCL);
  }
  Tcl_DeleteHashTable(&client->structs);
  /* free all allocated typedefs */
  for (entry = Tcl_FirstHashEntry(&client->dyntypes, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
      //printf("Freeing %s\n",Tcl_GetHashKey(&client->dyntypes,entry));
      CInv_Type* ctype=Tcl_GetHashValue(entry);
      type_release(ctype);
  }
  Tcl_DeleteHashTable(&client->dyntypes);
  for (entry = Tcl_FirstHashEntry(&client->types, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
      //printf("Freeing %s\n",Tcl_GetHashKey(&client->types,entry));
      CInv_Type* ctype=Tcl_GetHashValue(entry);
      type_release(ctype);
  }
  Tcl_DeleteHashTable(&client->types);
 
  /* free client structure */
  cinv_free((void *)client,MEM_TCL);
}
/* client allocation and initialization */
static cinv_client *client_alloc(Tcl_Interp *interp)
{
  cinv_client *client;

  /* allocate client data structure */
  client = (cinv_client *)cinv_alloc(sizeof(cinv_client),MEM_TCL,"client alloc",__LINE__);

  /* allocate hashtables for this load */
  Tcl_InitHashTable(&client->types, TCL_STRING_KEYS);
  Tcl_InitHashTable(&client->dyntypes, TCL_STRING_KEYS);
  Tcl_InitHashTable(&client->commands, TCL_STRING_KEYS);
  Tcl_InitHashTable(&client->structs, TCL_STRING_KEYS);
  /* arrange for cleanup on interpreter deletion */
  Tcl_CallWhenDeleted(interp, client_delete, (ClientData)client);

  /* finis */
  return client;
}

enum basiccinvtypes {
        _CINV_T_CHAR = 0, /**< Represents the \b char C type */
        _CINV_T_SHORT = 1, /**< Represents the \b short C type */
        _CINV_T_INT = 2, /**< Represents the \b int C type */
        _CINV_T_LONG = 3, /**< Represents the \b long C type */
        _CINV_T_EXTRALONG = 4, /**< Represents the <b>long long</b> C type */
        _CINV_T_FLOAT = 5, /**< Represents the \b float C type */
        _CINV_T_DOUBLE = 6, /**< Represents the \b double C type */
        _CINV_T_PTR = 7, /**< Represents any pointer type */
        _CINV_T_STRUCT = 8,
        _CINV_T_PTR_BYTE	= 9,	/* byte array pointer */
        _CINV_T_PTR_UTF8	= 10,	/* UTF-8 string pointer */
        _CINV_T_PTR_UTF16	= 11,	/* UTF-16 string pointer FAILS */
        _CINV_T_PTR_VAR	= 12,	/* byte array in variable UNUSED */
        _CINV_T_PTR_OBJ	= 13,	/* Tcl_Obj pointer UNUSED */
        _CINV_T_PTR_PROC	= 14,	/* Pointer to Tcl proc for callback */
        _CINV_T_CHARSTR	= 15,
        _CINV_T_INTERP = 16,    /* return the Tcl_Interp */
        _CINV_T_MAX=17,
} ;
#define SIGNEDTYPEINDEX(type_)                              \
    do {                                                    \
        if (sizeof(type_) == sizeof(signed char))           \
            typeIndex = _CINV_T_CHAR;                  \
        else if (sizeof(type_) == sizeof(signed short))     \
            typeIndex = _CINV_T_SHORT;                  \
        else if (sizeof(type_) == sizeof(signed int))       \
            typeIndex = _CINV_T_INT;                    \
        else if (sizeof(type_) == sizeof(signed long))      \
            typeIndex = _CINV_T_LONG;                   \
        else if (sizeof(type_) == sizeof(signed long long)) \
            typeIndex = _CINV_T_EXTRALONG;               \
        else                                                \
            typeIndex=-1;                                 \
    } while (0)

#define UNSIGNEDTYPEINDEX(type_)                              \
    do {                                                      \
        if (sizeof(type_) == sizeof(unsigned char))           \
            typeIndex = _CINV_T_CHAR;                    \
        else if (sizeof(type_) == sizeof(unsigned short))     \
            typeIndex = _CINV_T_SHORT;                   \
        else if (sizeof(type_) == sizeof(unsigned int))       \
            typeIndex = _CINV_T_INT;                     \
        else if (sizeof(type_) == sizeof(unsigned long))      \
            typeIndex = _CINV_T_LONG;                    \
        else if (sizeof(type_) == sizeof(unsigned long long)) \
            typeIndex = _CINV_T_EXTRALONG;                \
        else                                                  \
             typeIndex=-1;                                 \
    } while (0)
    
#define ADDTYPEINDEX(newtype_, existingIndex_,isSigned_)                                \
    static CInv_Type cinv_type_ ## newtype_ = init_type(0,0,0,0,0,0);                   \
    do {                                                                      \
        if(existingIndex_<0) return TCL_ERROR; \
        /*printf("cinv_type_"# newtype_"\n");*/ \
        def_type(&cinv_type_ ## newtype_, sizeof(newtype_), existingIndex_, existingIndex_, 0,isSigned_, 0, #newtype_); \
        type_define(cinvclient,  #newtype_ , &cinv_type_ ## newtype_); \
    } while (0)

#define ADDINTTYPE(type_)               \
    do {                                \
        int typeIndex;    \
        type_ val = (type_)-1;          \
        if (val < 0)                    \
            SIGNEDTYPEINDEX(type_);     \
        else                            \
            UNSIGNEDTYPEINDEX(type_);   \
        ADDTYPEINDEX(type_, typeIndex,(val<0?1:0)); \
    } while (0)

char* getcurrentns(Tcl_Interp* interp) {
    Tcl_Namespace* nsptr;
    nsptr=Tcl_GetCurrentNamespace(interp);
    return tcl_strdup(nsptr->fullName);
}
char* buildnamespacename(Tcl_Interp* interp,char* cmdname) {
    //return cmdname;
    char* currentns;
    currentns=getcurrentns(interp);
    if(strcmp(currentns,"::")==0) {
        cinv_free(currentns,MEM_TCL);
        return tcl_strdup(cmdname);
    }
    if((char)cmdname[0]!=':') {
        if((char)currentns[strlen(currentns)-1]!=':') {
            char* ncns=tcl_strcat(currentns,"::");
            cinv_free(currentns,MEM_TCL);
            currentns=ncns;
        }
    } 
    char* r= tcl_strcat(currentns,cmdname);
    cinv_free(currentns,MEM_TCL);
    return r;
}
Tcl_Command Tcl_CreateObjCommandNS(Tcl_Interp* interp, char* cmdName, void* proc, void* clientData, void* deleteProc) {
    char* nscmdname=buildnamespacename(interp,cmdName);
    Tcl_Command rv= Tcl_CreateObjCommand(interp, nscmdname, proc, clientData, deleteProc);
    entry_define(&cinvclient->commands,nscmdname,rv);
    Tcl_AppendResult(interp,nscmdname,NULL);
    cinv_free(nscmdname,MEM_TCL);
    return rv;
}

int parseArray (char* str) {
    // parse if its an array ()
    // return -1 if not
    // return 0 if undefined size ()
    // return size otherwise
    // set str to stripped name
    
    char *tok;
    int rv=-1;
    char delimiter[] = ")(";
    // initialisieren und ersten Abschnitt erstellen
    tok = strtok(str, delimiter);
    
    if(tok != NULL) {
        // naechsten Abschnitt erstellen
        tok = strtok(NULL, delimiter);
    }    
    if(tok != NULL) {
        rv=0;
        char* end;
        if(tok[0]!=' ') {
            int size=strtol(tok,&end,10);
            if(end==tok) {
                return 0;
            }
            rv=size;
        }
    }
    return rv;
}
int parsePtr (char* str) {
    // parse if its a Ptr *
    // return -1 if not
    // return 1 if is
    // set str to stripped name
    char* tok;
    int rv=-1;
    char delimiter[] = "*";

    // initialisieren und ersten Abschnitt erstellen
    tok = strtok(str, delimiter);
    
    if(tok != NULL) {
        // naechsten Abschnitt erstellen
        tok = strtok(NULL, delimiter);
        return 1;
    }    
    return rv;        
}
char* parsePtrTag (char* str) {
    // parse if its mixed ptr.tag
    // return tag if it has one
    // return NULL if there is no tag
    // set str to stripped name
    char* tok;
    char delimiter[] = ".";

    // initialisieren und ersten Abschnitt erstellen
    tok = strtok(str, delimiter);
    if(tok != NULL) {
        // naechsten Abschnitt erstellen
        tok = strtok(NULL, delimiter);
        return tok;
    }    
    
    return NULL;        
}

int iparse_type(char fmt) {
    static char _typeformats[CINV_NUM_TYPES] = {
        'c', 's', 'i', 'l', 'e', 'f', 'd', 'p'
    };
	int i;
	char lower = tolower(fmt);
	if (lower == '2') {
		return CINV_T_2BYTE;
	}
	if (lower == '4') {
		return CINV_T_4BYTE;
	}
	if (lower == '8') {
		return CINV_T_8BYTE;
	}
	for (i = 0; i < CINV_NUM_TYPES; i++) {
		if (_typeformats[i] == lower) {
			return (cinv_type_t)i;
		}
	}
	return -1;
}
char* isig_type(int itype) {
    static char* _typeformats[CINV_NUM_TYPES] = {
        "c", "s", "i", "l", "e", "f", "d", "p"
    };
    if(itype<0) return "p";
    if(itype>=CINV_NUM_TYPES) return "p";
    return _typeformats[itype];
}
void removeChar(char * str, char charToRemmove){
    int i, j;
    int len = strlen(str);
    for(i=0; i<len; i++)
    {
        if(str[i] == charToRemmove)
        {
            for(j=i; j<len; j++)
            {
                str[j] = str[j+1];
            }
            len--;
            i--;
        }
    }
}

int get_size_from_type(int type) {
	switch (type) {
        case _CINV_T_CHAR: return (1); /**< Represents the \b char C type */
        case _CINV_T_SHORT: return ( sizeof(short)); /**< Represents the \b short C type */
        case _CINV_T_INT: return ( sizeof(int)); /**< Represents the \b int C type */
        case _CINV_T_LONG: return ( sizeof(long)); /**< Represents the \b long C type */
        case _CINV_T_EXTRALONG: return ( sizeof(long long)); /**< Represents the <b>long long</b> C type */
        case _CINV_T_FLOAT: return ( sizeof(float)); /**< Represents the \b float C type */
        case _CINV_T_DOUBLE: return ( sizeof(double)); /**< Represents the \b double C type */
        case _CINV_T_PTR: return ( sizeof(void*)); /**< Represents any pointer type */
        case _CINV_T_STRUCT: return ( sizeof(void*)); /**< Represents a struct */
        case _CINV_T_PTR_BYTE	: return (sizeof(void*));	/* byte array pointer */
        case _CINV_T_PTR_UTF8	: return (sizeof(void*));	/* UTF-8 string pointer */
        case _CINV_T_PTR_UTF16: return (sizeof(void*));	/* UTF-16 string pointer FAILS */
        case _CINV_T_PTR_VAR	: return (sizeof(void*));	/* byte array in variable UNUSED */
        case _CINV_T_PTR_OBJ	: return (sizeof(void*));	/* Tcl_Obj pointer UNUSED */
        case _CINV_T_PTR_PROC	: return (sizeof(void*));	/* Pointer to Tcl proc UNUSED */
        case _CINV_T_CHARSTR: return(sizeof(void*)); /* copy the string into an strdup allocated one */
        case _CINV_T_INTERP: return(sizeof(void*)); 
	};
	
    return sizeof(void*);
}
int get_type_from_size(int size) {
    for (int i=0;i<=_CINV_T_PTR;i++) {
        if(get_size_from_type(i)==size) {
            return i;
        }
    }
    return -1;
}
int get_type_from_str(Tcl_Interp *interp, Tcl_Obj *strin, int *type, CInv_Type** xtype) {
    // define basic and more common types
    // string will be used as const char*
    // otherwise use CType string and use getptr
    // returns a FIXED ADRESS of a type
    // COPY if necessary with alloc//copytype
    
    static CONST char *cinvtypes[] = {
		"char", "short", "int", "long", "longlong", "float", "double", "ptr",
		"struct","bytearray","string","utfstring","bytevar","tclobj","tclproc",
		"char*", "interp", 
		
		(char *) NULL
	};
	
	//printf("Lookup Type %s %d\n",Tcl_GetString(strin),Tcl_IsShared(strin));
    char* str1=tcl_strdup(Tcl_GetString(strin));
    char* tag=parsePtrTag(str1);
    Tcl_Obj* str;
    //printf("found ptr %s tag %s\n",str1,tag);
    if((strcmp(str1,"ptr")==0)&&(tag!=NULL)) {
        //printf("found ptr %s tag %s\n",str1,tag);
        str=Tcl_NewStringObj(str1,-1);
    } else {
        tag="";
        str=strin;
    }
    if (Tcl_GetIndexFromObj(interp, str, cinvtypes, "type", TCL_EXACT,
                type) != TCL_OK) {
        // lookup extended types
        Tcl_ResetResult(interp);
        CInv_Type* ixtype=type_lookup(cinvclient,str1);
        if(ixtype==NULL) {
            Tcl_AppendResult(interp, "...Type not found ",Tcl_GetString(str)," ",tag,NULL);
            cinv_free(str1,MEM_TCL);
            return TCL_ERROR;
        }
        *type=ixtype->cinv_inttype;
        if(xtype!=NULL) {
            CInv_Type* ntype=cinv_alloc(sizeof(CInv_Type),MEM_TCL,Tcl_GetString(strin),__LINE__);
            copytype(ntype,ixtype);
            ntype->typename=tcl_strdup(ixtype->typename);
            ntype->typetag=tcl_strdup(ixtype->typetag);
            *xtype=ntype;
        }
        //printf("found ext type for %s %d %p\n",Tcl_GetString(str),*type,*xtype);
    } else {
        // define pseudotype from basic type
        if(xtype!=NULL) {
            //printf("Defining pseudotype %s %s\n",Tcl_GetString(str),tag);
            CInv_Type* ntype=cinv_alloc(sizeof(CInv_Type),MEM_TCL,Tcl_GetString(str),__LINE__);
            ntype->size=get_size_from_type(*type);
            ntype->cinv_basetype=min(*type,_CINV_T_PTR);
            ntype->cinv_inttype= *type;
            ntype->alignment=newtype.size;
            ntype->isSigned=1;
            ntype->isPtr=0;
            ntype->typename=tcl_strdup(Tcl_GetString(str));
            if(tag) {
                ntype->typetag=tcl_strdup(tag);
            } else {
                ntype->typetag=tcl_strdup("");
            }
            *xtype=ntype;
            //printf("returning newtype %s %p %p\n",Tcl_GetString(str),*xtype,&newtype);
        } else {
            //printf("NOT returning newtype %p %p\n",xtype,xtype);
        }

    }
    if(str!=strin) Tcl_DecrRefCount(str);
    cinv_free(str1,MEM_TCL);
    
    return TCL_OK;
}
int get_type_from_name(Tcl_Interp *interp,TclCStructState *ts, Tcl_Obj *name, CInv_Type **xtype, int *asarray) {
	Tcl_Obj** ndeflist;
	int llength, isarray;
	char *cstr,*elem;
    Tcl_Obj **sublist;
    int sublength;
	int type=-1;
	
	Tcl_Obj* odeflist=ts->deflist;
	llength=ts->deflen;
	
	if (Tcl_ListObjGetElements(interp, odeflist, &llength, &ndeflist)!=TCL_OK) {
	    return TCL_ERROR;
	}
	
	cstr=Tcl_GetString(name);
	int found=0;
	
    *asarray=-1;
	for (int i=0;i<llength;i+=2) {
        elem=sys_strdup(Tcl_GetString(ndeflist[i+1]));
        
        if((strcmp(Tcl_GetString(ndeflist[i]),"struct")==0)||(strcmp(Tcl_GetString(ndeflist[i]),"union")==0)) {
            if (Tcl_ListObjGetElements(interp, ndeflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            TclCStructState *tssub;
            tssub = (void *) cinv_alloc(sizeof(*tssub),MEM_TCL,"get_type_from_name",__LINE__);
            tssub->deflist=ndeflist[i+1];
            tssub->deflen=sublength;
            if (get_type_from_name(interp,tssub,name, xtype,asarray)==TCL_OK) {
                cinv_free(tssub,MEM_TCL);
                cinv_free(elem,MEM_SYSTEM);
                return TCL_OK;
            }
            cinv_free(tssub,MEM_TCL);
            cinv_free(elem,MEM_SYSTEM);
            continue;
        }
        
        isarray=parseArray(elem);
        if(strcmp(elem,cstr)==0) {
            if (isarray>0) {
                *asarray=isarray;
            } 
            if (get_type_from_str(interp,ndeflist[i], &type,xtype)!=TCL_OK) return TCL_ERROR;
            //printf("Getting type from name %d %p %p\n",type,xtype,&xtype);
            found++;
            cinv_free(elem,MEM_SYSTEM);
            break;
        }
        cinv_free(elem,MEM_SYSTEM);
        
	}
    

	if ((found==0)||(&type<0)) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

CInv_Type* size_type(CInv_Type* strt, int size,int _isSigned, char* _typename) {
if(strt==NULL) {
    strt=cinv_alloc(sizeof(CInv_Type),MEM_TCL,"size_type",__LINE__);
    copytype(strt,&newtype);
}
strt->size=size;
strt->cinv_basetype=get_type_from_size(size);
strt->cinv_inttype=get_type_from_size(size);
strt->alignment=size;
strt->isSigned=_isSigned;
strt->isPtr=0;
if(strt->typename!=NULL) cinv_free(strt->typename,MEM_TCL);
strt->typename=tcl_strdup(_typename);
if(strt->typetag!=NULL) cinv_free(strt->typetag,MEM_TCL);
strt->typetag=tcl_strdup("");
return strt;
}

int GetStructPtr(Tcl_Interp *interp, Tcl_Obj *obj, void* value) {
    
}
int get_value_from_obj(Tcl_Interp *interp, int cinvtype, CInv_Type* xtype, Tcl_Obj *obj, PTR_TYPE* value, int* size) {
    char *pcv,*retpcv;
    unsigned char* upcv;
    int iv;
    long il;
    long long ill;
    double fd;
    int _isSigned=1;
    //int _isPtr=0;
    //int _alignment=0;
    //int _size=0;
    PTR_TYPE* tw;
    char* _typename="";
    char* _typetag="";
    
    int rv;
    //char buf[2256];
    int len=0;
    int maxlen=*size;
    *(int*)size=1;
    if(xtype!=NULL) {
        //printf("got detailed info on type %d: %d %d sz %d sig %d ptr %d name %s\n",cinvtype,xtype->cinv_basetype, xtype->cinv_inttype, xtype->size, xtype->isSigned,xtype->isPtr, xtype->typename);
        _isSigned=xtype->isSigned;
        _typename=xtype->typename;
        _typetag=xtype->typetag;
        if(strcmp(_typetag,"")) {
           _typename= _typetag;
        }
        //_alignment=xtype->alignment;
        //_isPtr=xtype->isPtr;
        //_size=xtype->size;
        //cinvtype=xtype->cinv_inttype;
    } else {
        Tcl_AppendResult(interp, "ERROR: No type given",NULL);
        Tcl_Panic("TYPE ERROR NO TYPE");
        return TCL_ERROR;
    }        
    if(cinvtype!=xtype->cinv_inttype) {
        Tcl_AppendResult(interp, "GET ERROR: Type mismatch",NULL);
        Tcl_Panic("TYPE ERROR TYPE MISMATCH");
        return TCL_ERROR;
    }
    switch (cinvtype) {
        case _CINV_T_CHAR:
            rv=Tcl_GetIntFromObj(interp,obj,&iv);
            if(_isSigned==0)  {
                *(unsigned char*)value=(unsigned char)iv;
            } else {
                *(char*)value=(char)iv;
            }
            return rv;
            
        case _CINV_T_SHORT:
            rv=Tcl_GetIntFromObj(interp,obj,&iv);
            if(_isSigned==0)  {
                *(unsigned short*)value=(unsigned short)iv;
            }else {
                *(short*)value=(short)iv;
            }
            return rv;
            
        case _CINV_T_INT:
            rv=Tcl_GetIntFromObj(interp,obj,&iv);
            if(_isSigned==0)  {
                *(unsigned int*)value=(unsigned int)iv;
            } else {
                *(int*)value=(int)iv;
            }
            return rv;
            
        case _CINV_T_LONG:
            rv=Tcl_GetLongFromObj(interp,obj,&il);
            if(_isSigned==0)  {
                *(unsigned long*)value=(unsigned long)il;
            } else {
                *(long*)value=(long)il;
            }
            return rv;
            
        case _CINV_T_EXTRALONG:
            rv=Tcl_GetWideIntFromObj(interp,obj,&ill);
            if(_isSigned==0)  {
                *(unsigned long long*)value=(unsigned long long)ill;
            } else {
                *(long long*)value=(long long)ill;
            }
            return rv;
            
        case _CINV_T_FLOAT:
            rv=Tcl_GetDoubleFromObj(interp,obj,&fd);
            *(float*)value=(float)fd;
            return rv;
            
        case _CINV_T_DOUBLE:
            rv=Tcl_GetDoubleFromObj(interp,obj,&fd);
            *(double*)value=(double)fd;
            return rv;
        case _CINV_T_PTR: 
            rv=Ffidl_GetPointerFromObj(interp,obj,&tw,_typename);
            *(PTR_TYPE*) value=(PTR_TYPE)tw;
            return rv;
        case _CINV_T_STRUCT: 
            // not implemenented
            //printf("Got struct ptr %s %s\n",_typename, Tcl_GetString(obj));
            rv=Ffidl_GetPointerFromObj(interp,obj,&tw,_typename);
            *(PTR_TYPE*) value=(PTR_TYPE)tw;
            return rv;
        case _CINV_T_PTR_BYTE:
            upcv=Tcl_GetByteArrayFromObj(obj, &len);
            if((maxlen>0)&&(len>maxlen)) {
                len=maxlen;
            }
            retpcv = cinv_alloc(max(len,maxlen) + 1,MEM_TCL,"get_value_from_obj",__LINE__);  // Space for length plus nul
            if (retpcv == NULL) return TCL_ERROR;          // No memory
            memcpy(retpcv, upcv,len);                     // Copy the characters
            
            // init zero
            if(len<maxlen) {
                char* dest=retpcv+len;
                len=maxlen-len; 
                while (len) { 
                    *(dest) = '\0'; 
                    (dest)++; 
                    (len)--;
                }   
                len=maxlen;
            }
            *(char**)value=retpcv;
            *(int*)size=len;
            return TCL_OK;
        case _CINV_T_PTR_VAR:
            obj = Tcl_ObjGetVar2(interp, obj, NULL, TCL_LEAVE_ERR_MSG);
            if (obj == NULL) return TCL_ERROR;
            if (Tcl_IsShared(obj)) {
                obj = Tcl_ObjSetVar2(interp, obj, NULL, Tcl_DuplicateObj(obj), TCL_LEAVE_ERR_MSG);
                if (obj == NULL) {
                    return TCL_ERROR;
                }
                //printf("Duplicated obj\n");
            }
            *(void **)value = (void *)Tcl_GetByteArrayFromObj(obj, &len);
            //printf("pointer-var -> %p %d\n", *value,len); 
            Tcl_InvalidateStringRep(obj);
            //printf("invalidated \n");
            if((maxlen>0)&&(len>maxlen)) {
                len=maxlen;
            }
            *(int*)size=len;
            return TCL_OK;            
        case _CINV_T_PTR_OBJ:
            //printf("Getting tclobj\n");
            rv=Ffidl_GetPointerFromObj(interp,obj,&tw,_typename);
            *(PTR_TYPE*) value=(PTR_TYPE)tw;
            return rv;
        case _CINV_T_PTR_UTF8:
            pcv=Tcl_GetString(obj);
            *(int*) size=strlen(pcv);
            retpcv=tcl_strdup(pcv);
            *(char**)value=retpcv;
            return TCL_OK;
        case _CINV_T_CHARSTR:
            pcv=Tcl_GetString(obj);
            *(int*) size=strlen(pcv);
            retpcv=sys_strdup(pcv);
            *(char**)value=retpcv;
            return TCL_OK;
        case _CINV_T_PTR_UTF16:
            // no unicode support, soorry
            pcv=Tcl_GetString(obj);
            *(int*) size=strlen(pcv);
            retpcv=tcl_strdup(pcv);
            *(char**)value=retpcv;
            return TCL_OK;
        case _CINV_T_PTR_PROC:
            // support for callbacks
            rv=Ffidl_GetPointerFromObj(interp,obj,&tw,_typename);
            *(PTR_TYPE*) value=(PTR_TYPE)tw;
            return rv;
        case _CINV_T_INTERP:
            // support for callbacks
            rv=TCL_OK;
            *(PTR_TYPE*) value=(PTR_TYPE)(PTR_TYPE*)interp;
            return rv;
        default:
            *size=0;
            
            Tcl_AppendResult(interp,"internal error during type cast ",NULL);
            return TCL_ERROR;
    }
	
    return TCL_OK;
}
Tcl_Obj* set_obj_from_value(Tcl_Interp *interp, int cinvtype, CInv_Type* xtype, void* value, int size) {
	
    //char buf[256];
    Tcl_Obj* obj;
    int _isSigned=0;
    //int _isPtr=0;
    //int _alignment=0;
    //int _size=0;
    char* _typename="";
    char* _typetag="";
    
    if(value==NULL) return NULL;
    
    if(xtype!=NULL) {
        //printf("got detailed info on type %s.%s %d: %d %d sz %d sig %d \n",xtype->typename, xtype->typetag, cinvtype,xtype->cinv_basetype, xtype->cinv_inttype, xtype->size, xtype->isSigned);
        _isSigned=xtype->isSigned;
        _typename=xtype->typename;
        _typetag=xtype->typetag;
        if(strcmp(_typetag,"")) {
           _typename= _typetag;
        }
        //_alignment=xtype->alignment;
        //_isPtr=xtype->isPtr;
        //_size=xtype->size;
        //cinvtype=xtype->cinv_inttype;
    } else {
        Tcl_AppendResult(interp, "SET ERROR: No type given",NULL);
        return NULL;
    }        
    if(cinvtype!=xtype->cinv_inttype) {
        Tcl_AppendResult(interp, "SET ERROR: Type mismatch",NULL);
        return NULL;
    }
    
    switch (cinvtype) {
        case _CINV_T_CHAR:
            if(_isSigned==0)  {
                obj= (Tcl_NewIntObj(*(unsigned char *)value));
           } else {
                obj= (Tcl_NewIntObj(*(char *)value));
            }
            return obj;
            
        case _CINV_T_SHORT: 
            if(_isSigned==0)  {
                obj= (Tcl_NewIntObj(*(unsigned short *)value));
           } else {
                obj= (Tcl_NewIntObj(*(short *)value));
            }
            return obj;
            
        case _CINV_T_INT:
            if(_isSigned==0)  {
                obj= (Tcl_NewIntObj(*(unsigned int *)value));
           } else {
                obj= (Tcl_NewIntObj(*(int *)value));
            }
            return obj;
            
        case _CINV_T_LONG:
            if(_isSigned==0)  {
                obj= (Tcl_NewLongObj((unsigned long) *(unsigned long *)value));
           } else {
                obj= (Tcl_NewLongObj(*(long *)value));
            }
            return obj;
            
        case _CINV_T_EXTRALONG:
            if(_isSigned==0)  {
                obj= (Tcl_NewWideIntObj(*(unsigned long long *)value));
           } else {
                obj= (Tcl_NewWideIntObj(*(long long*)value));
            }
            return obj;
            
        case _CINV_T_FLOAT:
            obj= Tcl_NewDoubleObj(*(float*)value);
            return obj;
            
        case _CINV_T_DOUBLE:
            obj= Tcl_NewDoubleObj(*(double*)value);
            return obj;
            
        case _CINV_T_PTR: 
            //printf("creating pointer of type %s %s\n",_typename,_typetag);
            obj=Ffidl_NewPointerObj(*(void**)value,_typename);
            return obj;
            
        case _CINV_T_STRUCT: 
            obj=Ffidl_NewPointerObj(*(void**)value,_typename);
            return obj;
        case _CINV_T_PTR_BYTE:
          obj= Tcl_NewByteArrayObj(*(unsigned char **)value, size); //XXX get a real value here!!!
          return obj;
        case _CINV_T_PTR_OBJ:
            //printf("Setting tclobj\n");
          obj= *(Tcl_Obj **)value;
          return obj;
        case _CINV_T_PTR_UTF8:
          obj= Tcl_NewStringObj(*(char **)value, -1);
          return obj;
        case _CINV_T_PTR_UTF16:
          obj= Tcl_NewUnicodeObj(*(Tcl_UniChar **)value, -1);
          return obj;            
        case _CINV_T_CHARSTR:
          obj= Tcl_NewStringObj(*(char **)value, -1);
          return obj;
        case _CINV_T_INTERP:
            obj=Ffidl_NewPointerObj(*(void**)value,_typename);
            return obj;
          
        default:
            return NULL;
        }
	
    return NULL;
}

static int CCalloutCallout  (ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]) {
    //
	Tcl_Obj** paramlist;
	int paramlength;
	char *paramstring,*paramtypes;
    char* retsig="\0";
    char* paramsig="";
    int ret_type=-1;
    int int_type, cinv_type,len, is_PtrType;
    int callingconvention;
    
	CInv_Type* xtype,*rxtype;
    CInvContext *ctx ;
	PTR_TYPE returnval = 0;
	void **parameters = NULL;
	void  *val;
        
    TclCCalloutState* ts;
    ts=(TclCCalloutState*) cdata;
    ctx=ts->ctx;
    
    void* funcptr=ts->entrypoint;
    ret_type=ts->return_type;
    
    if(funcptr==NULL) {
        Tcl_AppendResult(interp,"Function entrypoint not found \n",NULL);
        return TCL_ERROR;
    }
    
    if (ret_type>=_CINV_T_STRUCT) {
        retsig="p";
    } else {
        retsig=isig_type(ret_type);
    }
    
    if (Tcl_ListObjGetElements(interp, ts->paramlist, &paramlength, &paramlist)!=TCL_OK) {
        return TCL_ERROR;
    }

    len=0;
    if (objc>1) {
        // pars params and prepare
        len = paramlength;
        parameters = cinv_alloc(len * sizeof(void*),MEM_TCL,"CInvokeHandleCmd",__LINE__);
        paramsig= cinv_alloc((len+1) * sizeof(char),MEM_TCL,"CInvokeHandleCmd",__LINE__);
        paramtypes=cinv_alloc(len * sizeof(int),MEM_TCL,"CInvokeHandleCmd",__LINE__);
        for (int i = 0; i < len; i++) {
            
            paramstring=Tcl_GetString(paramlist[i]);
            //Tcl_AppendResult(interp,paramstring,NULL);
            if (get_type_from_str(interp,paramlist[i], &int_type,&xtype)!=TCL_OK) return TCL_ERROR;
//printf("got type %s as %d %p\n",paramstring,int_type,&xtype);
//printf("%d %d size %d %s\n",xtype->cinv_basetype,xtype->cinv_inttype, xtype->size,xtype->typetag);

            if (int_type<0) {
                Tcl_AppendResult(interp, "Unknown type ",TclGetString(paramlist[i]),"\n",NULL);
                Tcl_AppendResult(interp, "function ",TclGetString(objv[0])," ",TclGetString(ts->paramlist),"\n",NULL);
                return TCL_ERROR;
            }
            
            if (int_type>=_CINV_T_STRUCT) {
                is_PtrType=1;
                cinv_type=_CINV_T_PTR;
            } else {
                is_PtrType=0;
                cinv_type=int_type;
            }
            paramtypes[i]=int_type;
            // hack to transfer const char* and certain ptrs correctly
            if(strcmp(paramstring,"string")==0) is_PtrType=0;
            if(strcmp(paramstring,"char*")==0) is_PtrType=0;
            if(strcmp(paramstring,"utfstring")==0) is_PtrType=0;
            if(strcmp(paramstring,"tclproc")==0) is_PtrType=0;
            if(strcmp(paramstring,"interp")==0) is_PtrType=0;
            if(strcmp(paramstring,"bytevar")==0) is_PtrType=0;
            if (int_type==_CINV_T_STRUCT) {
                is_PtrType=0;
            }

            if(objc>1+i) {
                val=cinv_alloc(get_size_from_type(cinv_type),MEM_TCL,"CInvokeHandleCmd",__LINE__);
                int oklen;
                if(get_value_from_obj(interp,int_type,xtype,objv[1+i],val,&oklen)!=TCL_OK) {
                    cinv_free(paramsig,MEM_TCL);
                    cinv_free(parameters,MEM_TCL);
                    cinv_free(paramtypes,MEM_TCL);
                    cinv_free(val,MEM_TCL);
                    type_release(xtype);
                    Tcl_AppendResult(interp, "\nfunction ",TclGetString(objv[0])," ",TclGetString(ts->paramlist),NULL);
                    return TCL_ERROR;
                }
                paramsig[i]=isig_type(cinv_type)[0];
                if(is_PtrType==0) {
                    parameters[i]=val;
                } else {
                    void* pval=cinv_alloc(sizeof(void*),MEM_TCL,"CInvokeHandleCmd",__LINE__);
                    pval=&val;
                    parameters[i]=pval;
                    paramsig[i]='p';
                }
            }

            paramsig[i+1]='\0';
            type_release(xtype);
        }
    }

    Tcl_ResetResult(interp);
    if(ts->callingconvention<0) {
        callingconvention=CINV_CC_DEFAULT;
    } else {
        callingconvention=ts->callingconvention;
    }
        
    CInvFunction *f = cinv_function_create(ctx, callingconvention, retsig, paramsig);
    //printf("Calling %s %s\n",retsig,paramsig);
    if (!cinv_function_invoke(ctx, f, funcptr, &returnval, parameters)) {
        Tcl_AppendResult(interp,"invoke failed: \n", cinv_context_geterrormsg(ctx),NULL);
        return TCL_ERROR;
    }
    //printf("ready %s %s\n",retsig,paramsig);
    if(ret_type>0) {
        Tcl_Obj* o=Tcl_NewStringObj(ts->returnstring,-1);
        if (get_type_from_str(interp,o, &ret_type,&rxtype)!=TCL_OK) return TCL_ERROR;
        Tcl_Obj *obj=set_obj_from_value(interp, ret_type,rxtype, &returnval, 0);
        if(obj!=NULL) Tcl_SetObjResult(interp,obj);
        type_release(rxtype);
        Tcl_DecrRefCount(o);
    }
    cinv_function_delete(ctx, f);
    
    for (int i = 0; i < len; i++) {
        //
        if(paramsig[i]=='p') {
            int_type=paramtypes[i];
            if((int_type!=_CINV_T_PTR_VAR)&&(int_type!=_CINV_T_PTR)&&(int_type!=_CINV_T_STRUCT)
                    &&(int_type!=_CINV_T_PTR_OBJ)&&(int_type!=_CINV_T_PTR_PROC)&&(int_type!=_CINV_T_INTERP)) {
                if(*(char**)parameters[i]!=NULL) {
                    cinv_free(*(char**)parameters[i],MEM_TCL);
                }
            }
            cinv_free(parameters[i],MEM_TCL);
        } else {
            cinv_free(parameters[i],MEM_TCL);
        }
    }
    if(len>0) {
        cinv_free(paramsig,MEM_TCL);
        cinv_free(parameters,MEM_TCL);
        cinv_free(paramtypes,MEM_TCL);
    }
    //printf("call succeeded\n");
    return TCL_OK;     
}
void CCalloutCCommandDeleteProc (ClientData cdata) {
    //
    TclCCalloutState* ts;
    ts=(TclCCalloutState*) cdata;

	if (NULL!=ts->paramlist) {
        Tcl_DecrRefCount(ts->paramlist);
    }
	
	cinv_free((char *) ts->returnstring,MEM_TCL);
	cinv_free((void *) ts,MEM_TCL);
	
    return;
    
};

static void CInvokeCCommandDeleteProc(ClientData cdata) {
	TclCInvokeState *ts;
	CInvContext *ctx ;
    CInvLibrary *lib;
    Tcl_LoadHandle tcllib;
    //char buf[256];
	ts = (TclCInvokeState *) cdata;
	ctx = ts->ctx;
	lib = ts->lib;
	tcllib=ts->tcllib;
	Tcl_Interp* interp= ts->interp;

	if (lib!=NULL) {
        if (!cinv_library_delete(ctx, lib)) {
            //Tcl_AppendResult(interp, "Error deleting library",cinv_context_geterrormsg(ctx),NULL);
            //return;
        }
    }
	if (tcllib!=NULL) {
	    Tcl_FSUnloadFile(interp,tcllib);
    }
    if(ctx!=NULL) {
        if (!cinv_context_delete(ctx)) {
            //printf("failed deleting ctx %p\n",ctx);
        }
    }
    //printf("deleting ts %p\n",ts);
	cinv_free((void *) ts,MEM_TCL);
    return;

}
static int CInvokeHandleCmd( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	void  *val;
	int index;
	char *symbol,*funcname,*retsig,*paramsig;
	Tcl_Obj** paramlist;
	int paramlength;
	char *retstring,*paramstring,*paramtypes;
	char* tclsym;
	//char buf[256];
	int len,is_PtrType;
    int int_type,cinv_type, ret_type;

	TclCInvokeState *ts;
	CInvContext *ctx ;
	CInv_Type* xtype, *rxtype;
    Tcl_LoadHandle libhandle;

	PTR_TYPE returnval = 0;
	void **parameters = NULL;
	
	static CONST char *options[] = {
		"getadr",  "__call", "__linkvar", "function",
		(char *) NULL
	};
	enum options {
		CINV_GETADR,
		CINV_CALLADR,
        CINV_LINKVAR,
        CINV_FUNCCREATE,		
	};
	
	static CONST char *tcl_linktypes[] = {
	    "-",
"TCL_LINK_INT",
"TCL_LINK_DOUBLE",
"TCL_LINK_BOOLEAN",
"TCL_LINK_STRING",
"TCL_LINK_WIDE_INT",
"TCL_LINK_CHAR",
"TCL_LINK_UCHAR",
"TCL_LINK_SHORT",
"TCL_LINK_USHORT",
"TCL_LINK_UINT",
"TCL_LINK_LONG",
"TCL_LINK_ULONG",
"TCL_LINK_FLOAT",
"TCL_LINK_WIDE_UINT",
        (char *) NULL
    };
	
	ts = (TclCInvokeState *) cdata;
	ctx = ts->ctx;
	libhandle = ts->tcllib;
	
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "subcommand arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
                &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch (index) {
        case CINV_GETADR:
            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "functionToFind");
                return TCL_ERROR;
            } else {
                symbol=Tcl_GetString(objv[2]);
                if (libhandle==NULL) {
                    Tcl_AppendResult (interp,"Error: No Library loaded",NULL);
                    return TCL_ERROR;
                }
                void* funcptr=Tcl_FindSymbol(interp, libhandle, symbol);
                Tcl_Obj *sym_addr = Ffidl_NewPointerObj(funcptr,"SYMBOL");
                Tcl_SetObjResult(interp, sym_addr);
                return TCL_OK; 
            }
        case CINV_LINKVAR:
            if (objc < 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "adrToLink tclname type");
                return TCL_ERROR;
            } else {
                int typeindex;
                symbol=Tcl_GetString(objv[2]);
                tclsym=Tcl_GetString(objv[3]);
                //tcltype=Tcl_GetString(objv[4]);
                void* funcptr=Tcl_FindSymbol(interp, libhandle, symbol);
                if(funcptr==0) {
                    Tcl_AppendResult(interp, "Symbol not found ",symbol,NULL);
                    return TCL_ERROR;
                }
                if (Tcl_GetIndexFromObj(interp, objv[4], tcl_linktypes, "tcl_linktypes", 0,
                            &typeindex) != TCL_OK) {
                    return TCL_ERROR;
                }
                Tcl_LinkVar(interp, tclsym, funcptr, typeindex);
                return TCL_OK; 
            }
        case CINV_CALLADR:
            if (objc < 5) {
                Tcl_WrongNumArgs(interp, 2, objv, "functionToCall retsig paramsig params");
                return TCL_ERROR;
            } else {
                retsig="\0";
                paramsig="";
                ret_type=-1;
                symbol=Tcl_GetString(objv[2]);
                retstring=Tcl_GetString(objv[3]);

                if(strlen(retstring)>0) {
                    if (get_type_from_str(interp,objv[3], &ret_type,NULL)!=TCL_OK) return TCL_ERROR;
                    if (ret_type<0) {
                        Tcl_AppendResult(interp, "Unknown type ",TclGetString(objv[3]),NULL);
                        return TCL_ERROR;
                    }
                    
                    if (ret_type>=_CINV_T_STRUCT) {
                        retsig="p";
                    } else {
                        retsig=isig_type(ret_type);
                    }
                }
                
                if (Tcl_ListObjGetElements(interp, objv[4], &paramlength, &paramlist)!=TCL_OK) {
                    return TCL_ERROR;
                }
                len=0;
                if (objc>5) {
                    // pars params and prepare
                    len = paramlength;
                    parameters = cinv_alloc(len * sizeof(void*),MEM_TCL,"CInvokeHandleCmd",__LINE__);
                    paramsig= cinv_alloc((len+1) * sizeof(char),MEM_TCL,"CInvokeHandleCmd",__LINE__);
                    paramtypes=cinv_alloc(len * sizeof(int),MEM_TCL,"CInvokeHandleCmd",__LINE__);
                    for (int i = 0; i < len; i++) {
                        xtype=NULL;
                        paramstring=Tcl_GetString(paramlist[i]);
                        if (get_type_from_str(interp,paramlist[i], &int_type,&xtype)!=TCL_OK) {
                            return TCL_ERROR;
                        }
                        //printf("got type as %d %p %p\n",int_type,&xtype,xtype);
                        //printf("%d %d size %d\n",xtype->cinv_basetype,xtype->cinv_inttype, xtype->size);

                        if (int_type<0) {
                            Tcl_AppendResult(interp, "Unknown type ",TclGetString(paramlist[i]),NULL);
                            return TCL_ERROR;
                        }
                        
                        if (int_type>=_CINV_T_STRUCT) {
                            is_PtrType=1;
                            cinv_type=_CINV_T_PTR;
                        } else {
                            is_PtrType=0;
                            cinv_type=int_type;
                        }
                        paramtypes[i]=int_type;
                        //printf("Paramstring %s %s\n",paramstring,Tcl_GetString(objv[5+i]));
                        // hack to transfer const char* and certain ptrs correctly
                        if(strcmp(paramstring,"string")==0) is_PtrType=0;
                        if(strcmp(paramstring,"char*")==0) is_PtrType=0;
                        if(strcmp(paramstring,"utfstring")==0) is_PtrType=0;
                        if(strcmp(paramstring,"tclproc")==0) is_PtrType=0;
                        if(strcmp(paramstring,"interp")==0) is_PtrType=0;
                        if(strcmp(paramstring,"bytevar")==0) is_PtrType=0;
                        if (int_type==_CINV_T_STRUCT) {
                            is_PtrType=0;
                        }

                        if(objc>5+i) {
                            val=cinv_alloc(get_size_from_type(cinv_type),MEM_TCL,"CInvokeHandleCmd",__LINE__);
                            int oklen;
                            if(get_value_from_obj(interp,int_type,xtype,objv[5+i],val,&oklen)!=TCL_OK) {
                                cinv_free(paramsig,MEM_TCL);
                                cinv_free(parameters,MEM_TCL);
                                cinv_free(paramtypes,MEM_TCL);
                                cinv_free(val,MEM_TCL);
                                type_release(xtype);
                                return TCL_ERROR;
                            }
                            paramsig[i]=isig_type(cinv_type)[0];
                            if(is_PtrType==0) {
                                parameters[i]=val;
                            } else {
                                void* pval=cinv_alloc(sizeof(void*),MEM_TCL,"CInvokeHandleCmd",__LINE__);
                                pval=&val;
                                parameters[i]=pval;
                                paramsig[i]='p';
                            }
                        }

                        paramsig[i+1]='\0';
                        type_release(xtype);
                    }
                }
                
                PTR_TYPE* symadr=0;
                void* funcptr=0;
                //printf("Getting symbol adr\n");
                if(Ffidl_GetPointerFromObj(interp,objv[2],&symadr,"SYMBOL")==TCL_OK) {
                    if(symadr>0) {
                        // this is an adress it seems
                        funcptr=(PTR_TYPE*)symadr;
                    }
                }
                if(funcptr==NULL) {
                    if (libhandle==NULL) {
                        Tcl_AppendResult (interp,"Error: No Library loaded",NULL);
                        return TCL_ERROR;
                    }
                    funcptr=Tcl_FindSymbol(interp, libhandle, symbol);
                }
                if(funcptr==NULL) {
                    Tcl_AppendResult(interp,"Function ",symbol," not found",NULL);
                    return TCL_ERROR;
                }
                Tcl_ResetResult(interp);
                CInvFunction *f = cinv_function_create(ctx, CINV_CC_DEFAULT, retsig, paramsig);
                //printf("Calling %s %s\n",retsig,paramsig);
                if (!cinv_function_invoke(ctx, f, funcptr, &returnval, parameters)) {
                    Tcl_AppendResult(interp,"invoke failed: \n", cinv_context_geterrormsg(ctx),NULL);
                    return TCL_ERROR;
                }
                //printf("ready %s %s\n",retsig,paramsig);
                if(ret_type>0) {
                    //printf("returntype is %s\n",Tcl_GetString(objv[3])); 
                    if (get_type_from_str(interp,objv[3], &ret_type,&rxtype)!=TCL_OK) return TCL_ERROR;
                    Tcl_Obj *obj=set_obj_from_value(interp, ret_type,rxtype, &returnval, 0);
                    if(obj!=NULL) Tcl_SetObjResult(interp,obj);
                    type_release(rxtype);
                }
                cinv_function_delete(ctx, f);
                for (int i = 0; i < len; i++) {
                    //
                    if(paramsig[i]=='p') {
                        int_type=paramtypes[i];
                        if((int_type!=_CINV_T_PTR_VAR)&&(int_type!=_CINV_T_PTR)&&(int_type!=_CINV_T_STRUCT)
                                &&(int_type!=_CINV_T_PTR_OBJ)&&(int_type!=_CINV_T_PTR_PROC)&&(int_type!=_CINV_T_INTERP)) {
                            if(*(char**)parameters[i]!=NULL) {
                                cinv_free(*(char**)parameters[i],MEM_TCL);
                            }
                        }
                        cinv_free(parameters[i],MEM_TCL);
                    } else {
                        cinv_free(parameters[i],MEM_TCL);
                    }
                }
                if(len>0) {
                    cinv_free(paramsig,MEM_TCL);
                    cinv_free(parameters,MEM_TCL);
                    cinv_free(paramtypes,MEM_TCL);
                }
                //printf("call succeeded\n");
                return TCL_OK; 
            }  
        case CINV_FUNCCREATE:
            if (objc < 6) {
                Tcl_WrongNumArgs(interp, 2, objv, "functionToCall tclfuncname retsig paramsig");
                return TCL_ERROR;
            } else {
                symbol=Tcl_GetString(objv[2]);
                funcname=Tcl_GetString(objv[3]);
                retstring=Tcl_GetString(objv[4]);
                ret_type=-1;
                
                if (Tcl_ListObjGetElements(interp, objv[5], &paramlength, &paramlist)!=TCL_OK) {
                    return TCL_ERROR;
                }
                
                if(strlen(retstring)>0) {
                    if (get_type_from_str(interp,objv[4], &ret_type,NULL)!=TCL_OK) return TCL_ERROR;
                    if (ret_type<0) {
                        Tcl_AppendResult(interp, "Unknown type ",TclGetString(objv[4]),NULL);
                        return TCL_ERROR;
                    }
                }
                
                PTR_TYPE* symadr=0;
                void* funcptr=0;
                if(Ffidl_GetPointerFromObj(interp,objv[2],&symadr,"SYMBOL")==TCL_OK) {
                    if(symadr>0) {
                        // this is an adress it seems
                        funcptr=(PTR_TYPE*)symadr;
                    }
                }
                if(funcptr==NULL) {
                    if (libhandle==NULL) {
                        Tcl_AppendResult (interp,"Error: No Library loaded",NULL);
                        return TCL_ERROR;
                    }
                    funcptr=Tcl_FindSymbol(interp, libhandle, symbol);
                }
                if(funcptr==NULL) {
                    Tcl_AppendResult(interp,"Function ",symbol," not found",NULL);
                    return TCL_ERROR;
                }
                Tcl_ResetResult(interp);
                TclCCalloutState* tsc=cinv_alloc(sizeof(TclCCalloutState),MEM_TCL,"CINV_FUNCCREATE",__LINE__);
                tsc->interp=interp;
                tsc->ctx=ctx;
                tsc->cfunc=NULL;
                tsc->callingconvention=ts->callingconvention; //--> use CINV_CC_DEFAULT
                tsc->entrypoint=funcptr;
                tsc->paramlength=paramlength;
                tsc->paramlist=Tcl_DuplicateObj(objv[5]);
                Tcl_IncrRefCount(tsc->paramlist);
                tsc->returnstring=tcl_strdup(retstring);
                tsc->return_type=ret_type;
                Tcl_CreateObjCommandNS(interp,funcname,CCalloutCallout,tsc,CCalloutCCommandDeleteProc);
                
                //printf("call succeeded\n");
                return TCL_OK; 
            }              
        default:
            Tcl_AppendResult(interp,"internal error during option lookup",NULL);
            return TCL_ERROR;
    }
    return TCL_OK;
} 
static int CInvokeCreateCmd( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	TclCInvokeState *ts;
	CInvContext *ctx;
	//CInvLibrary *lib;
    Tcl_LoadHandle libhandle;

	if (objc < 2 || objc > 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "libraryname handle [callingconvention]");
		return TCL_ERROR;
	}

	ctx = cinv_context_create();
	if (ctx == NULL) {
		return(TCL_ERROR);
	}
	
	ts = (void *) cinv_alloc(sizeof(*ts),MEM_TCL,"CInvokeCreateCmd",__LINE__);
	//printf("created ts %p\n",ts);
	ts->ctx = ctx;
	ts->tcllib = NULL;
	ts->lib=NULL;
	ts->callingconvention=CINV_CC_DEFAULT;
	if(strlen(Tcl_GetString(objv[1]))>0) {
	    Tcl_LoadFile(interp, objv[1], NULL, 0, NULL, &libhandle);
        ts->tcllib = libhandle;
        ts->interp=interp;
    }
    
	if (objc== 4) {
	    int index;
        static CONST char *options[] = {
            "CINV_CC_CDECL",  "CINV_CC_STDCALL", "CINV_CC_FASTCALL", 
            (char *) NULL
        };
        if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0,
            &index) != TCL_OK) {
            return TCL_ERROR;
            }
        ts->callingconvention=index;
    }


	Tcl_CreateObjCommandNS(interp,Tcl_GetString(objv[objc-1]),CInvokeHandleCmd,ts,CInvokeCCommandDeleteProc);

	return TCL_OK;
}

static int DeleteStruct(Tcl_Interp *interp, TclCStructState *ts, Tcl_Obj** deflist, int llength, CInvStructure *tstruct);
static int DeleteUnion(Tcl_Interp *interp, TclCStructState *ts, Tcl_Obj** deflist, int llength, CInvStructure *outstruct) {
    //
	CInvContext *ctx=ts->ctx;
	CInvStructure *tstruct;
	char *elem;

    Tcl_Obj **sublist;
    int sublength;
	
	tstruct= outstruct;
	for (int i=0;i<llength;i+=2) {
        elem=Tcl_GetString(deflist[i+1]);
        int itype=-1;
        if(strcmp(Tcl_GetString(deflist[i]),"struct")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(DeleteStruct(interp, ts,sublist,sublength, tstruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error deleting struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            continue;
        }

        if(strcmp(Tcl_GetString(deflist[i]),"union")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(DeleteUnion(interp, ts,sublist,sublength, tstruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error deleting struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            continue;
        }
        
        if (get_type_from_str(interp,deflist[i], &itype,NULL)!=TCL_OK) return TCL_ERROR;
        if (itype<0) {
            Tcl_AppendResult(interp, "Unknown type ",TclGetString(deflist[i]),NULL);
            return TCL_ERROR;
        }
        int isPtr=0;
        if(elem[0]=='*') {
            isPtr=1;
            removeChar(elem, '*');
        }
        if (isPtr>0) {
            itype=_CINV_T_PTR;
        }
        char* realname=sys_strdup(elem);
        int arraysize=parseArray(realname);
        if (arraysize>0) {
            //delete array
            for(int i=0;i<arraysize;i++) {
                void* valo=cinv_structure_instance_getarrayvalue(ctx, tstruct, ts->instance, realname, i);
                // need we to free something here?
                if(valo==NULL) continue;
                if((itype==_CINV_T_PTR_BYTE)||(itype==_CINV_T_PTR_UTF8)
                    ||(itype==_CINV_T_PTR_UTF16)) {
                    cinv_free(*(char**)valo,MEM_TCL);
                }
                if(itype==_CINV_T_CHARSTR) {
                    cinv_free(*(char**)valo,MEM_SYSTEM);
                }
            }
            cinv_structure_instance_zeroarrayvalue(ctx, tstruct, ts->instance, realname, i);
        } else {
            //delete value
            void* valo=cinv_structure_instance_getvalue(ctx, tstruct, ts->instance, realname);
            // need we to free something here?
            if(valo==NULL) continue;
            if((itype==_CINV_T_PTR_BYTE)||(itype==_CINV_T_PTR_UTF8)
                ||(itype==_CINV_T_PTR_UTF16)) {
                cinv_free(*(char**)valo,MEM_TCL);
            }
            if(itype==_CINV_T_CHARSTR) {
                cinv_free(*(char**)valo,MEM_SYSTEM);
            }
            cinv_structure_instance_zerovalue(ctx, tstruct, ts->instance, realname);
        }
        cinv_free(realname,MEM_SYSTEM);
	}
	return TCL_OK;
}
static int DeleteStruct(Tcl_Interp *interp, TclCStructState *ts, Tcl_Obj** deflist, int llength, CInvStructure *tstruct) {
    //
	CInvContext *ctx=ts->ctx;
	char *elem;
    Tcl_Obj **sublist;
    int sublength;
	for (int i=0;i<llength;i+=2) {
        elem=Tcl_GetString(deflist[i+1]);
        int itype=-1;
        
        if(strcmp(Tcl_GetString(deflist[i]),"struct")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(DeleteStruct(interp, ts,sublist,sublength, tstruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error deleting struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            continue;
        }

        if(strcmp(Tcl_GetString(deflist[i]),"union")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(DeleteUnion(interp, ts,sublist,sublength, tstruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error creating struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            continue;
        }
        
        if (get_type_from_str(interp,deflist[i], &itype,NULL)!=TCL_OK) return TCL_ERROR;
        //printf("Deleting struct %s %s\n",elem,Tcl_GetString(deflist[i]));
        if (itype<0) {
            Tcl_AppendResult(interp, "Unknown type ",TclGetString(deflist[i]),NULL);
            return TCL_ERROR;
        }
        int isPtr=0;
        if(elem[0]=='*') {
            isPtr=1;
            removeChar(elem, '*');
        }
        if (isPtr>0) {
            itype=_CINV_T_PTR;
        }
        char* realname=sys_strdup(elem);
        int arraysize=parseArray(realname);
        if (arraysize>0) {
            //delete array
            for(int i=0;i<arraysize;i++) {
                void* valo=cinv_structure_instance_getarrayvalue(ctx, tstruct, ts->instance, realname, i);
                // need we to free something here?
                if(valo==NULL) continue;
                if((itype==_CINV_T_PTR_BYTE)||(itype==_CINV_T_PTR_UTF8)
                    ||(itype==_CINV_T_PTR_UTF16)) {
                    cinv_free(*(char**)valo,MEM_TCL);
                }
                if(itype==_CINV_T_CHARSTR) {
                    cinv_free(*(char**)valo,MEM_SYSTEM);
                }
            }
        } else {
            //delete value
            void* valo=cinv_structure_instance_getvalue(ctx, tstruct, ts->instance, realname);
            // need we to free something here?
            if(valo==NULL) continue;
            if((itype==_CINV_T_PTR_BYTE)||(itype==_CINV_T_PTR_UTF8)
                ||(itype==_CINV_T_PTR_UTF16)) {
                cinv_free(*(char**)valo,MEM_TCL);
            }
            if(itype==_CINV_T_CHARSTR) {
                cinv_free(*(char**)valo,MEM_SYSTEM);
            }
        }
        cinv_free(realname,MEM_SYSTEM);
	}
	return TCL_OK;
	
}
static int CreateStruct(Tcl_Interp *interp, TclCStructState *ts, Tcl_Obj** deflist, int llength, CInvStructure **outstruct);
static int CreateUnion(Tcl_Interp *interp, TclCStructState *ts, Tcl_Obj** deflist, int llength, CInvStructure *outstruct) {
    //
	CInvContext *ctx=ts->ctx;
	CInvStructure *tstruct;
	CInv_Type* xtype;
	char *elem;
	char substrname[128];

    CInvStructure *substruct;
    Tcl_Obj **sublist;
    int sublength;
	
	tstruct= outstruct;
	
	for (int i=0;i<llength;i+=2) {
        elem=Tcl_GetString(deflist[i+1]);
        int itype=-1;
        if(strcmp(Tcl_GetString(deflist[i]),"struct")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(CreateStruct(interp, ts,sublist,sublength, &substruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error creating struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            // create name for substuct
            sprintf(substrname,"struct%d",i);
            tstruct->nextoffset=tstruct->lastoffset;
            if(cinv_structure_insert_union_struct(ctx, tstruct, substrname, substruct)==CINV_ERROR) {
                Tcl_AppendResult(interp,"Error creating substruct ",elem,"\n",cinv_context_geterrormsg(ctx),NULL);
                return TCL_ERROR;
            }
            continue;
        }

        if(strcmp(Tcl_GetString(deflist[i]),"union")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(CreateUnion(interp, ts,sublist,sublength, tstruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error creating struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            continue;
        }
        
        if (get_type_from_str(interp,deflist[i], &itype,&xtype)!=TCL_OK) return TCL_ERROR;
        if (itype<0) {
            Tcl_AppendResult(interp, "Unknown type ",TclGetString(deflist[i]),NULL);
            return TCL_ERROR;
        }
        if (itype>=_CINV_T_STRUCT) {
            itype=_CINV_T_PTR;
        }
        int isPtr=0;
        if(elem[0]=='*') {
            isPtr=1;
            removeChar(elem, '*');
        }
        if (isPtr>0) {
            itype=_CINV_T_PTR;
        }
        char* realname=sys_strdup(elem);
        int arraysize=parseArray(realname);
        if (arraysize>0) {
            Tcl_AppendResult(interp,"Error creating union: Array not supported in ",elem,NULL);
            return TCL_ERROR;
        }
        type_release(xtype);
        cinv_free(realname,MEM_SYSTEM);    
        
        if(cinv_structure_addmember_union(ctx, tstruct, elem, itype)==CINV_ERROR) {
            Tcl_AppendResult(interp,"Error creating struct ",cinv_context_geterrormsg(ctx),elem,NULL);
            return TCL_ERROR;
        }
	}
	return TCL_OK;
}
static int CreateStruct(Tcl_Interp *interp, TclCStructState *ts, Tcl_Obj** deflist, int llength, CInvStructure **outstruct) {
    //
	CInvContext *ctx=ts->ctx;
	CInvStructure *tstruct= cinv_structure_create(ctx);
	char *elem;
	char substrname[128];
    CInvStructure *substruct;
    Tcl_Obj **sublist;
    int sublength;
	
	for (int i=0;i<llength;i+=2) {
        elem=Tcl_GetString(deflist[i+1]);
        int itype=-1;
        
        if(strcmp(Tcl_GetString(deflist[i]),"struct")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(CreateStruct(interp, ts,sublist,sublength, &substruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error creating struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            // create name for substuct
            sprintf(substrname,"struct%d",i);
            if(cinv_structure_insert_struct(ctx, tstruct, substrname, substruct)==CINV_ERROR) {
                Tcl_AppendResult(interp,"Error creating substruct ",elem,"\n",cinv_context_geterrormsg(ctx),NULL);
                return TCL_ERROR;
            }
            continue;
        }

        if(strcmp(Tcl_GetString(deflist[i]),"union")==0) {
            if (Tcl_ListObjGetElements(interp, deflist[i+1], &sublength, &sublist)!=TCL_OK) {
                return TCL_ERROR;
            }
            if(CreateUnion(interp, ts,sublist,sublength, tstruct)!=TCL_OK) {
                Tcl_AppendResult(interp,"Error creating struct from ",Tcl_GetString(deflist[i+1]),NULL);
                return TCL_ERROR;
            }
            continue;
        }
        
        if (get_type_from_str(interp,deflist[i], &itype,NULL)!=TCL_OK) return TCL_ERROR;
        if (itype<0) {
            Tcl_AppendResult(interp, "Unknown type ",TclGetString(deflist[i]),NULL);
            return TCL_ERROR;
        }
        if (itype>=_CINV_T_STRUCT) {
            itype=_CINV_T_PTR;
        }
        int isPtr=0;
        if(elem[0]=='*') {
            isPtr=1;
            removeChar(elem, '*');
        }
        if (isPtr>0) {
            itype=_CINV_T_PTR;
        }
        char* realname=sys_strdup(elem);
        int arraysize=parseArray(realname);
        if (arraysize>0) {
            if(cinv_structure_addmember_array(ctx, tstruct, realname, itype,arraysize)==CINV_ERROR) {
                Tcl_AppendResult(interp,"Error creating struct ",cinv_context_geterrormsg(ctx),realname,NULL);
                return TCL_ERROR;
            }                
        } else {
            if(cinv_structure_addmember_value(ctx, tstruct, elem, itype)==CINV_ERROR) {
                Tcl_AppendResult(interp,"Error creating struct ",cinv_context_geterrormsg(ctx),realname,NULL);
                return TCL_ERROR;
            }                
        }
        cinv_free(realname,MEM_SYSTEM);
	}
	/*printf("tstruct %d\n",sizeof(CInvStructure));
	printf("hasht %d\n",sizeof(struct hashtable));
	printf("member %d\n",sizeof(CInvStructMember));
	printf("nmembers %d\n",hashtable_count(tstruct->members));*/
	
	cinv_structure_finish(ctx, tstruct);
	*outstruct=tstruct;
	return TCL_OK;
	
}

static void CStructCCommandDeleteProc(ClientData cdata) {
	TclCStructState *ts;
	CInvContext *ctx ;
    CInvStructure *tstruct;
    void* instance;
    //char buf[256];
    Tcl_Obj** deflist;
    int llength;
    
	ts = (TclCStructState *) cdata;
	ctx = ts->ctx;
	tstruct=ts->cstruct;
	instance=ts->instance;
	Tcl_Interp* interp=ts->interp;
	//printf("Deleting %p %p\n",tstruct,instance);
    if (Tcl_ListObjGetElements(interp, ts->deflist, &llength, &deflist)==TCL_OK) {
        if(DeleteStruct(interp, ts,deflist,llength, tstruct)!=TCL_OK) {
            // error, but we wont handle it
            Tcl_Panic("EERRR killing struct");
        }
    }
    if(Tcl_IsShared(ts->deflist)) Tcl_DecrRefCount(ts->deflist);
	Tcl_DecrRefCount(ts->deflist);
	 
    if(ts->allocsystem>0) {
        if (instance!=NULL) {
            cinv_structure_delete_instance(ctx,instance);
        }
    }

    if (tstruct!=NULL) cinv_structure_delete(ctx,tstruct);
	
	if (!cinv_context_delete(ctx)) {
        return;
	}
	if(ts->typename!=NULL) {
	    //printf("deleting %s \n",ts->typename);
	    cinv_free(ts->typename,MEM_TCL);
	}
	cinv_free(ts,MEM_TCL);
    return;

}
static int CStructHandleCmd( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]) {
    CInv_Type* xtype;
    void* val;

	Tcl_Obj* obj;
	int index,len;
	char buf[256];
	
    Tcl_Obj **deflist;
    int llength;
	
	
	TclCStructState *ts;
	CInvContext *ctx ;
	CInvStructure *tstruct;
	void* instance;
	PTR_TYPE *newinstance;
	
	static CONST char *options[] = {
		"set",
		"get",
		"setptr",
		"getptr",
		"info",
		"struct",
		(char *) NULL
	};
	enum options {
		CSTRUCT_SETVALUE,
		CSTRUCT_GETVALUE,
		CSTRUCT_SETPTR,
		CSTRUCT_GETPTR,
		CSTRUCT_GETINFO,
		CSTRUCT_GETDEF,
	};
	
	ts = (TclCStructState *) cdata;
	if(ts==NULL) return TCL_ERROR;
	ctx = ts->ctx;
	tstruct=ts->cstruct;
	instance=ts->instance;
	int allocold= ts->allocsystem;
	
	if(ctx==NULL) return TCL_ERROR;
	if(instance==NULL) return TCL_ERROR;
	if(tstruct==NULL) return TCL_ERROR;
	
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "subcommand arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
                &index) != TCL_OK) {
        return TCL_ERROR;
    }
    char* structname=ts->typename;
    switch (index) {
        case CSTRUCT_SETVALUE:
            if (objc < 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "key [idx] value");
                return TCL_ERROR;
            } else {
                int itype,iarray;
                char *name;
                Tcl_Obj* key, *value, *idx;
                
                if (objc==4) {
                    key=objv[2];
                    idx=NULL;
                    value=objv[3];
                } else {
                    key=objv[2];
                    idx=objv[3];
                    value=objv[4];
                }
                
                if (get_type_from_name(interp,ts,key,&xtype,&iarray)!=TCL_OK) {
                    Tcl_AppendResult(interp, "Struct-Type not found ",Tcl_GetString(key),NULL);
                    return TCL_ERROR;
                }
                
                //printf("setting value to %s %p\n","",&xtype);
                itype=xtype->cinv_inttype;
                val=cinv_alloc(get_size_from_type(itype),MEM_TCL,"CStructHandleCmd",__LINE__);
                name=Tcl_GetString(objv[2]);
                
                int setbulk=0;
                if(idx==NULL&&iarray>0) {
                    // try getting a list of values 
                    if (Tcl_ListObjGetElements(interp, value, &llength, &deflist)!=TCL_OK) {
                        return TCL_ERROR;
                    }
                    if(llength>1) {
                        setbulk=1;
                    }
                    
                } 
                if(setbulk) {
                    // try getting a list of values and set it at once
                    for(int i=0;i<min(iarray,llength);i++) {
                        void* valo=cinv_structure_instance_getarrayvalue(ctx, tstruct, instance, name, i);
                        // need we to free something here?
                        if((itype==_CINV_T_PTR_BYTE)||(itype==_CINV_T_PTR_UTF8)
                            ||(itype==_CINV_T_PTR_UTF16)) {
                            cinv_free(*(char**)valo,MEM_TCL);
                        }
                        if(itype==_CINV_T_CHARSTR) {
                            cinv_free(*(char**)valo,MEM_SYSTEM);
                        }

                        if(get_value_from_obj(interp,itype,xtype,deflist[i],val,&len)!=TCL_OK) return TCL_ERROR;
                        
                        if(cinv_structure_instance_setarrayvalue(ctx, tstruct, instance, name, i, val)==CINV_ERROR) {
                            Tcl_AppendResult(interp, "Array set error: ",cinv_context_geterrormsg(ctx)," ",name,NULL);
                            return TCL_ERROR;
                        }
                    }
                    //
                } else {
                    //
                    if(get_value_from_obj(interp,itype,xtype,value,val,&len)!=TCL_OK) return TCL_ERROR;
                    if(idx==NULL) {
                        cinv_structure_instance_setvalue(ctx, tstruct, instance, name, val);
                    } else {
                        int index;
                        Tcl_GetIntFromObj(interp,idx,&index);
                        if(cinv_structure_instance_setarrayvalue(ctx, tstruct, instance, name, index, val)==CINV_ERROR) {
                            Tcl_AppendResult(interp, "Array set error: ",cinv_context_geterrormsg(ctx)," ",name,NULL);
                            return TCL_ERROR;
                        }
                    }
                }
                type_release(xtype);
                cinv_free(val,MEM_TCL);
                return TCL_OK; 
            }
        case CSTRUCT_GETVALUE:
            if (objc < 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "key [idx]");
                return TCL_ERROR;
            } else {
                int itype,iarray;
                char* name;
                void* val;
                Tcl_Obj* key, *idx;
                
                if (objc==3) {
                    key=objv[2];
                    idx=NULL;
                } else {
                    key=objv[2];
                    idx=objv[3];
                }
                
                if (get_type_from_name(interp,ts,key, &xtype,&iarray)!=TCL_OK) {
                    Tcl_AppendResult(interp, "Element not found ",Tcl_GetString(objv[2]),NULL);
                    return TCL_ERROR;
                }
                itype=xtype->cinv_inttype;
                int size=get_size_from_type(itype);
                name=Tcl_GetString(key);                
                int setbulk=0;
                if(idx==NULL&&iarray>0) {
                    // try getting a list of values 
                    setbulk=1;
                    
                } 
                if(setbulk>0) {
                    // try getting a list of values and set it at once
                    obj= Tcl_NewListObj(iarray,NULL);
                    
                    for(int i=0;i<iarray;i++) {
                        val=cinv_structure_instance_getarrayvalue(ctx, tstruct, instance, name, i);
                        if(val==NULL) {
                            Tcl_AppendResult(interp, "Array get error: ",cinv_context_geterrormsg(ctx),NULL);
                            return TCL_ERROR;
                        }
                        Tcl_Obj* tmp=set_obj_from_value(interp,itype,xtype,val, size);
                        if(tmp==NULL) return TCL_ERROR;
                        if(Tcl_ListObjAppendElement(interp, obj, tmp)) {
                            Tcl_AppendResult(interp, "Array get error... invalid list",NULL);
                            return TCL_ERROR;
                            //
                        }
                        
                    }
                } else {
                    if(idx==NULL) {
                        val=cinv_structure_instance_getvalue(ctx, tstruct, instance, name);
                    } else {
                        int index;
                        Tcl_GetIntFromObj(interp,idx,&index);
                        val=cinv_structure_instance_getarrayvalue(ctx, tstruct, instance, name, index);
                        if(val==NULL) {
                            Tcl_AppendResult(interp, "Array get error: ",cinv_context_geterrormsg(ctx),NULL);
                            return TCL_ERROR;
                        }
    
                    }
                    obj=set_obj_from_value(interp,itype,xtype,val, size);
                }
                if(obj==NULL) return TCL_ERROR;
                Tcl_SetObjResult(interp, obj);
                type_release(xtype);
                return TCL_OK; 
            }
        case CSTRUCT_GETPTR: 
            obj=Ffidl_NewPointerObj(instance,structname);
            Tcl_SetObjResult(interp, obj);
            return TCL_OK;
        case CSTRUCT_SETPTR: 
            if (objc<3 || objc > 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "ptr subtype");
                return TCL_ERROR;
            } else {
                if(objc > 3) {
                    char* subtype=Tcl_GetString(objv[3]);
                    if(strcmp(subtype,"mem_system")==0) {
                        ts->allocsystem=MEM_SYSTEM;
                    }
                    if(strcmp(subtype,"mem_tcl")==0) {
                        ts->allocsystem=MEM_TCL;
                    }
                    if(strcmp(subtype,"mem_none")==0) {
                        ts->allocsystem=-1;
                    }
                }
                Tcl_AppendResult(interp,"getting ptr from ",Tcl_GetString(objv[2]),NULL);
                if(Ffidl_GetPointerFromObj(interp,objv[2],&newinstance,structname)!=TCL_OK) return TCL_ERROR;
                if(newinstance==NULL) {
                    Tcl_AppendResult(interp, "Couldn't load instance from ptr, PTR is NULL",NULL);
                    return TCL_ERROR;
                }
                if ((instance!=NULL)&&(instance!=newinstance)) {
                    if(allocold>0) {
                        cinv_structure_delete_instance(ctx,instance);
                    }
                    if(ts->allocsystem==MEM_TCL) {
                        ts->instance=cinv_structure_create_instance(ctx, ts->cstruct);
                        memcpy(ts->instance, newinstance,ts->binsize);
                    } else {
                        ts->instance=newinstance;
                    }
                }
            }
            return TCL_OK;
        case CSTRUCT_GETINFO:
            sprintf(buf,"Elements: %d\nBinary Size: %d\n",ts->deflen/2,ts->binsize);
            Tcl_AppendResult(interp,buf,Tcl_GetString(ts->deflist),NULL);
            return TCL_OK;
        case CSTRUCT_GETDEF:
            Tcl_AppendResult(interp,Tcl_GetString(ts->deflist),NULL);
            return TCL_OK;
        default:
            Tcl_AppendResult(interp,"internal error during option lookup",NULL);
            return TCL_ERROR;
    }
    return TCL_OK;
} 
static int CStructCreateCmd( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	TclCStructState *ts;
	CInvContext *ctx;
	CInvStructure *tstruct;
	//char buf[256];
	if (objc < 3 || objc > 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "structname structtype definition");
		return TCL_ERROR;
	}

	ctx = cinv_context_create();
	
	if (ctx == NULL) {
		return(TCL_ERROR);
	}

	ts = cinv_alloc(sizeof(*ts),MEM_TCL,"CStructCreateCmd",__LINE__);
	ts->ctx = ctx;
	ts->interp=interp;
	
	Tcl_Obj **deflist;
	int llength;
	

	if (Tcl_ListObjGetElements(interp, objv[3], &llength, &deflist)!=TCL_OK) {
	    return TCL_ERROR;
	}
	ts->typename=tcl_strdup(Tcl_GetString(objv[2]));
	ts->deflist=Tcl_DuplicateObj(objv[3]);
	ts->deflen=llength;
	ts->cstruct =NULL;
	ts->instance=NULL;
	ts->binsize=0;
	
	if(CreateStruct(interp, ts,deflist,llength, &tstruct)!=TCL_OK) {
	    Tcl_AppendResult(interp,"Error creating struct from ",Tcl_GetString(objv[2]),NULL);
	    return TCL_ERROR;
	}
	
    ts->cstruct = tstruct;
	ts->instance=cinv_structure_create_instance(ctx, tstruct);
	cinv_structure_getsize(ctx, tstruct,&ts->binsize);
    ts->allocsystem=MEM_SYSTEM;
	
	// init to zero
	int len=ts->binsize; char* dest=(void*)ts->instance;
	while (len) { 
	    *(dest) = '\0'; 
	    (dest)++; 
	    (len)--; 
	}   
	
	Tcl_IncrRefCount(ts->deflist);
	Tcl_CreateObjCommandNS(interp,Tcl_GetString(objv[1]),CStructHandleCmd,ts,CStructCCommandDeleteProc);
	return TCL_OK;
}

static int CStructDeclareCmd( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	TclCStructDef *ts;
	//char buf[256];
	if (objc < 2 || objc > 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "structname definition");
		return TCL_ERROR;
	}
	
	ts = (void *) cinv_alloc(sizeof(*ts),MEM_TCL,"CStructDeclareCmd",__LINE__);
	//printf("Declaring struct %s\n",Tcl_GetString(objv[1]));
	ts->structname=objv[1];
	ts->definition=objv[2];
	Tcl_IncrRefCount(ts->structname);
    Tcl_IncrRefCount(ts->definition);
    Tcl_HashEntry *entry = Tcl_FindHashEntry(&cinvclient->structs,Tcl_GetString(objv[1]));
    if(entry!=NULL) {
      TclCStructDef* tso=Tcl_GetHashValue(entry);
      Tcl_DecrRefCount(tso->structname);
      Tcl_DecrRefCount(tso->definition);
      cinv_free(tso,MEM_TCL);
      Tcl_DeleteHashEntry(entry);
    }      
    
	entry_define(&cinvclient->structs,Tcl_GetString(objv[1]),ts);
	//typedef here
	return TCL_OK;
}

static void CTypeCCommandDeleteProc(ClientData cdata) {
	TclCTypeState *ts;
	CInvContext *ctx ;
    void* val;
    int arrlen;
    //char buf[256];
    
	ts = (TclCTypeState *) cdata;
	ctx = ts->ctx;
	val=ts->value;
	arrlen=1;
	if(ts->isarray) arrlen=ts->deflen;
	if((arrlen>1)) {
	    // delete array data
	    // careful, might be dragons here
        for(int i=0;i<arrlen;i++) {
            PTR_TYPE* valptr=(PTR_TYPE*)((PTR_TYPE)ts->value+(i*get_size_from_type(ts->cinv_type)));
            // need we to free something here?
            if((ts->int_type==_CINV_T_PTR_BYTE)||(ts->int_type==_CINV_T_PTR_UTF8)
                ||(ts->int_type==_CINV_T_PTR_UTF16)) {
                cinv_free((void*)*valptr,ts->allocsystem);
            }
            if(ts->int_type==_CINV_T_CHARSTR) {
                cinv_free(*(char**)valptr,MEM_SYSTEM);
            }
        }
	} else {
        if((ts->int_type==_CINV_T_PTR_BYTE)||(ts->int_type==_CINV_T_PTR_UTF8)
            ||(ts->int_type==_CINV_T_PTR_UTF16)) {
            cinv_free(*(char**)val,ts->allocsystem);
        }
        if(ts->int_type==_CINV_T_CHARSTR) {
            cinv_free(*(char**)val,MEM_SYSTEM);
        }
    }

    if (val!=NULL) cinv_free(val,ts->allocsystem);
	type_release(ts->xtype);
	cinv_free(ts,MEM_TCL);
	
	if (!cinv_context_delete(ctx)) {
        return;
	}
	
    return;

}
static int CTypeHandleCmd ( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	void *dest;
	Tcl_Obj* obj;
	int index;
	int len, simpleptr,arrlen;
	char buf[300];
	
	TclCTypeState *ts;
	CInvContext *ctx ;
	
	static CONST char *options[] = {
	    "length",
		"set",
		"get",
		"setptr",
		"getptr",
		"getptr*",
		(char *) NULL
	};
	enum options {
	    CTYPE_SETLENGTH,
		CTYPE_SETVALUE,
		CTYPE_GETVALUE,
		CTYPE_SETPTR,
		CTYPE_GETPTR,
		CTYPE_GETDPTR,
	};
	
	ts = (TclCTypeState *) cdata;
	if(ts==NULL) return TCL_ERROR;
	ctx = ts->ctx;
	
	if(ctx==NULL) return TCL_ERROR;
	
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "subcommand arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
                &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch (index) {
        case CTYPE_SETLENGTH:
            // set new array length, so any type can be used as an array
            // 
            if (objc < 3) {
                Tcl_SetObjResult(interp, Tcl_NewIntObj(ts->deflen));
                return TCL_OK;
            } else {
                int newlen=0;
                if(Tcl_GetIntFromObj(interp,objv[2],&newlen)!=TCL_OK) return TCL_ERROR;
                if(newlen>1) {
                    ts->deflen=newlen;
                    ts->isarray=1;
                }
                return TCL_OK;
            }
        case CTYPE_SETVALUE:
            if (objc < 3) {
                Tcl_WrongNumArgs(interp, 2, objv, " value");
                return TCL_ERROR;
            } else {
                arrlen=1;
                if(ts->isarray) arrlen=ts->deflen;
                if(arrlen<=1) {
                    //singlevalue
                        if((ts->int_type==_CINV_T_PTR_BYTE)||(ts->int_type==_CINV_T_PTR_UTF8)
                            ||(ts->int_type==_CINV_T_PTR_UTF16)) {
                            cinv_free(*(char**)ts->value,MEM_TCL);
                        }
                        if(ts->int_type==_CINV_T_CHARSTR) {
                            cinv_free(*(char**)ts->value,MEM_SYSTEM);
                        }
                        if(get_value_from_obj(interp,ts->int_type,ts->xtype,objv[2],ts->value,&len)!=TCL_OK) return TCL_ERROR;
                } else {
                    //init array
                    Tcl_Obj** deflist;
                    int llength;
                    PTR_TYPE* valptr;
                    int index=0;
                    if(objc==4) {
                        // got a start index
                        if(Tcl_GetIntFromObj(interp,objv[2],&index)!=TCL_OK) return TCL_ERROR;
                    }
                        
                    if (Tcl_ListObjGetElements(interp, objv[objc-1], &llength, &deflist)!=TCL_OK) {
                        return TCL_ERROR;
                    }
                    for(int i=index;i<min(arrlen,llength+index);i++) {
                        valptr=(PTR_TYPE*)((PTR_TYPE)ts->value+(i*get_size_from_type(ts->cinv_type)));
                        // need we to free something here?
                        /*if((ts->int_type==_CINV_T_PTR_BYTE)||(ts->int_type==_CINV_T_PTR_UTF8)
                            ||(ts->int_type==_CINV_T_PTR_UTF16)) {
                            cinv_free(*(char**)valptr,MEM_TCL);
                        }
                        if(ts->int_type==_CINV_T_CHARSTR) {
                            cinv_free(*(char**)valptr,MEM_SYSTEM);
                        }*/
                        if(get_value_from_obj(interp,ts->int_type,ts->xtype,deflist[i-index],valptr,&len)!=TCL_OK) return TCL_ERROR;
                    }
                }
                return TCL_OK; 
            }
        case CTYPE_GETVALUE:
            if (objc <2) {
                Tcl_WrongNumArgs(interp, 1, objv, "");
                return TCL_ERROR;
            } else {
               arrlen=1;
               if(ts->isarray) arrlen=ts->deflen;
                if(arrlen<=1) {
                    //singlevalue
                    obj=set_obj_from_value(interp,ts->int_type,ts->xtype,ts->value, ts->binsize);
                    if(obj==NULL) return TCL_ERROR;
                    Tcl_SetObjResult(interp, obj);
                    return TCL_OK; 
                } else {
                    //init array
                    PTR_TYPE* valptr;
                    int index=0;
                    if(objc>=3) {
                        // got a start index
                        if(Tcl_GetIntFromObj(interp,objv[2],&index)!=TCL_OK) return TCL_ERROR;
                        if(index>=arrlen) return TCL_ERROR;//outofbounds
                        valptr=(PTR_TYPE*)((PTR_TYPE)ts->value+(index*get_size_from_type(ts->cinv_type)));
                        obj=set_obj_from_value(interp,ts->int_type,ts->xtype,valptr, ts->binsize);
                        
                        if(obj==NULL) {
                            sprintf(buf,"int_type: %d xtype: %d",ts->int_type,ts->xtype->cinv_inttype);
                            Tcl_AppendResult(interp, "Type mismatch from array ",buf,NULL);
                            return TCL_ERROR;
                        }
                            
                        Tcl_SetObjResult(interp, obj);
                        return TCL_OK; 
                        
                    } else {
                        obj= Tcl_NewListObj(arrlen,NULL);
                        index=0;
                        
                        for(int i=0;i<arrlen;i++) {
                            valptr=(PTR_TYPE*)((PTR_TYPE)ts->value+(i*get_size_from_type(ts->cinv_type)));
                            // need we to free something here?
                            Tcl_Obj* tmp=set_obj_from_value(interp,ts->int_type,ts->xtype,valptr,get_size_from_type(ts->cinv_type));
                            if(tmp==NULL) {
                                Tcl_AppendResult(interp, "Type mismatch from array",NULL);
                                return TCL_ERROR;
                            }
                            if(Tcl_ListObjAppendElement(interp, obj, tmp)) {
                                Tcl_AppendResult(interp, "Array get error... invalid list",NULL);
                                return TCL_ERROR;
                            }
                        }
                        Tcl_SetObjResult(interp, obj);
                        return TCL_OK;
                    }
                }
            }
        case CTYPE_GETPTR: 
            // get a pointer to ts->value
            // if its a basic type, this will result in a pointer to basic type (as int*)
            // if its already a pointer this results in double-pointer (char**)
            obj=Ffidl_NewPointerObj(ts->value,ts->xtype->typename);
            if(obj==NULL) return TCL_ERROR;
            Tcl_SetObjResult(interp, obj);
            return TCL_OK;
        case CTYPE_GETDPTR: 
            // if its a ptrtype, deref the pointer to get original ptr (char*)
            // else return normal pointer
            if(ts->cinv_type==_CINV_T_PTR) {
                dest=*(char**)ts->value;
                obj=Ffidl_NewPointerObj(dest,ts->xtype->typename);
            } else {
                obj=Ffidl_NewPointerObj(ts->value,ts->xtype->typename);
            }
            if(obj==NULL) return TCL_ERROR;
            Tcl_SetObjResult(interp, obj);
            return TCL_OK;
        case CTYPE_SETPTR: 
            if ((objc < 3) || (objc > 4)) {
                Tcl_WrongNumArgs(interp, 2, objv, "ptr subtype");
                return TCL_ERROR;
            } else {
                simpleptr=0;
                int allocold=ts->allocsystem;
                ts->allocsystem=MEM_TCL;
                if(objc > 3) {
                    char* subtype=Tcl_GetString(objv[3]);
                    if(strcmp(subtype,"char*")==0) {
                        simpleptr=1;
                    }
                    if(strcmp(subtype,"mem_system")==0) {
                        ts->allocsystem=MEM_SYSTEM;
                    }
                    if(strcmp(subtype,"mem_none")==0) {
                        ts->allocsystem=-1;
                    }
                }
                PTR_TYPE* ptr=NULL;
                Tcl_AppendResult(interp,"getting ptr from ",Tcl_GetString(objv[2]),NULL);
                if(Ffidl_GetPointerFromObj(interp,objv[2],&ptr,ts->xtype->typename)!=TCL_OK) {
                    return TCL_ERROR;
                }
                if(ptr==NULL) {
                    Tcl_AppendResult(interp, "Couldn't load instance from ptr, PTR is NULL",NULL);
                    return TCL_ERROR;
                }
                if(simpleptr) {
                    //defunct
                    char* str;
                    str=tcl_strdup((char*)ptr);
                    *ts->value=(PTR_TYPE)(void**)str;
                } else {
                    if (ts->value!=NULL) cinv_free(ts->value,allocold);
                    ts->value=(void*)ptr;
                }
            }
            return TCL_OK;
        default:
            Tcl_AppendResult(interp,"internal error during option lookup",NULL);
            return TCL_ERROR;
    }
    return TCL_OK;
} 
static int CTypeCreateCmd( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	TclCTypeState *ts;
	CInvContext *ctx;
    CInv_Type* xtype=NULL;
    int itype=-1;
    int istypedarray=0;
    int arrlen=1;
    int len=0,size=0;
    //char buf[256];
    Tcl_Obj** deflist;
    int llength;
    PTR_TYPE* valptr;
    
    	
	if (objc < 3 || objc > 5) {
		Tcl_WrongNumArgs(interp, 1, objv, "typename type [length] [value]");
		return TCL_ERROR;
	}

	ctx = cinv_context_create();
	
	if (ctx == NULL) {
		return(TCL_ERROR);
	}
	
	ts = (void *) cinv_alloc(sizeof(*ts),MEM_TCL,"CTypeCreateCmd",__LINE__);
	ts->ctx = ctx;
    ts->isarray=0;
    ts->iscdata=0;
    ts->allocsystem=MEM_TCL;
	
    char needle[] = "()";
    if(strstr(Tcl_GetString(objv[2]), needle)!=NULL) {
        if(objc<4) {
            Tcl_WrongNumArgs(interp, 1, objv, "typename type length [value]");
            return TCL_ERROR;
        }
        istypedarray=1;
	} 
	if(istypedarray) { 
        // type array needs different init
        ts->isarray=1;
        ts->iscdata=0;
        char* string=tcl_strdup(Tcl_GetString(objv[2]));
        strtok(string,"()");
        Tcl_Obj* o= Tcl_NewStringObj(string,-1);
        cinv_free(string,MEM_TCL);
        if (get_type_from_str(interp,o, &itype,&xtype)!=TCL_OK) return TCL_ERROR;
        Tcl_DecrRefCount(o);
        if(Tcl_GetIntFromObj(interp,objv[3],&arrlen)!=TCL_OK) return TCL_ERROR;
    } else {
        if (get_type_from_str(interp,objv[2], &itype,&xtype)!=TCL_OK) return TCL_ERROR;
    }
    if (itype<0) {
        Tcl_AppendResult(interp, "Unknown type ",TclGetString(objv[2]),NULL);
        return TCL_ERROR;
    }
    ts->int_type=itype;
    if (itype==_CINV_T_STRUCT) {
        type_release(xtype);

        char* tname=Tcl_GetString(objv[2]);
        // handle struct def here
        //printf("Instancing struct %s\n",tname);
        TclCStructDef *entry=entry_lookup(&cinvclient->structs,tname);
        if(entry==NULL) {
            Tcl_AppendResult(interp,"Struct not found ",Tcl_GetString(objv[2]),NULL);
            return TCL_ERROR;
        }
        ts->int_type=_CINV_T_PTR;
        itype=_CINV_T_PTR;
        ts->cinv_type=itype;
        ts->deflen=1;
        size=get_size_from_type(itype);
        ts->binsize=size;
        Tcl_Obj* cmdobjv[4];
        cmdobjv[0]=Tcl_NewStringObj("StructCreate",-1);
        cmdobjv[1]=objv[1];
        cmdobjv[2]=objv[2];
        cmdobjv[3]=entry->definition;
        CStructCreateCmd(NULL,interp,4,cmdobjv);
        Tcl_DecrRefCount(cmdobjv[0]);
        cinv_free(ts,MEM_TCL);
        if (!cinv_context_delete(ctx)) {
            return TCL_ERROR;
        }
        return TCL_OK;

    }
    ts->xtype=xtype;
    //printf("got type %s as %p %p %p\n",Tcl_GetString(objv[2]),&xtype,ts->xtype,&newtype);
    //printf("%d %d size %d\n",ts->xtype->cinv_basetype,ts->xtype->cinv_inttype, ts->xtype->size);
    if (itype>=_CINV_T_STRUCT) {
        itype=_CINV_T_PTR;
    }
	ts->cinv_type=itype;
	ts->deflen=1;
    void* val;

    size=get_size_from_type(itype)*arrlen;
    ts->deflen=arrlen;
    ts->binsize=size;
    val=cinv_alloc(size,MEM_TCL,"CTypeCreateCmd",__LINE__);
    ts->value=val;
    
    char* dest=val;
    while (size) { 
    *(dest) = '\0'; 
    (dest)++; 
    (size)--; 
    }

	if ((objc>=4)) {
	    // we have an initial value;
	    if(ts->deflen<=1) {
	        //singlevalue
	        len=0;
	        if(get_value_from_obj(interp,ts->int_type,ts->xtype,objv[3],ts->value,&len)!=TCL_OK) return TCL_ERROR;
            if (len>0) {
                ts->binsize=len;
            }
	    } else {
	        //init array
	        if(objc>4) {  
                if (Tcl_ListObjGetElements(interp, objv[objc-1], &llength, &deflist)!=TCL_OK) {
                    return TCL_ERROR;
                }
                for(int i=0;i<min(arrlen,llength);i++) {
                    valptr=(PTR_TYPE*)((PTR_TYPE)ts->value+(i*get_size_from_type(ts->cinv_type)));

                    if(get_value_from_obj(interp,ts->int_type,ts->xtype,deflist[i],valptr,&len)!=TCL_OK) return TCL_ERROR;
                }
            }
            
	    }
	    
	}
	
	Tcl_CreateObjCommandNS(interp,Tcl_GetString(objv[1]),CTypeHandleCmd,ts,CTypeCCommandDeleteProc);
	return TCL_OK;
}

static void CDATACCommandDeleteProc(ClientData cdata) {
	TclCTypeState *ts;
	CInvContext *ctx ;
    void* val;
    int arrlen;
    //char buf[256];
    
	ts = (TclCTypeState *) cdata;
	ctx = ts->ctx;
	val=ts->value;
	arrlen=1;
	if(ts->isarray) arrlen=ts->deflen;
	if((arrlen>1)) {
	    // delete array data
	    // careful, might be dragons here
        for(int i=0;i<arrlen;i++) {
            PTR_TYPE* valptr=(PTR_TYPE*)((PTR_TYPE)ts->value+(i*get_size_from_type(ts->cinv_type)));
            // need we to free something here?
            if((ts->int_type==_CINV_T_PTR_BYTE)||(ts->int_type==_CINV_T_PTR_UTF8)
                ||(ts->int_type==_CINV_T_PTR_UTF16)) {
                cinv_free((void*)*valptr,MEM_TCL);
            }
            if(ts->int_type==_CINV_T_CHARSTR) {
                cinv_free(*(char**)valptr,MEM_SYSTEM);
            }
        }
	} else {
        if((ts->int_type==_CINV_T_PTR_BYTE)||(ts->int_type==_CINV_T_PTR_UTF8)
            ||(ts->int_type==_CINV_T_PTR_UTF16)) {
            cinv_free(*(char**)val,ts->allocsystem);
        }
        if(ts->int_type==_CINV_T_CHARSTR) {
            cinv_free(*(char**)val,MEM_SYSTEM);
        }
    }
    type_release(ts->xtype);
    if (val!=NULL) cinv_free(val,MEM_TCL);
	cinv_free(ts,MEM_TCL);
	
	if (!cinv_context_delete(ctx)) {
        return;
	}
	
    return;

}
static int CDATAHandleCmd ( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	void *dest;
	Tcl_Obj* obj;
	int index;
	int len, simpleptr;
	//char buf[300];
	
	int poke_adr;
	unsigned char poke_val;
	
	TclCTypeState *ts;
	CInvContext *ctx ;
	
	static CONST char *options[] = {
		"set",
		"get",
		"peek",
		"poke",
		"setptr",
		"getptr",
		"getptr*",
		"memcpy",
		(char *) NULL
	};
	enum options {
		CDATA_SETVALUE,
		CDATA_GETVALUE,
		CDATA_PEEK,
		CDATA_POKE,
		CDATA_SETPTR,
		CDATA_GETPTR,
		CDATA_GETDPTR,
		CDATA_MEMCPY,
	};
	
	ts = (TclCTypeState *) cdata;
	if(ts==NULL) return TCL_ERROR;
	ctx = ts->ctx;
	
	if(ctx==NULL) return TCL_ERROR;
	
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "subcommand arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
                &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch (index) {
        case CDATA_SETVALUE:
            if (objc < 3) {
                Tcl_WrongNumArgs(interp, 2, objv, " value");
                return TCL_ERROR;
            } else {
                //singlevalue
                if((ts->int_type==_CINV_T_PTR_BYTE)||(ts->int_type==_CINV_T_PTR_UTF8)
                    ||(ts->int_type==_CINV_T_PTR_UTF16)) {
                    cinv_free(*(char**)ts->value,MEM_TCL);
                }
                if(ts->int_type==_CINV_T_CHARSTR) {
                    cinv_free(*(char**)ts->value,MEM_SYSTEM);
                }
                len=ts->deflen;
                // this will silently set the bytearray to the defined length
                // and truncate or fill zero the rest
                if(get_value_from_obj(interp,ts->int_type,ts->xtype,objv[2],ts->value,&len)!=TCL_OK) return TCL_ERROR;
                if(len!=ts->deflen) {
                    // this should never be reached
                    Tcl_AppendResult(interp,"Length of set byte array != defined length",NULL);
                    return TCL_ERROR;
                }
                //int size=get_size_from_type(ts->cinv_type);
                return TCL_OK; 
            }
        case CDATA_GETVALUE:
            if (objc <2) {
                Tcl_WrongNumArgs(interp, 1, objv, "");
                return TCL_ERROR;
            } else {
               //singlevalue
                obj=set_obj_from_value(interp,ts->int_type,ts->xtype,ts->value, ts->binsize);
                if(obj==NULL) return TCL_ERROR;
                Tcl_SetObjResult(interp, obj);
                return TCL_OK; 
            }
        case CDATA_POKE:
            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, " ADR VALUE");
                return TCL_ERROR;
            } else {
                int pkv;
                if(Tcl_GetIntFromObj(interp,objv[2],&poke_adr)!=TCL_OK) return TCL_ERROR;
                if(Tcl_GetIntFromObj(interp,objv[3],&pkv)!=TCL_OK) return TCL_ERROR;
                poke_val=pkv&255;
                if(poke_adr>=ts->deflen) {
                    Tcl_AppendResult(interp, "Adr not in data range",NULL);
                    return TCL_ERROR;
                }
                char* dest=*(char**)ts->value;
                *(dest+poke_adr)=poke_val;
                return TCL_OK; 
            }
        case CDATA_PEEK:
            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 1, objv, "ADR");
                return TCL_ERROR;
            } else {
                if(Tcl_GetIntFromObj(interp,objv[2],&poke_adr)!=TCL_OK) return TCL_ERROR;
                if(poke_adr>=ts->deflen) {
                    Tcl_AppendResult(interp, "Adr not in data range",NULL);
                    return TCL_ERROR;
                }
                char* dest=*(char**)ts->value;
                poke_val=*(char*)(dest+poke_adr);
                obj=Tcl_NewIntObj(poke_val);
                if(obj==NULL) return TCL_ERROR;
                Tcl_SetObjResult(interp, obj);
                return TCL_OK; 
            }
        case CDATA_GETPTR: 
            // get a pointer to ts->value
            // if its a basic type, this will result in a pointer to basic type (as int*)
            // if its already a pointer this results in double-pointer (char**)
            obj=Ffidl_NewPointerObj(ts->value,"CDATA");
            if(obj==NULL) return TCL_ERROR;
            Tcl_SetObjResult(interp, obj);
            return TCL_OK;
        case CDATA_GETDPTR: 
            // if its a ptrtype, deref the pointer to get original ptr (char*)
            // else return normal pointer
            if(ts->cinv_type==_CINV_T_PTR) {
                dest=*(char**)ts->value;
                obj=Ffidl_NewPointerObj(dest,"CDATA");
            } else {
                obj=Ffidl_NewPointerObj(ts->value,"CDATA");
            }
            if(obj==NULL) return TCL_ERROR;
            Tcl_SetObjResult(interp, obj);
            return TCL_OK;
        case CDATA_SETPTR: 
            if ((objc < 3) || (objc > 4)) {
                Tcl_WrongNumArgs(interp, 2, objv, "ptr subtype");
                return TCL_ERROR;
            } else {
                simpleptr=0;
                int allocold=ts->allocsystem;
                ts->allocsystem=MEM_TCL;
                if(objc > 3) {
                    char* subtype=Tcl_GetString(objv[3]);
                    if(strcmp(subtype,"char*")==0) {
                        simpleptr=1;
                    }
                    if(strcmp(subtype,"mem_system")==0) {
                        ts->allocsystem=MEM_SYSTEM;
                    }
                    if(strcmp(subtype,"mem_none")==0) {
                        ts->allocsystem=-1;
                    }
                }
                PTR_TYPE* ptr;
                if(Ffidl_GetPointerFromObj(interp,objv[2],&ptr,ts->xtype->typename)!=TCL_OK) return TCL_ERROR;
                //printf("Got pointer %p %s\n",ptr,ts->xtype->typename);
                if(ptr==NULL) {
                    Tcl_AppendResult(interp, "Couldn't load instance from ptr",NULL);
                    return TCL_ERROR;
                }
                if(simpleptr) {
                    //defunct
                    char* str;
                    str=tcl_strdup((char*)ptr);
                    *ts->value=(PTR_TYPE)(void**)str;
                } else {
                    char *retpcv = (char*)ptr;
                    if((PTR_TYPE)ts->value!=(PTR_TYPE)retpcv) {
                        if (ts->value!=NULL) {
                            cinv_free(*(char**)ts->value,allocold);
                            cinv_free(ts->value,MEM_TCL);
                        }
                        char* val=cinv_alloc(sizeof(void*),MEM_TCL,"CDATAHandleCmd",__LINE__);
                        *(char**)val=retpcv;
                        ts->value=(PTR_TYPE*)val;
                    }
                }
            }
            return TCL_OK;
        case CDATA_MEMCPY: 
            if ((objc < 3) || (objc > 4)) {
                Tcl_WrongNumArgs(interp, 2, objv, "ptr [direction in/out default in]");
                return TCL_ERROR;
            } else {
                //
                PTR_TYPE* ptr,*retpcv;
                int direction=0; // 0 in/ 1 out
                if(Ffidl_GetPointerFromObj(interp,objv[2],&ptr,ts->xtype->typename)!=TCL_OK) return TCL_ERROR;
                if(ptr==NULL) {
                    Tcl_AppendResult(interp, "Couldn't copy instance from NULL PTR",NULL);
                    return TCL_ERROR;
                }
                if((ts->deflen>0)&&(len>ts->deflen)) {
                    len=ts->deflen;
                }
                retpcv = *(PTR_TYPE**)ts->value;  
                if (retpcv == NULL) return TCL_ERROR;          // No memory
                if(objc==3) {
                    if(strcmp(Tcl_GetString(objv[3]),"out")==0) {
                        direction=1;
                    }
                }
                PTR_TYPE* from, *to;
                
                if(direction==0) {
                    to=retpcv;from=ptr;
                } else {
                    from=retpcv;to=ptr;
                }
                memmove(to, from,len);                     // Copy the characters in
                
                // init zero
                if(len<ts->deflen) {
                    PTR_TYPE* dest=to+len;
                    len=ts->deflen-len; 
                    while (len) { 
                        *(dest) = '\0'; 
                        (dest)++; 
                        (len)--;
                    }   
                    len=ts->deflen;
                }
                return TCL_OK;
            }
            
        default:
            Tcl_AppendResult(interp,"internal error during option lookup",NULL);
            return TCL_ERROR;
    }
    return TCL_OK;
} 
static int CDATACreateCmd( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	TclCTypeState *ts;
	CInvContext *ctx;
	//CInv_Type* xtype;
    int itype=-1;
    int datasize=1;
    //char buf[256];
    	
	if (objc < 3 || objc > 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "typename length");
		return TCL_ERROR;
	}

	ctx = cinv_context_create();
	
	if (ctx == NULL) {
		return(TCL_ERROR);
	}
	
	ts = (void *) cinv_alloc(sizeof(*ts),MEM_TCL,"CDATACreateCmd",__LINE__);
	ts->ctx = ctx;
    // ok, we got raw data we want later to use it for direct access
    itype=_CINV_T_PTR_BYTE;

    ts->iscdata=1;
    ts->isarray=0;
    
    ts->allocsystem=MEM_TCL;
    if(Tcl_GetIntFromObj(interp,objv[2],&datasize)!=TCL_OK) return TCL_ERROR;
    ts->int_type=itype;
	ts->cinv_type=_CINV_T_PTR;
    ts->xtype=cinv_alloc(sizeof(CInv_Type), MEM_TCL,"CDATACreateCmd",__LINE__);
    ts->xtype->size=get_size_from_type(itype);
    ts->xtype->cinv_basetype=_CINV_T_PTR;
    ts->xtype->cinv_inttype= itype;
    ts->xtype->typename=tcl_strdup("CDATA");
    ts->xtype->typetag=tcl_strdup("");
    void* val;
    ts->deflen=datasize;
    ts->binsize=datasize;
    val=cinv_alloc(datasize,MEM_TCL,"CDATACreateCmd",__LINE__);
    ts->value=cinv_alloc(sizeof(PTR_TYPE),MEM_TCL,"CDATACreateCmd",__LINE__);
    *(char**)ts->value=val;
    // init to zero
    char* dest=*(char**)ts->value;
    while (datasize) { 
    *(dest) = '\0'; 
    (dest)++; 
    (datasize)--; 
    }
    
	Tcl_CreateObjCommandNS(interp,Tcl_GetString(objv[1]),CDATAHandleCmd,ts,CDATACCommandDeleteProc);

	//Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
}

void CCallbackCallback (CInvFunction *f, void *args[], void *returnout,void *userdata) {
    TclCCallbackState *ts;
	Tcl_Interp* interp;
	CInvContext *ctx;
	CInv_Type* xtype;
    int rv, return_type;
    char buf[1024];
 	Tcl_Obj** paramlist;
	int paramlength;
	
	ts= (TclCCallbackState*) userdata;
	if(ts==NULL) return;
	ctx = ts->ctx;
	
	if(ctx==NULL) return;
	interp=ts->interp;
	if(interp==NULL) return;
	
    if (Tcl_ListObjGetElements(interp, ts->paramlist, &paramlength, &paramlist)!=TCL_OK) {
        return;
    }
    
    Tcl_Obj** params;
    Tcl_Obj* obj;
    params=cinv_alloc(sizeof(Tcl_Obj*)*(paramlength+1),MEM_TCL,"CCallbackCallback",__LINE__);
    Tcl_Obj* cmdname=Tcl_NewStringObj(ts->tcl_procname,-1);
    Tcl_Obj* returninfo=Tcl_NewStringObj(ts->returninfo,-1);
    //Tcl_IncrRefCount(cmdname);
    params[0]=cmdname;
    for (int i=0;i<paramlength;i++) {
        // test params
        int ntype;
        if (get_type_from_str(interp,paramlist[i],&ntype,&xtype)!=TCL_OK) return;
        obj=set_obj_from_value(interp,ntype,xtype,args[i], -1);
        if(obj==NULL) {
            Tcl_AppendResult(interp,"Error No result from ",Tcl_GetString(paramlist[i]),NULL);
            cinv_free(params,MEM_TCL);
            return;
        }
        params[i+1]=obj;
        Tcl_IncrRefCount(obj);
        type_release(xtype);
    }
    if ((rv = Tcl_EvalObjv(interp, paramlength+1, params, 0)) != TCL_OK) {
        snprintf(buf, 1024, "puts {Error calling TCL function %s\n %s}",ts->tcl_procname,Tcl_GetStringResult(interp));
        Tcl_Eval(interp, buf);
        cinv_free(params,MEM_TCL);
        return;
    }
    for (int i=0;i<paramlength+1;i++) {
        Tcl_DecrRefCount(params[i]);
    }
    cinv_free(params,MEM_TCL);
    if((ts->return_type>-1)&&(returnout!=NULL)) {
        int oklen;
        Tcl_Obj* _tmpVar_cmdResultObj;
        _tmpVar_cmdResultObj= Tcl_GetObjResult(interp);
        // ts->xtype doesn't work, need to copy beforehand on creation
        if(get_type_from_str(interp,returninfo, &return_type,&xtype)!=TCL_OK) return;
        if(get_value_from_obj(interp,return_type,xtype,_tmpVar_cmdResultObj,returnout,&oklen)!=TCL_OK) return;
        type_release(xtype);
    }
    Tcl_DecrRefCount(returninfo);
    return;
}
static void CCallbackCCommandDeleteProc(ClientData cdata) {
	TclCCallbackState *ts;
	CInvContext *ctx ;
    //char buf[256];
	ts = (TclCCallbackState *) cdata;
	ctx = ts->ctx;
	if (!cinv_callback_delete(ctx,ts->callback)) {
	}
	if (!cinv_function_delete(ctx,ts->prototype)) {
	}
	if (!cinv_context_delete(ctx)) {
	}
	type_release(ts->xtype);
	cinv_free(ts->tcl_procname,MEM_TCL);
	cinv_free(ts->returninfo,MEM_TCL);
	if(ts->paramlist!=NULL) Tcl_DecrRefCount(ts->paramlist);

	cinv_free((void *) ts,MEM_TCL);
    return;

}
static int CCallbackHandleCmd ( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	Tcl_Obj* obj;
	int index;

	TclCCallbackState *ts;
	CInvContext *ctx;
    //char buf[256];
	
	static CONST char *options[] = {
	    "getptr",
		"info",
		(char *) NULL
	};
	enum options {
	    CCALLBACK_GETPTR,
		CCALLBACK_INFO,
	};
	
	ts = (TclCCallbackState *) cdata;
	if(ts==NULL) return TCL_ERROR;
	ctx = ts->ctx;
	
	if(ctx==NULL) return TCL_ERROR;
	
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "subcommand arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
                &index) != TCL_OK) {
        return TCL_ERROR;
    }
    
    switch (index) {
        case CCALLBACK_GETPTR:
            obj=Ffidl_NewPointerObj(ts->entrypoint,"tclproc");
            Tcl_SetObjResult(interp, obj);
            return TCL_OK;
        case CCALLBACK_INFO:
            Tcl_AppendResult(interp,"(",ts->returninfo,") ",ts->tcl_procname," (",Tcl_GetString(ts->paramlist),")",NULL);
            return TCL_OK;
        default:
            Tcl_AppendResult(interp,"internal error during option lookup",NULL);
            return TCL_ERROR;
    }
    return TCL_OK;
} 
static int CCallbackCreateCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]){
	TclCCallbackState *ts;
	CInvContext *ctx;
    int return_type=-1;
    int callingconvention;
    //char buf[256];
 	Tcl_Obj** paramlist;
	int paramlength;
    char *retsig,*paramsig;
   	retsig="\0\0";
	if (objc < 2 || objc > 5) {
		Tcl_WrongNumArgs(interp, 1, objv, "objname tclprocname returntype argumenttypes");
		return TCL_ERROR;
	}

	ctx = cinv_context_create();
	
	if (ctx == NULL) {
		return(TCL_ERROR);
	}
	ts = cinv_alloc(sizeof(TclCCallbackState),MEM_TCL,"CCallbackCreateCmd",__LINE__);
	ts->ctx = ctx;
	ts->interp=interp;
	ts->xtype=NULL;
	ts->tcl_procname=tcl_strdup(Tcl_GetString(objv[2]));
    ts->int_type=-1;
    ts->return_type=-1;
    ts->callingconvention=-1; //--> use CINV_CC_DEFAULT

	char* retstr=Tcl_GetString(objv[3]);
	if(strlen(retstr)>0) { 
	    ts->returninfo=tcl_strdup(retstr);
        if (get_type_from_str(interp,objv[3], &return_type,NULL)!=TCL_OK) return TCL_ERROR;
        if (return_type<0) {
            Tcl_AppendResult(interp, "Unknown type ",TclGetString(objv[3]),NULL);
            return TCL_ERROR;
        }
        ts->int_type=return_type;
        if (return_type>=_CINV_T_STRUCT) {
            return_type=_CINV_T_PTR;
            retsig="p";
        } else {
            retsig=isig_type(return_type);
        }
        ts->return_type=return_type;
    }

	ts->paramlength=0;
	paramlength=0;
	if (objc==5) {
	    // we have an initial paramlist;
        if (Tcl_ListObjGetElements(interp, objv[4], &paramlength, &paramlist)!=TCL_OK) {
            return TCL_ERROR;
        }
        paramsig= cinv_alloc((paramlength+1) * sizeof(char),MEM_TCL,"CCallbackCreateCmd",__LINE__);
        int i;
        for (i=0;i<paramlength;i++) {
            // test params
            int ntype;
            if (get_type_from_str(interp,paramlist[i],&ntype,NULL)!=TCL_OK) return TCL_ERROR;
            paramsig[i]=isig_type(ntype)[0];
        }
        paramsig[i]='\0';
        ts->paramlist=Tcl_DuplicateObj(objv[4]);
        ts->paramlength=paramlength;
        Tcl_IncrRefCount(ts->paramlist);
	} else {
	    paramsig=NULL;
	}
	Tcl_DeleteCommand(interp,Tcl_GetString(objv[1]));
    if(ts->callingconvention<0) {
        callingconvention=CINV_CC_DEFAULT;
    } 

	CInvFunction *proto = cinv_function_create(ctx, callingconvention, retsig, paramsig);
	ts->prototype=proto;
    		
    CInvCallback *cb = cinv_callback_create(ctx, proto, (void*)ts, CCallbackCallback);
	ts->callback=cb;
	void *e = cinv_callback_getentrypoint(ctx, cb);
	ts->entrypoint=e;
	
	Tcl_CreateObjCommandNS(interp,Tcl_GetString(objv[1]),CCallbackHandleCmd,ts,CCallbackCCommandDeleteProc);

	//Tcl_SetObjResult(interp, objv[1]);
    if(paramsig!=NULL) cinv_free(paramsig,MEM_TCL);	
	return TCL_OK;
}

static int listtypes (Tcl_Interp* interp) {
    Tcl_HashSearch search;
    Tcl_HashEntry *entry;
    for (entry = Tcl_FirstHashEntry(&cinvclient->types, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
        Tcl_AppendResult(interp,Tcl_GetHashKey(&cinvclient->types,entry)," ",NULL);
    }
    for (entry = Tcl_FirstHashEntry(&cinvclient->dyntypes, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
        Tcl_AppendResult(interp,Tcl_GetHashKey(&cinvclient->dyntypes,entry)," ",NULL);
    }
    return TCL_OK;
}    
static int CTypeSize (ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj * CONST objv[]) {
    int int_type;
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "typename");
        return TCL_ERROR;
    }
    if ((get_type_from_str(interp,objv[1],&int_type,NULL)!=TCL_OK)||(int_type<0)) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(get_size_from_type(int_type)));
    return TCL_OK;
}
static int CTypeDefine (ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj * CONST objv[]) {
    //
    char* typename;
    int size, issigned;
    CInv_Type* ftype;    
    int int_type;
    if (objc < 3) {
        if(objc==1) {
            listtypes(interp);
            return TCL_OK;
        }
        if(objc==2) {
            Tcl_ResetResult(interp);
            if ((get_type_from_str(interp,objv[1],&int_type,NULL)!=TCL_OK)||(int_type<0)) {
                Tcl_ResetResult(interp);
                return TCL_OK;
            }
            Tcl_AppendResult(interp, Tcl_GetString(objv[1]),NULL);
            return TCL_OK;
        }
        Tcl_WrongNumArgs(interp, 1, objv, "typename basetype (size signed)");
        return TCL_ERROR;
    }
    //ftype=cinv_alloc(sizeof(CInv_Type),MEM_TCL,"typedef",__LINE__);
    
    typename=Tcl_GetString(objv[2]);
    if(strlen(typename)==0) {
        return TCL_ERROR;
    }
    CInv_Type* xtype;
    if (get_type_from_str(interp,objv[1],&int_type,&xtype)!=TCL_OK) return TCL_ERROR;
    ftype=xtype;
    if (objc>4) {
        if (Tcl_GetIntFromObj(interp,objv[3],&size)) return TCL_ERROR;
        if (Tcl_GetIntFromObj(interp,objv[4],&issigned)) return TCL_ERROR;
        if(size==0) {
            size=get_size_from_type(int_type);
        }
        
        ftype->size=(size_t)size;
        ftype->cinv_inttype=int_type;

        if (int_type>=_CINV_T_PTR) {
            ftype->isPtr=1;
            ftype->cinv_basetype=_CINV_T_PTR;
        } else {
            ftype->isPtr=0;
            ftype->cinv_basetype=int_type;
        }
        ftype->alignment=size;
        ftype->isSigned=(issigned>0?1:0);
    } else {
        //copytype(ftype,xtype);
        ftype->cinv_inttype=int_type;
        ftype->cinv_basetype=int_type;
    }
    
    if(ftype->typename!=NULL) cinv_free(ftype->typename,MEM_TCL);
    if(ftype->typetag!=NULL) cinv_free(ftype->typetag,MEM_TCL);
    ftype->typename=tcl_strdup(typename);
    ftype->typetag=tcl_strdup(typename);
    //printf("Defining type %s as %s\n",typename, Tcl_GetString(objv[1]));
    //printf("got detailed info on type: %d %d sz %d sig %d ptr %d\n",ftype->cinv_basetype, ftype->cinv_inttype, ftype->size, ftype->isSigned,ftype->isPtr);
    
    type_define_dyn(cinvclient, typename, ftype);
    return TCL_OK;
}
static int CPointerCast (ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj * CONST objv[]) {
    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "pointerobj newtag");
        return TCL_ERROR;
    }
    Tcl_Obj* obj=PointerCast(interp,objv[1],objv[2]);
    if(obj==NULL) {
        Tcl_AppendResult(interp,"Pointerconversion failed ",Tcl_GetString(objv[1]),Tcl_GetString(objv[2]),NULL);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,obj);
    return TCL_OK;
}
static int CCleanup (ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj * CONST objv[]) {
    client_delete(cinvclient, interp);
    cinvclient=NULL;
    client_delete(cinvclient, interp);
#ifdef MEMDEBUG
	tcc_memcheck();
#endif
    return TCL_OK;
}

#if (defined(_WIN32) && (defined(_MSC_VER)|| defined(__TINYC__)  || (defined(__BORLANDC__) && (__BORLANDC__ >= 0x0550)) || defined(__LCC__) || defined(__WATCOMC__) || (defined(__GNUC__) && defined(__declspec))))
#undef DLLIMPORT
#undef DLLEXPORT
#   define DLLIMPORT __declspec(dllimport)
#   define DLLEXPORT __declspec(dllexport)
#else
#undef DLLIMPORT
#undef DLLEXPORT
#   define DLLIMPORT __attribute__(dllimport)
#   if defined(__GNUC__) && __GNUC__ > 3
#       define DLLEXPORT __attribute__ ((visibility("default")))
#   else
#       define DLLEXPORT
#   endif
#endif

DLLEXPORT
int Cinvoke_tclcmd_Init(Tcl_Interp *interp) {
    globalinterp=interp;
    //return TCL_OK;

#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, TCL_VERSION , 0) == 0L) {
		return TCL_ERROR;
	}
#endif
    
    cinvclient = client_alloc(interp);
	Tcl_CreateObjCommand(interp, "CInvoke", CInvokeCreateCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, "CStruct", CStructDeclareCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, "CType", CTypeCreateCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, "CDATA", CDATACreateCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, "CCallback", CCallbackCreateCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, "typedef", CTypeDefine, NULL,NULL);
	Tcl_CreateObjCommand(interp, "sizeof", CTypeSize, NULL,NULL);
	Tcl_CreateObjCommand(interp, "CCleanup", CCleanup, NULL,NULL);
	Tcl_CreateObjCommand(interp, "PointerCast", CPointerCast, NULL,NULL);
	
	Ffidl_Init(interp);
	
	/* static  unsigned types */
    static CInv_Type cinv_type_uchar = init_type(sizeof(char), _CINV_T_CHAR, _CINV_T_CHAR, 0, 0, 0);
    static CInv_Type cinv_type_ushort = init_type(sizeof(short), _CINV_T_SHORT, _CINV_T_SHORT,0, 0, 0);
    static CInv_Type cinv_type_uint = init_type(sizeof(int), _CINV_T_INT, _CINV_T_INT, 0, 0,0);
    static CInv_Type cinv_type_ulong = init_type(sizeof(long), _CINV_T_LONG, _CINV_T_LONG, 0,0, 0);
    static CInv_Type cinv_type_ulonglong = init_type(sizeof(long long), _CINV_T_EXTRALONG, _CINV_T_EXTRALONG, 0,0, 0);
    
	type_define(cinvclient, "uchar", &cinv_type_uchar);
	type_define(cinvclient, "ushort", &cinv_type_ushort);
	type_define(cinvclient, "uint", &cinv_type_uint);
	type_define(cinvclient, "ulong", &cinv_type_ulong);
	type_define(cinvclient, "ulonglong", &cinv_type_ulonglong);
	
	/* dynamic types */
	
	type_define_dyn(cinvclient, "int8_t", size_type(NULL,1,1,"int8_t"));
	type_define_dyn(cinvclient, "uint8_t", size_type(NULL,1,0,"uint8_t"));
	type_define_dyn(cinvclient, "int16_t", size_type(NULL,2,1,"int16_t"));
	type_define_dyn(cinvclient, "uint16_t", size_type(NULL,2,0,"uint16_t"));
	type_define_dyn(cinvclient, "int32_t", size_type(NULL,4,1,"int32_t"));
	type_define_dyn(cinvclient, "uint32_t", size_type(NULL,4,0,"uint32_t"));
	type_define_dyn(cinvclient, "int64_t", size_type(NULL,8,1,"int64_t"));
	type_define_dyn(cinvclient, "uint64_t", size_type(NULL,8,0,"uint64_t"));

    ADDINTTYPE(_Bool);
    typedef _Bool bool;
    ADDINTTYPE(bool);

#ifdef _WIN32
	//def_type(&cinv_type_size_t, sizeof(size_t), get_type_from_size(sizeof(size_t)), get_type_from_size(sizeof(size_t)), 0,0, 0);
	//type_define(cinvclient, "size_t", &cinv_type_size_t);
	//
    ADDINTTYPE(BOOL);
    ADDINTTYPE(BOOLEAN);
    ADDINTTYPE(CHAR);
    ADDINTTYPE(BYTE);
    ADDINTTYPE(WORD);
    ADDINTTYPE(DWORD);
    ADDINTTYPE(DWORD_PTR);
    ADDINTTYPE(DWORDLONG);
    ADDINTTYPE(HALF_PTR);
    ADDINTTYPE(INT);
    ADDINTTYPE(INT_PTR);
    ADDINTTYPE(LONG);
    ADDINTTYPE(LONGLONG);
    ADDINTTYPE(LONG_PTR);
    ADDINTTYPE(LPARAM);
    ADDINTTYPE(LRESULT);
    ADDINTTYPE(SHORT);
    ADDINTTYPE(SIZE_T);
    ADDINTTYPE(SSIZE_T);
    ADDINTTYPE(UCHAR);
    ADDINTTYPE(UINT);
    ADDINTTYPE(UINT_PTR);
    ADDINTTYPE(ULONG);
    ADDINTTYPE(ULONGLONG);
    ADDINTTYPE(ULONG_PTR);
    ADDINTTYPE(USHORT);
    ADDINTTYPE(WPARAM);
    /* POSIX defines on Windows */
    ADDINTTYPE(time_t);
    ADDINTTYPE(ino_t);
    ADDINTTYPE(off_t);
    ADDINTTYPE(dev_t);
    ADDINTTYPE(size_t);
#else
    ADDINTTYPE(blkcnt_t);
    ADDINTTYPE(blksize_t);
    ADDINTTYPE(clock_t);
#ifdef NOTYET
    /* MacOS Catalina does not define this. Should we keep it in? */
    ADDINTTYPE(clockid_t);
#endif
    ADDINTTYPE(dev_t);
    ADDINTTYPE(fsblkcnt_t);
    ADDINTTYPE(fsfilcnt_t);
    ADDINTTYPE(gid_t);
    ADDINTTYPE(id_t);
    ADDINTTYPE(ino_t);
    ADDINTTYPE(key_t);
    ADDINTTYPE(mode_t);
    ADDINTTYPE(nlink_t);
    ADDINTTYPE(off_t);
    ADDINTTYPE(pid_t);
    ADDINTTYPE(size_t);
    ADDINTTYPE(ssize_t);
    ADDINTTYPE(suseconds_t);
    ADDINTTYPE(time_t);
    ADDINTTYPE(uid_t);
#endif
    Tcl_PkgProvide(interp, "tclcinvoke", "1.0");        
	return TCL_OK;
}

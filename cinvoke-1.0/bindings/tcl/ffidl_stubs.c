#include <tcl.h>
#include <tclInt.h>
#include <tclPort.h>

#define LOOKUP_TK_STUBS

#if defined(LOOKUP_TK_STUBS)
static const char *MyTkInitStubs(Tcl_Interp *interp, char *version, int exact);
static void *tkStubsPtr, *tkPlatStubsPtr, *tkIntStubsPtr, *tkIntPlatStubsPtr, *tkIntXlibStubsPtr;
#else
#define tkStubsPtr NULL
#define tkPlatStubsPtr NULL
#define tkIntStubsPtr NULL
#define tkIntPlatStubsPtr NULL
#define tkIntXlibStubsPtr NULL
#endif

/* usage: ffidl::stubsymbol library stubstable symbolnumber -> address */
static int tcl_ffidl_stubsymbol(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    library_ix,
    stubstable_ix,
    symbol_ix,
    nargs
  };

  int library, stubstable, symbolnumber; 
  void **stubs = NULL, *address;
  static const char *library_names[] = {
    "tcl", 
#if defined(LOOKUP_TK_STUBS)
    "tk",
#endif
    NULL
  };
  enum libraries {
    LIB_TCL,
#if defined(LOOKUP_TK_STUBS)
    LIB_TK,
#endif
  };
  static const char *stubstable_names[] = {
    "stubs", "intStubs", "platStubs", "intPlatStubs", "intXLibStubs", NULL
  };
  enum stubstables {
    STUBS, INTSTUBS, PLATSTUBS, INTPLATSTUBS, INTXLIBSTUBS,
  };

  if (objc != 4) {
    Tcl_WrongNumArgs(interp,1,objv,"library stubstable symbolnumber");
    return TCL_ERROR;
  }
  if (Tcl_GetIndexFromObj(interp, objv[library_ix], library_names, "library", 0, &library) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_GetIndexFromObj(interp, objv[stubstable_ix], stubstable_names, "stubstable", 0, &stubstable) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_GetIntFromObj(interp, objv[symbol_ix], &symbolnumber) != TCL_OK || symbolnumber < 0) {
    return TCL_ERROR;
  }

#if defined(LOOKUP_TK_STUBS)
  if (library == LIB_TK) {
    if (MyTkInitStubs(interp, "8.4", 0) == NULL) {
      return TCL_ERROR;
    }
  }
#endif
  switch (stubstable) {
    case STUBS:
      stubs = (void**)(library == LIB_TCL ? tclStubsPtr : tkStubsPtr); break;
    case INTSTUBS:
      stubs = (void**)(library == LIB_TCL ? tclIntStubsPtr : tkIntStubsPtr); break;
    case PLATSTUBS:
      stubs = (void**)(library == LIB_TCL ? tclPlatStubsPtr : tkPlatStubsPtr); break;
    case INTPLATSTUBS:
      stubs = (void**)(library == LIB_TCL ? tclIntPlatStubsPtr : tkIntPlatStubsPtr); break;
    case INTXLIBSTUBS:
      stubs = (void**)(library == LIB_TCL ? NULL : tkIntXlibStubsPtr); break;
  }

  if (!stubs) {
    Tcl_AppendResult(interp, "no stubs table \"", Tcl_GetString(objv[stubstable_ix]),
        "\" in library \"", Tcl_GetString(objv[library_ix]), "\"", NULL);
    return TCL_ERROR;
  }
  address = *(stubs + 2 + symbolnumber);
  if (!address) {
    Tcl_AppendResult(interp, "couldn't find symbol number ", Tcl_GetString(objv[symbol_ix]),
        " in stubs table \"", Tcl_GetString(objv[stubstable_ix]), "\"", NULL);
    return TCL_ERROR;
  }

  Tcl_SetObjResult(interp, Ffidl_NewPointerObj(address,"SYMBOL"));
  return TCL_OK;
}

/*
 * One function exported for pointer punning with ffidl::callout.
 */
void *ffidl_pointer_pun(void *p) { return p; }
void *ffidl_copy_bytes(void *dst, void *src, size_t len) {
  return memmove(dst, src, len);
}

/*
 *--------------------------------------------------------------
 *
 * Ffidl_Init
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
int Ffidl_Init(Tcl_Interp *interp)
{

  /* initialize commands */
  Tcl_CreateObjCommand(interp,"::ffidl::stubsymbol", tcl_ffidl_stubsymbol, NULL, NULL);

  /* done */
  return TCL_OK;
}

#if defined(LOOKUP_TK_STUBS)
typedef struct MyTkStubHooks {
    void *tkPlatStubs;
    void *tkIntStubs;
    void *tkIntPlatStubs;
    void *tkIntXlibStubs;
} MyTkStubHooks;

typedef struct MyTkStubs {
    int magic;
    struct MyTkStubHooks *hooks;
} MyTkStubs;

/* private copy of Tk_InitStubs to avoid having to depend on Tk at build time */
static const char *
MyTkInitStubs(interp, version, exact)
    Tcl_Interp *interp;
    char *version;
    int exact;
{
    const char *actualVersion;

    actualVersion = Tcl_PkgRequireEx(interp, "Tk", version, exact,
		(ClientData *) &tkStubsPtr);
    if (!actualVersion) {
	return NULL;
    }

    if (!tkStubsPtr) {
	Tcl_SetResult(interp,
		"This implementation of Tk does not support stubs",
		TCL_STATIC);
	return NULL;
    }
    
    tkPlatStubsPtr =    ((MyTkStubs*)tkStubsPtr)->hooks->tkPlatStubs;
    tkIntStubsPtr =     ((MyTkStubs*)tkStubsPtr)->hooks->tkIntStubs;
    tkIntPlatStubsPtr = ((MyTkStubs*)tkStubsPtr)->hooks->tkIntPlatStubs;
    tkIntXlibStubsPtr = ((MyTkStubs*)tkStubsPtr)->hooks->tkIntXlibStubs;
    
    return actualVersion;
}
#endif


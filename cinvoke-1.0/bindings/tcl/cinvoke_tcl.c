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
#include <tcl.h>
#include <Tcl_PtrObj.c>

#include <cinvoke.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

// stack reading functions, we define our own so we can specify
// the exact semantics/errors we want
void expected(Tcl_Interp* interp, const char *exp, Tcl_Obj *objv) {
	char err[256];
	snprintf(err, sizeof(err), "error, expected %s", exp);
	Tcl_AppendResult(interp, err);
	return TCL_ERROR;
}

const char *tolstring(Tcl_Interp* interp, Tcl_Obj *objv, size_t *len) {
	return Tcl_GetString(objv);
}
const char *tostring(Tcl_Interp* interp, Tcl_Obj *objv) {
	return tolstring(interp, objv, NULL);
}
void *touserdata(Tcl_Interp* interp, Tcl_Obj *objv) {
	RETPTR(objv);
}
void *tolightuserdata(Tcl_Interp* interp, Tcl_Obj *objv) {
    RETPTR(objv);
}
int tointeger(Tcl_Interp* interp, Tcl_Obj *objv) {
    int rv;
	Tcl_GetIntFromObj(interp, objv, &rv);
	return rv;	
}
int tonumber(Tcl_Interp* interp, Tcl_Obj *objv) {
    int rv;
	Tcl_GetIntFromObj(interp, objv, &rv);
	return rv;	
}
int toboolean(Tcl_Interp* interp, Tcl_Obj *objv) {
    int rv;
	Tcl_GetIntFromObj(interp, objv, &rv);
	return rv;	
}

struct LibStruct {
	CInvContext *ctx;
	CInvLibrary *lib;
	cinv_callconv_t cc;
	Tcl_Interp *interp;
};

int _clibrary_new(Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]) {
	CInvContext *ctx;
	CInvLibrary *lib;
	cinv_callconv_t cc = CINV_CC_DEFAULT;
	struct LibStruct *st;

	if (objc < 1 || objc > 2) {
		Tcl_AppendResult(interp, "usage: clibrary.new(libname[, callingconv])");
		return TCL_ERROR;
	}

	if (objc == 2) {
		const char *cs = tostring(interp, objv[2]);
		if (!strcmp("cdecl", cs))
			cc = CINV_CC_CDECL;
		else if (!strcmp("stdcall", cs))
			cc = CINV_CC_STDCALL;
		else if (!strcmp("fastcall", cs))
			cc = CINV_CC_FASTCALL;
		else if (strcmp("default", cs)) {
			Tcl_AppendResult(interp, "unknown calling convention");
			return TCL_ERROR;
		}
	}

	ctx = cinv_context_create();
	if (ctx == NULL) {
		Tcl_AppendResult(interp, "out of memory");
		return TCL_ERROR;
	}
	
	lib = cinv_library_create(ctx, tostring(interp, objv[1]));
	if (lib == NULL) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
		cinv_context_delete(ctx);
		return TCL_ERROR;
	}
	
	lua_newtable(l); // push return value

	lua_newtable(l); // metatable for return value
	lua_getglobal(interp, "clibrary");
	lua_setfield(interp, -2, "__index");
	lua_setmetatable(interp, -2);
	
	st = lua_newuserdata(interp, sizeof(struct LibStruct));
	st->ctx = ctx;
	st->lib = lib;
	st->cc = cc;
	st->interp=interp;

	lua_newtable(l); // metatable for userdata
	lua_getglobal(interp, "clibrary");
	lua_getfield(interp, -1, "dispose");
	lua_setfield(interp, -3, "__gc");
	lua_pop(interp, 1); // pop the clibrary global
	lua_setmetatable(interp, -2);

	lua_setfield(interp, -2, "ud"); // save userdata in return value

	lua_newtable(l); // table for holding tied variables
	lua_setfield(interp, -2, "ties");

	return 1;
}
int _clibrary_dispose(Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]) {
	struct LibStruct *st;

	if (objc != 1) {
		Tcl_AppendResult(interp, "usage: clibrary:dispose()");
		return TCL_ERROR;
	}

	// this function can be called from the __gc metatable
	// or by the user with dispose, handle both cases
	if (lua_type(interp, 1) == LUA_TUSERDATA)
		st = touserdata(interp, 1);
	else {
		lua_getfield(interp, 1, "ud");
		st = touserdata(interp, -1);
		lua_pop(interp, 1);
	}
	if (st->ctx) {
		if (st->lib) {
			cinv_library_delete(st->ctx, st->lib);
			st->lib = NULL;
		}
		cinv_context_delete(st->ctx);
		st->ctx = NULL;
	}
	return 0;
}

// some functions dealing with the type system
int isvoid(Tcl_Interp* interp, Tcl_Obj *objv) {
	int ret;
	lua_getfield(interp, index, "family");
	ret = !strcmp("void", tostring(interp, objv));
	lua_pop(interp, 1);
	return ret;
}
int isarray(Tcl_Interp* interp, Tcl_Obj *objv) {
	int ret = 0;
	lua_getfield(interp, index, "array");
	if (!lua_isnil(interp, objv))
		ret = 1;
	lua_pop(interp, 1);
	return ret;
}
int isstruct(Tcl_Interp* interp, Tcl_Obj *objv) {
	int ret;
	lua_getfield(interp, index, "family");
	ret = !strcmp("struct", tostring(interp, -1));
	lua_pop(interp, 1);
	return ret;
}
int isstring(Tcl_Interp* interp, Tcl_Obj *objv) {
	int ret;
	lua_getfield(interp, index, "family");
	ret = !strcmp("string", tostring(interp, -1));
	lua_pop(interp, 1);
	return ret;
}
int isbasic(Tcl_Interp* interp, Tcl_Obj *objv) {
	int ret;
	lua_getfield(interp, index, "family");
	ret = !strcmp("basic", tostring(interp, -1));
	lua_pop(interp, 1);
	return ret;
}
int iscallback(Tcl_Interp* interp, Tcl_Obj *objv) {
	int ret;
	lua_getfield(interp, index, "family");
	ret = !strcmp("callback", tostring(interp, -1));
	lua_pop(interp, 1);
	return ret;
}

// dont need explicit dispose methods for structures, callbacks or functions
struct StrStruct {
	CInvContext *ctx;
	CInvStructure *st;
};
int _cstructure_gc(Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]) {
	struct StrStruct *st = touserdata(interp, 1);
	if (st->ctx) {
		if (st->st) {
			cinv_structure_delete(st->ctx, st->st);
			st->st = NULL;
		}
		cinv_context_delete(st->ctx);
		st->ctx = NULL;
	}
	return 0;
}

int _cstructure_new(Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]) {
	struct StrStruct *ptr;
	CInvContext *ctx;
	CInvStructure *st;
	int numargs, i;
	int size;

	ctx = cinv_context_create();
	if (ctx == NULL) {
		Tcl_AppendResult(interp, "out of memory");
		return TCL_ERROR;
	}
	
	st = cinv_structure_create(ctx);
	if (!st) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
		cinv_context_delete(ctx);
		return TCL_ERROR;
	}

	numargs = objc;
	if ((numargs % 2) != 0) {
		Tcl_AppendResult(interp,
			"usage: cstructure.new(type, name[, type, name ...]");
		goto error;
	}

	for (i = 1; i <= numargs; i += 2) {
		const char *family;

		if (isarray(interp, i)) {
			Tcl_AppendResult(interp, "array structure members not supported");
			return TCL_ERROR;
		}

		lua_getfield(interp, i, "family");
		family = tostring(interp, -1);
		if (!strcmp(family, "void")) {
			Tcl_AppendResult(interp, "void is not a type");
			goto error;
		} else if (!strcmp(family, "callback")) {
			Tcl_AppendResult(interp, "callbacks cannot be struct members");
			goto error;
		} else if (!strcmp(family, "string")) {
			if (!cinv_structure_addmember_value(ctx, st,
				tostring(interp, i + 1), CINV_T_PTR)) {
				Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
				goto error;
			}
		} else if (!strcmp(family, "struct")) {
			struct StrStruct *mem;
			lua_getfield(interp, i, "ud");
			mem = touserdata(interp, -1);
			if (!cinv_structure_addmember_struct(ctx, st,
				tostring(interp, i + 1), mem->st)) {
				Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
				goto error;
			}
			lua_pop(interp, 1);
		} else if (!strcmp(family, "basic")) {
			lua_Integer id;
			lua_getfield(interp, i, "id");
			id = tointeger(interp, -1);
			if (!cinv_structure_addmember_value(ctx, st,
				tostring(interp, i + 1), (cinv_type_t)id)) {
				Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
				goto error;
			}
			lua_pop(interp, 1);
		} else {
			Tcl_AppendResult(interp, "unknown family");
			goto error;
		}
		lua_pop(interp, 1);
	} 

	if (!cinv_structure_finish(ctx, st)) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
		goto error;
	}

	lua_newtable(l); // return value
	// we dont need a metatable for this because there are no methods
	Tcl_AppendResult(interp, "struct");
	lua_setfield(interp, -2, "family");

	lua_newtable(l); // copy arguments to members table
	for (i = 1; i <= numargs; i += 2) {
		lua_pushvalue(interp, i + 1);
		lua_pushvalue(interp, i);
		lua_settable(interp, -3);
	}
	lua_setfield(interp, -2, "members");

	if (!cinv_structure_getsize(ctx, st, &size)) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
		goto error;
	}
	lua_pushinteger(interp, size);
	lua_setfield(interp, -2, "size");

	ptr = lua_newuserdata(interp, sizeof(struct StrStruct));
	ptr->ctx = ctx;
	ptr->st = st;
	lua_newtable(l); // metatable for userdata
	lua_pushcfunction(interp, _cstructure_gc);
	lua_setfield(interp, -2, "__gc");
	lua_setmetatable(interp, -2);

	lua_setfield(interp, -2, "ud"); // set userdata to field
	
	return 1;

error:
	cinv_structure_delete(ctx, st);
	cinv_context_delete(ctx);
	return TCL_ERROR;
	return 0; // never executed
}

// helper for parsefunction
char getcode(Tcl_Interp* interp, const char *family, int tblindex, int callback) {
	if (isarray(interp, tblindex)) {
		if (callback) {
			Tcl_AppendResult(interp, "callbacks cannot accept arrays as arguments");
			return (char)-1;
		}
		return 'p';
	}

	if (!strcmp(family, "struct")) {
		Tcl_AppendResult(interp,
			"passing or returning a struct by value is not yet supported");
		return (char)-1;
	} else if (!strcmp(family, "string")) {
		return 'p';
	} else if (!strcmp(family, "callback")) {
		if (callback) {
			Tcl_AppendResult(interp, "callbacks cannot accept callbacks as arguments");
			return (char)-1;
		}
		return 'p';
	} else if (!strcmp(family, "basic")) {
		lua_Integer charid;
		lua_getfield(interp, tblindex, "charid");
		charid = tointeger(interp, -1);
		lua_pop(interp, 1);
		return (char)charid;
	} else {
		Tcl_AppendResult(interp, "unknown family");
		return (char)-1;
	}
}

CInvFunction *parsefunction(Tcl_Interp* interp, CInvContext *ctx, int startarg,
	int callback, cinv_callconv_t cc) {
	CInvFunction *ret;
	char retfmt[2];
	char *parmfmt;
	int i;
	int numparms = objc - startarg - 1;
	const char *family;

	if (lua_isnil(interp, startarg)) {
		Tcl_AppendResult(interp, "got nil value instead of type");
		return TCL_ERROR;
	}
	lua_getfield(interp, startarg, "family");
	if (lua_isnil(interp, -1)) {
		Tcl_AppendResult(interp, "invalid type");
		return TCL_ERROR;
	}
	lua_pop(interp, 1);
	if (isarray(interp, startarg)) {
		Tcl_AppendResult(interp, "returning arrays not supported");
		return TCL_ERROR;
	}
	if (iscallback(interp, startarg)) {
		Tcl_AppendResult(interp, "returning callbacks not supported");
		return TCL_ERROR;
	}
	if (callback && isstring(interp, startarg)) {
		Tcl_AppendResult(interp, "callbacks cannot return strings");
		return TCL_ERROR;
	}

	lua_getfield(interp, startarg, "family");
	family = tostring(interp, -1);
	if (!strcmp("void", family))
		retfmt[0] = '\0';
	else {
		retfmt[0] = getcode(interp, family, startarg, callback);
		if (retfmt[0] == (char)-1)
			return TCL_ERROR;
		retfmt[1] = '\0';
	}
	lua_pop(interp, 1);

	parmfmt = malloc(numparms + 1);
	if (!parmfmt) {
		Tcl_AppendResult(interp, "out of memory");
		return TCL_ERROR;
	}
	for (i = startarg + 2; i < (numparms + startarg + 2); i++) {
		if (lua_isnil(interp, i)) {
			Tcl_AppendResult(interp, "got nil value instead of type");
			return TCL_ERROR;
		}
		lua_getfield(interp, i, "family");
		if (lua_isnil(interp, -1)) {
			Tcl_AppendResult(interp, "invalid type");
			return TCL_ERROR;
		}
		family = tostring(interp, -1);
		lua_pop(interp, 1);
		if (!strcmp("void", family)) {
			// only put up with void if it is the first and only parameter
			if (i == numparms && i == (startarg + 2)) {
				parmfmt[0] = '\0';
			} else {
				free(parmfmt);
				Tcl_AppendResult(interp, "void is an invalid parameter type");
				return TCL_ERROR;
			}
		} else {
			parmfmt[i - (startarg + 2)] = getcode(interp, family, i, callback);
			if (parmfmt[i - (startarg + 2)] == (char)-1) {
				free(parmfmt);
				return TCL_ERROR;
			}
		}
	}
	parmfmt[i - (startarg + 2)] = '\0';

	ret = cinv_function_create(ctx, cc, retfmt, parmfmt);
	if (!ret) {
		free(parmfmt);
		Tcl_AppendResult(interp, cinv_context_geterrormsg(ctx));
		return TCL_ERROR;
	}

	free(parmfmt);
	return ret;
}

int get_arrelement_size(Tcl_Interp* interp, int type) {
	int ret;
	lua_getfield(interp, type, "size");
	ret = (int)tointeger(interp, -1);
	lua_pop(interp, 1);
	return ret;
}
void push_ptr_val(Tcl_Interp* interp, void *ptr) {
	lua_pushlightuserdata(interp, ptr);
}
void *get_ptr_val(Tcl_Interp* interp, Tcl_Obj *objv) {
	void *ret;
	const char *pstr;
	if (lua_isnil(interp, index))
		return 0;
	if(lua_isuserdata(interp, index))
		return lua_touserdata(interp, index);
	pstr = lua_tostring(interp, index);
	if (strlen(pstr) == 0) return 0;
	if (strlen(pstr) == 1 ||
		strncmp("0x", pstr, 2)) {
		char *endptr;
		ret = (void *)strtol(pstr, &endptr, 16);
		if (*endptr != '\0') {
			Tcl_AppendResult(interp, "invalid pointer value");
			return TCL_ERROR;
		}
	} else {
		if (sscanf(pstr, "%p", &ret) == 0) {
			Tcl_AppendResult(interp, "invalid pointer value");
			return TCL_ERROR;
		}
	}
	return ret;
}


void *allocreturn(Tcl_Interp* interp, int typeindex) {
	int sz = get_arrelement_size(interp, typeindex);
	void *ret;

	ret = malloc(sz);
	if (!ret) {
		Tcl_AppendResult(interp, "out of memory");
		return TCL_ERROR;
	}
	return ret;
}
void marshal_callback(Tcl_Interp* interp, void *ret, int argindex) {
	if (lua_isnil(interp, argindex))
		*(void **)ret = NULL;
	else {
		lua_getfield(interp, argindex, "ep");
		*(void **)ret = tolightuserdata(interp, -1);
		lua_pop(interp, 1);
	}
}
void marshal_string(Tcl_Interp* interp, void *ret, int argindex) {
	if (lua_isnil(interp, argindex))
		*(const char **)ret = NULL;
	else
		*(const char **)ret = tostring(interp, argindex);
}
void marshal_basic(Tcl_Interp* interp, void *ret, int typeindex, int argindex) {
	cinv_type_t id;
	int isnil;

	lua_getfield(interp, typeindex, "id");
	id = (cinv_type_t)tointeger(interp, -1);
	lua_pop(interp, 1);

	isnil = lua_isnil(interp, argindex);
	if (isnil && id != CINV_T_PTR) {
		Tcl_AppendResult(interp, "cannot convert nil to a number");
		return TCL_ERROR;
	}

	switch (id) {
	case CINV_T_CHAR:
		*(char*)ret = tostring(interp, argindex)[0];
		break;
	case CINV_T_SHORT:
		*(short*)ret = (short)tointeger(interp, argindex);
		break;
	case CINV_T_INT:
		*(int*)ret = (int)tointeger(interp, argindex);
		break;
	case CINV_T_LONG:
		*(long*)ret = (long)tonumber(interp, argindex);
		break;
	case CINV_T_EXTRALONG:
		*(long long*)ret = (long long)tonumber(interp, argindex);
		break;
	case CINV_T_FLOAT:
		*(float*)ret = (float)tonumber(interp, argindex);
		break;
	case CINV_T_DOUBLE:
		*(double*)ret = (double)tonumber(interp, argindex);
		break;
	case CINV_T_PTR:
		*(void**)ret = get_ptr_val(interp, argindex);
		break;
	default:
		Tcl_AppendResult(interp, "unknown type");
		return TCL_ERROR;
	}
}
void marshal_struct(Tcl_Interp* interp, void *ret, int typeindex, int argindex) {
	struct StrStruct *st;
	int members;
	
	if (lua_isnil(interp, argindex)) {
		Tcl_AppendResult(interp, "structs cannot be nil");
		return TCL_ERROR;
	}

	lua_getfield(interp, typeindex, "ud");
	st = touserdata(interp, -1);
	lua_pop(interp, 1);

	lua_getfield(interp, typeindex, "members");
	members = objc;

	lua_pushnil(l);
	while (lua_next(interp, members) != 0) {
		const char *key = tostring(interp, -2);
		int type = objc;
		void *val;

		val = malloc(get_arrelement_size(interp, type));
		if (!val) {
			Tcl_AppendResult(interp, "out of memory");
			return TCL_ERROR;
		}

		lua_getfield(interp, argindex, key);
		if (isstruct(interp, type))
			marshal_struct(interp, val, type, objc);
		if (isstring(interp, type))
			marshal_string(interp, val, objc);
		else
			marshal_basic(interp, val, type, objc);

		if (!cinv_structure_instance_setvalue(st->ctx, st->st,
			ret, key, val)) {
			free(val);
			Tcl_AppendResult(interp, cinv_context_geterrormsg(st->ctx));
			return TCL_ERROR;
		}
		free(val);
		lua_pop(interp, 2); // pop the value and field
	}

	lua_pop(interp, 1);
}
void *marshal_param(Tcl_Interp* interp, int typeindex, int argindex) {
	void *ret = NULL;
	if (isarray(interp, typeindex)) {
		char **ptr = malloc(sizeof(char *));
		if (ptr) {
			if (lua_isnil(interp, argindex))
				*ptr = NULL;
			else {
				int arrlen = (int)lua_objlen(interp, argindex);
				int elsize = get_arrelement_size(interp, typeindex);
				*ptr = malloc(arrlen * elsize);
				if (*ptr) {
					int i;
					char *tmp = *ptr;

					for (i = 1; i <= arrlen; i++) {
						lua_pushinteger(interp, i);
						lua_gettable(interp, argindex);

						if (lua_isnil(interp, -1)) {
							lua_pop(interp, 1);
							break;
						}

						if (isstruct(interp, typeindex))
							marshal_struct(interp, tmp, typeindex, objc);
						else if (isstring(interp, typeindex))
							marshal_string(interp, tmp, objc);
						else
							marshal_basic(interp, tmp, typeindex, objc);
									
						lua_pop(interp, 1);
					
						tmp += elsize;
					}

					ret = ptr;
				} else
					free(ptr);
			}
		}
	} else {
		ret = malloc(get_arrelement_size(interp, typeindex));
		if (ret) {
			if (isstring(interp, typeindex))
				marshal_string(interp, ret, argindex);
			else if (iscallback(interp, typeindex))
				marshal_callback(interp, ret, argindex);
			else
				marshal_basic(interp, ret, typeindex, argindex);
		}
	}
	if (!ret) {
		Tcl_AppendResult(interp, "out of memory");
		return TCL_ERROR;
	}
	return ret;
}
void free_param(Tcl_Interp* interp, int typeindex, void *value) {
	if (isarray(interp, typeindex)) {
		char **arr = value;
		free(*arr);
		free(arr);
	} else
		free(value);
}
void unmarshal_retval(Tcl_Interp* interp, int typeindex, void *value) {
	// can be basic or string
	// push completed value on stack
	int famindex;
	lua_getfield(interp, typeindex, "family");
	famindex = objc;
	if (!strcmp("basic", tostring(interp, famindex))) {
		char strbuf[2];
		int idindex;
		lua_getfield(interp, typeindex, "id");
		idindex = objc;
		switch ((cinv_type_t)tointeger(interp, idindex)) {
		case CINV_T_CHAR:
			strbuf[0] = *(char *)value;
			strbuf[1] = '\0';
			lua_pushlstring(interp, strbuf, 1);
			break;
		case CINV_T_SHORT:
			lua_pushinteger(interp, *(short *)value);
			break;
		case CINV_T_INT:
			lua_pushinteger(interp, *(int *)value);
			break;
		case CINV_T_LONG:
			lua_pushnumber(interp, *(long *)value);
			break;
		case CINV_T_EXTRALONG:
			lua_pushnumber(interp, (lua_Number)*(long long *)value);
			break;
		case CINV_T_FLOAT:
			lua_pushnumber(interp, *(float *)value);
			break;
		case CINV_T_DOUBLE:
			lua_pushnumber(interp, *(double *)value);
			break;
		case CINV_T_PTR:
			push_ptr_val(interp, *(void **)value);
			break;
		default:
			Tcl_AppendResult(interp, "unknown type");
			return TCL_ERROR;
			break;
		}
		lua_remove(interp, idindex);
	} else
		Tcl_AppendResult(interp, *(char **)value);
	lua_remove(interp, famindex);
}
void unmarshal_struct(Tcl_Interp* interp, int typeindex, void *instance) {
	struct StrStruct *st;
	int members, ret;

	lua_newtable(l);
	ret = objc;

	lua_getfield(interp, typeindex, "ud");
	st = touserdata(interp, -1);
	lua_pop(interp, 1);

	lua_getfield(interp, typeindex, "members");
	members = objc;

	lua_pushnil(l);
	while (lua_next(interp, members) != 0) {
		const char *key = tostring(interp, -2);
		int type = objc;
		
		void *val = cinv_structure_instance_getvalue(st->ctx, st->st,
			instance, key);
		if (val == NULL) {
			Tcl_AppendResult(interp, cinv_context_geterrormsg(st->ctx));
			return TCL_ERROR;
		}

		if (isstruct(interp, type))
			unmarshal_struct(interp, type, val);
		else
			unmarshal_retval(interp, type, val);

		lua_setfield(interp, ret, key);

		lua_pop(interp, 1); // pop the value
	}

	lua_remove(interp, members);
}
void unmarshal_array(Tcl_Interp* interp, int typeindex, void *value, int outindex) {
	char *cptr = *(char **)value;
	int arrlen = (int)lua_objlen(interp, outindex), i;
	int elsize = get_arrelement_size(interp, typeindex);

	for (i = 1; i <= arrlen; i++) {
		lua_pushinteger(interp, i);

		if (isstruct(interp, typeindex))
			unmarshal_struct(interp, typeindex, cptr);
		else
			unmarshal_retval(interp, typeindex, cptr);

		lua_settable(interp, outindex);
		cptr += elsize;
	}
}

struct CBStruct {
	CInvCallback *cb;
	CInvFunction *func;
	CInvContext *ctx;
	lua_Integer key;
	Tcl_Interp* interp;
};

int _ccallback_gc(Tcl_Interp* interp) {
	struct CBStruct *cb = touserdata(interp, 1);

	lua_pushinteger(interp, cb->key);
	lua_pushnil(l);
	lua_settable(interp, LUA_GLOBALSINDEX);

	if (cb->cb) {
		cinv_callback_delete(cb->ctx, cb->cb);
		cb->cb = NULL;
	}
	if (cb->func) {
		cinv_function_delete(cb->ctx, cb->func);
		cb->func = NULL;
	}

	return 0;
}

void _ccallback_invoked(CInvFunction *f, void *parameters[],
	void *returnout, void *userdata) {
	struct CBStruct *cb = userdata;
	int usertable, pindex, index, retindex;
	int numargs, i;

	lua_pushinteger(cb->l, cb->key);
	lua_gettable(cb->l, LUA_GLOBALSINDEX);
	usertable = lua_gettop(cb->l);
	
	if (lua_isnil(cb->l, usertable)) {
		Tcl_AppendResult(cb->l,
			"C callback being called for an object which has been collected");
		lua_error(cb->l);
	}
	lua_getfield(cb->l, usertable, "cbfunc");

	lua_getfield(cb->l, usertable, "params");
	pindex = lua_gettop(cb->l);
	numargs = (int)lua_objlen(cb->l, pindex);

	for (i = 0; i < numargs; i++) {
		lua_pushinteger(cb->l, i + 1);
		lua_gettable(cb->l, pindex);
		index = lua_gettop(cb->l);

		unmarshal_retval(cb->l, index, parameters[i]);

		lua_remove(cb->l, index);
	}
	lua_remove(cb->l, pindex);

	lua_call(cb->l, numargs, 1);
	retindex = lua_gettop(cb->l);

	lua_getfield(cb->l, usertable, "return");
	index = lua_gettop(cb->l);
	if (!isvoid(cb->l, index)) {
		marshal_basic(cb->l, returnout, index, retindex);
	}
	lua_remove(cb->l, index);

	lua_pop(cb->l, 2); // return value and usertable
}

int _clibrary_new_callback(Tcl_Interp* interp) {
	struct CBStruct *cbs;
	struct LibStruct *lib;
	CInvFunction *func;
	CInvCallback *cb;
	int i;
	void *ep;
	int retval;
	int numargs = objc;
	if (numargs < 3) {
		Tcl_AppendResult(interp, "usage: clibrary:new_callback(rettype, cbfunc, ...)");
		return TCL_ERROR;
	}

	lua_getfield(interp, 1, "ud");
	lib = touserdata(interp, -1);
	lua_pop(interp, 1);

	func = parsefunction(interp, lib->ctx, 2, 1, lib->cc);

	lua_newtable(l);
	retval = objc;

	cbs = lua_newuserdata(interp, sizeof(struct CBStruct));
	cbs->func = func;
	cbs->ctx = lib->ctx;
	cbs->l = l;
	
	cb = cinv_callback_create(lib->ctx, func, cbs, _ccallback_invoked);
	if (!cb) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(lib->ctx));
		cinv_function_delete(lib->ctx, func);
		return TCL_ERROR;
	}
	cbs->cb = cb;
	cbs->key = time(NULL);

	while (1) {
		lua_pushinteger(interp, cbs->key);
		lua_gettable(interp, LUA_GLOBALSINDEX);
		if (!lua_isnil(interp, -1)) {
			lua_pop(interp, 1);
			cbs->key++;
			continue;
		}
		lua_pop(interp, 1);
		lua_pushinteger(interp, cbs->key);
		lua_pushvalue(interp, retval);
		lua_settable(interp, LUA_GLOBALSINDEX);
		break;
	}

	lua_newtable(l);
	lua_pushcfunction(interp, _ccallback_gc);
	lua_setfield(interp, -2, "__gc");
	lua_setmetatable(interp, -2);
	lua_setfield(interp, -2, "ud");

	ep = cinv_callback_getentrypoint(lib->ctx, cb);
	if (!ep) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(lib->ctx));
		return TCL_ERROR;
	}
	lua_pushlightuserdata(interp, ep);
	lua_setfield(interp, -2, "ep");

	lua_pushvalue(interp, 2);
	lua_setfield(interp, -2, "return");
	
	lua_newtable(l);
	for (i = 4; i <= numargs; i++) {
		lua_pushinteger(interp, i - 3);
		lua_pushvalue(interp, i);
		if (isvoid(interp, objc))
			lua_pop(interp, 2);
		else
			lua_settable(interp, -3);
	}
	lua_setfield(interp, -2, "params");

	lua_pushvalue(interp, 3);
	lua_setfield(interp, -2, "cbfunc");

	lua_pushvalue(interp, 1);
	lua_setfield(interp, -2, "lib");

	return 1;
}

int _cinv_ptr_to_callback(Tcl_Interp* interp) {
	void *p;
	if (objc != 1) {
		Tcl_AppendResult(interp, "usage: cinv.ptr_to_callback(cptr)");
		return TCL_ERROR;
	}

	p = get_ptr_val(interp, 1);

	lua_newtable(l);
	lua_pushlightuserdata(interp, p);
	lua_setfield(interp, -2, "ep");

	return 1;
}
int _cinv_callback_to_ptr(Tcl_Interp* interp) {
	void *p;
	if (objc != 1) {
		Tcl_AppendResult(interp, "usage: cinv.callback_to_ptr(callback)");
		return TCL_ERROR;
	}

	lua_getfield(interp, 1, "ep");
	p = tolightuserdata(interp, -1);
	lua_pop(interp, 1);

	push_ptr_val(interp, p);

	return 1;
}

struct FunStruct {
	void *ep;
	CInvFunction *func;
	CInvContext *ctx;
};

int _function_gc(Tcl_Interp* interp) {
	struct FunStruct *fs = touserdata(interp, 1);
	if (fs->func) {
		cinv_function_delete(fs->ctx, fs->func);
		fs->func = NULL;
	}
	return 0;
}

int _function_call(Tcl_Interp* interp) {
	struct FunStruct *fs;
	void *returnval = NULL;
	void **parameters = NULL;
	int numargs = 0;
	int numreturn = 0;
	int index, pindex, i;

	lua_getfield(interp, 1, "ud");
	fs = touserdata(interp, -1);
	lua_pop(interp, 1);

	lua_getfield(interp, 1, "return");
	index = objc;
	if (!isvoid(interp, index))
		returnval = allocreturn(interp, index);
	lua_remove(interp, index);

	lua_getfield(interp, 1, "params");
	pindex = objc;
	numargs = (int)lua_objlen(interp, pindex);
	if (numargs > 0) {
		parameters = malloc(numargs * sizeof(void*));
		for (i = 0; i < numargs; i++) {
			lua_pushinteger(interp, i + 1);
			lua_gettable(interp, pindex);

			index = objc;
			
			parameters[i] = marshal_param(interp, index, i + 2);

			lua_remove(interp, index);
		}
	}
	lua_remove(interp, pindex);

	cinv_function_invoke(fs->ctx, fs->func, fs->ep,
		returnval, parameters);
	
	lua_getfield(interp, 1, "params");
	pindex = objc;
	for (i = 0; i < numargs; i++) {
		lua_pushinteger(interp, i + 1);
		lua_gettable(interp, pindex);

		index = objc;
		lua_getfield(interp, index, "array");
		if (!lua_isnil(interp, -1))
			unmarshal_array(interp, index, parameters[i], i + 2);
		lua_pop(interp, 1);

		free_param(interp, index, parameters[i]);

		lua_remove(interp, index);
	}
	lua_remove(interp, pindex);

	if (returnval != NULL) {
		lua_getfield(interp, 1, "return");
		index = objc;
		unmarshal_retval(interp, index, returnval);
		numreturn++;
		lua_remove(interp, index);
	}

	free(parameters);
	free(returnval);

	return numreturn;
}

int _clibrary_get_function(Tcl_Interp* interp) {
	struct FunStruct *fs;
	struct LibStruct *lib;
	CInvFunction *func;
	void *ep;
	int i;
	int numargs = objc;
	if (numargs < 3) {
		Tcl_AppendResult(interp, "usage: clibrary:get_function(rettype, name, ...)");
		return TCL_ERROR;
	}

	lua_getfield(interp, 1, "ud");
	lib = touserdata(interp, -1);
	lua_pop(interp, 1);

	func = parsefunction(interp, lib->ctx, 2, 0, lib->cc);

	ep = cinv_library_load_entrypoint(lib->ctx, lib->lib, tostring(interp, 3));
	if (!ep) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(lib->ctx));
		cinv_function_delete(lib->ctx, func);
		return TCL_ERROR;
	}
	
	lua_newtable(l);

	lua_newtable(l);
	lua_pushcfunction(interp, _function_call);
	lua_setfield(interp, -2, "__call");
	lua_setmetatable(interp, -2);

	fs = lua_newuserdata(interp, sizeof(struct FunStruct));
	fs->ep = ep;
	fs->func = func;
	fs->ctx = lib->ctx;
	lua_newtable(l);
	lua_pushcfunction(interp, _function_gc);
	lua_setfield(interp, -2, "__gc");
	lua_setmetatable(interp, -2);
	lua_setfield(interp, -2, "ud");

	lua_pushvalue(interp, 2);
	lua_setfield(interp, -2, "return");
	
	lua_newtable(l);
	for (i = 4; i <= numargs; i++) {
		lua_pushinteger(interp, i - 3);
		lua_pushvalue(interp, i);
		if (isvoid(interp, objc))
			lua_pop(interp, 2);
		else
			lua_settable(interp, -3);
	}
	lua_setfield(interp, -2, "params");

	// maintain a reference to parent library
	lua_pushvalue(interp, 1);
	lua_setfield(interp, -2, "lib");

	return 1;
}

int _clibrary_tie_init(Tcl_Interp* interp) {
	struct LibStruct *lib;
	void *ep;

	if (objc != 3) {
		Tcl_AppendResult(interp, "usage: clibrary:tie_init(type, name)");
		return TCL_ERROR;
	}

	lua_getfield(interp, 1, "ud");
	lib = touserdata(interp, -1);
	lua_pop(interp, 1);

	if (isvoid(interp, 2)) {
		Tcl_AppendResult(interp, "void is not a type");
		return TCL_ERROR;
	}
	if (isarray(interp, 2)) {
		Tcl_AppendResult(interp, "tied variables cannot be arrays");
		return TCL_ERROR;
	}
	if (!isbasic(interp, 2)) {
		Tcl_AppendResult(interp,
			"tied variables cannot be strings, structs or callbacks");
		return TCL_ERROR;
	}

	lua_getfield(interp, 1, "ties");
	lua_getfield(interp, -1, tostring(interp, 3));
	if (!lua_isnil(interp, -1)) {
		Tcl_AppendResult(interp, "a tied variable with that name already exists");
		return TCL_ERROR;
	}
	lua_pop(interp, 1);

	ep = cinv_library_load_entrypoint(lib->ctx, lib->lib, tostring(interp, 3));
	if (!ep) {
		Tcl_AppendResult(interp, cinv_context_geterrormsg(lib->ctx));
		return TCL_ERROR;
	}
	
	lua_newtable(l);
	lua_pushvalue(interp, 2);
	lua_setfield(interp, -2, "type");
	lua_pushlightuserdata(interp, ep);
	lua_setfield(interp, -2, "ep");

	lua_setfield(interp, -2, lua_tostring(interp, 3));
	lua_pop(interp, 1);

	return 0;
}
int _clibrary_tie_get(Tcl_Interp* interp) {
	void *ep;

	if (objc != 2) {
		Tcl_AppendResult(interp, "usage: clibrary:tie_get(name)");
		return TCL_ERROR;
	}

	lua_getfield(interp, 1, "ties");
	lua_pushvalue(interp, 2);
	lua_gettable(interp, -2);
	if (lua_isnil(interp, -1)) {
		Tcl_AppendResult(interp, "a tied variable with that name was not found");
		return TCL_ERROR;
	}
	lua_getfield(interp, -1, "ep");
	ep = tolightuserdata(interp, -1);
	lua_pop(interp, 1);

	lua_getfield(interp, -1, "type");
	unmarshal_retval(interp, objc, ep);
	lua_remove(interp, -2);
	lua_remove(interp, -2);
	lua_remove(interp, -2);
	
	return 1;
}
int _clibrary_tie_set(Tcl_Interp* interp) {
	void *ep;

	if (objc != 3) {
		Tcl_AppendResult(interp, "usage: clibrary:tie_set(name, value)");
		return TCL_ERROR;
	}

	lua_getfield(interp, 1, "ties");
	lua_pushvalue(interp, 2);
	lua_gettable(interp, -2);
	if (lua_isnil(interp, -1)) {
		Tcl_AppendResult(interp, "a tied variable with that name was not found");
		return TCL_ERROR;
	}
	lua_getfield(interp, -1, "ep");
	ep = tolightuserdata(interp, -1);
	lua_pop(interp, 1);

	lua_getfield(interp, -1, "type");
	marshal_basic(interp, ep, objc, 3);
	lua_pop(interp, 3);
	
	return 0;
}

void declbasic(Tcl_Interp* interp, const char *g,
	cinv_type_t id, char charid, int size) {
	lua_newtable(l);
	Tcl_AppendResult(interp, "basic");
	lua_setfield(interp, -2, "family");
	lua_pushinteger(interp, (lua_Integer)id);
	lua_setfield(interp, -2, "id");
	lua_pushinteger(interp, (lua_Integer)charid);
	lua_setfield(interp, -2, "charid");
	lua_pushinteger(interp, (lua_Integer)size);
	lua_setfield(interp, -2, "size");
	lua_setglobal(interp, g);
}

void declfunc(Tcl_Interp* interp, const char *name, lua_CFunction func) {
	lua_pushcfunction(interp, func);
	lua_setfield(interp, -2, name);
}

int _cinv_string_to_chararray(Tcl_Interp* interp) {
	size_t len, i, end;
	int includenil = 1;
	const char *s;
	if (objc < 1 || objc > 2) {
		Tcl_AppendResult(interp,
			"usage: cinv.string_to_chararray(string[, includenil])");
		return TCL_ERROR;
	}
	if (objc == 2)
		includenil = toboolean(interp, 2);

	s = tolstring(interp, 1, &len);
	
	lua_newtable(l);
	end = len + 1;
	if (!includenil)
		end = len;
	for (i = 0; i < len; i++) {
		lua_pushinteger(interp, i + 1);
		lua_pushlstring(interp, s + i, 1);
		lua_settable(interp, -3);
	}

	return 1;
}

int _cinv_chararray_to_string(Tcl_Interp* interp) {
	size_t len, i;
	if (objc < 1 || objc > 2) {
		Tcl_AppendResult(interp, "usage: cinv.chararray_to_string(carray[, len])");
		return TCL_ERROR;
	}
	if (objc == 2) {
		len = lua_tointeger(interp, 2);
		if (len < 0) {
			Tcl_AppendResult(interp, "invalid length");
			return TCL_ERROR;
		}
	} else {
		len = lua_objlen(interp, 1);
	}
	if (len == 0) {
		lua_pushlstring(interp, "", len);
	} else {
		char *ret = malloc(len + 1);
		if (!ret) {
			Tcl_AppendResult(interp, "out of memory");
			return TCL_ERROR;
		}
		for (i = 0; i < len; i++) {
			lua_pushinteger(interp, i + 1);
			lua_gettable(interp, 1);

			if (lua_isnil(interp, -1)) {
				lua_pop(interp, 1);
				break;
			}
			ret[i] = tostring(interp, -1)[0];
			lua_pop(interp, 1);
		}

		ret[len] = '\0';
		lua_pushlstring(interp, ret, len);
		free(ret);
	}
	
	return 1;
}

int _cinv_ptr_to_string(Tcl_Interp* interp) {
	char *ptr;
	int len = -1;
	if (objc != 1 && objc != 2) {
		Tcl_AppendResult(interp, "usage: cinv.ptr_to_string(cptr[, len])");
		return TCL_ERROR;
	}
	ptr = get_ptr_val(interp, 1);
	if (!objc == 2) {
		len = (int)tointeger(interp, 2);
		if (len < 0) {
			Tcl_AppendResult(interp, "invalid length parameter");
			return TCL_ERROR;
		}
	}
	if (ptr == NULL)
		lua_pushnil(l);
	else if (len != -1)
		lua_pushlstring(interp, ptr, len);
	else
		Tcl_AppendResult(interp, ptr);
	return 1;
}

int _cinv_ptr_to_array(Tcl_Interp* interp) {
	int len, i, index;
	void *value;
	if (objc != 3) {
		Tcl_AppendResult(interp, "usage: cinv.ptr_to_array(cptr, arrtype, len)");
		return TCL_ERROR;
	}
	
	if (!isstruct(interp, 2) &&
		!isstring(interp, 2) &&
		!isbasic(interp, 2)) {
		Tcl_AppendResult(interp, "type is not a valid array type");
		return TCL_ERROR;
	}
	
	value = get_ptr_val(interp,  1);
	if (value == NULL)
		lua_pushnil(l);
	else {
		len = (int)tointeger(interp, 3);
		// create table of desired len and call unmarshal_array
		lua_newtable(l);
		index = objc;
		for (i = 1; i <= len; i++) {
			lua_pushinteger(interp, i);
			Tcl_AppendResult(interp, "");
			lua_settable(interp, index);
		}
		unmarshal_array(interp, 2, &value, index);
	}
	return 1;
}

int _cinv_ptr_to_struct(Tcl_Interp* interp) {
	void *ptr;
	if (objc != 2) {
		Tcl_AppendResult(interp, "usage: cinv.ptr_to_struct(cptr, type)");
		return TCL_ERROR;
	}
	ptr = get_ptr_val(interp, 1);
	if (ptr == NULL)
		lua_pushnil(l);
	else
		unmarshal_struct(interp, 2, ptr);
	return 1;
}

int _cinv_sizeof(Tcl_Interp* interp) {
	if (objc != 1) {
		Tcl_AppendResult(interp, "usage: cinv.sizeof(type)");
		return TCL_ERROR;
	}
	if (isvoid(interp, 1)) {
		Tcl_AppendResult(interp, "void is not a type");
		return TCL_ERROR;
	}
	if (isarray(interp, 1)) {
		Tcl_AppendResult(interp, "cannot get size of array types");
		return TCL_ERROR;
	}
	
	lua_pushinteger(interp, get_arrelement_size(interp, 1));
	return 1;
}
int _cinv_ptr_incr(Tcl_Interp* interp) {
	char *ptr;
	int num;
	if (objc != 2) {
		Tcl_AppendResult(interp, "usage: cinv.ptr_incr(cptr, numbytes)");
		return TCL_ERROR;
	}

	num = (int)tointeger(interp, 2);
	ptr = get_ptr_val(interp, 1);

	ptr += num;
	push_ptr_val(interp, ptr);

	return 1;
}

int _cinv_array(Tcl_Interp* interp) {
	if (objc != 1) {
		Tcl_AppendResult(interp, "usage: cinv.array(arrtype)");
		return TCL_ERROR;
	}
	
	if (isarray(interp, 1)) {
		Tcl_AppendResult(interp, "arrays of arrays not supported");
		return TCL_ERROR;
	}
	if (isvoid(interp, 1)) {
		Tcl_AppendResult(interp, "void is not a type");
		return TCL_ERROR;
	}
	if (iscallback(interp, 1)) {
		Tcl_AppendResult(interp, "arrays of callbacks not supported");
		return TCL_ERROR;
	}
	
	lua_newtable(l);
	lua_pushnil(l);
	while (lua_next(interp, 1) != 0) {
		lua_pushvalue(interp, -2);
		lua_pushvalue(interp, -2);
		lua_settable(interp, -5);
		lua_pop(interp, 1);
	}

	Tcl_AppendResult(interp, "yes");
	lua_setfield(interp, -2, "array");

	return 1;
}

DLLEXPORT int luaopen_cinvoke_lua(Tcl_Interp* interp) {
	declbasic(interp, "Cchar", CINV_T_CHAR, 'c', 1);
	declbasic(interp, "Cshort", CINV_T_SHORT, 's', sizeof(short));
	declbasic(interp, "Cint", CINV_T_INT, 'i', sizeof(int));
	declbasic(interp, "Clong", CINV_T_LONG, 'l', sizeof(long));
	declbasic(interp, "Clonglong", CINV_T_EXTRALONG, 'e', sizeof(long long));
	declbasic(interp, "Cfloat", CINV_T_FLOAT, 'f', sizeof(float));
	declbasic(interp, "Cdouble", CINV_T_DOUBLE, 'd', sizeof(double));
	declbasic(interp, "Cptr", CINV_T_PTR, 'p', sizeof(void*));
	declbasic(interp, "Cint16", CINV_T_2BYTE, '2', 2);
	declbasic(interp, "Cint32", CINV_T_4BYTE, '4', 4);
	declbasic(interp, "Cint64", CINV_T_8BYTE, '8', 8);

	lua_newtable(l);
	Tcl_AppendResult(interp, "string");
	lua_setfield(interp, -2, "family");
	lua_pushinteger(interp, sizeof(char*));
	lua_setfield(interp, -2, "size");
	lua_setglobal(interp, "Cstring");

	lua_newtable(l);
	Tcl_AppendResult(interp, "callback");
	lua_setfield(interp, -2, "family");
	lua_pushinteger(interp, sizeof(void*));
	lua_setfield(interp, -2, "size");
	lua_setglobal(interp, "Ccallback");

	lua_newtable(l);
	Tcl_AppendResult(interp, "void");
	lua_setfield(interp, -2, "family");
	lua_setglobal(interp, "Cvoid");

	lua_newtable(l);
	declfunc(interp, "array", _cinv_array);
	declfunc(interp, "chararray_to_string", _cinv_chararray_to_string);
	declfunc(interp, "string_to_chararray", _cinv_string_to_chararray);
	declfunc(interp, "ptr_to_string", _cinv_ptr_to_string);
	declfunc(interp, "ptr_to_array", _cinv_ptr_to_array);
	declfunc(interp, "ptr_to_struct", _cinv_ptr_to_struct);
	declfunc(interp, "ptr_to_callback", _cinv_ptr_to_callback);
	declfunc(interp, "callback_to_ptr", _cinv_callback_to_ptr);
	declfunc(interp, "sizeof", _cinv_sizeof);
	declfunc(interp, "ptr_incr", _cinv_ptr_incr);
	lua_setglobal(interp, "cinv");

	lua_newtable(l);
	declfunc(interp, "new", _clibrary_new);
	declfunc(interp, "get_function", _clibrary_get_function);
	declfunc(interp, "new_callback", _clibrary_new_callback);
	declfunc(interp, "dispose", _clibrary_dispose);
	declfunc(interp, "tie_init", _clibrary_tie_init);
	declfunc(interp, "tie_get", _clibrary_tie_get);
	declfunc(interp, "tie_set", _clibrary_tie_set);
	lua_setglobal(interp, "clibrary");

	lua_newtable(l);
	declfunc(interp, "new", _cstructure_new);
	lua_setglobal(interp, "cstructure");
	
	return 0;
}

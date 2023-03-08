/*
 * Pointer is a Tcl "type" whose internal representation is stored
 * as the pointer value and an associated C pointer/handle type.
 * The Tcl_Obj.internalRep.twoPtrValue.ptr1 holds the C pointer value
 * and Tcl_Obj.internalRep.twoPtrValue.ptr2 holds a Tcl_Obj describing
 * the type. This may be NULL if no type info needs to be associated
 * with the value.
 */
 
typedef Tcl_Obj *Tclh_PointerTypeTag;
 
static void DupPointerType(Tcl_Obj *srcP, Tcl_Obj *dstP);
static void FreePointerType(Tcl_Obj *objP);
static void UpdatePointerTypeString(Tcl_Obj *objP);
static int  SetPointerFromAny(Tcl_Interp *interp, Tcl_Obj *objP);

static struct Tcl_ObjType gPointerType = {
    "Pointer",
    FreePointerType,
    DupPointerType,
    UpdatePointerTypeString,
    NULL,
};
 void *PointerValueGet(Tcl_Obj *objP) {
    return objP->internalRep.twoPtrValue.ptr1;
}
 void PointerValueSet(Tcl_Obj *objP, void *valueP) {
    objP->internalRep.twoPtrValue.ptr1 = valueP;
}
/* May return NULL */
 Tclh_PointerTypeTag PointerTypeGet(Tcl_Obj *objP) {
    return objP->internalRep.twoPtrValue.ptr2;
}
 void PointerTypeSet(Tcl_Obj *objP, Tclh_PointerTypeTag tag) {
    objP->internalRep.twoPtrValue.ptr2 = (void*)tag;
}

int Tclh_PointerTagMatch(Tclh_PointerTypeTag pointer_tag, Tclh_PointerTypeTag expected_tag)
{
    if (expected_tag == NULL)
        return 1;               /* Anything can be a void pointer */
    if (pointer_tag == NULL)
        return 0;               /* But not the other way */

    if(!strcmp("ptr", Tcl_GetString(expected_tag)))
        return 1;               /* Anything can be a unspecified pointer */
    if(!strcmp("ptr", Tcl_GetString(pointer_tag)))
        return 1;               /* This can be converted as unspecified pointer */

    if(strcmp(Tcl_GetString(pointer_tag), Tcl_GetString(expected_tag))) {
        // this is just a warning
        printf("Pointertypes diff got %s wanted %s\n",Tcl_GetString(pointer_tag), Tcl_GetString(expected_tag));
        return 0;
    }
    return 1;
}

int
PointerTypeSame(Tclh_PointerTypeTag pointer_tag,
                Tclh_PointerTypeTag expected_tag) {
    return pointer_tag == expected_tag
               ? 1
               : Tclh_PointerTagMatch(pointer_tag, expected_tag);
}

static void
FreePointerType(Tcl_Obj *objP)
{
    Tclh_PointerTypeTag tag = PointerTypeGet(objP);
    if (tag)
        Tcl_DecrRefCount(tag);
    PointerTypeSet(objP, NULL);
    PointerValueSet(objP, NULL);
    objP->typePtr = NULL;
}

static void
DupPointerType(Tcl_Obj *srcP, Tcl_Obj *dstP)
{
    Tclh_PointerTypeTag tag;
    dstP->typePtr = &gPointerType;
    PointerValueSet(dstP, PointerValueGet(srcP));
    tag = PointerTypeGet(srcP);
    if (tag)
        Tcl_IncrRefCount(tag);
    PointerTypeSet(dstP, tag);
}
static int
SetPointerFromAny(Tcl_Interp *interp, Tcl_Obj *objP)
{
    void *pv;
    Tclh_PointerTypeTag tagObj;
    char *srep;
    char *s;

    if (objP->typePtr == &gPointerType)
        return TCL_OK;

    /* Pointers are address^tag, 0 or NULL*/
    srep = Tcl_GetString(objP);
    if (sscanf(srep, "%p^", &pv) == 1) {
        s = strchr(srep, '^');
        if (s == NULL)
            goto invalid_value;
        if (s[1] == '\0')
            tagObj = NULL;
        else {
            tagObj = Tcl_NewStringObj(s + 1, -1);
            Tcl_IncrRefCount(tagObj);
        }
    }
    else {
        if (strcmp(srep, "NULL"))
            goto invalid_value;
        pv = NULL;
        tagObj = NULL;
    }

    /* OK, valid opaque rep. Convert the passed object's internal rep */
    if (objP->typePtr && objP->typePtr->freeIntRepProc) {
        objP->typePtr->freeIntRepProc(objP);
    }
    objP->typePtr = &gPointerType;
    PointerValueSet(objP, pv);
    PointerTypeSet(objP, tagObj);

    return TCL_OK;

invalid_value: /* s must point to value */
    return TCL_ERROR;
}

char *TclhPrintAddress(const void *address, char *buf, int buflen)
{
    /*
     * Note we do not sue %p here because generated output differs
     * between compilers in terms of the 0x prefix. Moreover, gcc
     * prints (nil) for NULL pointers which is not what we want.
     */
    if (sizeof(void*) == sizeof(int)) {
        unsigned int i = (unsigned int) (intptr_t) address;
        snprintf(buf, buflen, "0x%.8x", i);
    }
    else {
        unsigned long long ull = (intptr_t)address;
        snprintf(buf, buflen, "0x%.16llx", ull);
    }
    return buf;
}

static void
UpdatePointerTypeString(Tcl_Obj *objP)
{
    Tclh_PointerTypeTag tagObj;
    int len;
    int taglen;
    char *tagStr;
    char *bytes;

    tagObj = PointerTypeGet(objP);
    if (tagObj) {
        tagStr = Tcl_GetStringFromObj(tagObj, &taglen);
    }
    else {
        tagStr = "";
        taglen = 0;
    }
    /* Assume 40 bytes enough for address */
    bytes = ckalloc(40 + 1 + taglen + 1);
    (void) TclhPrintAddress(PointerValueGet(objP), bytes, 40);
    len = strlen(bytes);
    bytes[len] = '^';
    memcpy(bytes + len + 1, tagStr, taglen+1);
    objP->bytes = bytes;
    objP->length = len + 1 + taglen;
}

Tcl_Obj *
Tclh_PointerWrap(void *pointerValue, Tclh_PointerTypeTag tag)
{
    Tcl_Obj *objP;

    objP = Tcl_NewObj();
    Tcl_InvalidateStringRep(objP);
    PointerValueSet(objP, pointerValue);
    if (tag)
        Tcl_IncrRefCount(tag);
    PointerTypeSet(objP, tag);
    objP->typePtr = &gPointerType;
    return objP;
}

int
Tclh_PointerUnwrap(Tcl_Interp *interp,
                   Tcl_Obj *objP,
                   void **pvP,
                   Tclh_PointerTypeTag expected_tag)
{
    Tclh_PointerTypeTag tag;
    void *pv;

    /* Try converting Tcl_Obj internal rep */
    if (objP->typePtr != &gPointerType) {
        if (SetPointerFromAny(interp, objP) != TCL_OK) {
            Tcl_AppendResult(interp,"Pointer error with ",Tcl_GetString(objP),NULL);
            return TCL_ERROR;
        }
    }

    tag = PointerTypeGet(objP);
    pv  = PointerValueGet(objP);
    if(pv==NULL) {
        printf("Got NULL pointer\n");
        // NULL pointers should be compatible to anything
    }
    /*
    * No tag check if
    * - expected_tag is NULL or
    * - pointer is NULL AND has no tag
    * expected_tag NULL means no type check
    * NULL pointers
    */
    if (expected_tag && (pv || tag)
        && !Tclh_PointerTagMatch(tag, expected_tag)) {
        Tcl_AppendResult(interp, "Pointertypes diff got ",Tcl_GetString(tag), " wanted ",Tcl_GetString(expected_tag),NULL);
        return TCL_ERROR;
    }

    *pvP = PointerValueGet(objP);
    return TCL_OK;
}

int
Tclh_PointerObjGetTag(Tcl_Interp *interp,
                   Tcl_Obj *objP,
                   Tclh_PointerTypeTag *tagPtr)
{
    /* Try converting Tcl_Obj internal rep */
    if (objP->typePtr != &gPointerType) {
        if (SetPointerFromAny(interp, objP) != TCL_OK)
            return TCL_ERROR;
    }
    *tagPtr = PointerTypeGet(objP);
    return TCL_OK;
}
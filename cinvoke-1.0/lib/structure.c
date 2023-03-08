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
#include "cinvoke.h"
#include "cinvoke-private.h"
#include <stdlib.h>
#include <stdio.h>

CInvStructure *cinv_structure_create(CInvContext *context) {
	CInvStructure *st = malloc(sizeof(CInvStructure));
	if (!st) {
		context_set_nomem(context);
		return NULL;
	}
	
	st->members = create_hashtable(16, str_hash, str_eq);
	if (!st->members) {
		free(st);
		context_set_nomem(context);
		return NULL;
	}
	st->nextoffset = 0;
	st->finished = 0;
	st->alignment = 1;

	context_clear_error(context);
	return st;
}
cinv_status_t cinv_structure_addmember_array(CInvContext *context,
	CInvStructure *structure, const char *name, cinv_type_t type, int array_size) {
	int sz, align, dummy;
	char *namecopy;
	CInvStructMember *member;
	if (structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is already finished", 0);
		return CINV_ERROR;
	}
	if (hashtable_search(structure->members, name)) {
		context_set_error(context, CINV_E_INVAL,
			"the structure already contains a member by that name", 0);
		return CINV_ERROR;
	}

	namecopy = strdup(name);
	if (!namecopy) {
		context_set_nomem(context);
		return CINV_ERROR;
	}
	member = malloc(sizeof(CInvStructMember));
	if (!member) {
		free(namecopy);
		context_set_nomem(context);
		return CINV_ERROR;
	}

	get_size(type, &dummy, &sz, &align);
	
	sz=sz*array_size;
	
#if ARCH_CLAMP_NONFIRST_STRUCTALIGN
	if (hashtable_count(structure->members))
		align = ARCH_CLAMP_NONFIRST_STRUCTALIGN;
#endif
    structure->lastoffset=structure->nextoffset;
	if ((structure->nextoffset % align) != 0)
		structure->nextoffset += align - (structure->nextoffset % align);

	member->structtype = NULL;
	member->type = type;
	member->offset = structure->nextoffset;

	hashtable_insert(structure->members, namecopy, member);

	if (align > structure->alignment)
		structure->alignment = align;

	structure->nextoffset += sz;
	member->nextoffset=sz;
    structure->lastoffset=structure->nextoffset;
	
	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_addmember_value(CInvContext *context,
	CInvStructure *structure, const char *name, cinv_type_t type) {
	int sz, align, dummy;
	char *namecopy;
	CInvStructMember *member;
	if (structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is already finished", 0);
		return CINV_ERROR;
	}
	if (hashtable_search(structure->members, name)) {
		context_set_error(context, CINV_E_INVAL,
			"the structure already contains a member by that name", 0);
		return CINV_ERROR;
	}

	namecopy = strdup(name);
	if (!namecopy) {
		context_set_nomem(context);
		return CINV_ERROR;
	}
	member = malloc(sizeof(CInvStructMember));
	if (!member) {
		free(namecopy);
		context_set_nomem(context);
		return CINV_ERROR;
	}

	get_size(type, &dummy, &sz, &align);

#if ARCH_CLAMP_NONFIRST_STRUCTALIGN
	if (hashtable_count(structure->members))
		align = ARCH_CLAMP_NONFIRST_STRUCTALIGN;
#endif
    structure->lastoffset=structure->nextoffset;

	if ((structure->nextoffset % align) != 0)
		structure->nextoffset += align - (structure->nextoffset % align);

	member->structtype = NULL;
	member->type = type;
	member->offset = structure->nextoffset;

	hashtable_insert(structure->members, namecopy, member);

	if (align > structure->alignment)
		structure->alignment = align;

	structure->nextoffset += sz;
    structure->lastoffset=structure->nextoffset;
	member->nextoffset=sz;
	/*printf("Member %s off %d sz %d next %d : struct last %d next %d\n",
	        name,member->offset,sz,member->nextoffset,
	        structure->lastoffset,structure->nextoffset);*/
	context_clear_error(context);
	return CINV_SUCCESS;
}
/* important: in unions always add biggest member first to get the right alignment! */
cinv_status_t cinv_structure_addmember_union(CInvContext *context,
	CInvStructure *structure, const char *name, cinv_type_t type) {
	int sz, align, dummy;
	char *namecopy;
	CInvStructMember *member;
	if (structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is already finished", 0);
		return CINV_ERROR;
	}
	if (hashtable_search(structure->members, name)) {
		context_set_error(context, CINV_E_INVAL,
			"the structure already contains a member by that name", 0);
		return CINV_ERROR;
	}

	namecopy = strdup(name);
	if (!namecopy) {
		context_set_nomem(context);
		return CINV_ERROR;
	}
	member = malloc(sizeof(CInvStructMember));
	if (!member) {
		free(namecopy);
		context_set_nomem(context);
		return CINV_ERROR;
	}

	get_size(type, &dummy, &sz, &align);

#if ARCH_CLAMP_NONFIRST_STRUCTALIGN
	if (hashtable_count(structure->members))
		align = ARCH_CLAMP_NONFIRST_STRUCTALIGN;
#endif
    //structure->lastoffset=structure->nextoffset;

	if ((structure->nextoffset % align) != 0)
		structure->nextoffset += align - (structure->nextoffset % align);
	if ((structure->lastoffset % align) != 0)
		structure->lastoffset += align - (structure->lastoffset % align);

	member->structtype = NULL;
	member->type = type;
	member->offset = structure->lastoffset;

	hashtable_insert(structure->members, namecopy, member);

	if (align > structure->alignment)
		structure->alignment = align;
	if(sz>(structure->nextoffset-structure->lastoffset)) 	structure->nextoffset = structure->lastoffset+sz;
	member->nextoffset=structure->nextoffset-structure->lastoffset;
	/*printf("Member %s off %d sz %d next %d : struct last %d next %d\n",
	        name,member->offset,sz,member->nextoffset,
	        structure->lastoffset,structure->nextoffset);*/
	
	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_addmember_struct(CInvContext *context,
	CInvStructure *structure, const char *name, CInvStructure *type) {
	int align;
	char *namecopy;
	CInvStructMember *member;
	if (structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is already finished", 0);
		return CINV_ERROR;
	}
	if (!type->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the child structure is not finished", 0);
		return CINV_ERROR;
	}

	if (hashtable_search(structure->members, name)) {
		context_set_error(context, CINV_E_INVAL,
			"the structure already contains a member by that name", 0);
		return CINV_ERROR;
	}

	namecopy = strdup(name);
	if (!namecopy) {
		context_set_nomem(context);
		return CINV_ERROR;
	}
	member = malloc(sizeof(CInvStructMember));
	if (!member) {
		free(namecopy);
		context_set_nomem(context);
		return CINV_ERROR;
	}

	align = type->alignment;

#if ARCH_CLAMP_NONFIRST_STRUCTALIGN
	if (hashtable_count(structure->members))
		align = ARCH_CLAMP_NONFIRST_STRUCTALIGN;
#endif
    structure->lastoffset=structure->nextoffset;
	
	if ((structure->nextoffset % align) != 0)
		structure->nextoffset += align - (structure->nextoffset % align);

	member->structtype = type;
	member->type = 0;
	member->offset = structure->nextoffset;
	
	hashtable_insert(structure->members, namecopy, member);
	if (align > structure->alignment)
		structure->alignment = align;

	structure->nextoffset += type->nextoffset;
	member->nextoffset=type->nextoffset;
	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_insert_struct(CInvContext *context,
	CInvStructure *structure, const char *name, CInvStructure *type) {
	int align;
	if (structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is already finished", 0);
		return CINV_ERROR;
	}
	if (!type->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the child structure is not finished", 0);
		return CINV_ERROR;
	}

	align = type->alignment;

#if ARCH_CLAMP_NONFIRST_STRUCTALIGN
	if (hashtable_count(structure->members))
		align = ARCH_CLAMP_NONFIRST_STRUCTALIGN;
#endif
    structure->lastoffset=structure->nextoffset;
	
	if ((structure->nextoffset % align) != 0)
		structure->nextoffset += align - (structure->nextoffset % align);

	/* resolve struct and insert here */
	//hashtable_insert(structure->members, namecopy, member);
	
		struct hashtable_itr it; 
		char *submembername;
		CInvStructMember *submember;
		hashtable_iterator(type->members, &it);

		do {
			submembername= hashtable_iterator_key(&it);
			submember= hashtable_iterator_value(&it);

            if (hashtable_search(structure->members, submembername)) {
                context_set_error(context, CINV_E_INVAL,
                    "the structure already contains a member by that name ", 0);
                return CINV_ERROR;
            }
			
			submember->offset+=structure->nextoffset;
			submember->nextoffset+=structure->nextoffset;
			hashtable_insert(structure->members, submembername, submember);
		} while (hashtable_iterator_advance(&it));
	
	/* instead of inserting strcut as new member */
	
	if (align > structure->alignment)
		structure->alignment = align;

	structure->nextoffset += type->nextoffset;
	structure->lastoffset=structure->nextoffset;
	
	// not freeing up will leak memory
	// but freeing up here will destroy the struct
	// so no way to get the size or anything back
    //cinv_structure_delete(context,type);
    hashtable_destroy(type->members,0);
    free(type);
	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_insert_union_struct(CInvContext *context,
	CInvStructure *structure, const char *name, CInvStructure *type) {
	int align,sz;
	if (structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is already finished", 0);
		return CINV_ERROR;
	}
	if (!type->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the child structure is not finished", 0);
		return CINV_ERROR;
	}

	align = type->alignment;
	sz= type->nextoffset;

#if ARCH_CLAMP_NONFIRST_STRUCTALIGN
	if (hashtable_count(structure->members))
		align = ARCH_CLAMP_NONFIRST_STRUCTALIGN;
#endif
    structure->nextoffset=structure->lastoffset;
	
	if ((structure->nextoffset % align) != 0)
		structure->nextoffset += align - (structure->nextoffset % align);

	/* resolve struct and insert here */
	//hashtable_insert(structure->members, namecopy, member);
	
		struct hashtable_itr it; 
		char *submembername;
		CInvStructMember *submember;
		hashtable_iterator(type->members, &it);

		do {
			submembername= hashtable_iterator_key(&it);
			submember= hashtable_iterator_value(&it);

            if (hashtable_search(structure->members, submembername)) {
                context_set_error(context, CINV_E_INVAL,
                    "the structure already contains a member by that name ", 0);
                return CINV_ERROR;
            }
			
			submember->offset+=structure->nextoffset;
			submember->nextoffset+=structure->nextoffset;
			hashtable_insert(structure->members, submembername, submember);
		} while (hashtable_iterator_advance(&it));
	
	/* instead of inserting strcut as new member */
	
	if (align > structure->alignment)
		structure->alignment = align;

	//structure->nextoffset += type->nextoffset;
	if(sz>(structure->nextoffset-structure->lastoffset)) 	structure->nextoffset = structure->lastoffset+sz;
	/*printf("Struct %s off %d sz %d next %d : struct last %d next %d\n",
	        name,0,sz,type->nextoffset,
	        structure->lastoffset,structure->nextoffset);*/
	//member->nextoffset=structure->nextoffset-structure->lastoffset;
	//structure->lastoffset=structure->nextoffset;
	
	// not freeing up will leak memory
	// but freeing up here will destroy the struct
	// so no way to get the size or anything back
    //cinv_structure_delete(context,type);
    hashtable_destroy(type->members,0);
    free(type);
	
	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_finish(CInvContext *context,
	CInvStructure *structure) {
	if (structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is already finished", 0);
		return CINV_ERROR;
	}
	
	// compute final padding
	if ((structure->nextoffset % structure->alignment) != 0) {
		structure->nextoffset += structure->alignment -
			(structure->nextoffset % structure->alignment);
	}
	
	structure->finished = 1;
	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_getsize(CInvContext *context,
	CInvStructure *structure, int *size_out) {
	if (!structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is not finished", 0);
		return CINV_ERROR;

	}
	*size_out = structure->nextoffset;
	context_clear_error(context);
	return CINV_SUCCESS;
}
void *cinv_structure_create_instance(CInvContext *context,
	CInvStructure *structure) {
	void *r;
	if (!structure->finished) {
		context_set_error(context, CINV_E_INVAL,
			"the structure is not finished", 0);
		return NULL;
	}
	r = malloc(structure->nextoffset);
	if (!r)
		context_set_nomem(context);
	else
		context_clear_error(context);
	return r;
}
int get_member(CInvContext *context, CInvStructure *structure,
	const char *name, int *sz, int *is_struct) {
	int dummy;

	CInvStructMember *m = hashtable_search(structure->members, name);
	if (!m) {
		context_set_error(context, CINV_E_INVAL,
			"cannot find member with that name", 0);
		return -1;
	}

	if (m->structtype) {
		*is_struct = 1;
		*sz = m->structtype->nextoffset;
	} else {
		*is_struct = 0;
		get_size(m->type, &dummy, sz, &dummy);
	}

	return m->offset;
}
int get_member_with_size(CInvContext *context, CInvStructure *structure,
	const char *name, int *sz, int *is_struct, int* nextoffset) {
	int dummy;

	CInvStructMember *m = hashtable_search(structure->members, name);
	if (!m) {
		context_set_error(context, CINV_E_INVAL,
			"cannot find member with that name", 0);
		return -1;
	}

	if (m->structtype) {
		*is_struct = 1;
		*sz = m->structtype->nextoffset;
	} else {
		*is_struct = 0;
		get_size(m->type, &dummy, sz, &dummy);
	}
	*nextoffset=m->nextoffset;
	return m->offset;
}
cinv_status_t cinv_structure_instance_zerovalue(CInvContext *context,
	CInvStructure *structure, void *instance, const char *name) {
	int sz, offset, is_struct;
	char *ptr = instance;

	offset = get_member(context, structure, name, &sz, &is_struct);
	if (offset == -1)
		return CINV_ERROR;

	ptr += offset;
	
    while (sz) { 
        *(ptr) = '\0'; 
        (ptr)++; 
        (sz)--;
    }   

	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_instance_setvalue(CInvContext *context,
	CInvStructure *structure, void *instance, const char *name,
	void *value_ptr) {
	int sz, offset, is_struct;
	char *ptr = instance;

	if (!value_ptr) {
		context_set_error(context, CINV_E_INVAL,
			"value_ptr cannot be NULL", 0);
		return CINV_ERROR;
	}
	
	offset = get_member(context, structure, name, &sz, &is_struct);
	if (offset == -1)
		return CINV_ERROR;

	ptr += offset;
	
	memcpy(ptr, value_ptr, sz);

	context_clear_error(context);
	return CINV_SUCCESS;
}
void *cinv_structure_instance_getvalue(CInvContext *context,
	CInvStructure *structure, void *instance, const char *name) {
	int sz, offset, is_struct;
	char *ptr = instance;

	offset = get_member(context, structure, name, &sz, &is_struct);
	if (offset == -1)
		return NULL;

	ptr += offset;
	
	context_clear_error(context);
	return ptr;
}
cinv_status_t cinv_structure_instance_setarrayvalue(CInvContext *context,
	CInvStructure *structure, void *instance, const char *name, int index,
	void *value_ptr) {
	int sz, offset, is_struct,nextoffset;
	char *ptr = instance;

	if (!value_ptr) {
		context_set_error(context, CINV_E_INVAL,
			"value_ptr cannot be NULL", 0);
		return CINV_ERROR;
	}
	
	offset = get_member_with_size(context, structure, name, &sz, &is_struct,&nextoffset);
	if (offset == -1)
		return CINV_ERROR;
	
	if((sz+1)*index>nextoffset) {
	    context_set_error(context, CINV_E_INVAL,"Array out of bounds", 0);
	    return CINV_ERROR;
	}
	    
	
	ptr += offset+sz*index; 
	
	memcpy(ptr, value_ptr, sz);

	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_instance_zeroarrayvalue(CInvContext *context,
	CInvStructure *structure, void *instance, const char *name, int index) {
	int sz, offset, is_struct,nextoffset;
	char *ptr = instance;

	offset = get_member_with_size(context, structure, name, &sz, &is_struct,&nextoffset);
	if (offset == -1)
		return CINV_ERROR;
	
	if((sz+1)*index>nextoffset) {
	    context_set_error(context, CINV_E_INVAL,"Array out of bounds", 0);
	    return CINV_ERROR;
	}
	    
	
	ptr += offset+sz*index; 
	
    while (sz) { 
        *(ptr) = '\0'; 
        (ptr)++; 
        (sz)--;
    }   

	context_clear_error(context);
	return CINV_SUCCESS;
}
void *cinv_structure_instance_getarrayvalue(CInvContext *context,
	CInvStructure *structure, void *instance, const char *name, int index) {
	int sz, offset, is_struct,nextoffset;
	char *ptr = instance;

	offset = get_member_with_size(context, structure, name, &sz, &is_struct,&nextoffset);
	if (offset == -1)
		return NULL;
	
	if((sz+1)*index>nextoffset) {
	    context_set_error(context, CINV_E_INVAL,"Array out of bounds", 0);
	    return NULL;
	}
	
	ptr += offset+sz*index;
	
	context_clear_error(context);
	return ptr;
}
cinv_status_t cinv_structure_delete_instance(CInvContext *context,
	void *instance) {
	free(instance);
	context_clear_error(context);
	return CINV_SUCCESS;
}
cinv_status_t cinv_structure_delete(CInvContext *context,
	CInvStructure *structure) {

	if (hashtable_count(structure->members) > 0) {
		struct hashtable_itr it;
		hashtable_iterator(structure->members, &it);

		do {
			free(hashtable_iterator_key(&it));
			free(hashtable_iterator_value(&it));
		} while (hashtable_iterator_remove(&it));
	}

	hashtable_destroy(structure->members, 0);

	free(structure);

	context_clear_error(context);
	return CINV_SUCCESS;
}

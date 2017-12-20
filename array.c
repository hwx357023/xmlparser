#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "array.h"
#include "dbg.h"

#define EXPAND_ARRAY_INIT_STEP  10

int expand_array(ARRAY* arr , size_t need)
{
	size_t left = arr->cap - arr->num;
	size_t expand;
	void * new_item;

	if(left > need) return 1;

	if(arr->cap > 0) 
		expand = 2 * arr->cap;
	else
		expand = EXPAND_ARRAY_INIT_STEP;

	new_item = malloc(sizeof(void*) * expand);
	if(new_item == NULL)
	{
		debug("malloc return NULL");
		return 0;
	}

	if(arr->num)
	{
		memcpy(new_item, arr->item, sizeof(void*)*arr->num);
	}

	free(arr->item);
	arr->item = new_item;

	arr->cap = expand;

	return 1;
}

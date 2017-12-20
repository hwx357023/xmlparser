#include <stdio.h>


typedef struct array{
	void ** item;
	size_t num;
	size_t cap;
}ARRAY;

int expand_array(ARRAY* arr , size_t need);

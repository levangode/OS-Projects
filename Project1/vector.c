
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>



void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation){
	assert(elemSize > 0 && initialAllocation >= 0);
	if(initialAllocation == 0){
		initialAllocation = defAllocation;
	}
	v->elemSize = elemSize;
	v->logLength = 0;
	v->allocLength = initialAllocation;
	v->freeFunction = freeFn;
	v->elems = malloc(initialAllocation*elemSize);
	assert(v->elems != NULL);
}

void VectorDispose(vector *v){
	if(v->freeFunction != NULL){
		int i;
		for(i=0; i < v->logLength; i++){
			void* pointer = (char*)v->elems + i * v->elemSize;
			v->freeFunction(pointer);
		}
	}
	free(v->elems);
	v->logLength = 0;
}

int VectorLength(const vector *v){
	return v->logLength;
}

void *VectorNth(const vector *v, int position){
	assert(position >= 0);
	assert(position <= v->logLength-1);
	void* result = (char*)v->elems + v->elemSize * position;
	return result;
}

void VectorReplace(vector *v, const void *elemAddr, int position){
	void* replaceIndex = VectorNth(v, position);
	if(v->freeFunction != NULL){
		v->freeFunction(replaceIndex);
	}
	memcpy(replaceIndex, elemAddr, (size_t)v->elemSize);
}

void VectorGrow(vector* v){
	v->allocLength = v->allocLength + v->allocLength;
	void* realloced = realloc(v->elems, (size_t)(v->allocLength)*(v->elemSize));
	assert(realloced != NULL);
	v->elems = realloced;
}
void VectorInsert(vector *v, const void *elemAddr, int position){
	void* replPosition = (char*)v->elems + (position * v->elemSize);
	if(v->allocLength == v->logLength){
		VectorGrow(v);
	}
	void* dest = (char*)replPosition + v->elemSize;
	memmove(dest, replPosition, (size_t)v->elemSize*(v->logLength - position));
	v->logLength++;
	memcpy(replPosition, elemAddr, (size_t)v->elemSize);
}

void VectorAppend(vector *v, const void *elemAddr){
	if(v->allocLength == v->logLength){
		VectorGrow(v);
	}
	void* replaceIndex = (char*)v->elems + (v->elemSize * v->logLength);
	memcpy(replaceIndex, elemAddr, (size_t)v->elemSize);
	v->logLength++;
}

void VectorDelete(vector *v, int position){
	void* pointerToElement = VectorNth(v, position);
	if(v->freeFunction != NULL){
		v->freeFunction(pointerToElement);
	}
	void* source = (char*)pointerToElement + v->elemSize;
	memmove(pointerToElement, source, (size_t)v->elemSize * (v->logLength-position-1));
	v->logLength--;
}

void VectorSort(vector *v, VectorCompareFunction compare){
	assert(compare != NULL);
	qsort(v->elems, (size_t)v->logLength, (size_t)v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData){  
	assert(mapFn != NULL);
	int i;
	for(i = 0; i < v->logLength; i++){
		void* pointer = (char*)v->elems + i*v->elemSize;
		mapFn(pointer, auxData);
	}
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted){
	assert(startIndex <= v->logLength);
	assert(key != NULL);
	assert(searchFn != NULL);
	void* result;
	size_t nmemb = v->logLength-startIndex;
	if(!isSorted){
		result = lfind(key, (char*)v->elems + (startIndex*v->elemSize), &nmemb, v->elemSize, searchFn);
	} else {
		result = bsearch(key, (char*)v->elems + startIndex*v->elemSize, nmemb, v->elemSize, searchFn);
	}
	if(result != NULL){
		return ((char*)result - (char*)v->elems)/v->elemSize;
	} else {
		return kNotFound;
	}
} 

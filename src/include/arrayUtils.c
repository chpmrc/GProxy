/**
 * This module allows to manage an array used to store any kind of data
 */
 
#include <limits.h>

#define INIT_SIZE 500

typedef struct Array {
	void *data[INIT_SIZE];

	int length = INIT_SIZE;

	int size = 0;
	
} Array;


/* Functions */

/**
 * Init the array
 */
Array init(){
	Array *newArray = (Array *)malloc(sizeof(Array));
	return newArray; 
}

/**
 * Create a new array bigger than the original one.
 * Copy the content from the original one.
 * Replace the old array with the new one.
 */
void resize(Array a, int times){
	
}

void append(void *obj){
	mainArray[size] = obj;
	size++;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pos-lib.h"

int n_objects;
long long object_size_1k;
long long object_size;
int* fd;
char** arr_object_name;
void** p_object;

void init_test_object_names(void);
void create_test_objects(void);
void open_test_objects(void);
void alloc_memory_to_test_objects(void);
void free_test_objects(void);

int main(int argc, char* argv[])
{

	int i;
	long long object_size_1k;
	long long object_size;

	n_objects		= atoi(argv[1]);
	object_size_1k 		= atoi(argv[2]);	// total object size (kbyte)
	object_size 		= object_size_1k * 1024;	// object size for each threads

	printf("[POS TEST] Start Test for %d objects (%ld Kbyte)\n", n_objects, object_size_1k);
	init_test_object_names();
	printf("\t Init object names complete!\n", n_objects, object_size_1k);
	create_test_objects();
	printf("\t Create objects complete! \n", n_objects, object_size_1k);
	alloc_memory_to_test_objects();
	printf("\t Alloc memory complete! \n", n_objects, object_size_1k);
	free_test_objects();
	printf("\t Free test objects complete! \n", n_objects, object_size_1k);

	/* Free memory for name array */
        for(i=0; i<n_objects; i++){
                free(arr_object_name[i]);
        }
	free(arr_object_name);

	printf("[POS Test] Test end. \n", n_objects, object_size_1k);
	return 0;
}

void init_test_object_names(void)
{
	int i;
	char temp_c[4];
	char* object_name;

	/* Allocate Memory for object name variable */
	object_name = (char*)malloc(strlen(path)+6);
	arr_object_name = (char**)malloc(sizeof(object_name) * n_objects);
	for(i=0; i<n_objects; i++){
		arr_object_name[i] = (char*)malloc(strlen(path)+6);
	}

	p_object = (void**)malloc(sizeof(void*) * n_objects);

	for(i=0; i<n_objects; i++){
		/* Make object name*/
		strcpy(object_name, path);
		strncat(object_name, "/", 1);
		sprintf(temp_c,"%d",i);
		strncat(object_name, temp_c, 4);

		/* Update object name array */
		strcpy(arr_object_name[i], object_name);

		/* Reset object name */
		memset(object_name, '\0', sizeof(object_name));
	}
	
	/* Make object descriptor array */
	fd = (int*)malloc(sizeof(int)*n_objects);

	/* free memory */
	free(object_name);
}

void create_test_objects(void)
{
	int i;
	for(i = 0; i<n_objects; i++){
		pos_create(arr_object_name[i]);
	}
}  

void alloc_memory_to_test_objects(void)
{
	int i;
	for(i = 0; i<n_objects; i++){
		p_object[i] = pos_malloc(arr_object_name[i], objectsize);
	}

}

void free_test_objects(void)
{
	int i;
	for(i = 0; i<n_objects; i++){
		pos_free(arr_object_name[i], p_object[i]);
	}
}

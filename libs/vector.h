#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

typedef void (*vector_destructor_fn)(void *element);

typedef struct  {
    void *data;
    size_t element_size;
    size_t size;
    size_t capacity;
    vector_destructor_fn destructor;
} Vector;

///initializes the vector with the specified element size and optional destructor function, returns 1 on success, 0 on failure (e.g. memory allocation failure)
/**
    * @param v pointer to the vector to initialize
    * @param element_size size of each element in the vector, must be greater than 0
    * @param destructor optional function to call on each element when it is removed or the vector is destroyed, can be NULL if no special cleanup is needed
    * @return 1 on success, 0 on failure (e.g. memory allocation failure)
 */
int  vector_init(Vector *v, size_t element_size, vector_destructor_fn destructor);
///destroys the vector and frees all associated memory, calls the destructor for each element if it was provided in vector_init
/**
    * @param v pointer to the vector to destroy
    * After calling this function, the vector should not be used unless it is re-initialized with vector_init.
     The destructor function will be called for each element in the vector 
     if it was provided during initialization, so any necessary cleanup for the elements will be performed.
      The vector's internal data will be freed, and the vector's size and capacity will be reset to 0.
*/
void vector_destroy(Vector *v);
///returns the number of elements currently stored in the vector
/**
    * @param v pointer to the vector
    * @return number of elements currently stored in the vector, or 0 if the vector is NULL
 */
static inline size_t vector_size(const Vector *v) {return v->size;}

///reserves capacity for at least reserve_size elements, returns 1 on success, 0 on failure (e.g. memory allocation failure)
/**
    * @param v pointer to the vector
    * @param reserve_size minimum capacity to reserve for the vector, must be greater than 0
    * @return 1 on success, 0 on failure (e.g. memory allocation failure)
    * @note if current capacity is already greater than or equal to reserve_size, this function does nothing and returns 1.
 */
int vector_reserve(Vector *v, size_t reserve_size);

///returns a pointer to the internal data array of the vector, or NULL if the vector is NULL
/**
    * @param v pointer to the vector
    * @return pointer to the internal data array of the vector, or NULL if the vector is NULL.
        The returned pointer is valid until the next modification of the vector (push_back, set, remove, etc.)
        that may cause a reallocation or element destruction.
    You can use this pointer to convert the vector to an array of the appropriate type, for example:
    int *arr = (int*)vector_data(&my_vector);
*/
void *vector_data(Vector *v);

///adds a new element to the end of the vector, returns 1 on success, 0 on failure (e.g. memory allocation failure)
/**
    * @param v pointer to the vector
    * @param element pointer to the element to add, must point to a valid memory of size at least element_size specified in vector_init
    * @return 1 on success, 0 on failure (e.g. memory allocation failure)
 */
int vector_push_back(Vector *v, const void *element);
///removes the last element from the vector and returns a pointer to it, returns NULL if the vector is empty
/**
    * @param v pointer to the vector
    * @return pointer to the removed element, or NULL if the vector is empty.
              The destructor is not called for the removed element, so the caller is responsible for managing its memory if needed.
              The returned pointer is valid until the next modification of the vector (push_back, set, remove, etc.) that may cause a reallocation or element destruction.
 */

void *vector_pop_back(Vector *v);

///returns a pointer to the element at the specified index, or NULL if the index is out of bounds
void *vector_get(Vector *v, size_t index);
///replaces the element at the specified index with a new value, returns 1 on success, 0 on failure (e.g. index out of bounds)
/**
    * @param v pointer to the vector
    * @param index zero-based index of the element to replace, must be less than the current size of the vector
    * @param element pointer to the new value, must point to a valid memory of size at least element_size specified in vector_init
    * @return 1 on success, 0 on failure (e.g. index out of bounds)
    If the index is valid, the function replaces the element at the specified index with the new value provided by the caller. 
If a destructor function was provided during vector initialization, it will be called on the old element before it is replaced, allowing for any necessary cleanup. 
The new value is copied into the vector's internal data array, and the function returns 1 to indicate success. 
If the index is out of bounds (greater than or equal to the current size of the vector), the function does nothing and returns 0 to indicate failure.
 */
int  vector_set(Vector *v, size_t index, const void *element);

///removes the element at the specified index, returns 1 on success, 0 on failure (e.g. index out of bounds)
/**
    * @param v pointer to the vector
    * @param index zero-based index of the element to remove, must be less than the current size of the vector
    * @param count number of elements to remove, if index + count exceeds the current size of the vector, only elements up to the end of the vector will be removed
    * @return 1 on success, 0 on failure (e.g. initial index out of bounds)    
 */
int  vector_remove(Vector *v, size_t index, size_t count);

int vector_exchange(Vector *v, size_t index, void *element);

///inserts a new element at the specified index, shifting subsequent elements to the right, returns 1 on success, 0 on failure (e.g. index out of bounds, memory allocation failure)
int vector_insert(Vector *v, size_t index, const void *element, size_t count);

///linear search for an element in the vector, returns index or -1 if not found
/**
    * @param data pointer to the vector data
    * @param count number of elements in the vector
    * @param element_size size of each element in the vector
    * @param cmp comparison function that returns 0 if elements are equal. If NULL, memcmp is used.
    * @param target pointer to the element to find   
    * @return index of the found element or -1 if not found
 */
int linear_find(const void *data, size_t count, size_t element_size, int (*cmp)(const void *a, const void *b), const void *target);

///linear search for an element in the vector, removes it if found and returns 1, otherwise returns 0
/**
    * @param data pointer to the vector data
    * @param count pointer to the number of elements in the vector, will be decremented if an element is removed
    * @param element_size size of each element in the vector
    * @param predicate function that returns 1 if the element should be removed, 0 otherwise, the function is also responsible to destroy removing element if needed
    * @return final count of elements in the vector after removals
*/
int linear_remove_if(void *data, size_t *count, size_t element_size, int (*predicate)(void *element));


///Swap memory
/**
@note pointers must be aligned (depend on size)
 */
void swap_memory(void *a, void *b, size_t size);

#endif
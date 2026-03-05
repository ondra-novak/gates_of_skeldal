#include "vector.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define VECTOR_INITIAL_CAPACITY 4




static int vector_resize(Vector *v, size_t new_capacity)
{
    
    void *new_data = realloc(v->data, new_capacity * v->element_size);
    if (!new_data)
        return 0;

    v->data = new_data;
    v->capacity = new_capacity;
    return 1;
}

int vector_reserve(Vector *v, size_t reserve_size) {
    if (v->capacity < reserve_size) 
        return vector_resize(v, reserve_size);
    return 1;
}

void *vector_data(Vector *v) {
    return v->data;
}

int vector_init(Vector *v, size_t element_size, vector_destructor_fn destructor)
{
    if (!v || element_size == 0)
        return 0;

    v->element_size = element_size;
    v->size = 0;
    v->capacity = VECTOR_INITIAL_CAPACITY;
    v->destructor = destructor;

    v->data = malloc(v->capacity * element_size);
    if (!v->data)
        return 0;

    return 1;
}

void vector_destroy(Vector *v)
{
    if (!v)
        return;

    if (v->destructor) {
        for (size_t i = 0; i < v->size; ++i) {
            void *elem = (char*)v->data + i * v->element_size;
            v->destructor(elem);
        }
    }

    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}

int vector_push_back(Vector *v, const void *element)
{
    if (v->size == v->capacity) {
        if (!vector_resize(v, v->capacity * 2))
            return 0;
    }

    void *dest = (char*)v->data + v->size * v->element_size;
    memcpy(dest, element, v->element_size);
    v->size++;

    return 1;
}

void *vector_pop_back(Vector *v)
{
    if (v->size == 0)
        return NULL;

    v->size--;
    void *elem = (char*)v->data + v->size * v->element_size;
 
    return elem;
}

void *vector_get(Vector *v, size_t index)
{
    if (!v || index >= v->size)
        return NULL;

    return (char*)v->data + index * v->element_size;
}

int vector_set(Vector *v, size_t index, const void *element)
{
    if (!v || index >= v->size)
        return 0;

    void *dest = (char*)v->data + index * v->element_size;

    if (v->destructor)
        v->destructor(dest);

    memcpy(dest, element, v->element_size);
    return 1;
}


int vector_exchange(Vector *v, size_t index, void *element) {
    if (!v || index >= v->size)
        return 0;

    void *dest = (char*)v->data + index * v->element_size;
    swap_memory(dest,element,v->element_size);  
    return 1;

}

int vector_remove(Vector *v, size_t index, size_t count)
{
    if (!v || index >= v->size || count == 0) {
        return 0;
    }

    size_t end_index = index + count;
    if (end_index > v->size)
        end_index = v->size;

    if (v->destructor) {
        for (size_t i = index; i < end_index; ++i) {
            void *elem = (char*)v->data + i * v->element_size;
            v->destructor(elem);
        }
    }

    if (end_index < v->size) {
        void *dest = (char*)v->data + index * v->element_size;
        void *src = (char*)v->data + end_index * v->element_size;
        memmove(dest, src, (v->size - end_index) * v->element_size);
    }

    v->size -= (end_index - index);
    return 1;
}

int vector_insert(Vector *v, size_t index, const void *element, size_t count) {
    if (!v || index > v->size || count == 0) {
        return 0;
    }

    if (v->size + count > v->capacity) {
        size_t new_capacity = v->capacity;
        while (new_capacity < v->size + count) {
            new_capacity *= 2;
        }
        if (!vector_resize(v, new_capacity))
            return 0;
    }

    void *dest = (char*)v->data + index * v->element_size;
    if (index < v->size) {
        void *src = (char*)v->data + index * v->element_size;
        memmove((char*)dest + count * v->element_size, src, (v->size - index) * v->element_size);
    }

    for (size_t i = 0; i < count; ++i) {
        void *elem_dest = (char*)dest + i * v->element_size;
        memcpy(elem_dest, element, v->element_size);
    }

    v->size += count;
    return 1;
}

int linear_find(const void *data, size_t count, size_t element_size, int (*cmp)(const void *a, const void *b), const void *target) {
    if (!data || !cmp || !target) 
        return -1;  
    if (cmp == NULL) {
        for(size_t i = 0; i < count; ++i) {
            const void *elem = (const char*)data + i * element_size;
            if (memcmp(elem, target, element_size) == 0) {
                return (int)i;
            }
        }
        return -1;
    }
    for(size_t i = 0; i < count; ++i) {
        const void *elem = (const char*)data + i * element_size;
        if (cmp(elem, target) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int linear_remove_if(void *data, size_t *count, size_t element_size, int (*predicate)(void *element)) {
    if (!data || !count || !predicate) 
        return 0;  
    size_t write_index = 0;
    for(size_t read_index = 0; read_index < *count; ++read_index) {
        void *elem = (char*)data + read_index * element_size;
        if (!predicate(elem)) {
            if (write_index != read_index) {
                void *dest = (char*)data + write_index * element_size;
                memmove(dest, elem, element_size);
            }
            write_index++;
        }
    }
    size_t removed_count = *count - write_index;
    *count = write_index;
    return (int)removed_count;
}

void swap_memory(void *a, void *b, size_t size) {
    size_t s = (size & ((~size)+1));
    size_t selector = s == size?(size | ((uintptr_t)a & (s-1)) | ((uintptr_t)b & (s-1))):0;
    switch (selector) {
        case 1: {
            char *iter_a = (char *)a;
            char *iter_b = (char *)b;
            char x = *iter_a; *iter_a = *iter_b; *iter_b = x;
            return;
        }
        case 2: {
            uint16_t *iter_a = (uint16_t *)a;
            uint16_t *iter_b = (uint16_t *)b;
            uint16_t x = *iter_a; *iter_a = *iter_b; *iter_b = x;
            return;
        }
        case 4:  {
            uint32_t *iter_a = (uint32_t *)a;
            uint32_t *iter_b = (uint32_t *)b;
            uint32_t x = *iter_a; *iter_a = *iter_b; *iter_b = x;
            return;
        }
        case 8:  {
            uint64_t *iter_a = (uint64_t *)a;
            uint64_t *iter_b = (uint64_t *)b;
            uint64_t x = *iter_a; *iter_a = *iter_b; *iter_b = x;
            return;
        }
        case 16:  {
            uint64_t *iter_a = (uint64_t *)a;
            uint64_t *iter_b = (uint64_t *)b;
            uint64_t x = iter_a[0];
            uint64_t y = iter_a[1];
            iter_a[0] = iter_b[0];
            iter_a[1] = iter_b[1];  
            iter_b[0] = x;
            iter_b[1] = y;
            return;
        } 
        default: {    
            char *iter_a = (char *)a;
            char *iter_b = (char *)b;
            if (size == 0 || iter_a == iter_b) return;
            char buff[64];
            while (size >= sizeof(buff)) {
                memcpy(buff, iter_a, sizeof(buff));
                memcpy(iter_a, iter_b, sizeof(buff));
                memcpy(iter_b, buff, sizeof(buff));
                iter_a += sizeof(buff);
                iter_b += sizeof(buff);
                size -= sizeof(buff); 
            }
            if (size) {
                memcpy(buff, iter_a, size);
                memcpy(iter_a, iter_b, size);
                memcpy(iter_b, buff, size);
            }
            return;
        }
    }

}
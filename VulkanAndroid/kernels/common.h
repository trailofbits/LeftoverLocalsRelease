
// All in size of 8 bit int

#if !defined(SHARED_MEMORY_SIZE) 
#define SHARED_MEMORY_SIZE 4096
#endif

// In case we want to try out of bounds accesses
#if !defined(SHARED_MEMORY_SIZE_TRAVERSED) 
#define SHARED_MEMORY_SIZE_TRAVERSED (SHARED_MEMORY_SIZE)
#endif




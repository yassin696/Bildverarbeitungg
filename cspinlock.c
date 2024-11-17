#include <stdatomic.h>
#include <stdbool.h>
#include "cspinlock.h"
#include <stdlib.h>

//to initialise the lock : atomic_store(&lock->locked, false);
struct cspinlock {
    atomic_bool locked;
};

int cspin_lock(cspinlock_t *lock) {
    bool expected = false;
    while (!atomic_compare_exchange_strong(&lock->locked, &expected, true)) {
        expected = false; // Reset    
    }
    return 0; 
}

int cspin_trylock(cspinlock_t *slock){
    bool expected = false;
   return  atomic_compare_exchange_strong(&slock->locked, &expected, true);
}

 cspinlock_t* cspin_alloc(){
    cspinlock_t * lock = (cspinlock_t*) malloc(sizeof(cspinlock_t));
    if(lock){
        atomic_store(&lock->locked, false);
    }
    return lock ;
}

void cspin_free(cspinlock_t* slock){
    free(slock);
}

int cspin_unlock(cspinlock_t *slock){
    if (!atomic_load(&slock->locked)) {
        // cant unlock
        return -1;
    }
    
    //unlock
    atomic_store(&slock->locked, false);
    return 0;
}
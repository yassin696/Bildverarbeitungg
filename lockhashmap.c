#include "cspinlock.h"
#include "chashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

    
HM* alloc_hashmap(size_t n_buckets){
    HM* hm=(HM*) (malloc(sizeof(HM)));
    if(!hm) { return NULL;}
    hm->nbuckets =n_buckets;
    hm->buckets= (List**)malloc(n_buckets*sizeof(List*));
    if(!hm->buckets){free(hm); return NULL;}
    hm->lock=cspin_alloc();
    for(int i=0;i<n_buckets;i++){
        hm->buckets[i]=(List*) (malloc(sizeof(List)));
        if (!hm->buckets[i]) { 
            free(hm->buckets);
            free(hm);
            return NULL;
        }
        hm->buckets[i]->sentinel = (Node_HM*)malloc(sizeof(Node_HM)); 
        if (!hm->buckets[i]->sentinel) { 
            free(hm->buckets[i]);
            free(hm->buckets);
            free(hm);
            return NULL;
        }
        hm->buckets[i]->sentinel->m_next = NULL; 
      //  hm->buckets[i]->lock = cspin_alloc();
    }

    return hm;
}

void free_hashmap(HM* hm){
    for(int i=0;i<hm->nbuckets;i++){
        Node_HM* current = hm->buckets[i]->sentinel->m_next;
        while(current){
            Node_HM* aux = current;
            free(aux);
            current = current->m_next;
        }
        free(hm->buckets[i]->sentinel); 
        free(hm->buckets[i]);
    }
    free(hm->buckets);
    free(hm->lock);
    free(hm);
    
}

int insert_item(HM* hm, long val){
    int nb = labs(val) % (hm->nbuckets);
    cspin_lock(hm->lock);
    Node_HM* current = hm->buckets[nb]->sentinel->m_next;

    while(current){
        if(current->m_val ==val) {
            printf("value already exists \n");
            cspin_unlock(hm->lock);
            return 0;
        }
        current = current->m_next;
    }

    Node_HM* new_node = (Node_HM*) (malloc(sizeof(Node_HM)));
    if (!new_node) {
        cspin_unlock(hm->lock);
        return 1; 
    }
    new_node->m_next = hm->buckets[nb]->sentinel->m_next;
    new_node->m_val = val;
    hm->buckets[nb]->sentinel->m_next = new_node;

    cspin_unlock(hm->lock);
    return 0;
}

int lookup_item(HM* hm, long val){
    int nb = labs(val) % (hm->nbuckets);
    List* bucket = hm->buckets[nb];
    cspin_lock(hm->lock);
    Node_HM* current = bucket->sentinel->m_next;
    
    while(current){
        if(current->m_val ==val) {
            cspin_unlock(hm->lock);
            return 0;
        }
        current = current->m_next;
    }
    cspin_unlock(hm->lock);
    printf("value doesn't exist \n");
    return 1;
}

int remove_item(HM* hm, long val){
    int nb = labs(val) % (hm->nbuckets);
    List* bucket = hm->buckets[nb];
    cspin_lock(hm->lock);
    Node_HM* current = bucket->sentinel->m_next;
    Node_HM* previous = bucket->sentinel;
    
    while(current){
        if(current->m_val == val) {
            printf("value found \n");
            previous->m_next = current->m_next;
            free(current);
            cspin_unlock(hm->lock);
            return 0;
        }
        previous = current; 
        current = current->m_next;
    }
    cspin_unlock(hm->lock);
    printf("value doesn't exist \n");
    return 1;
}

void print_hashmap(HM* hm) {
    for (int i = 0; i < hm->nbuckets; i++) {
        List* bucket = hm->buckets[i];
        cspin_lock(hm->lock);
        printf("Bucket %d: ", i + 1);

        Node_HM* current = bucket->sentinel->m_next;
        if (current == NULL) {
            printf("Empty\n");
        } else {
            while (current) {
                printf("%ld ", current->m_val);
                current = current->m_next;
            }
            printf("\n");
        }

        cspin_unlock(hm->lock);
    }
}



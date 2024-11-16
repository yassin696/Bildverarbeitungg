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
            free(hm->lock);
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
        free(hm->buckets[i]->sentinel); // Free the sentinel node
        free(hm->buckets[i]);
    }
    free(hm->buckets);
    free(hm->lock);
    free(hm);
    
}

int insert_item(HM* hm, long val){
    int nb = labs(val) % (hm->nbuckets);
    cspin_lock(hm->lock);
    
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

int lookup_item(HM* hm, long val) {
    int nb = labs(val) % (hm->nbuckets);
    List* bucket = hm->buckets[nb];
    cspin_lock(hm->lock);
    Node_HM* current = bucket->sentinel->m_next;

    while (current) {
        if (current->m_val == val) {
            cspin_unlock(hm->lock);
            return 0; // Found the value
        }
        current = current->m_next;
    }
    cspin_unlock(hm->lock);
    return 1; // Value not found
}


int remove_item(HM* hm, long val){
    int nb = labs(val) % (hm->nbuckets);
    List* bucket = hm->buckets[nb];
    cspin_lock(hm->lock);
    Node_HM* current = bucket->sentinel->m_next;
    Node_HM* previous = bucket->sentinel;
    
    while(current){
        if(current->m_val == val) {
            previous->m_next = current->m_next;
            free(current);
            cspin_unlock(hm->lock);
            return 0;
        }
        previous = current; 
        current = current->m_next;
    }
    cspin_unlock(hm->lock);
    return 1;
}

void print_hashmap(HM* hm) {
   // printf("1500 -\n");
    // Iterate over all the buckets in the hashmap
    for (int i = 0; i < hm->nbuckets; i++) {
        List* bucket = hm->buckets[i];
        Node_HM* current = bucket->sentinel->m_next;

        // Print the bucket header to stderr
        printf( "Bucket %d", i + 1);

        // Print all the values in the current bucket to stderr
        while (current) {
            printf(" - %ld", current->m_val); // Print the value
            current = current->m_next; // Move to the next node in the bucket
        }

        // Move to the next line after printing all elements in the bucket
        printf( "\n");
    }
}




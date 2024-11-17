#include "chashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#define PAD 64

HM* alloc_hashmap(size_t n_buckets) {
    HM* hm=(HM*)malloc(sizeof(HM));
    if(!hm) { return NULL; }

    hm->nbuckets=n_buckets;
    hm->buckets=(List**)malloc(n_buckets * sizeof(List*));
    if(!hm->buckets) { free(hm); return NULL; }

    for(size_t i=0; i < n_buckets; i++) {
        hm->buckets[i]=(List*)malloc(sizeof(List));
        if(!hm->buckets[i]) { 
             for(size_t j=0; j < i; j++) {
                free(hm->buckets[j]->sentinel);free(hm->buckets[j]);}
            free(hm->buckets); 
            free(hm); 
            return NULL; 
        }
        
        hm->buckets[i]->sentinel=(Node_HM*)malloc(sizeof(Node_HM));
        if(!hm->buckets[i]->sentinel) {
            for(size_t j=0; j < i; j++) {
                 free(hm->buckets[j]->sentinel);
                free(hm->buckets[j]);
            }
            free(hm->buckets[i]);
            free(hm->buckets);
            free(hm);
            return NULL;
        }

        hm->buckets[i]->sentinel->m_next=NULL;
        atomic_init(&hm->buckets[i]->lockk, 0);atomic_init(&hm->buckets[i]->version, 0);
        
    }
    return hm;
}

void free_hashmap(HM* hm) {
    if(!hm) return;
    
    for(size_t i=0; i < hm->nbuckets; i++) {
        if(hm->buckets[i]) {
            Node_HM* current=hm->buckets[i]->sentinel->m_next;
            while (current) {
                Node_HM* next=current->m_next;
                 free(current);
                current=next;}
             free(hm->buckets[i]->sentinel);
             free(hm->buckets[i]);
        }
    }
    free(hm->buckets);
    free(hm);
}

int insert_item(HM* hm, long val) {
    if(!hm) return 1;
    
    size_t nb=(size_t)labs(val) % hm->nbuckets;
    List* bucket=hm->buckets[nb];  
        while (atomic_exchange_explicit(&bucket->lockk, 1, memory_order_acquire));

     Node_HM* new_node=(Node_HM*)malloc(sizeof(Node_HM));
    if(!new_node) {
        atomic_store_explicit(&bucket->lockk, 0, memory_order_release);
        return 1;
    }

    new_node->m_val=val;
    new_node->m_next=bucket->sentinel->m_next;
    bucket->sentinel->m_next=new_node;
    
    atomic_fetch_add_explicit(&bucket->version, 1, memory_order_relaxed);
    atomic_store_explicit(&bucket->lockk, 0, memory_order_release);
    return 0;
}

int lookup_item(HM* hm, long val) {
    if(!hm) return 1;
    
    size_t nb=(size_t)labs(val) % hm->nbuckets;
    List* bucket=hm->buckets[nb];
    
    while (1) {
        unsigned int version=atomic_load_explicit(&bucket->version, memory_order_acquire);
        Node_HM* current=bucket->sentinel->m_next;
        int mwjoud=0;
        
        while (current) {
            if(current->m_val==val) {
                 mwjoud=1;break;}               
            
            current=current->m_next;
        }
        
        if(version==atomic_load_explicit(&bucket->version, memory_order_acquire)) {
            return !mwjoud;
        }
        
        __asm__ volatile("pause");
    }
}

int remove_item(HM* hm, long val) {
    if(!hm) return 1;
    
    size_t nb=(size_t)labs(val) % hm->nbuckets;
    List* bucket=hm->buckets[nb];
        while (atomic_exchange_explicit(&bucket->lockk, 1, memory_order_acquire));

    Node_HM* current=bucket->sentinel->m_next;
    Node_HM* previous=bucket->sentinel;
    int mwjoud=0;

        while (current) {
            if(current->m_val==val) {
            previous->m_next=current->m_next;
            free(current);
             mwjoud=1;
            break;
            }
        previous=current;
        current=current->m_next;
    }
    
    atomic_fetch_add_explicit(&bucket->version, 1, memory_order_relaxed);
    atomic_store_explicit(&bucket->lockk, 0, memory_order_release);
    return !mwjoud;
}

void print_hashmap(HM* hm) {
    if(!hm) return;
    
    for(size_t i=0; i < hm->nbuckets; i++) {
        List* bucket=hm->buckets[i];
        
         while (atomic_exchange_explicit(&bucket->lockk, 1, memory_order_acquire));        
        Node_HM* current=bucket->sentinel->m_next;
        printf("Bucket %zu", i + 1);

        while (current) {
            printf(" - %ld", current->m_val);
            current=current->m_next;
        }
        printf("\n");
           atomic_store_explicit(&bucket->lockk, 0, memory_order_release);
    }
}
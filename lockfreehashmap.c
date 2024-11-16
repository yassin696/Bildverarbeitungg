#include "chashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>

#define PAD 64


// Allocate a hashmap with the given number of buckets
HM* alloc_hashmap(size_t n_buckets) {
    HM* hm = (HM*)malloc(sizeof(HM));
    if (!hm) { return NULL; }

    hm->nbuckets = n_buckets;
    hm->buckets = (List**)malloc(n_buckets * sizeof(List*));
    if (!hm->buckets) { free(hm); return NULL; }

    for (int i = 0; i < n_buckets; i++) {
        hm->buckets[i] = (List*)malloc(sizeof(List));
        if (!hm->buckets[i]) { free(hm->buckets); free(hm); return NULL; }
        
        hm->buckets[i]->sentinel = (Node_HM*)malloc(sizeof(Node_HM));
        if (!hm->buckets[i]->sentinel) {
            free(hm->buckets[i]);
            free(hm->buckets);
            free(hm);
            return NULL;
        }

        hm->buckets[i]->sentinel->m_next = NULL;
        atomic_init(&hm->buckets[i]->lock, 0); 
    }

    return hm;
}


void free_hashmap(HM* hm) {
    for (int i = 0; i < hm->nbuckets; i++) {
        Node_HM* current = hm->buckets[i]->sentinel->m_next;
        while (current) {
            Node_HM* aux = current;
            free(aux);
            current = current->m_next;
        }
        free(hm->buckets[i]->sentinel);
        free(hm->buckets[i]);
    }
    free(hm->buckets);
    free(hm);
}

int insert_item(HM* hm, long val) {
    int nb = labs(val) % hm->nbuckets;
    List* bucket = hm->buckets[nb];

    while (atomic_exchange(&bucket->lock, 1)) {  
    }

    Node_HM* new_node = (Node_HM*)malloc(sizeof(Node_HM));
    if (!new_node) {
        atomic_store(&bucket->lock, 0);  
        return 1;  
    }

    new_node->m_val = val;
    new_node->m_next = bucket->sentinel->m_next;
    bucket->sentinel->m_next = new_node;

    atomic_store(&bucket->lock, 0);  
    return 0;
}

int lookup_item(HM* hm, long val) {
    int nb = labs(val) % hm->nbuckets;
    List* bucket = hm->buckets[nb];

    while (atomic_exchange(&bucket->lock, 1)) {
    }

    Node_HM* current = bucket->sentinel->m_next;
    while (current) {
        if (current->m_val == val) {
            atomic_store(&bucket->lock, 0);  
            return 0;  
        }
        current = current->m_next;
    }

    atomic_store(&bucket->lock, 0);  
    return 1;  
}

int remove_item(HM* hm, long val) {
    int nb = labs(val) % hm->nbuckets;
    List* bucket = hm->buckets[nb];

    while (atomic_exchange(&bucket->lock, 1)) {
    }

    Node_HM* current = bucket->sentinel->m_next;
    Node_HM* previous = bucket->sentinel;

    while (current) {
        if (current->m_val == val) {
            previous->m_next = current->m_next;
            free(current);
            atomic_store(&bucket->lock, 0);  
            return 0;  
        }
        previous = current;
        current = current->m_next;
    }

    atomic_store(&bucket->lock, 0);  
    return 1;
}

void print_hashmap(HM* hm) {
    for (int i = 0; i < hm->nbuckets; i++) {
        List* bucket = hm->buckets[i];
        Node_HM* current = bucket->sentinel->m_next;

        // Print the bucket header
        printf("Bucket %d", i + 1);

        // Print all the values in the bucket
        while (current) {
            printf(" - %ld", current->m_val);
            current = current->m_next;
        }
        printf("\n");
    }
}

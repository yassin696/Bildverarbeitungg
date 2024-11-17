#include "cspinlock.h"
#include "chashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

HM* alloc_hashmap(size_t n_buckets) {
    HM* hm = (HM*)malloc(sizeof(HM));
    if (!hm) { return NULL; }

    hm->nbuckets = n_buckets;
    hm->buckets = (List**)malloc(n_buckets * sizeof(List*));
    if (!hm->buckets) {
        free(hm);
        return NULL;
    }

    for (size_t i = 0; i < n_buckets; i++) {
        hm->buckets[i] = (List*)malloc(sizeof(List));
        if (!hm->buckets[i]) {
            for (size_t j = 0; j < i; j++) {
                cspin_free(hm->buckets[j]->lock);
                free(hm->buckets[j]->sentinel);
                free(hm->buckets[j]);
            }
            free(hm->buckets);
            free(hm);
            return NULL;
        }

        hm->buckets[i]->sentinel = (Node_HM*)malloc(sizeof(Node_HM));
        if (!hm->buckets[i]->sentinel) {
            for (size_t j = 0; j < i; j++) {
                cspin_free(hm->buckets[j]->lock);
                free(hm->buckets[j]->sentinel);
                free(hm->buckets[j]);
            }
            free(hm->buckets[i]);
            free(hm->buckets);
            free(hm);
            return NULL;
        }

        // Initialize the bucket's lock
        hm->buckets[i]->lock = cspin_alloc();
        if (!hm->buckets[i]->lock) {
            for (size_t j = 0; j < i; j++) {
                cspin_free(hm->buckets[j]->lock);
                free(hm->buckets[j]->sentinel);
                free(hm->buckets[j]);
            }
            free(hm->buckets[i]->sentinel);
            free(hm->buckets[i]);
            free(hm->buckets);
            free(hm);
            return NULL;
        }

        hm->buckets[i]->sentinel->m_next = NULL;
    }

    return hm;
}

void free_hashmap(HM* hm) {
    if (!hm) return;

    for (size_t i = 0; i < hm->nbuckets; i++) {
        if (hm->buckets[i]) {
            Node_HM* current = hm->buckets[i]->sentinel->m_next;
            while (current) {
                Node_HM* next = current->m_next;
                free(current);
                current = next;
            }
            cspin_free(hm->buckets[i]->lock);
            free(hm->buckets[i]->sentinel);
            free(hm->buckets[i]);
        }
    }
    free(hm->buckets);
    free(hm);
}

int insert_item(HM* hm, long val) {
    if (!hm) return 1;

    size_t nb = (size_t)labs(val) % hm->nbuckets;
    List* bucket = hm->buckets[nb];
    
    if (cspin_lock(bucket->lock) != 0) return 1;

    Node_HM* new_node = (Node_HM*)malloc(sizeof(Node_HM));
    if (!new_node) {
        cspin_unlock(bucket->lock);
        return 1;
    }

    new_node->m_val = val;
    new_node->m_next = bucket->sentinel->m_next;
    bucket->sentinel->m_next = new_node;

    cspin_unlock(bucket->lock);
    return 0;
}

int lookup_item(HM* hm, long val) {
    if (!hm) return 1;

    size_t nb = (size_t)labs(val) % hm->nbuckets;
    List* bucket = hm->buckets[nb];

    // Lock only the target bucket
    if (cspin_lock(bucket->lock) != 0) return 1;

    Node_HM* current = bucket->sentinel->m_next;
    while (current) {
        if (current->m_val == val) {
            cspin_unlock(bucket->lock);
            return 0;
        }
        current = current->m_next;
    }

    cspin_unlock(bucket->lock);
    return 1;
}

int remove_item(HM* hm, long val) {
    if (!hm) return 1;

    size_t nb = (size_t)labs(val) % hm->nbuckets;
    List* bucket = hm->buckets[nb];

    if (cspin_lock(bucket->lock) != 0) return 1;

    Node_HM* current = bucket->sentinel->m_next;
    Node_HM* previous = bucket->sentinel;

    while (current) {
        if (current->m_val == val) {
            previous->m_next = current->m_next;
            free(current);
            cspin_unlock(bucket->lock);
            return 0;
        }
        previous = current;
        current = current->m_next;
    }

    cspin_unlock(bucket->lock);
    return 1;
}

void print_hashmap(HM* hm) {
    if (!hm) return;

    for (size_t i = 0; i < hm->nbuckets; i++) {
        List* bucket = hm->buckets[i];
        
        if (cspin_lock(bucket->lock) == 0) {
            Node_HM* current = bucket->sentinel->m_next;

            printf("Bucket %zu", i + 1);

            while (current) {
                printf(" - %ld", current->m_val);
                current = current->m_next;
            }
            printf("\n");
            
            cspin_unlock(bucket->lock);
        }
    }
}
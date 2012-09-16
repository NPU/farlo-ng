/*********************************************************************************
 * Copyright (c) 2012, Chema Garcia                                              *
 * All rights reserved.                                                          *
 *                                                                               *
 * Redistribution and use in source and binary forms, with or                    *
 * without modification, are permitted provided that the following               *
 * conditions are met:                                                           *
 *                                                                               *
 *    * Redistributions of source code must retain the above                     *
 *      copyright notice, this list of conditions and the following              *
 *      disclaimer.                                                              *
 *                                                                               *
 *    * Redistributions in binary form must reproduce the above                  *
 *      copyright notice, this list of conditions and the following              *
 *      disclaimer in the documentation and/or other materials provided          *
 *      with the distribution.                                                   *
 *                                                                               *
 *    * Neither the name of the SafetyBits nor the names of its contributors may *
 *      be used to endorse or promote products derived from this software        *
 *      without specific prior written permission.                               *
 *                                                                               *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"   *
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE     *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE    *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE     *
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF          *
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS      *
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN       *
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)       *
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE    *
 * POSSIBILITY OF SUCH DAMAGE.                                                   *
 *********************************************************************************/

#include <stdlib.h>
#include <pthread.h>

#include "includes/htable.h"

unsigned long htable_sdbm_hash ( unsigned char *str )
{
    unsigned long hash = 0;

    if ( !str )
	return hash;

    while ( *str )
        hash = *str++ + (hash << 6) + (hash << 16) - hash;

    return hash;
}

/* map the hash table */
phtable_t htable_map ( size_t size )
{
    phtable_t ret = 0;

    if ( !size )
        return 0;

    ret = (phtable_t) calloc ( 1 , sizeof ( htable_t ) );
    ret->table = (phtnode_t*) calloc ( size , sizeof ( phtnode_t ) );
    ret->table_size = size;

    return ret;
}

/* insert a pair key-value into the hash table */
int htable_insert ( phtable_t ht  , unsigned long key , void *val )
{
    phtnode_t node = 0;
    phtnode_t aux = 0;
    unsigned int index = 0;

    if ( !ht || !val )
        return 0;

    node = (phtnode_t) calloc ( 1 , sizeof ( htnode_t ) );
    node->key = key;
    node->val = val;

    index = key % ht->table_size;

    if ( ht->table[index] == NULL )
    {
        ht->table[index] = node;
        return 1;
    }

    /* collision resolution by chaining */
    aux = ht->table[index];
    while ( aux->next != 0 )
        aux = aux->next;

    aux->next = node;

    return 1;
}

/* returns the value associated to the given key */
void *htable_find ( phtable_t ht , unsigned long key )
{
    unsigned int index = 0;
    phtnode_t node = 0;

    if ( !ht )
        return 0;

    index = key % ht->table_size;

    node = ht->table[index];
    while ( node != 0 && node->key != key )
        node = node->next;

    if ( !node )
        return 0;

    return node->val;
}

/* removes a key-value pair from the hash table */
void *htable_remove ( phtable_t ht , unsigned long key )
{
    unsigned int index = 0;
    phtnode_t node = 0;
    phtnode_t aux = 0;
    void *ret = 0;

    if ( !ht )
        return 0;

    index = key % ht->table_size;
    node = ht->table[index];

    if ( node->key == key )
        ht->table[index] = node->next;
    else
    {
        while ( node->next != 0 && node->next->key != key )
            node = node->next;

        if ( node->next != 0 )
        {
            aux = node;
            node = node->next;
            aux->next = node->next;
        }
    }

    if ( !node )
        return 0;

    ret = node->val;
    free ( node );

    return ret;
}

/* count the key-value pairs in a hash table */
unsigned int htable_count ( phtable_t ht )
{
    unsigned int i = 0;
    unsigned int ret = 0;
    phtnode_t aux = 0;

    if ( !ht )
        return ret;

    for ( i = 0 ; i < ht->table_size ; i++ )
        for ( aux = ht->table[i] ; aux != 0 ; ret++ , aux = aux->next );

    return ret;
}

/* gets the first key in a hash table */
unsigned int htable_first ( phtable_t ht )
{
    unsigned int ret = 0;
    unsigned int i = 0;

    if ( ! ht )
        return ret;

    for ( i = 0 ; i < ht->table_size && ht->table[i] == 0 ; i++ );

    if ( i < ht->table_size )
        ret = ht->table[i]->key;

    return ret;
}

/* destroys entire hash table */
void htable_destroy ( phtable_t *ht )
{
    unsigned int i = 0;
    phtnode_t aux = 0;

    if ( !ht || !(*ht) )
        return;

    for ( i = 0 ; i < (*ht)->table_size ; i++ )
        while ( (*ht)->table[i] != 0 )
        {
            aux = (*ht)->table[i]->next;
            free ( (*ht)->table[i] );
            (*ht)->table[i] = aux;
        }

    free ( (*ht)->table );
    free ( *ht );

    *ht = 0;

    return;
}

void lock_access ( plock_t lock )
{
    pthread_mutex_lock( &lock->mutex );

    while ( lock->use )
        pthread_cond_wait( &lock->pcond, &lock->mutex );

    lock->use = 1;

    pthread_mutex_unlock( &lock->mutex );

    return;
}

void unlock_access ( plock_t lock )
{
    pthread_mutex_lock( &lock->mutex );

    lock->use = 0;
    pthread_cond_signal( &lock->pcond );

    pthread_mutex_unlock( &lock->mutex );

    return;
}

void free_lockaccess ( plock_t lock )
{
    pthread_cond_destroy( &lock->pcond );
    pthread_mutex_destroy( &lock->mutex );

    return;
}

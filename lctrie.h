
/* To have a wide range of possible machines, supporting c89 forward, as long as there is a 32 bit type */

#ifndef LCTRIE_H
#define LCTRIE_H

#include <limits.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define HAS_C99 1
#else
    #define HAS_C99 0
#endif

#ifndef __cplusplus
    typedef unsigned char bool;
    #define true 1
    #define false 0
#endif

#ifndef NULL
    #define NULL ((void*)0)
#endif

#if HAS_C99
    #include <stdint.h>
    typedef uint32_t uint32;
    typedef uint16_t uint16;
#else

    #if (UINT_MAX == 4294967295)
        typedef unsigned int uint32;
    #elif (ULONG_MAX == 4294967295)
        typedef unsigned long uint32;
    #elif (USHRT_MAX == 4294967295)
        typedef unsigned short uint32;
    #else
        #error "Cannot find 32-bit integer type"
    #endif

    #if (UINT_MAX == 65535)
        typedef unsigned int uint16;
    #elif (USHRT_MAX == 65535)
        typedef unsigned short uint16;
    #else
        #error "Cannot find 16-bit integer type"
    #endif
#endif

typedef int Value;

typedef struct Trie Trie;

/* Regular ip trie */
Trie* alloc_trie();
void free_trie(Trie* trie);

bool trie_insert_ip(Trie* trie, uint32 ip, Value value);
bool trie_remove_ip(Trie* trie, uint32 ip);

Value trie_lookup_ip(Trie* trie, uint32 ip);
Value trie_lookup_ip_top_subnet(Trie* trie, uint32 ip);
Value trie_lookup_ip_buttom_subnet(Trie* trie, uint32 ip);

bool trie_insert_subnet(Trie* trie, uint32 ip, unsigned int subnet_bits, Value value);
bool trie_remove_subnet(Trie* trie, uint32 ip, unsigned int subnet_bits);
Value trie_lookup_subnet(Trie* trie, uint32 ip, unsigned int subnet_bits);

/* Debugging functions for ip trie */

void print_trie(Trie* trie);
void print_all_ips(Trie* trie);
void print_all_subnets(Trie* trie);

/* lctrie with 2 bits per node */

typedef struct LcTrie2 LcTrie2;

LcTrie2* alloc_lctrie2();
void free_trie2(LcTrie2* trie);

bool lctri2_insert_ip(LcTrie2* trie, uint32 ip, Value value);
bool lctri2_remove_ip(LcTrie2* trie, uint32 ip);

Value lctri2_lookup_ip(LcTrie2* trie, uint32 ip);
Value lctri2_lookup_ip_top_subnet(LcTrie2* trie, uint32 ip);
Value lctri2_lookup_ip_buttom_subnet(LcTrie2* trie, uint32 ip);

bool lctri2_insert_subnet(LcTrie2* trie, uint32 ip, unsigned int subnet_bits, Value value);
bool lctri2_remove_subnet(LcTrie2* trie, uint32 ip, unsigned int subnet_bits);
Value lctri2_lookup_subnet(LcTrie2* trie, uint32 ip, unsigned int subnet_bits);

/* Debugging functions for lctrie with 2 bits per node */

void print_lctrie2(LcTrie2* trie);
void print_all_lctrie2_ips(LcTrie2* trie);
void print_all_lctrie2_subnets(LcTrie2* trie);

/* lctrie with 4 bits per node */

typedef struct LcTrie4 LcTrie4;

LcTrie4* alloc_lctrie4();
void free_trie4(LcTrie4* trie);

bool lctri4_insert_ip(LcTrie4* trie, uint32 ip, Value value);
bool lctri4_remove_ip(LcTrie4* trie, uint32 ip);

Value lctri4_lookup_ip(LcTrie4* trie, uint32 ip);
Value lctri4_lookup_ip_top_subnet(LcTrie4* trie, uint32 ip);
Value lctri4_lookup_ip_buttom_subnet(LcTrie4* trie, uint32 ip);

bool lctri4_insert_subnet(LcTrie4* trie, uint32 ip, unsigned int subnet_bits, Value value);
bool lctri4_remove_subnet(LcTrie4* trie, uint32 ip, unsigned int subnet_bits);
Value lctri4_lookup_subnet(LcTrie4* trie, uint32 ip, unsigned int subnet_bits);

/* Debugging functions for lctrie with 2 bits per node */

void print_lctrie4(LcTrie4* trie);
void print_all_lctrie4_ips(LcTrie4* trie);
void print_all_lctrie4_subnets(LcTrie4* trie);

#endif /* LCTRIE_H */
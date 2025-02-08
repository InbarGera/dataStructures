
/* To have a wide range of possible machines, supporting c89 forward, as long as there is a 32 bit type */

#ifndef LCTRIE_H
#define LCTRIE_H

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define HAS_C99 1
#else
    #define HAS_C99 0
#endif

typedef unsigned char bool;

#ifndef NULL
    #define NULL 0
#endif

#if HAS_C99
    #include <stdint.h>
    typedef uint32_t uint32;    
#else

    #if (UINT_MAX == 4294967295U)
        typedef unsigned int uint32;
    #elif (ULONG_MAX == 4294967295U)
        typedef unsigned long uint32;
    #elif (USHORT_MAX == 4294967295U)
        typedef unsigned short uint32;
    #else
        #error "Cannot find 32-bit integer type"
    #endif
#endif

typedef int Value;

typedef struct Node {
    Value value;
    struct Node* zero;
    struct Node* one;
} Node;


typedef struct Trie {
    Node root;
} Trie;


// Regular ip trie
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

// Debugging functions for ip trie

void print_trie(Trie* trie);
void print_all_ips(Trie* trie);
void print_all_subnets(Trie* trie);

// lctrie with 2 bits per node

typedef struct Node2 {
    Value values [2];
    struct Node2* next_nodes[4];
} Node2;


typedef struct LcTrie2 {
    Node2 root;
} LcTrie2;

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

// Debugging functions for lctrie with 2 bits per node

void print_lctrie2(LcTrie2* trie);
void print_all_lctrie2_ips(LcTrie2* trie);
void print_all_lctrie2_subnets(LcTrie2* trie);


#endif // LCTRIE_H
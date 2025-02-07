
#include <stdlib.h>
#include <stdio.h>

#include "./lctrie.h"

static void
init_node(Node* node) {
    node->value = 0;
    node->zero = NULL;
    node->one = NULL;
}

void
free_node(Node* node) {
    if (node == NULL) {
        return;
    }

    free_node(node->zero);
    free_node(node->one);

    free(node);
}

Trie*
alloc_trie() {
    Trie* trie = (Trie*)malloc(sizeof(Trie));
    if (trie == NULL) {
        return NULL;
    }
    
    init_node(&trie->root);

    return trie;
}

void
free_trie(Trie* trie) {
    if (trie == NULL) {
        return;
    }

    free_node(trie->root.zero);
    free_node(trie->root.one);
    free(trie);
}

Node
*alloc_node() {
    Node* node = (Node*)malloc(sizeof(Node));
    if (node == NULL) {
        return NULL;
    }

    init_node(node);

    return node;
}

bool
trie_insert_ip(Trie* trie, uint32 ip, int value) {
    Node* node;
    uint32 mask;

    mask = 0b10000000000000000000000000000000;
    node = &trie->root;

    for (; mask; mask >>= 1) {

        if (ip & (mask)) {
            if (node->one == NULL) {
                node->one = alloc_node();
                if (node->one == NULL) {
                    return 0;
                }
            }
            node = node->one;
        } else {
            if (node->zero == NULL) {
                node->zero = alloc_node();
                if (node->zero == NULL) {
                    return 0;
                }
            }
            node = node->zero;
        }
    }

    node->value = value;

    return 1;
}

bool
trie_remove_ip(Trie* trie, uint32 ip) {
    Node* nodes[33];
    Node* current_node;
    Node* prev_node;
    
    int i;
    uint32 mask;
    bool to_delete_current_node;
    
    mask = 0b10000000000000000000000000000000;
    nodes[0] = &trie->root;
    for (i = 0; mask; i++, mask >>= 1) {
        current_node = nodes[i];

        if (ip & (mask)) {
            if (current_node->one == NULL) {
                return 0;
            }
            nodes[i + 1] = current_node->one;
        } else {
            if (current_node->zero == NULL) {
                return 0;
            }
            nodes[i + 1] = current_node->zero;
        }
    }

    for (i = 31; i >= 0; i--) {
        current_node = nodes[i];

        to_delete_current_node = current_node->value == 0; // If the current node has a value, it means it is a subnet and we can't delete it
        to_delete_current_node = to_delete_current_node && (current_node->zero == NULL || current_node->one == NULL);
            
        if (to_delete_current_node) {    
            continue;
        }

        if (current_node->zero == nodes[i + 1]) {
            current_node->zero = NULL;
        } else {
            current_node->one = NULL;
        }

        free_node(nodes[i + 1]);
        return 1;
    }

    // If we reach here, it means that the root node is the only node in the trie and we need to free its children
    free_node(trie->root.one);
    free_node(trie->root.zero);

    trie->root.one = NULL;
    trie->root.zero = NULL;

    return 1;
}

Value trie_lookup_ip(Trie* trie, uint32 ip);
Value trie_lookup_ip_top_subnet(Trie* trie, uint32 ip);
Value trie_lookup_ip_buttom_subnet(Trie* trie, uint32 ip);

bool trie_insert_subnet(Trie* trie, uint32 ip, int mask, Value value);
bool trie_remove_subnet(Trie* trie, uint32 ip, int mask, Value value);
Value trie_lookup_subnet(Trie* trie, uint32 ip, int mask);


// Debugging functions

void
print_ident(int ident) {
    int i;
    for (i = 0; i < ident; i++) {
        printf("  ");
    }
}

void
print_node(Node* node, int depth, int bit) {
    print_ident(depth);
    printf("bit %d : ", bit);
    
    if (node == NULL) {
        printf("NULL\n");
        return;
    }

    printf("Value: %d\n", node->value);

    print_node(node->zero, depth + 1, 0);
    print_node(node->one, depth + 1, 1);
}   

void
print_trie(Trie* trie) {
    print_node(&trie->root, 0, 0);
}

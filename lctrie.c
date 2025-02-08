
#include <stdlib.h>
#include <stdio.h>

#include "./lctrie.h"

static void
init_node(Node* node) {
    node->value = 0;
    node->zero = NULL;
    node->one = NULL;
}

static Node*
alloc_node() {
    Node* node = (Node*)malloc(sizeof(Node));
    if (node == NULL) {
        return NULL;
    }

    init_node(node);

    return node;
}

static void
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

static void
shrink_path(Node** nodes, int num_nodes) {
    Node* current_node;
    Node* child_node;
    bool should_keep_current_node;
    
    int i;
    for (i = num_nodes - 1; i > 0; i--) {
        current_node = nodes[i];

        // Should keep node if it is a subnet, or if it has 2 children
        should_keep_current_node = current_node->value != 0 || (current_node->zero != NULL && current_node->one != NULL);
        
        if (should_keep_current_node) {    
            break;
        }
    }
    
    current_node = nodes[i];
    child_node = nodes[i + 1];

    if (current_node->zero == child_node) {
        current_node->zero = NULL;
    } else {
        current_node->one = NULL;
    }

    free_node(child_node);
} 

bool
trie_remove_ip(Trie* trie, uint32 ip) {
    Node* nodes[33];
    Node* current_node;
    
    int i;
    uint32 mask;
    
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

    shrink_path(nodes, 32);

    return 1;
}

Value
trie_lookup_ip(Trie* trie, uint32 ip) {
    Node* node;
    uint32 mask;

    mask = 0b10000000000000000000000000000000;
    node = &trie->root;

    for (; mask; mask >>= 1) {
        if (ip & (mask)) {
            if (node->one == NULL) {
                return 0;
            }
            node = node->one;
        } else {
            if (node->zero == NULL) {
                return 0;
            }
            node = node->zero;
        }
    }

    return node->value;
}

Value
trie_lookup_ip_top_subnet(Trie* trie, uint32 ip) {
    Node* node;
    uint32 mask;

    mask = 0b10000000000000000000000000000000;
    node = &trie->root;

    for (; mask; mask >>= 1) {
        if (node->value != 0) {
            return node->value;
        }

        if (ip & (mask)) {
            if (node->one == NULL) {
                return 0;
            }
            node = node->one;
        } else {
            if (node->zero == NULL) {
                return 0;
            }
            node = node->zero;
        }
    }

    return 0;
}

Value
trie_lookup_ip_buttom_subnet(Trie* trie, uint32 ip) {
    Node* node;
    uint32 mask;
    Value last_seen_subnet_value;

    mask = 0b10000000000000000000000000000000;
    node = &trie->root;
    last_seen_subnet_value = 0;

    for (; mask; mask >>= 1) {
        if (node->value != 0) {
            last_seen_subnet_value = node->value;
        }

        if (ip & (mask)) {
            if (node->one == NULL) {
                return last_seen_subnet_value;
            }
            node = node->one;
        } else {
            if (node->zero == NULL) {
                return last_seen_subnet_value;
            }
            node = node->zero;
        }
    }

    return last_seen_subnet_value;
}

bool
trie_insert_subnet(Trie* trie, uint32 ip, unsigned int subnet_bits, Value value) {
    Node* node;
    uint32 mask;
    int i;

    mask = 0b10000000000000000000000000000000;
    node = &trie->root;

    if (subnet_bits > 31) {
        return 0;
    }

    for (i = 0; i < subnet_bits; i++, mask >>= 1) {
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
trie_remove_subnet(Trie* trie, uint32 ip, unsigned int subnet_bits) {
    Node* nodes[33];
    Node* current_node;
    
    int i;
    uint32 mask;

    if (subnet_bits > 31) {
        return 0;
    }
    
    mask = 0b10000000000000000000000000000000;
    nodes[0] = &trie->root;
    for (i = 0; i < subnet_bits; i++, mask >>= 1) {
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

    current_node = nodes[i];
    
    // Not a subnet
    if (current_node->value == 0) {
        return 0;
    }

    current_node->value = 0;

    // Try to remove the subnet
    if (current_node->zero == NULL && current_node->one == NULL) {
        shrink_path(nodes, subnet_bits);
    }

    return 1;
}

Value
trie_lookup_subnet(Trie* trie, uint32 ip, unsigned int subnet_bits) {
    Node* node;
    uint32 mask;
    int i;

    mask = 0b10000000000000000000000000000000;
    node = &trie->root;

    if (subnet_bits > 31) {
        return 0;
    }

    for (i = 0; i < subnet_bits; i++, mask >>= 1) {
        if (ip & (mask)) {
            if (node->one == NULL) {
                return 0;
            }
            node = node->one;
        } else {
            if (node->zero == NULL) {
                return 0;
            }
            node = node->zero;
        }
    }

    return node->value;
}


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
    printf("%d ", depth);
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

static void
print_all_ips_internal(Node* node, uint32 ip, int depth) {
    if (node == NULL) {
        return;
    }

    if (depth == 32) {
        printf("%d.%d.%d.%d - %d\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, node->value);
    }

    print_all_ips_internal(node->zero, ip << 1, depth + 1);
    print_all_ips_internal(node->one, (ip << 1) | 1, depth + 1);
}

void
print_all_ips(Trie* trie) {
    print_all_ips_internal(&trie->root, 0, 0);
}

static void
print_all_subnets_internal(Node* node, uint32 ip, int depth) {
    if (node == NULL || depth == 32) {
        return;
    }

    if (node->value != 0) {
        uint32 ip_to_print = ip << (32 - depth);
        printf("%d.%d.%d.%d/%d - %d\n", (ip_to_print >> 24) & 0xFF, (ip_to_print >> 16) & 0xFF, (ip_to_print >> 8) & 0xFF, ip_to_print & 0xFF, depth, node->value);
    }
    
    print_all_subnets_internal(node->zero, ip << 1, depth + 1);
    print_all_subnets_internal(node->one, (ip << 1) | 1, depth + 1);
}

void
print_all_subnets(Trie* trie) {
    print_all_subnets_internal(&trie->root, 0, 0);
}

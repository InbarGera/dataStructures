
#include <stdlib.h>
#include <stdio.h>

#include "./lctrie.h"

#define FIRST_BIT_ON 0x80000000
#define FIRST_TWO_BITS_ON 0xc0000000

typedef struct Node {
    Value value;
    struct Node* zero;
    struct Node* one;
} Node;

struct Trie {
    Node root;
};

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

    mask = FIRST_BIT_ON;
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

        /* Should keep node if it is a subnet, or if it has 2 children */
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
    
    mask = FIRST_BIT_ON;
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

    mask = FIRST_BIT_ON;
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

    mask = FIRST_BIT_ON;
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

    mask = FIRST_BIT_ON;
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

    mask = FIRST_BIT_ON;
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
    
    mask = FIRST_BIT_ON;
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
    
    /* not a subnet */
    if (current_node->value == 0) {
        return 0;
    }

    current_node->value = 0;

    /* Try to remove the subnet */
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

    mask = FIRST_BIT_ON;
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


/* Debugging functions */

static void
print_ident(int ident) {
    int i;
    for (i = 0; i < ident; i++) {
        printf("  ");
    }
}

static void
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
    uint32 ip_to_print;

    if (node == NULL || depth == 32) {
        return;
    }

    if (node->value != 0) {
        ip_to_print = ip << (32 - depth);
        printf("%d.%d.%d.%d/%d - %d\n", (ip_to_print >> 24) & 0xFF, (ip_to_print >> 16) & 0xFF, (ip_to_print >> 8) & 0xFF, ip_to_print & 0xFF, depth, node->value);
    }
    
    print_all_subnets_internal(node->zero, ip << 1, depth + 1);
    print_all_subnets_internal(node->one, (ip << 1) | 1, depth + 1);
}

void
print_all_subnets(Trie* trie) {
    print_all_subnets_internal(&trie->root, 0, 0);
}


/* LcTrie with 2 bits per node */

typedef struct Node2 {
    Value value;
    Value one_value;
    Value zero_value;
    
    struct Node2* next_nodes[4];
} Node2;


struct LcTrie2 {
    Node2 root;
};

static void
init_node2(Node2* node) {
    node->value = 0;
    node->zero_value = 0;
    node->one_value = 0;
    node->next_nodes[0] = NULL;
    node->next_nodes[1] = NULL;
    node->next_nodes[2] = NULL;
    node->next_nodes[3] = NULL;
}

static Node2*
alloc_node2() {
    Node2* node = (Node2*)malloc(sizeof(Node2));
    if (node == NULL) {
        return NULL;
    }

    init_node2(node);

    return node;
}

static void
free_node2(Node2* node) {
    if (node == NULL) {
        return;
    }

    free_node2(node->next_nodes[0]);
    free_node2(node->next_nodes[1]);
    free_node2(node->next_nodes[2]);
    free_node2(node->next_nodes[3]);

    free(node);
}

LcTrie2*
alloc_lctrie2() {
    LcTrie2* trie = (LcTrie2*)malloc(sizeof(LcTrie2));
    if (trie == NULL) {
        return NULL;
    }
    
    init_node2(&trie->root);

    return trie;
}

void
free_trie2(LcTrie2* trie) {
    if (trie == NULL) {
        return;
    }
    
    free_node2(trie->root.next_nodes[0]);
    free_node2(trie->root.next_nodes[1]);
    free_node2(trie->root.next_nodes[2]);
    free_node2(trie->root.next_nodes[3]);

    free(trie);
}

bool
lctri2_insert_ip(LcTrie2* trie, uint32 ip, Value value) {
    Node2* node;
    uint32 mask;
    int required_shift;
    unsigned int next_two_bits;

    mask = FIRST_TWO_BITS_ON;
    required_shift = 30;
    node = &trie->root;

    for (; mask; mask >>= 2, required_shift -= 2) {
        next_two_bits = (ip & mask) >> required_shift;
        if (node->next_nodes[next_two_bits] == NULL) {
            node->next_nodes[next_two_bits] = alloc_node2();
            if (node->next_nodes[next_two_bits] == NULL) {
                return 0;
            }
        }

        node = node->next_nodes[next_two_bits];
    }

    node->value = value;

    return 1;
}

static void
shrink_path_with_2_bits(Node2** nodes, int num_nodes) {
    Node2* current_node;
    Node2* child_node;
    bool should_keep_current_node;
    
    int i;
    int j;
    int num_children;
    for (i = num_nodes - 1; i > 0; i--) {
        current_node = nodes[i];

        /* Should keep node if it is a subnet (or direct subnet under it), or if it has more than 1 child */
        num_children = 0;
        for (j = 0; j < 4; j++) {
            if (current_node->next_nodes[j] != NULL) {
                num_children++;
            }
        }
        
        should_keep_current_node = current_node->value != 0 || current_node->zero_value != 0 || current_node->one_value != 0;
        should_keep_current_node = should_keep_current_node || num_children > 1;

        if (should_keep_current_node) {    
            break;
        }
    }

    current_node = nodes[i];
    child_node = nodes[i + 1];
    for (j = 0; j < 4; j++) {
        if (current_node->next_nodes[j] == child_node) {
            free_node2(child_node);
            current_node->next_nodes[j] = NULL;
            break;
        }
    }
}

bool
lctri2_remove_ip(LcTrie2* trie, uint32 ip) {
    Node2* nodes[17];
    
    int i;
    uint32 mask;
    int required_shift;
    unsigned int next_two_bits;

    mask = FIRST_TWO_BITS_ON;
    required_shift = 30;
    nodes[0] = &trie->root;
    for (i = 1; mask; i++, mask >>= 2, required_shift -= 2) {
        next_two_bits = (ip & mask) >> required_shift;
        nodes[i] = nodes[i - 1]->next_nodes[next_two_bits];
        if (nodes[i] == NULL) {
            return 0;
        }
    }

    shrink_path_with_2_bits(nodes, 16);

    return 1;
}

Value
lctri2_lookup_ip(LcTrie2* trie, uint32 ip) {
    Node2* node;
    uint32 mask;
    int required_shift;
    unsigned int next_two_bits;

    mask = FIRST_TWO_BITS_ON;
    required_shift = 30;
    node = &trie->root;

    for (; mask; mask >>= 2, required_shift -= 2) {
        next_two_bits = (ip & mask) >> required_shift;
        node = node->next_nodes[next_two_bits];
        if (node == NULL) {
            return 0;
        }
    }

    return node->value;
}

Value
lctri2_lookup_ip_top_subnet(LcTrie2* trie, uint32 ip) {
    Node2* node;
    uint32 mask;
    int required_shift;
    unsigned int next_two_bits;
    unsigned int next_bit;

    mask = FIRST_TWO_BITS_ON;
    required_shift = 30;
    node = &trie->root;

    for (; node && mask; mask >>= 2, required_shift -= 2) {
        if (node->value != 0) {
            return node->value;
        }

        next_two_bits = (ip & mask) >> required_shift;
        
        next_bit = next_two_bits >> 1;
        
        /* TODO - store subnets in a way that enables faster lookup */
        if (next_bit == 1 && node->one_value != 0) {
            return node->one_value;
        }

        if (next_bit  == 0 && node->zero_value != 0) {
            return node->zero_value;
        }

        if (node->value != 0) {
            return node->value;
        }
        
        node = node->next_nodes[next_two_bits];
    }

    return 0;
}

Value
lctri2_lookup_ip_buttom_subnet(LcTrie2* trie, uint32 ip) {
    Node2* node;
    uint32 mask;
    int required_shift;
    unsigned int next_two_bits;
    unsigned int next_bit;
    Value last_seen_subnet_value;

    mask = FIRST_TWO_BITS_ON;
    required_shift = 30;
    node = &trie->root;
    last_seen_subnet_value = 0;

    for (; node && mask; mask >>= 2, required_shift -= 2) {
        
        if (node->value != 0) {
            last_seen_subnet_value = node->value;
        }

        next_two_bits = (ip & mask) >> required_shift;
        next_bit = next_two_bits >> 1;
        
        if (next_bit == 1 && node->one_value != 0) {
            last_seen_subnet_value = node->one_value;
        }

        if (next_bit == 0 && node->zero_value != 0) {
            last_seen_subnet_value = node->zero_value;
        }

        node = node->next_nodes[next_two_bits];
    }

    return last_seen_subnet_value;
}

bool
lctri2_insert_subnet(LcTrie2* trie, uint32 ip, unsigned int subnet_bits, Value value) {
    Node2* node;
    uint32 mask;
    int i;
    unsigned int next_two_bits;
    int next_upper_bit;

    mask = FIRST_TWO_BITS_ON;
    node = &trie->root;

    if (subnet_bits > 31) {
        return 0;
    }

    /* Runnin until hitting the father of the node(s) to assign */
    for (i = 0; i + 2 < subnet_bits; i += 2, mask >>= 2) {
        next_two_bits = (ip & mask) >> (30 - i);
        if (node->next_nodes[next_two_bits] == NULL) {
            node->next_nodes[next_two_bits] = alloc_node2();
            if (node->next_nodes[next_two_bits] == NULL) {
                return 0;
            }
        }

        node = node->next_nodes[next_two_bits];
    }

    /* Last iteration is done here:
       in case we need to assign more that one node due to subnet that is not aligned with 2 bits
     */
    
     /* Case of single node to assign  */
    if (i + 2 == subnet_bits) {
        next_two_bits = (ip & mask) >> (30 - i);
        if (node->next_nodes[next_two_bits] == NULL) {
            node->next_nodes[next_two_bits] = alloc_node2();
            if (node->next_nodes[next_two_bits] == NULL) {
                return 0;
            }
        }

        node->next_nodes[next_two_bits]->value = value;
        return 1;
    }


    /* Case of subnet in the middle, assign to the father node in propper place */

    next_upper_bit = (ip & mask) >> (31 - i); /* One more than the regular iteration */

    if (next_upper_bit == 0) {
        node->zero_value = value;
    } else {
        node->one_value = value;
    }

    return 1;
}

bool
lctri2_remove_subnet(LcTrie2* trie, uint32 ip, unsigned int subnet_bits) {
    Node2* nodes[17];
    Node2* current_node;
    Node2* node_to_remove_subnet_from;
    
    int i;
    uint32 mask;
    unsigned int next_two_bits;

    if (subnet_bits > 31) {
        return 0;
    }
    
    mask = FIRST_TWO_BITS_ON;
    nodes[0] = &trie->root;
    for (i = 0; i + 2 < subnet_bits; i += 2, mask >>= 2) {
        next_two_bits = (ip & mask) >> (30 - i);
        current_node = nodes[i >> 1];

        if (current_node->next_nodes[next_two_bits] == NULL) {
            return 0;
        }

        nodes[(i >> 1) + 1] = current_node->next_nodes[next_two_bits];
    }

    
    if (i + 2 == subnet_bits) {
        next_two_bits = (ip & mask) >> (30 - i);
        current_node = nodes[i >> 1];

        node_to_remove_subnet_from = current_node->next_nodes[next_two_bits];
        if (node_to_remove_subnet_from == NULL) {
            return 0;
        }

        i += 2;

        nodes[(i >> 1)] = node_to_remove_subnet_from;

        node_to_remove_subnet_from->value = 0;
    } else {

        next_two_bits = (ip & mask) >> (30 - i);

        node_to_remove_subnet_from = current_node->next_nodes[next_two_bits];

        if (next_two_bits >> 1 == 0) {
            if (node_to_remove_subnet_from->zero_value == 0) {
                return 0;
            }

            node_to_remove_subnet_from->zero_value = 0;
        } else {
            if (node_to_remove_subnet_from->one_value == 0) {
                return 0;
            }

            node_to_remove_subnet_from->one_value = 0;
        }
    }

    /* Try to remove the subnet */
    if (node_to_remove_subnet_from->value == 0 &&
        node_to_remove_subnet_from->zero_value == 0 &&
        node_to_remove_subnet_from->one_value == 0 &&
        node_to_remove_subnet_from->next_nodes[0] == NULL &&
        node_to_remove_subnet_from->next_nodes[1] == NULL &&
        node_to_remove_subnet_from->next_nodes[2] == NULL &&
        node_to_remove_subnet_from->next_nodes[3] == NULL) {
        
        shrink_path_with_2_bits(nodes, i >> 1);
    }

    return 1;
}

Value
lctri2_lookup_subnet(LcTrie2* trie, uint32 ip, unsigned int subnet_bits) {
    Node2* node;
    uint32 mask;
    int i;
    unsigned int next_two_bits;

    mask = FIRST_TWO_BITS_ON;
    node = &trie->root;

    if (subnet_bits > 31) {
        return 0;
    }

    for (i = 0; i + 2 < subnet_bits; i += 2, mask >>= 2) {
        next_two_bits = (ip & mask) >> (30 - i);
        node = node->next_nodes[next_two_bits];
        if (node == NULL) {
            return 0;
        }
    }

    if (i + 2 == subnet_bits) {
        next_two_bits = (ip & mask) >> (30 - i);
        node = node->next_nodes[next_two_bits];
        if (node == NULL) {
            return 0;
        }

        return node->value;
    }

    if (next_two_bits >> 1 == 0) {
        return node->zero_value;
    } else {
        return node->one_value;
    }
}


/* Debugging functions */

static void
print_node2(Node2* node, int depth, int bit) {
    printf("%d", depth);
    print_ident(depth);
    printf("bit %d%d : ", (bit >>1) %2, bit % 2);
    
    if (node == NULL) {
        printf("NULL\n");
        return;
    }

    printf("Value: %d, 0 value: %d, 1 value: %d\n", node->value, node->zero_value, node->one_value);

    print_node2(node->next_nodes[0], depth + 1, 0);
    print_node2(node->next_nodes[1], depth + 1, 1);
    print_node2(node->next_nodes[2], depth + 1, 2);
    print_node2(node->next_nodes[3], depth + 1, 3);
}  

void
print_lctrie2(LcTrie2* trie) {
    print_node2(&trie->root, 0, 0);
}


static void
print_all_lctrie2_ips_internal(Node2* node, uint32 ip, int depth) {
    if (node == NULL) {
        return;
    }

    if (depth == 16) {
        printf("%d.%d.%d.%d - %d\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, node->value);
    }

    print_all_lctrie2_ips_internal(node->next_nodes[0], ip << 2, depth + 1);
    print_all_lctrie2_ips_internal(node->next_nodes[1], (ip << 2) | 1, depth + 1);
    print_all_lctrie2_ips_internal(node->next_nodes[2], (ip << 2) | 2, depth + 1);
    print_all_lctrie2_ips_internal(node->next_nodes[3], (ip << 2) | 3, depth + 1);
}

void
print_all_lctrie2_ips(LcTrie2* trie) {
    print_all_lctrie2_ips_internal(&trie->root, 0, 0);    
}

static void
print_all_lctrie2_subnets_internal(Node2* node, uint32 ip, int depth) {
    uint32 ip_to_print;

    if (node == NULL || depth == 32) {
        return;
    }

    if (node->value) {
        ip_to_print = ip << (32 - depth);
        printf("%d.%d.%d.%d/%d - %d\n", (ip_to_print >> 24) & 0xFF, (ip_to_print >> 16) & 0xFF, (ip_to_print >> 8) & 0xFF, ip_to_print & 0xFF, depth, node->value);
    }

    if(node->zero_value) {
        ip_to_print = ip << (32 - depth);
        printf("%d.%d.%d.%d/%d - %d\n", (ip_to_print >> 24) & 0xFF, (ip_to_print >> 16) & 0xFF, (ip_to_print >> 8) & 0xFF, ip_to_print & 0xFF, depth + 1, node->zero_value);
    }

    if(node->one_value) {
        ip_to_print = ip << (32 - depth);
        printf("%d.%d.%d.%d/%d - %d\n", (ip_to_print >> 24) & 0xFF, (ip_to_print >> 16) & 0xFF, (ip_to_print >> 8) & 0xFF, ip_to_print & 0xFF, depth + 1, node->one_value);
    }

    print_all_lctrie2_subnets_internal(node->next_nodes[0], ip << 2, depth + 2);
    print_all_lctrie2_subnets_internal(node->next_nodes[1], (ip << 2) | 1, depth + 2);
    print_all_lctrie2_subnets_internal(node->next_nodes[2], (ip << 2) | 2, depth + 2);
    print_all_lctrie2_subnets_internal(node->next_nodes[3], (ip << 2) | 3, depth + 2);
}

void
print_all_lctrie2_subnets(LcTrie2* trie) {
    print_all_lctrie2_subnets_internal(&trie->root, 0, 0);    
}
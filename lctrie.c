
#include <stdlib.h>
#include <stdio.h>

#include "./lctrie.h"

#define FIRST_BIT_ON 0x80000000
#define FIRST_TWO_BITS_ON 0xc0000000
#define FIRST_FOUR_BITS_ON 0xf0000000

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
    Node* nodes_stack[32];
    short leaf_index_stack[32]; 

    int leaf_index;
    int stack_index;
    
    if (node == NULL) {
        return;
    }

    stack_index = 0;
    nodes_stack[stack_index] = node;
    leaf_index_stack[stack_index] = 0;

    while(stack_index >= 0) {
        node = nodes_stack[stack_index];
        leaf_index = leaf_index_stack[stack_index];

        if (leaf_index == 0) {
            leaf_index_stack[stack_index] = leaf_index = 1;
            if (node->zero != NULL) {
                stack_index++;
                nodes_stack[stack_index] = node->zero;
                leaf_index_stack[stack_index] = 0;
                continue;
            }
        }

        if (leaf_index == 1) {
            leaf_index_stack[stack_index] = 2;

            if (node->one != NULL) {
                stack_index++;
                nodes_stack[stack_index] = node->one;
                leaf_index_stack[stack_index] = 0;
                continue;
            }
        }

        /* Leaf index is 2, done with current node */
        free(node);
        stack_index--;
    }
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
    Node2* nodes_stack[16];
    short leaf_index_stack[16]; 

    int leaf_index;
    int stack_index;

    if (node == NULL) {
        return;
    }

    stack_index = 0;
    nodes_stack[stack_index] = node;
    leaf_index_stack[stack_index] = 0;

    while(stack_index >= 0) {
        node = nodes_stack[stack_index];
        leaf_index = leaf_index_stack[stack_index];

        while (leaf_index < 4 && node->next_nodes[leaf_index] == NULL) {
            leaf_index++;
        }

        if (leaf_index == 4) {
            /* Finished with current node, free it and go back to previous node */
            free(node);
            stack_index--;
            continue;
        }

        /* Increment leaf index for next iteration */
        leaf_index_stack[stack_index] = leaf_index + 1;
        
        /* Push next node to stack */
        stack_index++;
        nodes_stack[stack_index] = node->next_nodes[leaf_index];
        leaf_index_stack[stack_index] = 0;
    }
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

    /* Running until hitting the father of the node(s) to assign */
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
    nodes[0] = current_node = &trie->root; /* Putting current_node here is required for subnet_bits == 1 */
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
    next_two_bits = (ip & mask) >> 30; /* This line is required for subnet_bits == 1 */
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


/* LcTrie with 4 bits per node */

/* Static class variable */

/* 
 * Mapping between pair of number of bits and last bits, to the number of the bit in the subnet_bits bitmap
 * bits 0 - 2 are the subnet suffix in a node
 * bits 3 - 4 are the number of bits in the suffix
 * 
 * For example:
 * subnet bits are 01, then the number of bits is 2 (10 in binary) so the coresponds number last 5 bits are 10001
 */
static uint16 LcTrie4_subnet_to_index[32] = {
         0, 0x0000, 0x0000, 0x0000, /* No extra bits - one entry */ 
    0x0000, 0x0000, 0x0000, 0x0000,

         1,      2, 0x0000, 0x0000, /* One extra bit - two entries */
    0x0000, 0x0000, 0x0000, 0x0000,
    
         3,     4,       5,      6, /* Two extra bits - four entries */
    0x0000, 0x0000, 0x0000, 0x0000,
    
        7,      8,       9,     10, /* Three extra bits - all entries */
       11,     12,      13,     14 
};


/*
 * Mapping between the bits in the next node and the subnet bits in the current node
 * Each next node has 4 bits, and the path that those 4 dictates are coresponding with 4 bits in the subnet_bits bitmap
 * In the comment near the values in the table, are the bits that are set in the subnet_bits bitmap
 * 
 * This is the subnet tree indexing:
            0
         /     \
       1         2
      /  \      /  \
     3    4     5    6
    / \  / \   / \  / \
   7  8  9 10 11 12 13 14


   This map shows the path through subnets into each next node:
                      0                          Subnet
               /            \          
            1                     2              Subnet
          /   \                /     \ 
        3       4            5         6         Subnet
       / \     / \         /   \      / \
     7    8    9   10     11   12    13  14      Subnet
    / \  / \  / \  / \   / \   / \  / \   / \ 
    0  1 2  3 4  5 6  7 8  9 10 11 12 13 14 15   Next Nodes

 */

static uint16 LcTrie4_next_node_to_subnet_bits[16] = {
    0x008b, 0x008b, /* 0,1,3,7 */
    0x010b, 0x010b, /* 0,1,3,8 */
    0x0213, 0x0213, /* 0,1,4,9 */
    0x0413, 0x0413, /* 0,1,4,10 */
    0x0825, 0x0825, /* 0,2,5,11 */
    0x1025, 0x1025, /* 0,2,5,12 */
    0x2045, 0x2045, /* 0,2,6,13 */
    0x4045, 0x4045  /* 0,2,6,14 */
};

// TODO - explain thoroughly
/* How many bits are between each 1 bit in the table above
 *
 */
static uint16 LcTrie4_next_node_to_subnet_space[16] = {
    0x0421, 0x0421,
    0x0521, 0x0521,
    0x0531, 0x0531,
    0x0631, 0x0631,
    0x0632, 0x0632,
    0x0732, 0x0732,
    0x0742, 0x0742,
    0x0842, 0x0842 
};

/* Bit map manipulation and query functions - implemented as macros for runtime efficiency */

#define IS_NODE_CHILDREN_ARE_LEAVES(node)   (!!(node->subnet_bits & 0x8000))
#define IS_NODE_CHILDREN_ARE_NODES(node)    (! (node->subnet_bits & 0x8000))
#define SET_NODE_CHILDREN_ARE_LEAVES(node)  (node->subnet_bits |= 0x8000)
#define SET_NODE_CHILDREN_ARE_NODES(node)   (node->subnet_bits &= 0x7fff)

#define SET_NODE_CHILD_BIT(node, index)     (node->children_bits |= (0x1 << index))
#define CLEAR_NODE_CHILD_BIT(node, index)   (node->children_bits &= ~(0x1 << index))
#define IS_NODE_CHILD_BIT_SET(node, index)  (!!(node->children_bits & (0x1 << index)))

#define IS_ANY_SUBNET_BIT_SET(node)         (!!(node->subnet_bits & 0x7fff))
#define IS_ANY_CHILD_BIT_SET(node)          (!!(node->children_bits))

#define IS_ANY_SUBNET_BIT_SET_EXCEPT(node, index)  (!!((node->subnet_bits & 0x7fff) & ~(0x1 << index)))
#define IS_ANY_CHILD_BIT_SET_EXCEPT(node, index)   (!!(node->children_bits & ~(0x1 << index)))

/* Utility for subnet macros */
#define GET_SUBNET_INDEX(last_bits, num_last_bits) (LcTrie4_subnet_to_index[(num_last_bits << 3 | (last_bits & 0x7))])

#define SET_NODE_SUBNET_BIT(node, index)    (node->subnet_bits |= (0x1 << index))
#define CLEAR_NODE_SUBNET_BIT(node, index)  (node->subnet_bits &= ~(0x1 << index))
#define IS_NODE_SUBNET_BIT_SET(node, index) (!!(node->subnet_bits & (0x1 << index)))

#define GET_SUBNET_BITS_FOR_NEXT_NODE(node, index) (node->subnet_bits & LcTrie4_next_node_to_subnet_bits[index])
#define GET_SUBNET_SPACE_FOR_NEXT_NODE(node, index) (LcTrie4_next_node_to_subnet_space[index])

typedef struct Node4 {

    /* 
     * Bitmap for subnets in this node, 1 means there is a subnet in this position
     * There are 15 subnets (including one for this node)
     * The last bit is used to indicate if this node childrens are leaves or nodes
     */
    uint16 subnet_bits;

    /* Bitmap for children, 1 means there is a child in this position */
    uint16 children_bits;

    Value subnet_values[16];

    /*
     * Ip node need only value, so instead of allocating one big node for it, just hold the value
     * The last bit in subnet_bits bitmap indicating if this is a leaf or a node
     */
     union lctrie4_node {
        struct Node4* node;
        Value leaf;
    } next_nodes[16];

} Node4;

struct LcTrie4 {
    Node4 root;
};

static void
init_node4(Node4* node) {
    int i;
    
    node->subnet_bits = 0; /* This includes initializatoin for the union type - node */
    node->children_bits = 0;

    for (i = 0; i < 16; i++) {
        node->subnet_values[i] = 0;
        node->next_nodes[i].node = NULL;
    }
}

static Node4*
alloc_node4() {
    Node4* node = (Node4*)malloc(sizeof(Node4));
    if (node == NULL) {
        return NULL;
    }

    init_node4(node);

    return node;
}

static void
free_node4(Node4* node) {
    Node4* nodes_stack[7];
    short children_index_stack[7]; 

    int child_index;
    int stack_index;

    if (node == NULL) {
        return;
    }

    stack_index = 0;
    nodes_stack[stack_index] = node;
    children_index_stack[stack_index] = 0;

    while(stack_index >= 0) {
        node = nodes_stack[stack_index];
        child_index = children_index_stack[stack_index];

        if (IS_NODE_CHILDREN_ARE_LEAVES(node)) {
            /* Finished with current node, as all of its children are leaves (ip's) */
            free(node);
            stack_index--;
            continue;
        }

        while (child_index < 16 && !IS_NODE_CHILD_BIT_SET(node, child_index)) {
            child_index++;
        }

        if (child_index == 16) {
            /* Finished with current node, free it and go back to previous node */
            free(node);
            stack_index--;
            continue;
        }

        /* Increment leaf index for next iteration */
        children_index_stack[stack_index] = child_index + 1;

        /* Push next node to stack */
        stack_index++;
        nodes_stack[stack_index] = node->next_nodes[child_index].node;
        children_index_stack[stack_index] = 0;
    }
}

LcTrie4*
alloc_lctrie4() {
    LcTrie4* trie = (LcTrie4*)malloc(sizeof(LcTrie4));
    if (trie == NULL) {
        return NULL;
    }
    
    init_node4(&trie->root);

    return trie;
}

void
free_trie4(LcTrie4* trie) {
    int i;
    Node4* root;

    if (trie == NULL) {
        return;
    }
    
    root = &trie->root;
    for(i = 0; i < 16; i++) {
        if (IS_NODE_CHILD_BIT_SET(root, i)) {
            free_node4(root->next_nodes[i].node);
        }
    }

    free(trie);
}

bool
lctri4_insert_ip(LcTrie4* trie, uint32 ip, Value value) {
    Node4* node;
    uint32 mask;
    int required_shift;
    unsigned int next_four_bits;

    mask = FIRST_FOUR_BITS_ON;
    required_shift = 28;
    node = &trie->root;

    for (; required_shift > 0; mask >>= 4, required_shift -= 4) {
        next_four_bits = (ip & mask) >> required_shift;
        if (node->next_nodes[next_four_bits].node == NULL) {
            node->next_nodes[next_four_bits].node = alloc_node4();

            if (node->next_nodes[next_four_bits].node == NULL) {
                return 0;
            }

            SET_NODE_CHILD_BIT(node, next_four_bits);
        }

        node = node->next_nodes[next_four_bits].node;
    }

    /* Last iteration is done outside - to handle the leaf child */
    next_four_bits = (ip & mask);
    node->next_nodes[next_four_bits].leaf = value;
    SET_NODE_CHILDREN_ARE_LEAVES(node);
    SET_NODE_CHILD_BIT(node, next_four_bits);

    return 1;    
}

static void
shrink_path_with_4_bits(Node4** nodes_stack, uint16* bits_stack, int num_nodes) {
    Node4* current_node;
    int i;

    for (i = num_nodes - 1; i > 0; i--) {
        current_node = nodes_stack[i];
        if (IS_ANY_CHILD_BIT_SET_EXCEPT(current_node, bits_stack[i]) || IS_ANY_SUBNET_BIT_SET(current_node)) {    
            break;
        }
    }

    current_node = nodes_stack[i];

    if (IS_NODE_CHILDREN_ARE_NODES(current_node)) {
        free_node4(current_node->next_nodes[bits_stack[i]].node);
        current_node->next_nodes[bits_stack[i]].node = NULL;
    }

    CLEAR_NODE_CHILD_BIT(current_node, bits_stack[i]);
}

/* static*/ void
dbg_print_ip(uint32 ip) {
    uint32 mask = FIRST_FOUR_BITS_ON;
    int shift = 31;
    
    
    printf("IP: ");
    
    while(mask) {
        printf("%d", (ip & mask) >> shift);
        
        shift--;
        mask >>= 1;
    }
    
    printf("\n");
}

/* static */ void
dbg_print_node4_subnets(Node4* node) {
    

    printf("                      %d                          \n", node->subnet_bits & 0x1);
    printf("               /            \\                    \n");
    printf("            %d                     %d              \n", (node->subnet_bits >> 1) & 0x1, (node->subnet_bits >> 2) & 0x1);
    printf("          /   \\                /     \\           \n");
    printf("        %d       %d            %d         %d         \n", (node->subnet_bits >> 3) & 0x1, (node->subnet_bits >> 4) & 0x1, (node->subnet_bits >> 5) & 0x1, (node->subnet_bits >> 6) & 0x1);
    printf("       / \\     / \\         /   \\      / \\        \n");
    printf("     %d    %d   %d   %d        %d   %d     %d  %d        \n", (node->subnet_bits >> 7) & 0x1, (node->subnet_bits >> 8) & 0x1, (node->subnet_bits >> 9) & 0x1, (node->subnet_bits >> 10) & 0x1, (node->subnet_bits >> 11) & 0x1, (node->subnet_bits >> 12) & 0x1, (node->subnet_bits >> 13) & 0x1, (node->subnet_bits >> 14) & 0x1);
    printf("\n\n");
}

bool
lctri4_remove_ip(LcTrie4* trie, uint32 ip) {
    Node4* nodes_stack[8];
    uint16 bits_stack[8];

    Node4* current_node;

    int i;
    uint32 mask;
    unsigned int next_four_bits;
    
    nodes_stack[0] = &trie->root;
    bits_stack[0] = (ip & FIRST_FOUR_BITS_ON) >> 28;
    
    mask = FIRST_FOUR_BITS_ON >> 4;
    for (i = 1; i < 8; i++, mask >>= 4) {
        current_node = nodes_stack[i - 1];
        next_four_bits = bits_stack[i - 1];

        if (!IS_NODE_CHILD_BIT_SET(current_node, next_four_bits)) {
            return 0;
        }

        nodes_stack[i] = current_node->next_nodes[next_four_bits].node;
        bits_stack[i] = (ip & mask) >> (28 - (i * 4));
    }

    current_node = nodes_stack[7];
    next_four_bits = bits_stack[7];
    
    if (IS_ANY_CHILD_BIT_SET_EXCEPT(current_node, next_four_bits) || IS_ANY_SUBNET_BIT_SET(current_node)) {
        /* There are more children or subnets, no need to shrink */
        CLEAR_NODE_CHILD_BIT(current_node, next_four_bits);
        return 1;
    }

    shrink_path_with_4_bits(nodes_stack, bits_stack, 7);

    return 1;
}

Value
lctri4_lookup_ip(LcTrie4* trie, uint32 ip) {
    Node4* node;
    uint32 mask;
    int required_shift;
    unsigned int next_four_bits;

    mask = FIRST_FOUR_BITS_ON;
    required_shift = 28;
    node = &trie->root;

    for (; required_shift > 0; mask >>= 4, required_shift -= 4) {
        next_four_bits = (ip & mask) >> required_shift;
        if (!IS_NODE_CHILD_BIT_SET(node, next_four_bits)) {
            return 0;
        }

        node = node->next_nodes[next_four_bits].node;
    }

    next_four_bits = (ip & mask);
    if (IS_NODE_CHILD_BIT_SET(node, next_four_bits)) {
        return node->next_nodes[next_four_bits].leaf;
    }

    return 0;
}

Value
lctri4_lookup_ip_top_subnet(LcTrie4* trie, uint32 ip) {
    Node4* node;
    uint32 mask;
    int required_shift;
    unsigned int next_four_bits;
    uint16 subnet_bits;
    uint16 subnet_bits_space;
    uint16 space;
    uint16 total_space;

    mask = FIRST_FOUR_BITS_ON;
    required_shift = 28;
    node = &trie->root;

    for (; mask; mask >>= 4, required_shift -= 4) {
        next_four_bits = (ip & mask) >> required_shift;
        
        subnet_bits = GET_SUBNET_BITS_FOR_NEXT_NODE(node, next_four_bits);

        if (subnet_bits) {
            /* This node has at least one subnet that controls that ip in this path */

            /* Searching through all 4 subnets */

            /* Top subnet */
            if (subnet_bits & 0x1) {
                return node->subnet_values[0];
            }

            subnet_bits_space = GET_SUBNET_SPACE_FOR_NEXT_NODE(node, next_four_bits);

            /* Second subnet */

            total_space = subnet_bits_space & 0xf;
            subnet_bits >>= total_space;

            if (subnet_bits & 0x1) {
                return node->subnet_values[total_space];
            }

            /* Third subnet */

            space = (subnet_bits_space >> 4) & 0xf;
            total_space += space;
            subnet_bits >>= space;

            if (subnet_bits & 0x1) {
                return node->subnet_values[total_space];
            }

            /* Fourth subnet */

            total_space += (subnet_bits_space >> 8) & 0xf;

            /* If the logic is correct, the third one must be on, so no need to check it */
            return node->subnet_values[total_space];
        }

        /* No subnets in this node, continue to the next one */

        if (!IS_NODE_CHILD_BIT_SET(node, next_four_bits)) {
            return 0;
        }

        node = node->next_nodes[next_four_bits].node;
    }

    /* No subnet is found in the path */
    return 0;
}

Value lctri4_lookup_ip_buttom_subnet(LcTrie4* trie, uint32 ip);

bool
lctri4_insert_subnet(LcTrie4* trie, uint32 ip, unsigned int subnet_bits, Value value) {
    Node4* node;
    uint32 mask;
    int i;
    unsigned int next_four_bits;
    unsigned int subnet_depth_in_node;
    

    mask = FIRST_FOUR_BITS_ON;
    node = &trie->root;

    if (subnet_bits > 31) {
        return 0;
    }

    /* Running until hitting the node to assign */
    for (i = 0; i + 3 < subnet_bits; i += 4, mask >>= 4) {
        next_four_bits = (ip & mask) >> (28 - i);
        if (!IS_NODE_CHILD_BIT_SET(node, next_four_bits)) {
            node->next_nodes[next_four_bits].node = alloc_node4();
            if (node->next_nodes[next_four_bits].node == NULL) {
                return 0;
            }

            SET_NODE_CHILD_BIT(node, next_four_bits);
        }

        node = node->next_nodes[next_four_bits].node;
    }

    /* Finish creation - setting last node to leaf children if it i sthe last in the tree */
    if (subnet_bits > 28) {
        SET_NODE_CHILDREN_ARE_LEAVES(node);
    }

    /* How many bits of the subnet are in this node (supposed to be equivalent to subnet_bits % 4)*/
    subnet_depth_in_node = subnet_bits - i;

    /*
     * Fetching the "number" of the subnet. 
     * If no more bits are required, the it should be 0.
     * If one bit is left, it should be the first relevant bit.
     * If two bits are left, it should be the first two bits.
     * If three bits are left, it should be the first three bits.
     */
    next_four_bits = (ip & mask) >> ((28 - i) + (4 - subnet_depth_in_node));

    i = GET_SUBNET_INDEX(next_four_bits, subnet_depth_in_node);

    SET_NODE_SUBNET_BIT(node, i);
    node->subnet_values[i] = value;

    return 1;
}

bool
lctri4_remove_subnet(LcTrie4* trie, uint32 ip, unsigned int subnet_bits) {
    Node4* nodes_stack[8];
    uint16 bits_stack[8];

    Node4* current_node;

    int i;
    int subnet_index;
    int subnet_depth_in_node;
    uint32 mask;
    unsigned int next_four_bits;

    if (subnet_bits > 31) {
        return 0;
    }
    
    nodes_stack[0] = current_node = &trie->root;
    bits_stack[0] = (ip & FIRST_FOUR_BITS_ON) >> 28;
    
    mask = FIRST_FOUR_BITS_ON >> 4;
    for (i = 1; i*4 -1 < subnet_bits; i++, mask >>= 4) {
        current_node = nodes_stack[i - 1];
        next_four_bits = bits_stack[i - 1];

        if (!IS_NODE_CHILD_BIT_SET(current_node, next_four_bits)) {
            return 0;
        }

        nodes_stack[i] = current_node->next_nodes[next_four_bits].node;
        bits_stack[i] = (ip & mask) >> (28 - (i * 4));
    }

    /* Revert last iteration */
    i--;
    mask <<= 4;

    current_node = nodes_stack[i];
    next_four_bits = bits_stack[i];
    
    /*
    * Fetching the "number" of the subnet. 
    * If no more bits are required, it should be 0.
    * If one bit is left, it should be the first relevant bit.
    * If two bits are left, it should be the first two bits.
    * If three bits are left, it should be the first three bits.
    */
    subnet_depth_in_node = subnet_bits - (i*4);
    next_four_bits = (ip & mask) >> ((28 - (i*4)) + (4 - subnet_depth_in_node));
    subnet_index = GET_SUBNET_INDEX(next_four_bits, subnet_depth_in_node);

    if (!IS_NODE_SUBNET_BIT_SET(current_node, subnet_index)) {
        return 0;
    }

    if (IS_ANY_SUBNET_BIT_SET_EXCEPT(current_node, subnet_index) || IS_ANY_CHILD_BIT_SET(current_node)) {
        CLEAR_NODE_SUBNET_BIT(current_node, subnet_index);
        current_node->subnet_values[subnet_index] = 0; /* Not necessary */
        return 1;
    }

    shrink_path_with_4_bits(nodes_stack, bits_stack, i);

    return 1;
}

Value
lctri4_lookup_subnet(LcTrie4* trie, uint32 ip, unsigned int subnet_bits) {
    Node4* node;
    uint32 mask;
    int i;
    unsigned int next_four_bits;
    unsigned int subnet_depth_in_node;

    mask = FIRST_FOUR_BITS_ON;
    node = &trie->root;

    if (subnet_bits > 31) {
        return 0;
    }

    /* Running until hitting the node to assign */
    for (i = 0; i + 3 < subnet_bits; i += 4, mask >>= 4) {
        next_four_bits = (ip & mask) >> (28 - i);
        if (!IS_NODE_CHILD_BIT_SET(node, next_four_bits)) {
            return 0;
        }

        node = node->next_nodes[next_four_bits].node;
    }

    /* How many bits of the subnet are in this node (supposed to be equivalent to subnet_bits % 4)*/
    subnet_depth_in_node = subnet_bits - i;

    /*
     * Fetching the "number" of the subnet. 
     * If no more bits are required, the it should be 0.
     * If one bit is left, it should be the first relevant bit.
     * If two bits are left, it should be the first two bits.
     * If three bits are left, it should be the first three bits.
     */
    next_four_bits = (ip & mask) >> ((28 - i) + (4 - subnet_depth_in_node));

    i = GET_SUBNET_INDEX(next_four_bits, subnet_depth_in_node);
    
    if (IS_NODE_SUBNET_BIT_SET(node, i)) {
        return node->subnet_values[i];
    }

    return 0;
}

/* Debugging functions for lctrie with 2 bits per node */

/* static */ void
print_node4(Node4* node, int depth, int bit) {
    int i;
    
    if (node == NULL) {
        printf("%d:", depth);
        print_ident(depth);
        printf("bit %d%d%d%d : NULL\n",(bit >>3) %2, (bit >>2) %2, (bit >>1) %2, bit % 2);
        return;
    }

    for (i=0; i < 15; ++i) {
        if (IS_NODE_SUBNET_BIT_SET(node, i)) {
            printf("%d:", depth);
            print_ident(depth);
            printf("subnet %d%d%d%d : %d\n",(i >>3) %2, (i >>2) %2, (i >>1) %2, i % 2, node->subnet_values[i]);
        }
    }

    if (IS_NODE_CHILDREN_ARE_LEAVES(node)) {
        
        for (i = 0; i < 16; i++) {
            if (IS_NODE_CHILD_BIT_SET(node, i)) {
                printf("%d:", depth + 1);
                print_ident(depth + 1);
                printf("bit %d%d%d%d : %d\n",(i >>3) %2, (i >>2) %2, (i >>1) %2, i % 2, node->next_nodes[i].leaf);
            }
        }
        return;
    }

    printf("%d:", depth);
    print_ident(depth);
    printf("bit %d%d%d%d :\n",(bit >>3) %2, (bit >>2) %2, (bit >>1) %2, bit % 2);    

    for (i = 0; i < 16; i++) {
        if (IS_NODE_CHILD_BIT_SET(node, i)) {
            print_node4(node->next_nodes[i].node, depth + 1, i);
        }
    }
}   

void
print_lctrie4(LcTrie4* trie) {
    print_node4(&trie->root, 0, 0);
}


void print_all_lctrie4_ips(LcTrie4* trie);
void print_all_lctrie4_subnets(LcTrie4* trie);

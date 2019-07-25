#include "AVL_Tree.h"
#include <stdlib.h>
using std::max;

template <typename T>
void AVLTree<T>::balance(node* to_balance){
    int left = -1;
    int right = -1;
    int right_right, right_left, left_right, left_left, t_left, t_right;
    node* temp, *bandit;

    if(to_balance->left){
        left = to_balance->left->height;
    }
    if(to_balance->right){
        right = to_balance->right->height;
    }
    to_balance->height = max(right, left) + 1;
    if(right - left <= 1 && left - right <= 1){
        if(to_balance->father){
            balance(to_balance->father);
        }
        return;
    }
    // needs to balance

    if(right > left){
        temp = to_balance->right;
        right_right = -1;
        right_left = -1;
        if(temp->left){
            right_left = temp->left->height;
        }
        if(temp->right){
            right_right = temp->right->height;
        }
        if(right_right >= right_left){
            to_balance->right = temp->left;
            if(temp->left){
                temp->left->father = to_balance;
            }
            to_balance->height = 1 + max(left, right_left);
            temp->left = to_balance;
            temp->height = 1 + max(to_balance->height, right_right);
            temp->father = to_balance->father;
            to_balance->father = temp;
            if(!temp->father){ //to_balance was the root
                root = temp;
            }
            else{
                if(temp->father->right == to_balance){
                    temp->father->right = temp;
                }
                else{
                    temp->father->left = temp;
                }
                balance(temp->father);
            }
        }
        else{
            bandit = temp->left;
            t_left = -1;
            t_right = -1;
            if(bandit->left){
                t_left = temp->left->left->height;
            }
            if(bandit->right){
                t_right = temp->left->right->height;
            }

            to_balance->right = bandit->left;
            if(to_balance->right){
                to_balance->right->father = to_balance;
            }
            to_balance->height = 1 + max(left, t_left);

            temp->left = bandit->right;
            if(t_right != -1){
                temp->left->father = temp;
            }
            temp->height = 1 + max(t_right, right_right);
            bandit->right = temp;
            temp->father = bandit;
            bandit->left = to_balance;
            bandit->father = to_balance->father;
            to_balance->father = bandit;
            bandit->height = 1 + max(to_balance->height, temp-> height);
            if(!bandit->father){
                root = bandit;
            } else{
                if(bandit->father->right == to_balance){
                    bandit->father->right = bandit;
                } else{
                    bandit->father->left = bandit;
                }
                balance(bandit->father);
            }
        }
    }
    else{ //same code just mirrored the directions, made automatically
        temp = to_balance->left;
        left_left = -1;
        left_right = -1;
        if(temp->right){
            left_right = temp->right->height;
        }
        if(temp->left){
            left_left = temp->left->height;
        }
        if(left_left >= left_right){
            to_balance->left = temp->right;
            if(temp->right){
                temp->right->father = to_balance;
            }
            to_balance->height = 1 + max(right, left_right);
            temp->right = to_balance;
            temp->height = 1 + max(to_balance->height, left_left);
            temp->father = to_balance->father;
            to_balance->father = temp;
            if(!temp->father){ //to_balance was the root
                root = temp;
            }
            else{
                if(temp->father->left == to_balance){
                    temp->father->left = temp;
                }
                else{
                    temp->father->right = temp;
                }
                balance(temp->father);
            }
        }
        else{
            bandit = temp->right;
            t_right = -1;
            t_left = -1;
            if(bandit->right){
                t_right = temp->right->right->height;
            }
            if(bandit->left){
                t_left = temp->right->left->height;
            }

            to_balance->left = bandit->right;
            if(to_balance->left){
                to_balance->left->father = to_balance;
            }
            to_balance->height = 1 + max(right, t_right);

            temp->right = bandit->left;
            if(t_left != -1){
                temp->right->father = temp;
            }
            temp->height = 1 + max(t_left, left_left);
            bandit->left = temp;
            temp->father = bandit;
            bandit->right = to_balance;
            bandit->father = to_balance->father;
            to_balance->father = bandit;
            bandit->height = 1 + max(to_balance->height, temp-> height);
            if(!bandit->father){
                root = bandit;
            } else{
                if(bandit->father->left == to_balance){
                    bandit->father->left = bandit;
                } else{
                    bandit->father->right = bandit;
                }
                balance(bandit->father);
            }
        }
    }
}

template <typename T>
T* AVLTree<T>::pop(int key){
    if(size == 0){
        return NULL;
    }

    T* to_return = NULL;
    node* current = root, *father, *temp;

    while(current->key != key){
        if(current->key < key){
            if(current->right){
                current = current->right;
                continue;
            }
            return NULL;
        }
        else{
            if(current->left){
                current = current->left;
                continue;
            }
            return NULL;
        }
    }

    // current exist and is the required one.
    --size;
    to_return = current->data;

    if(current->left && current->right){
        // added some randomness to this part, to search through both directions.
        if(rand()%2) {
            if (!current->right->left) {
                temp = current->right;
                current->data = current->right->data;
                current->key = current->right->key;
                current->right = current->right->right;
                if (current->right) {
                    current->right->father = current;
                }
                balance(current);
                delete (temp);
                return to_return;
            } else {
                temp = current->right;
                while (temp->left) {
                    temp = temp->left;
                }
                current->data = temp->data;
                current->key = temp->key;
                temp->father->left = temp->right;
                if (temp->right) {
                    temp->right->father = temp->father;
                }
                balance(temp->father);
                delete (temp);
                return to_return;
            }
        }
        else{ // same as above, just searching through the left side instead of the right side
            if (!current->left->right) {
                temp = current->left;
                current->data = current->left->data;
                current->key = current->left->key;
                current->left = current->left->left;
                if (current->left) {
                    current->left->father = current;
                }
                balance(current);
                delete (temp);
                return to_return;
            } else {
                temp = current->left;
                while (temp->right) {
                    temp = temp->right;
                }
                current->data = temp->data;
                current->key = temp->key;
                temp->father->right = temp->left;
                if (temp->left) {
                    temp->left->father = temp->father;
                }
                balance(temp->father);
                delete (temp);
                return to_return;
            }
        }
    }

    father = current->father;
    if(!father){ //deleting the root
        if(!current->left && !current->right){
            root = NULL;
            delete(current);
            return to_return;
        }

        if(current->right){
            root = current->right;
            current->right->father = NULL;
        }
        else{ //current->left
            root = current->left;
            current->left->father = NULL;
        }
        delete(current);
        return to_return;
    }

    if(!current-> left && !current->right){
        if(current == father->right){
            father->right = NULL;
            if(!father->left){
                father->height = 0;
                balance(father);
            }
        }
        else{
            father->left = NULL;
            if(!father->right){
                father->height = 0;
                balance(father);
            }
        }
        delete(current);
        balance(father);
        return to_return;
    }

    if(!current->left){
        if(father->right == current){
            father->right = current->right;
        }
        else{
            father->left = current->right;
        }
        current->right->father = father;
    }
    else{ // !current->right
        if(father->right == current){
            father->right = current->left;
        }
        else{
            father->left = current->left;
        }
        current->left->father = father;
    }
    father->height = max(father->left->height, father->right->height) + 1;
    balance(father);

    delete(current);
    return to_return;
}

template <typename T>
T* AVLTree<T>::insert(int key, T* data){
    if(size == 0){
        root = new node(key, data, 0);
        size = 1;
        return NULL;
    }

    node* current = root;
    while(true){
        if(current->key == key){
            T* old_data = current->data;
            current->data = data;
            return old_data;
        }

        if(current->key < key){
            if(current->right){
                current = current->right;
                continue;
            }
            current->right = new node(key, data, current);
            if(!current->left) {
                current->height = 1;
                balance(current);
            }
            size ++;
            return NULL;
        }
        else{
            if(current->left){
                current = current->left;
                continue;
            }
            current->left = new node(key, data, current);
            if(!current->right) {
                current->height = 1;
                balance(current);
            }
            size ++;
            return NULL;
        }
    }
}

template <typename T>
T* AVLTree<T>::get(int key){
    node* current = root;
    while(current != NULL){
        if (current->key == key){
            return current->data;
        }
        if (current->key < key){
            current = current->right;
        }
        else{
            current = current->left;
        }
    }
    return NULL;
}

template <typename T>
bool AVLTree<T>::contains(int key){
    return get(key) != NULL;
}

template <typename T>
AVLTree<T>::~AVLTree(){
    if(root){
        root->del();
    }
}

template <typename T>
AVLTree<T>::AVLTree(): size(0), root(NULL) {}

template <typename T>
AVLTree<T>::AVLTree(const AVLTree& avi): size(avi.size), root(NULL){
    if(!avi.root){
        return;
    }
    root = new node(avi.root);
}

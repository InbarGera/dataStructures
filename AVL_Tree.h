//
// Created by Inbar Gera on 25/07/2019.
//

#ifndef AVL_TREE_AVL_TREE_H
#define AVL_TREE_AVL_TREE_H

#include <iostream>
using std::max;

template<typename T>
class AVLTree{
    class node{
    public:
        // for debugging
        int size(){
            int sum = 1;
            if(left){
                sum += left->size();
            }
            if(right){
                sum += right->size();
            }
            return sum;
        }
        // for debugging
        bool valid(){
            int r = -1, l =-1;
            if(left){
                l = left->height;
                if(left->father != this){
                    std::cout << "node with id " << left->key << "thinks her father is " << (left->father ? -1 : left->father->key) << " but the real father is " << this->key << std::endl;
                    return false;
                }
                if(!left->valid()){
                    return false;
                }
            }

            if(right){
                r = right->height;
                if(right->father != this){
                    std::cout << "node with id " << right->key << "thinks her father is " << (right->father ? -1 : right->father->key) << " but the real father is " << this->key << std::endl;
                    return false;
                }
                if(!right->valid()){
                    return false;
                }
            }

            if(height != max(l, r) + 1){
                std::cout << "node with id " << this->key << " has problem with heights, her height is " << this->height << ", left son height is " << (left ? left->height : -1) << ", right son is " << (right ? right->height : -1) << std::endl;
                return false;
            }
            if(l - r > 1 || r - l > 1){
                std::cout << "node with id " << this->key << " is not balanced, her height is " << height << ", left son height is " << (left ? left->height : -1) << ", right son is " << (right ? right->height : -1) << std::endl;
                return false;
            }
            return true;
        }


        node(node* n): key(n->key), data(n->data),height(n->height), left(NULL), right(NULL), father(NULL){
            if(n->left){
                left = new node(n->left);
                left->father = this;
            }
            if(n->right){
                right = new node(n->right);
                right->father = this;
            }
        }

        void del(){
            if(left){
                left->del();
            }
            if(right){
                right->del();
            }
            delete(this);
        }

        T* data;
        int key, height;
        node* left, *right, *father;
        node(int key, T* data, node* father): key(key), data(data), height(0), left(NULL), right(NULL), father(father){};
    };

    int size;
    node* root;
    void balance(node* current);

protected:
    // for debugging
    bool valid(){
        if(size == 0){
            if(root){
                std::cout << "tree has problem with keeping size, its size is " << size << " when real size is " << (root ? root->size() : 0) << std::endl;
                return false;
            }
            return !root;
        }
        if(size != root->size()){
            std::cout << "tree has problem with keeping size, its size is " << size << " when real size is " << (root ? root->size() : 0) << std::endl;
            return false;
        }
        return root->valid();
    };
public:
    AVLTree();
    AVLTree(const AVLTree& avi);
    ~AVLTree();

    T* get(int key); // returns the data associated with the key if it exist, else NULL.
    T* insert(int key, T* data); // returns the previous data if exist, else NULL.
    T* pop(int key); // returns the data associated with the key if it exist, and delete in from the tree.
    bool contains(int key); // answer whether the key exist or not.
};

#endif //AVL_TREE_AVL_TREE_H

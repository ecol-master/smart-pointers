#include "./src/shared/shared.h"
#include "./src/weak/weak.h"
#include <iostream>

// A simple Node structure to demonstrate smart pointer usage
struct Node {
    int value;
    SharedPtr<Node> next; 
    WeakPtr<Node> prev;  

    Node(int val) : value(val) {
        std::cout << "Node created with value " << value << std::endl;
    }

    ~Node() {
        std::cout << "Node with value " << value << " destroyed" << std::endl;
    }
};

int main() {
    // Create the head node with a unique pointer
    SharedPtr<Node> head(new Node(1));
    

    SharedPtr<Node> second = MakeShared<Node>(2);
    head->next = second;

    SharedPtr<Node> third = MakeShared<Node>(3);
    second->next = third;

    second->prev = head;
    third->prev = second;

    if (auto lockedPrev = third->prev.Lock()) {
        std::cout << "The previous node to third has value: " << lockedPrev->value << std::endl;
    } else {
        std::cout << "The previous node has expired." << std::endl;
    }

    // Calling Destructors for pointers
}

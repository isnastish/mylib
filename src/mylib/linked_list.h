#pragma once 

#include "arena.h"

namespace mylib
{
template<class Object>
class LinkedList {
public:
    class const_iterator;
    class iterator;

    explicit LinkedList(Arena* arena);
    ~LinkedList();
    
private:
    struct ListNode {
        Object* data;
        ListNode* next;
        ListNode* prev;
    };

    Arena*    m_arena;
    Chunk*    m_memory;
    ListNode* m_nodes;
};



} // namespace mylib
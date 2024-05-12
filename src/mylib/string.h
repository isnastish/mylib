#pragma once

#include "arena.h"
#include <ostream>
#include <initializer_list>

namespace mylib
{
class String {
public:
    explicit String(const char *src);
    explicit String(const std::string& src);
    String(std::initializer_list<const char *> list);
    ~String();

    String(const String& rhs);
    String& operator=(const String& rhs);
    String(String&& rhs);
    String& operator=(String&& rhs);

    size_t size() const { return m_size; }

    bool operator==(const String& str);
    bool operator!=(const String& str);
    String& operator+=(const String& str);

    friend String operator+(const String& a, const String& b);
    friend String operator+(const char *a, const String& b);
    friend String operator+(const String& a, const char *b);

    friend std::ostream& operator<<(std::ostream& os, const String& rhs); 
private:
    void zeroMembers();

    Arena* m_arena;
    Chunk* m_memory;
    char *m_data{nullptr}; // remove
    size_t m_size; 
};

} // namespace mylib
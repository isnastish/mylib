#include "string.h"
#include <cstring>

namespace ml
{
    String::String(const std::string& src)
    : m_size(src.size()) {
        m_data = new char[m_size + 1]{};
        memcpy(m_data, src.c_str(), m_size);
        m_data[m_size] = 0;
    }

    String::String(const char *str) {
        size_t size = std::strlen(str);
        m_data = new char[size + 1]{};
        memcpy(m_data, str, size);
        m_data[size] = 0;
        m_size = size;
    }

    String::~String() { delete[]m_data; }  

    String::String(const String& rhs)
    : m_size(rhs.m_size) {
        m_data = new char[m_size + 1]{};
        memcpy(m_data, rhs.m_data, m_size + 1);
    }

    String& String::operator=(const String& rhs) {
        if (this == &rhs) return *this;
        char *tmp = m_data;
        m_size = rhs.m_size;
        m_data = new char[m_size + 1]{};
        memcpy(m_data, rhs.m_data, rhs.m_size + 1);
        delete []tmp;
        return *this;
    }

    String::String(String&& rhs)
    : m_size(rhs.m_size), m_data(rhs.m_data)
    { rhs.zeroMembers(); }

    String& String::operator=(String&& rhs) {
        if (this == &rhs) return *this;
        char* tmp = m_data;
        m_data = rhs.m_data;
        m_size = rhs.m_size;
        rhs.zeroMembers();
        delete []tmp;
        return *this;
    }

    void String::zeroMembers()
    { m_data = nullptr; m_size =  0; }

    bool String::operator==(const String& str) {
        if (this->m_size != str.m_size)
            return false;
        for (size_t i = 0; i < m_size; i++)
            if (m_data[i] != str.m_data[i])
                return false;
        return true;
    }

    bool String::operator!=(const String& str)
    { return !(*this == str); }
    
    String operator+(const String& a, const String& b) {
        // TODO: Pull out this operation into a function
        size_t size = a.m_size + b.m_size;
        char *data = new char[size + 1]{};
        memcpy(data, a.m_data, a.m_size);
        memcpy(data + a.m_size, b.m_data, b.m_size);
        data[size] = 0;
        String res(data);
        delete[]data;
        return std::move(res);
    } 

    String operator+(const char *a, const String& b) {
        size_t a_size = std::strlen(a);
        size_t size = a_size + b.m_size;
        char *data = new char[size + 1]{};
        memcpy(data, a, a_size);
        memcpy(data + a_size, b.m_data, b.m_size);
        data[size] = 0;
        String res(data);
        return std::move(res);
    }

//     String operator+(const String& a, const char *b)
//     {
// #if 0
//         size_t b_size = std::strlen(b);
//         size_t new_str_size = b_size + a.m_size;
//         char *data = new char[new_str_size + 1]{};
//         memcpy(data, a, a_size);
//         memcpy(data + a_size, b.m_data, b.m_size);
//         data[new_str_size] = 0;
//         String res(data);
//         return std::move(res);
// #endif
//         return String("str");
//     }

    std::ostream& operator<<(std::ostream& os, const String& rhs) {
        os << rhs.m_data;
        return os;
    }
}

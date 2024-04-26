#include "string.h"
#include <cstring>

namespace ml
{
    String::String(const char *str)
    {
        size_t size = std::strlen(str);
        m_data = new char[size + 1]{};
        memcpy(m_data, str, size);
        m_data[size] = 0;
    }

    String::~String() { delete[]m_data; }  

    String::String(const String& rhs)
    : m_size(rhs.m_size)
    {
        m_data = new char[m_size + 1]{};
        memcpy(m_data, rhs.m_data, m_size + 1);
    }

    String& String::operator=(const String& rhs)
    {
        if (this == &rhs)
            return *this;

        delete []m_data;
        m_size = rhs.m_size;
        m_data = new char[m_size + 1]{};
        memcpy(m_data, rhs.m_data, rhs.m_size + 1);

        return *this;
    }

    String::String(String&& rhs)
    : m_size(rhs.m_size), m_data(rhs.m_data)
    {
        rhs.m_data = nullptr;
        rhs.m_size =  0;
    }

    std::ostream& operator<<(std::ostream& os, const String& rhs)
    {
        os << rhs.m_data;
        return os;
    }
}

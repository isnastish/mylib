#pragma once

#include <ostream>

namespace ml
{
    class String
    {
    public:
        explicit String(const char *str);
        ~String();

        String(const String& rhs);
        String& operator=(const String& rhs);
        String(String&& rhs);

        friend std::ostream& operator<<(std::ostream& os, const String& rhs); 
    private:
        char *m_data{nullptr};
        size_t m_size;
    };
}

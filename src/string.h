#pragma once

#include <ostream>
#include <initializer_list>

namespace ml
{
    class String
    {
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

        char *m_data{nullptr};
        size_t m_size;
    };

    template<class T>
    concept string_convertible = std::convertible_to<T, const char *>;

    // TODO: Experiment with templates and concepts instead of overloads
    // template<class T, class K>
    String operator+(const String&a, const String& b);
    String operator+(const char *a, const String& b);
    String operator+(const String& a, const char *b);
} // namespace ml
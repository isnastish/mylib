#include "./containers/string.h"
#include "./containers/growing_array.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

template<class Container>
static void _display(const char *arr_name, const Container& arr) 
{
    cout << arr_name << ": {";
    for(auto itr = arr.begin(); itr != arr.end(); itr++)
    {
        cout << *itr << ", ";
    } 
    cout << "};" << endl;
}
#define display(arr) _display(#arr, arr)

static void test_growing_array()
{
    {
        ml::GrowingArray<ml::String> s;
        s.grow(64);
        cout << "size: " << s.size() << endl;
        cout << "cap: " << s.capacity() << endl; 
        s.push_back(ml::String("my first element"));
        display(s);
        auto itr = s.erase(s.begin());
        cout << "size: " << s.size() << endl;
        cout << *itr << endl;
        display(s);

        s.insert(s.begin(), ml::String("newly inserted string"));
        display(s);
        s.insert(s.begin(), ml::String("before begin"));
        display(s);
        s.insert(s.end(), ml::String("at the end"));
        display(s);
        cout << "s.size: " << s.size() << endl;
        // insert in the middle
        ml::String obj("in the middle");
        s.insert(s.begin() + 1, obj);
        display(s);
        cout << "s.size: " << s.size() << endl;

        // insert before end()
        s.insert(s.end(), 3, ml::String("copy"));
        cout << "s.size: " << s.size() << endl;
        display(s);

        // insert before begin()
        s.insert(s.begin(), 4, ml::String("4 copies"));
        cout << "s.size: " << s.size() << endl;
        display(s);

        s.insert(s.begin() + 2, 2, ml::String("MIDDLE"));
        cout << "s.size: " << s.size() << endl;
        display(s);

        // insert 5 elements starting from end() - 2
        s.insert(s.end() - 2, 5, ml::String("Last"));
        display(s);
        cout << "s.size: " << s.size() << endl;

        std::vector<ml::String> vstr;
        enum { N = 5 };
        vstr.reserve(N);
        for(int i = 0; i < N; i++)
            vstr.push_back(ml::String("index" + std::to_string(i)));

        s.insert(s.begin(), vstr.begin(), vstr.end());
        cout << "s.size: " << s.size() << endl;
        display(s);

        s.emplace(s.begin(), "string");
        cout << "s.size: " << s.size() << endl;
        display(s);

        s.emplace(s.end(), std::string("Terminal element"));
        cout << "s.size: " << s.size() << endl;
        display(s);

        auto str = s[0];
        cout << "str: " << str << endl;

        // assignment by index
        s[1] = ml::String("new string");
        display(s);
    }
}

int main(int argc_count, char **argv)
{
    test_growing_array();
    return 0;
}

#undef display
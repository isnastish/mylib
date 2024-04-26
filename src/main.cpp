#include "string.h"
#include "growing_array.h"
#include <iostream>
#include <vector>

using namespace std;

template<class T>
static void _display(const char *arr_name, const ml::GrowingArray<T>& arr) 
{
    cout << arr_name << ": {";
    for(auto itr = arr.begin(); itr != arr.end(); itr++) 
        cout << *itr << ", ";
    cout << "};" << endl;
}
#define display(arr) _display(#arr, arr)

static void test_string()
{
    ml::String s1("my string");
    cout << s1 << endl;
}

static void test_growing_array()
{
    ml::GrowingArray<int> arr = {2334, 978, 8878};
    cout << "size: " << arr.size() << endl;
    display(arr);
    cout << arr.back() << endl;
    arr.pop_back();
    cout << "size: " << arr.size() << endl;
    arr.push_back(444);
    arr.push_back(89);
    arr.push_back(-3434);
    display(arr);
    cout << "size: " << arr.size() << endl;
    auto next = arr.erase(arr.begin());
    cout << "after erasing: ";
    display(arr);
    cout << "next: " << *next << endl;
    // TODO: Try to erase on empty array.
    arr.erase(arr.begin(), arr.begin() + 3);
    cout << "after erasing three elements: ";
    display(arr);
    cout << "size: " << arr.size() << endl;
}

static void test_std_vector()
{
    std::vector<int> v{4};
    // __debugbreak();
}

int main(int argc_count, char **argv)
{
    test_string();
    test_growing_array();
    test_std_vector();

    return 0;
}

#undef display
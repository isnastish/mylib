# Load balancer
Is a process of distributing a set of tasks (pieces of work) over the set of resources to make their
overall processing more efficient. Load balancing can increase the response time and avoid 
situations where some compute nodes are overloaded whereas some others remain idle. 

# Time to live
Time to live (TTL) or hop limit is a mechanism which limits the lifespan or lifetime of data in a 
computer network. Can be implemented as a counter of a timestamp attached to a data. 

# Class templates 
```cpp
#include <vecotor>

template<typename T>
class Grid
{
public:
    explicit Grid(size_t height = default_height, size_t width = default_width) 
    virtual ~Grid();

    // Sets an element at a given location. The element is copied.
    void setElementAt(size_t x, size_t y, const T& elem);
    T& getElement(size_t x, size_t y);
    const T& getElement(size_t x, size_t y) const;

    static constexpr size_t default_height = 10;
    static constexpr size_t default_width = 10;

private:
    void initializeCellsContainer();
    std::vector<std::vector<T>> m_cells;
    size_t m_width, m_height;
};

template<typename InputItr, typename Predicate>
InputItr find_if(InputItr first, InputItr last, Predicate pred) {
    for (InputItr itr = first; itr != last; itr++) {
        if (pred(*itr)) {
            return itr;
        }
    }
    // thus for std::vector<std::unique_ptr<Grid>> we would have to pass
    // [](const std::unique_ptr<Grid>& grid) { 
    //    const auto& element = grid.getElement();
    //    return (element.type() == ElementType::Queen); {
    // }
}
```
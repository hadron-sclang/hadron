#ifndef SRC_COMPILER_INCLUDE_HADRON_PAGE_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_PAGE_HPP_

#include <cstddef>
#include <vector>

namespace hadron {

// Represents a contiguous region of memory mapped memory. Contains Heap allocations of a fixed size. Not
// representative of a page of memory in the operating system, as may be a different size.
class Page {
public:
    Page() = delete;
    Page(size_t objectSize, size_t totalSize, bool isExecutable = false);
    ~Page();

    bool map();
    bool unmap();

    // Returns a pointer to available memory for a new object of size |objectSize|, or nullptr if no additional capacity
    // available.
    void* allocate();
    // Returns available room in the page *in number of stored objects*
    size_t capacity();


    // Reserve the 2 most significant bits for coloring objects in the m_collectionCounts field.
    enum Color : uint8_t {
        kWhite = 0,
        kGray = 0x40,
        kBlack = 0x80
    };
    // Mark the object contained by address.
    void mark(void* address, Color color);

    uint8_t* startAddress() const { return m_startAddress; }
    size_t totalSize() const { return m_totalSize; }
    size_t objectSize() const { return m_objectSize; }

private:
    // Mmaped start of the address of this page.
    uint8_t* m_startAddress;
    // Individual size of an object stored in this page, in bytes.
    size_t m_objectSize;
    // Total size of page in bytes.
    size_t m_totalSize;
    // If true the Page needs to be marked for JIT bytecode on mapping.
    bool m_isExecutable;
    // Index of next free object in Page.
    size_t m_nextFreeObject;
    // Number of allocated objects in Page.
    size_t m_allocatedObjects;
    // Maintains an entry per-object of the collection iterations each object has survived + 1, meaning that if a count
    // is zero that slot is unallocated.
    std::vector<uint8_t> m_collectionCounts;
};

}

#endif // SRC_COMPILER_INCLUDE_HADRON_PAGE_HPP_
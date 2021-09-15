#ifndef SRC_COMPILER_INCLUDE_HADRON_PAGE_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_PAGE_HPP_

namespace hadron {

// Represents a contiguous region of memory mapped memory. Contains Heap allocations of a fixed size. Not
// representative of a page of memory in the operating system, as may be a different size.
class Page {
public:
    Page() = delete;
    Page(size_t objectSize, size_t totalSize);
    ~Page();

    bool map();
    bool unmap();

    // Returns a pointer to available memory for a new object of size |objectSize|, or nullptr if no additional capacity
    // available.
    void* allocate();
    // Returns available room in the page (past high-water mark) *in number of stored objects*
    size_t capacity();

private:
    // Mmaped start of the address of this page.
    void* m_startAddress;
    // Individual size of an object stored in this page, in bytes.
    size_t m_objectSize;
    // Total size of page in bytes.
    size_t m_totalSize;
    // Offset of the first un-allocated object within this page, in bytes.
    size_t m_highWaterMark;
};

}

#endif // SRC_COMPILER_INCLUDE_HADRON_PAGE_HPP_
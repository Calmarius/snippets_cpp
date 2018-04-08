#include <stddef.h>

/** This enum contains the possible errors in this class. */
enum class DynArrayError
{
    OK, ///< Everything is alright
    ALLOCATION_FAILURE, ///< Indicates memory allocation failure
    INDEX_OUT_OF_RANGE, ///< Index out of range when attempted to access the array.
    INVALID_CAPACITY ///< Wrong capacity provided when attempted to resize the object.
};

typedef void (*DynArrayErrorCallback)(DynArrayError error, void *context);

/**
 * Dynamic array structure.
 *
 * Random access variable sized list data structure.
 *
 * @tparam T The type of elements
 * @tparam Alloc The allocator to be used to allocate the elements.
 * @tparam ErrorCb The callback that's called when an error happens.
 */
template <class T, class Alloc>
class DynArray
{
    template <class X, class Y> friend class DynArray; // To make different instantiations access each other.

private:
    T* buf = nullptr; ///< The buffer that holds the data.
    size_t n = 0; ///< The number of elements in the array.
    size_t nAllocd = 0; ///< The number of elements allocated in the array.
    Alloc ator; ///< An instance of the allocator.
    bool alive = true; ///< True if the object is alive and usable;
    DynArrayErrorCallback errorCb = [](DynArrayError, void*){};
    void *errorCbCtx = nullptr;

    /**
     * Constructs object from another object.
     *
     * @param[in] arr Array to construct from.
     * @returns Zero on success, non-zero on failure.
     */
    int constructFrom(const DynArray<T, Alloc> &arr)
    {
        nAllocd = arr.nAllocd;
        n = arr.n;
        ator = arr.ator;
        buf = ator.allocate(n);
        if (buf == nullptr)
        {
            errorCb(DynArrayError::ALLOCATION_FAILURE, errorCbCtx);
            alive = false;
            nAllocd = 0;
            n = 0;
            return -1;
        }
        errorCb = arr.errorCb;
        errorCbCtx = arr.errorCbCtx;

        for (size_t i = 0; i < n; i++)
        {
            buf[i] = arr.buf[i];
        }

        return 0;
    }


    /**
     * Constructs the object by moving another object.
     *
     * @param[in] arr Array to move from.
     *
     * @remarks
     *  The source array will be cleared.
     */
    void moveFrom(DynArray<T, Alloc> &&arr)
    {
        n = arr.n;
        nAllocd = arr.nAllocd;
        buf = arr.buf;
        ator = arr.ator;
        alive = arr.alive;
        errorCb = arr.errorCb;
        errorCbCtx = arr.errorCbCtx;

        arr.n = 0;
        arr.nAllocd = 0;
        arr.buf = nullptr;
    }


    /**
     * The destructor function.
     */
    void destruct()
    {
        if (buf) ator.deallocate(buf);
    }


    int ensureSize(size_t newN)
    {
        if (nAllocd > newN) return 0;

        size_t newAllocd = nAllocd;

        if (newAllocd == 0) newAllocd = 8;

        while (newAllocd < newN) newAllocd *= 2;

        T *newBuf = ator.reallocate(buf, newAllocd);

        if (!newBuf)
        {
            errorCb(DynArrayError::ALLOCATION_FAILURE, errorCbCtx);
            return -1;
        }

        buf = newBuf;
        nAllocd = newAllocd;

        return 0;
    }
public:

    /**
     * Default constructor. Initializes and empty array.
     *
     * @param[in] alloc An instance of the allocator used to allocate the elements for this structure.
     */
    DynArray(const Alloc &alloc = Alloc()) : ator(alloc) {}

    // Standard stuff

    /**
     * Destructor. Releases the memory.
     */
    ~DynArray() {destruct();}

    /**
     * Copy constructor.
     *
     * @param[in] arr: object to contruct from.
     *
     * @remarks
     * Upon allocation failure the object may remain in a zombie state, so use isAlive() function
     * to determine the object is usable.
     */
    DynArray(const DynArray<T, Alloc> &arr)
    {
        constructFrom(arr);
    }

    /**
     * Assignment operator.
     *
     * @param[in] arr object to deep copy from.
     * @returns Reference to self.
     *
     * @remarks
     * Upon allocation failure the object may remain in a zombie state, so use isAlive() function
     * to determine the object is usable.
     */
    DynArray<T, Alloc>& operator=(const DynArray<T, Alloc> &arr)
    {
        if (this == &arr) return *this;

        destruct();
        constructFrom(arr);
    }

    /**
     * Move constructor.
     *
     * @param[in,out] arr array to move from.
     *
     * @remarks
     * No deep copy is performed, so the object will be alive.
     * The source object will be cleared to empty.
     */
    DynArray(DynArray<T, Alloc> &&arr)
    {
        moveFrom(static_cast<DynArray<T, Alloc>&&>(arr));
    }

    /**
     * Move assignment operator.
     *
     * @param[in,out] arr array to move from.
     *
     * @returns reference to the left side of the assignment.
     */
    DynArray<T, Alloc>& operator=(DynArray<T, Alloc> &&arr)
    {
        if (this == &arr) return *this;

        moveFrom(arr);
    }

    // Iterators (for range based for loop)

    /**
     * @returns an iterator to the beginning of the array.
     *
     * @warning
     *  Operations on this iterator is unchecked, be careful when using it.
     *
     * @remarks
     *  It's highly recommended to use range based for loops when using these iterators as it's less error prone.
     */
    // @{
    const T *begin() const {return buf;}
    T *begin() {return buf;}
    // @}

    /**
     * @returns and iterator to the end (one element beyond the end).
     *
     * @warning
     *  Dereferencing this iterator is undefined behavior.
     *
     * @remarks
     *  It's highly recommended to use range based for loops when using these iterators as it's less error prone.
     */
    // @{
    T *end() {return buf + n;}
    const T *end() const {return buf + n;}
    // @}

    // Operations

    /**
     * Queries the element at the given index.
     *
     * @param index The index to query.
     * @returns A reference to an element there.
     *
     * @remarks
     *  On out of bounds access the INDEX_OUT_OF_RANGE error will be set and the operator returns
     *  a nullptr reference. Which crashes the program immediately if used.
     *
     *  For bulk operations prefer using iterators and range based for loops.
     */
    T& operator[](size_t index)
    {
        if (index >= n)
        {
            errorCb(DynArrayError::INDEX_OUT_OF_RANGE, errorCbCtx);
            return *static_cast<T*>(nullptr);
        }

        return buf[index];
    }

    /**
     * Checks if the object is alive.
     *
     * When memory allocation fails during construction, it leaves the object in a "zombie" state.
     * The user must check if the object is alive after copy or move construction or assignment.
     *
     * @returns True if the object is alive.
     */
    bool isAlive() {return alive;}

    /**
     * Adds element to the dynamic array.
     *
     * @param[in] elem Element to add.
     * @returns On success the return value is 0. On failure the return value is -1.
     *
     * @remarks
     * The only possible error is ALLOCATION_FAILURE.
     */
    int add(const T &elem)
    {
        if (ensureSize(n + 1)) return -1;
        buf[n++] = elem;

        return 0;
    }


    /**
     * Adds multiple elements to the dynamic array.
     *
     * @param[in] start Iterator to the start of the range.
     * @param[in] end Iterator to the end of the range (on element beyond the last).
     * @returns Zero on success, non-zero on failure.
     *
     * @remarks
     *    If we run out of memory the ALLOCATION_FAILURE error is set.
     */
    template <typename ForwardIterator> int addRange(ForwardIterator start, ForwardIterator end)
    {
        for (ForwardIterator current = start; start != end; ++current)
        {
            if (add(*current)) return -1;
        }

        return 0;
    }


    /**
     * Performs binary search.
     *
     * @tparam Compare A comparator type. It must be a functor with the signature:
     *      int compare(const T &a, const T &b); Which returns negative if a < b, positive if a > b,
     *      zero if a == b.
     *
     * @param[in] start The start index of the range.
     * @param[in] length The number of elements in the range.
     * @param[in] elem The element to search for.
     * @returns true if the elements is found, false if not.
     *
     * @remarks
     * The array must be sorted in order to work.
     * If the indexes are out of range the function returns false and the INDEX_OUT_OF_RANGE error is set.
     */
    template <typename Compare> bool binarySearch(size_t start, size_t length, const T &elem)
    {
        size_t left = start;
        size_t right = start + length - 1;
        Compare c;

        if ((left >= n) || (right >= n))
        {
            errorCb(DynArrayError::INDEX_OUT_OF_RANGE, errorCbCtx);
            return false;
        }

        while (left <= right)
        {
            size_t mid = (left + right) / 2;
            int comparison = c(elem, buf[mid]);

            if (comparison == 0) return true;

            if (comparison < 0)
            {
                right = mid - 1;
            }
            else if (comparison > 0)
            {
                left = mid + 1;
            }
        }

        return false;
    }


    /**
     * Performs binary search.
     *
     * @tparam Compare Comparator.
     *
     * @param[in] elem The element to search form.
     * @returns True if the elements is found, false otherwise.
     *
     * @remarks
     * See the other overloads for more details.
     */
    template <typename Compare> bool binarySearch(const T &elem)
    {
        return binarySearch<Compare>(0, n, elem);
    }

    /**
     * Performs binary search.
     *
     * @param elem The element to search for.
     * @returns true if the elements is found, false otherwise.
     *
     * @remarks
     *      Use this function only if T supports the == and < operators.
     */
    bool binarySearch(const T &elem)
    {
        struct Compare
        {
            int operator()(const T &a, const T &b)
            {
                if (a == b) return 0;
                if (a < b) return -1;
                return 1;
            }
        };

        return binarySearch<Compare>(elem);
    }


    /**
     * @returns The number of elements that can be stored in the array without resizing.
     */
    size_t getCapacity() {return nAllocd;}


    /**
     * Changes the capacity of the array. (Capacity is the number of elements it can store before it needs to be reallocated).
     *
     * @param[in] newCapacity The desired number of elements.
     * @returns Zero on success, non-zero of failure.
     *
     * @remarks
     *  Possible errors are:
     *
     *  - INVALID_CAPACITY if the requested capacity is less than the number of the elements the array currently hold.
     *  - ALLOCATION_FAILURE if it fails to allocate the desired number of elements.
     *
     *  In both errors the underlying buffer is not touched and the object can be used as usual.
     */
    int setCapacity(size_t newCapacity)
    {
        if (newCapacity < n)
        {
            errorCb(DynArrayError::INVALID_CAPACITY, errorCbCtx);
            return -1;
        }

        T *newBuf = ator.reallocate(buf, newCapacity);
        if (newBuf == nullptr)
        {
            errorCb(DynArrayError::ALLOCATION_FAILURE, errorCbCtx);
            return -1;
        }
        buf = newBuf;
        nAllocd = newCapacity;

        return 0;
    }


    /**
     * Removes all elements from the dynamic array.
     *
     * @remarks
     * The elements will be destroyed.
     */
    void clear()
    {
        for (size_t i = 0; i < n; i++)
        {
            buf[i].~T();
        }

        n = 0;
    }


    /**
     * Performs linear search to find the given element.
     *
     * @tparam Compare A functor, whose signature is bool(const T&, const T&), it must return true if the two values are equal.
     * @param[in] elem The element to find.
     * @param [in] c An instance of the comparer.
     * @returns true if the element is found.
     */
    template <class EqualityCompare> bool contains(const T &elem, const EqualityCompare &c)
    {
        for (size_t i = 0; i < n; i++)
        {
            if (c(buf[i], elem))
            {
                return true;
            }
        }

        return false;
    }


    /**
     * Performs linerar search to find the given elements.
     *
     * @param[in] elem The element to find.
     * @returns true if the element is found.
     *
     * Use this if T supports the == operator.
     */
    bool contains(const T &elem)
    {
        return contains(elem, [](const T &x, const T &y){return x == y;} );
    }


    /**
     * Converts the dynamic array to a different dynamic array.
     *
     * @tparam U The elem type of the another array.
     * @tparam UAlloc Allocator for the new array.
     * @tparam Converter The converter. It must be a functor with the following signature:
     *   U convert(const T&)
     * @param[in] c An instance of the converter.
     * @param[in] ualloc An instance of the allocator to be used to allocate the elements of the new list.
     * @returns A new dynamic array.
     *
     * @remarks.
     *  On failure it returns an empty array. For the error reason call getLastError().
     */
    template<class U, class UAlloc, class Converter> DynArray<U, UAlloc> convertAll(const Converter &c, const UAlloc &ualloc = UAlloc())
    {
        DynArray<U, UAlloc> newArray(ualloc);

        newArray.errorCb = errorCb;

        if (newArray.setCapacity(nAllocd))
        {
            errorCb(DynArrayError::ALLOCATION_FAILURE, errorCbCtx);
            return DynArray<U, UAlloc>();
        }

        for (size_t i = 0; i < n; i++)
        {
            newArray.buf[i] = c(buf[i]);
        }
        newArray.n = n;

        return newArray;
    }


    /**
     * @returns The number of elements in the array.
     */
    size_t getCount() {return n;}


    /**
     * Copies the entire list to the given C array.
     *
     * @param[out] array to be written
     * @param[in] start The start position to start writing.
     *
     * @warning
     *      There is no way to do bounds checking in the array.
     *      Make sure the array can hold the desired the number of elements without overflowl!
     */
    void copyTo(T *array, size_t start = 0)
    {
        for (size_t i = 0; i < n; i++)
        {
            array[start + i] = buf[i];
        }
    }


    DynArrayErrorCallback getErrorCb() {return errorCb;}
    void* getErrorCbCtx() {return errorCbCtx;}
    void setErrorCb(DynArrayErrorCallback ecb, void *ctx) {errorCb = ecb; errorCbCtx = ctx;}



    /**
     * Check if an element with a given property exists in the collection.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param[in, out] p The preficate to test.
     * @returns True if the given element is found, false otherwise.
     */
    template <class Predicate> bool exists(const Predicate &p)
    {
        for (size_t i = 0; i < n; i++)
        {
            if (p(buf[i])) return true;
        }

        return false;
    }


    /**
     * Finds the first element with a given property.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] p An instance of the predicate.
     * @returns An iterator to the element if found. nullptr if not.
     */
    template <class Predicate> T* find(const Predicate &p)
    {
        for (size_t i = 0; i < n; i++)
        {
            if (p(buf[i])) return &buf[i];
        }

        return nullptr;
    }


    /**
     * Finds all element with a given property.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] p An instance of the predicate.
     * @param [in] alloc An allocator to be used for the new array.
     *
     * @returns A new array of the matches. If no matches are found an empty array is returned.
     *      On error it returns an empty array to find out the reason call getLastError().
     */
    template <class Predicate> DynArray<T, Alloc> findAll(const Predicate &p, Alloc alloc = Alloc())
    {
        DynArray<T, Alloc> array(alloc);

        array.errorCb = errorCb;

        for (size_t i = 0; i < n; i++)
        {
            if (p(buf[i]))
            {
                if (array.add(buf[i]))
                {
                    // Error adding.
                    return DynArray<T, Alloc>();
                }
            }
        }

        return array;
    }


    /**
     * Find an element in the given index range. Returns the index of the element.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] start The start index the finding starts.
     * @param [in] count The number of elements to find in.
     * @param [in] p An instance of a predicate.
     *
     * @returns The index of the first match. -1 if not found.
     * If the index is out of range -1 is returned the the last error is set to INDEX_OUT_OF_RANGE.
     */
    template <class Predicate> ssize_t findIndex(size_t start, size_t count, const Predicate &p)
    {
        size_t end = start + count;

        if ((start >= n) || (end > n))
        {
            errorCb(DynArrayError::INDEX_OUT_OF_RANGE, errorCbCtx);
            return -1;
        }

        for (size_t i = start; i < end; i++)
        {
            if (p(buf[i])) return i;
        }

        return -1;
    }

    /**
     * Finds an element starting from the given index. Returns the index of the element.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] start The start index the finding starts.
     * @param [in] p An instance of a predicate.
     *
     * @returns The index of the first match. -1 if not found.
     * If the index is out of range -1 is returned the the last error is set to INDEX_OUT_OF_RANGE.
     */
    template <class Predicate> ssize_t findIndex(size_t start, const Predicate &p)
    {
        return findIndex(start, n - start, p);
    }

    /**
     * Find an element in the array. Returns the index of the element.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] p An instance of a predicate.
     *
     * @returns The index of the first match. -1 if not found.
     */
    template <class Predicate> ssize_t findIndex(const Predicate &p)
    {
        if (n == 0) return -1;

        return findIndex(0, n, p);
    }


    /**
     * Finds the last element with a given property.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] p An instance of the predicate.
     * @returns An iterator to the element if found. nullptr if not.
     */
    template <class Predicate> T* findLast(const Predicate &p)
    {
        size_t i = n;

        while (i --> 0)
        {
            if (p(buf[i])) return &buf[i];
        }

        return nullptr;
    }


    /**
     * Find an element in the given index range. Returns the index of the element.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] start The start index the finding starts.
     * @param [in] count The number of elements to find in.
     * @param [in] p An instance of a predicate.
     *
     * @returns The index of the first match. -1 if not found.
     * If the index is out of range -1 is returned the the last error is set to INDEX_OUT_OF_RANGE.
     */
    template <class Predicate> ssize_t findLastIndex(size_t start, size_t count, const Predicate &p)
    {
        size_t end = start + count;

        if ((start >= n) || (end > n))
        {
            errorCb(DynArrayError::INDEX_OUT_OF_RANGE, errorCbCtx);
            return -1;
        }

        size_t i = end;

        while (i --> 0)
        {
            if (p(buf[i])) return i;
        }

        return -1;
    }


    /**
     * Find an element in the range starting from the given index. Returns the index of the element.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] start The start index the finding starts.
     * @param [in] p An instance of a predicate.
     *
     * @returns The index of the first match. -1 if not found.
     * If the index is out of range -1 is returned the the last error is set to INDEX_OUT_OF_RANGE.
     */
    template <class Predicate> ssize_t findLastIndex(size_t start, const Predicate &p)
    {
        return findLastIndex(start, n - start, p);
    }


    /**
     * Find an element in the array. Returns the index of the element.
     *
     * @tparam [in] Predicate A functor with the following signature: bool predicate(const T &elem) which returns true if the element matches the given condition.
     * @param [in] p An instance of a predicate.
     *
     * @returns The index of the first match. -1 if not found.
     */
    template <class Predicate> ssize_t findLastIndex(const Predicate &p)
    {
        if (n == 0) return -1;

        return findLastIndex(0, n, p);
    }


    /**
     * Performs an action on each element of the array.
     *
     * @tparam [in] Action A functor whose signature is: void action(T&). It represents an action that will be performed on each element in the array.
     * @param [in] a An instance of the action functor.
     */
    template <class Action> void forEach(const Action &a)
    {
        for (size_t i = 0; i < n; i++)
        {
            a(buf[i]);
        }
    }


    /**
     * Returns a part of the array.
     *
     * @param [in] start The start index the extraction starts at.
     * @param [in] count The count of elements to extract.
     * @param [in] alloc An instance of the allocator to be used in the new array.
     *
     * @returns The new array that contains the subset of elements. The elements are copied (to avoid copy use pointers or smart pointers as T).
     *      Or error an empty array is returned. Check getLastError() to find out the reason of the failure.
     */
    DynArray<T, Alloc> getRange(size_t start, size_t count, Alloc alloc = Alloc())
    {
        DynArray<T, Alloc> range(alloc);
        size_t end = start + count;

        if ((start >= n) || (end > n))
        {
            errorCb(DynArrayError::INDEX_OUT_OF_RANGE, errorCbCtx);
            return DynArray<T, Alloc>();
        }

        if (range.setCapacity(count))
        {
            return DynArray<T, Alloc>();
        }

        size_t j = 0;
        range.n = count;
        for (size_t i = start; i < end; i++)
        {
            range.buf[j++] =  buf[i];
        }

        return range;
    }


#if 0
    /**
     * Returns the index of the given element in the range.
     *
     * @tparam EqualityCompare a functor that decides if the two elements are equal. It's signature is: bool (const T&, const T&), it must return true if its two arguments are considered equal.
     * @param [in] elem The element to find.
     * @param [in] index The start index where the search starts.
     * @param [in] count The number of elements to scan.
     * @param [in] c An instance of the comparator.
     * @returns The index of the element if it's found. -1 if not found.
     *      On an index out of range condition it returns -1 and sets the last error to INDEX_OUT_OF_RANGE.
     */
    template <class EqualityCompare> ssize_t indexOf(const T &elem, size_t index, size_t count, const EqualityCompare &c)
    {
        size_t end = index + count;

        if ((index >= n) || (end > n))
        {
            lastError = INDEX_OUT_OF_RANGE;
            return -1;
        }

        for (size_t i = index; i < end; i++)
        {
            if (c(buf[i], elem)) return i;
        }

        return -1;
    }


    /**
     * Returns the index of the given element starting from the given index.
     *
     * @tparam EqualityCompare a functor that decides if the two elements are equal. It's signature is: bool (const T&, const T&), it must return true if its two arguments are considered equal.
     * @param [in] elem The element to find.
     * @param [in] index The start index where the search starts.
     * @param [in] c An instance of the comparator.
     * @returns The index of the element if it's found. -1 if not found.
     *      On an index out of range condition it returns -1 and sets the last error to INDEX_OUT_OF_RANGE.
     */
    template <class EqualityCompare> ssize_t indexOf(const T &elem, size_t index, const EqualityCompare &c)
    {
        return indexOf(elem, index, n - index, c);
    }


    /**
     * Returns the index of the given element in that array.
     *
     * @tparam EqualityCompare a functor that decides if the two elements are equal. It's signature is: bool (const T&, const T&), it must return true if its two arguments are considered equal.
     * @param [in] c An instance of the comparator.
     * @returns The index of the element if it's found. -1 if not found.
     *      On an index out of range condition it returns -1 and sets the last error to INDEX_OUT_OF_RANGE.
     */
    template <class EqualityCompare> ssize_t indexOf(const T &elem, const EqualityCompare &c)
    {
        return indexOf(elem, 0, n, c);
    }
#endif

    /**
     * Returns the index of the given element in the range.
     *
     * @param [in] elem The element to find.
     * @param [in] index The start index where the search starts.
     * @param [in] count The number of elements to scan.
     * @returns The index of the element if it's found. -1 if not found.
     *      On an index out of range condition it returns -1 and sets the last error to INDEX_OUT_OF_RANGE.
     *
     * @remarks T must support the operator ==.
     */
    ssize_t indexOf(const T &elem, size_t index, size_t count)
    {
        size_t end = index + count;

        if ((index >= n) || (end > n))
        {
            errorCb(DynArrayError::INDEX_OUT_OF_RANGE, errorCbCtx);
            return -1;
        }

        for (size_t i = index; i < end; i++)
        {
            if (buf[i] == elem) return i;
        }

        return -1;
        //return indexOf(elem, index, count, [](const T &x, const T &y){return x == y;});
    }


    /**
     * Returns the index of the given element starting at the given index.
     *
     * @param [in] elem The element to find.
     * @param [in] index The start index where the search starts.
     * @returns The index of the element if it's found. -1 if not found.
     *      On an index out of range condition it returns -1 and sets the last error to INDEX_OUT_OF_RANGE.
     *
     * @remarks T must support the operator ==.
     */
    ssize_t indexOf(const T &elem, size_t index)
    {
        return indexOf(elem, index, n - index);
    }


    /**
     * Returns the index of the given element starting at the given index.
     *
     * @param [in] elem The element to find.
     * @returns The index of the element if it's found. -1 if not found.
     *      On an index out of range condition it returns -1 and sets the last error to INDEX_OUT_OF_RANGE.
     *
     * @remarks T must support the operator ==.
     */
    ssize_t indexOf(const T &elem)
    {
        return indexOf(elem, 0, n);
    }







};

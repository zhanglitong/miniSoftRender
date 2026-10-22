#pragma     once

namespace   FE
{
    template <typename T>
    class   FERef
    {
    public:
        using   TValue  =   T;
    public:
        /// <summary>
        /// Create a nullptr
        /// </summary>
        FERef()
            :_ptr(nullptr)
        {}

        /// <summary>
        /// Construct a reference from a pointer
        /// </summary>
        FERef(T *ptr) : _ptr(ptr)
        {
            if (_ptr)
            {
                _ptr->incRef();
            }
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        FERef(const FERef &r) : _ptr(r._ptr)
        {
            if (_ptr)
            {
                _ptr->incRef();
            }
        }

        /// <summary>
        /// Move constructor
        /// </summary>
        FERef(FERef &&r) noexcept : _ptr(r._ptr)
        {
            r._ptr = nullptr;
        }
        /// <summary>
        /// Destroy this reference
        /// </summary>
        ~FERef()
        {
            if (_ptr)
                _ptr->decRef();
        }

        /// Move another reference into the current one
        FERef& operator=(FERef&& r) noexcept
        {
            if (&r != this)
            {
                if (_ptr)
                    _ptr->decRef();
                _ptr    =   r._ptr;
                r._ptr =   nullptr;
            }
            return *this;
        }

        /// Overwrite this reference with another reference
        FERef& operator=(const FERef& r) noexcept
        {
            if (_ptr != r._ptr)
            {
                if (r._ptr)
                    r._ptr->incRef();
                if (_ptr)
                    _ptr->decRef();
                _ptr = r._ptr;
            }
            return *this;
        }
        /// Overwrite this reference with a pointer to another object
        FERef& operator=(T *ptr) noexcept
        {
            if (_ptr != ptr)
            {
                if (ptr)
                    ptr->incRef();
                if (_ptr)
                    _ptr->decRef();
                _ptr = ptr;
            }
            return *this;
        }

        /// <summary>
        /// Compare this reference with another reference
        /// </summary>
        bool operator==(const FERef &r) const
        {
            return _ptr == r._ptr;
        }

        /// Compare this reference with another reference
        bool operator!=(const FERef &r) const
        {
            return _ptr != r._ptr;
        }

        /// <summary>
        /// Compare this reference with a pointer
        /// </summary>
        bool operator==(const T* ptr) const
        {
            return _ptr == ptr;
        }

        /// Compare this reference with a pointer
        bool operator!=(const T* ptr) const
        {
            return _ptr != ptr;
        }
        /// Compare this reference with a pointer
        bool operator < (const T* ptr) const
        {
            return _ptr < ptr;
        }
        /// Compare this reference with a pointer
        bool operator < (const FERef &r) const
        {
            return _ptr < r._ptr;
        }

        /// Compare this reference with a pointer
        bool operator > (const T* ptr) const
        {
            return _ptr > ptr;
        }
        /// Compare this reference with a pointer
        bool operator > (const FERef &r) const
        {
            return _ptr > r._ptr;
        }

        /// Access the object referenced by this reference
        T* operator->()
        {
            return _ptr;
        }

        /// Access the object referenced by this reference
        const T* operator->() const
        {
            return _ptr;
        }

        /// Return a C++ reference to the referenced object
        T& operator*()
        {
            return *_ptr;
        }

        /// Return a const C++ reference to the referenced object
        const T& operator*() const
        {
            return *_ptr;
        }

        /// <summary>
        /// Return a pointer to the referenced object
        /// </summary>
        operator T* ()
        {
            return _ptr;
        }

        /// <summary>
        /// Return a const pointer to the referenced object
        /// </summary>
        inline  T* get()
        {
            return _ptr;
        }

        /// Return a pointer to the referenced object
        const   T*  get() const
        {
            return _ptr;
        }
        bool        isValid() const
        {
            return _ptr != nullptr;
        }
        explicit operator bool() const
        {
            return _ptr != nullptr;
        }
    protected:
        T*  _ptr;
    };
}

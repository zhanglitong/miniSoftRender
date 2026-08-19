#pragma     once

namespace   FE
{
    template <typename T>
    class   FERef
    {
    public:
        /// Create a nullptr
        FERef()
            :_ptr(nullptr)
        {}

        /// Construct a reference from a pointer
        FERef(T *ptr) : _ptr(ptr)
        {
            if (_ptr)
            {
                _ptr->incRef();
            }
        }

        /// Copy constructor
        FERef(const FERef &r) : _ptr(r._ptr)
        {
            if (_ptr)
            {
                _ptr->incRef();
            }
        }

        /// Move constructor
        FERef(FERef &&r) noexcept : _ptr(r._ptr)
        {
            r._ptr = nullptr;
        }
        /// Destroy this reference
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

        /// Compare this reference with another reference
        bool operator==(const FERef &r) const
        {
            return _ptr == r._ptr;
        }

        /// Compare this reference with another reference
        bool operator!=(const FERef &r) const
        {
            return _ptr != r._ptr;
        }

        /// Compare this reference with a pointer
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

        /// Return a pointer to the referenced object
        operator T* ()
        {
            return _ptr;
        }

        /// Return a const pointer to the referenced object
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

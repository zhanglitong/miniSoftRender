#pragma     once

namespace   FE
{

    template <typename T, size_t N>
    class   AlignedBuffer
    {
    public:
        T*    data()
        {
            return reinterpret_cast<T *>(_data);
        }

    private:
        alignas(T) char _data[sizeof(T) * N];
    };
    template <typename T>
    class   AlignedBuffer<T, 0>
    {
    public:
        T*  data()
        {
            return nullptr;
        }
    };
    template <typename T>
    class   TVectorView
    {
    public:
        T&        operator[](size_t i) 
        {
            return _ptr[i];
        }
        
        const T&    operator[](size_t i) const 
        {
            return _ptr[i];
        }
        
        inline  bool    empty() const 
        {
            return _size == 0;
        }
        
        inline  size_t  size() const 
        {
            return _size;
        }
        
        inline  T*      data() 
        {
            return _ptr;
        }
        
        const   T*      data() const 
        {
            return _ptr;
        }
        
        inline  T*      begin() 
        {
            return _ptr;
        }
        
        inline  T*      end() 
        {
            return _ptr + _size;
        }
        
        const   T*      begin() const 
        {
            return _ptr;
        }
        
        const   T*      end() const 
        {
            return _ptr + _size;
        }
        
        inline  T&      front() 
        {
            return _ptr[0];
        }
        
        const   T&      front() const 
        {
            return _ptr[0];
        }
        
        inline  T&      back() 
        {
            return _ptr[_size - 1];
        }
        
        const   T&      back() const 
        {
            return _ptr[_size - 1];
        }
        
        explicit operator std::vector<T>() const &
        {
            return std::vector<T>(_ptr, _ptr + _size);
        }
        
        // If we are converting as an r-value, we can pilfer our elements.
        explicit operator std::vector<T>() &&
        {
            return std::vector<T>(std::make_move_iterator(_ptr), std::make_move_iterator(_ptr + _size));
        }
        
        // Avoid sliced copies. Base class should only be read as a reference.
        TVectorView(const TVectorView &)    =    delete;
        void operator=(const TVectorView &) =    delete;
    protected:
        TVectorView()           =    default;
        size_t      _size       =    0;
        T*          _ptr        =    nullptr;
        
    };
    
    template <typename T, size_t N = 8>
    class   TSmallVector : public TVectorView<T>
    {
    public:
        TSmallVector() 
        {
            _ptr  =   _memory.data();
            _capacity   =   N;
        }
        
        template <typename U>
        TSmallVector(const U *arg_list_begin, const U *arg_list_end)  
            : TSmallVector()
        {
            auto    count   =   size_t(arg_list_end - arg_list_begin);
            reserve(count);
            for (size_t i = 0; i < count; i++, arg_list_begin++)
                new (&_ptr[i]) T(*arg_list_begin);
            _size   =   count;
        }
        
        template <typename U>
        TSmallVector(std::initializer_list<U> init)  
            : TSmallVector(init.begin(), init.end())
        {
        }
        
        template <typename U, size_t M>
        explicit TSmallVector(const U (&init)[M])  : TSmallVector(init, init + M)
        {
        }
        
        TSmallVector(TSmallVector &&other)  
            : TSmallVector()
        {
            *this   =   std::move(other);
        }
        
        TSmallVector &operator=(TSmallVector &&other) 
        {
            clear();
            if (other._ptr != other._memory.data())
            {
                // Pilfer allocated pointer.
                if (_ptr != _memory.data())
                    free(_ptr);
                _ptr          =   other._ptr;
                _size   =   other._size;
                _capacity           =   other._capacity;
                other._ptr          =   nullptr;
                other._size   =   0;
                other._capacity     =   0;
            }
            else
            {
                // Need to move the stack contents individually.
                reserve(other._size);
                for (size_t i = 0; i < other._size; i++)
                {
                    new (&_ptr[i]) T(std::move(other._ptr[i]));
                    other._ptr[i].~T();
                }
                _size   =   other._size;
                other._size   =   0;
            }
            return *this;
        }
        
        TSmallVector(const TSmallVector &other)  : TSmallVector()
        {
            *this   =   other;
        }
        
        TSmallVector &operator=(const TSmallVector &other) 
        {
            if (this == &other)
                return *this;

            clear();
            reserve(other._size);
            for (size_t i = 0; i < other._size; i++)
                new (&_ptr[i]) T(other._ptr[i]);
            _size = other._size;
            return *this;
        }
        
        explicit TSmallVector(size_t count)  : TSmallVector()
        {
            resize(count);
        }
        
        ~TSmallVector()
        {
            clear();
            if (_ptr != _memory.data())
                free(_ptr);
        }
        void    clear() 
        {
            for (size_t i = 0; i < _size; i++)
                _ptr[i].~T();
            _size   =   0;
        }
        
        void    push_back(const T &t) 
        {
            reserve(_size + 1);
            new (&_ptr[_size]) T(t);
            _size++;
        }
        
        void    push_back(T &&t) 
        {
            reserve(_size + 1);
            new (&_ptr[_size]) T(std::move(t));
            _size++;
        }
        
        void    pop_back() 
        {
            // Work around false positive warning on GCC 8.3.
            // Calling pop_back on empty vector is undefined.
            if (!empty())
                resize(_size - 1);
        }
        
        template <typename... Ts>
        void    emplace_back(Ts &&... ts) 
        {
            reserve(_size + 1);
            new (&_ptr[_size]) T(std::forward<Ts>(ts)...);
            _size++;
        }
        
        void    reserve(size_t count) 
        {
            if ((count > (std::numeric_limits<size_t>::max)() / sizeof(T)) ||
                (count > (std::numeric_limits<size_t>::max)() / 2))
            {
                // Only way this should ever happen is with garbage input, terminate.
                std::terminate();
            }

            if (count > _capacity)
            {
                size_t target_capacity = _capacity;
                if (target_capacity == 0)
                    target_capacity = 1;

                // Weird parens works around macro issues on Windows if NOMINMAX is not used.
                target_capacity = (std::max)(target_capacity, N);

                // Need to ensure there is a POT value of target capacity which is larger than count,
                // otherwise this will overflow.
                while (target_capacity < count)
                    target_capacity <<= 1u;

                T*  new_buffer = target_capacity > N ? static_cast<T *>(malloc(target_capacity * sizeof(T))) : _memory.data();

                // If we actually fail this malloc, we are hosed anyways, there is no reason to attempt recovery.
                if (!new_buffer)
                    std::terminate();

                // In case for some reason two allocations both come from same stack.
                if (new_buffer != _ptr)
                {
                    // We don't deal with types which can throw in move constructor.
                    for (size_t i = 0; i < _size; i++)
                    {
                        new (&new_buffer[i]) T(std::move(_ptr[i]));
                        _ptr[i].~T();
                    }
                }

                if (_ptr != _memory.data())
                    free(_ptr);
                _ptr  =   new_buffer;
                _capacity   =   target_capacity;
            }
        }
        
        void    insert(T *itr, const T *insert_begin, const T *insert_end) 
        {
            auto count = size_t(insert_end - insert_begin);
            if (itr == end())
            {
                reserve(_size + count);
                for (size_t i = 0; i < count; i++, insert_begin++)
                    new (&_ptr[_size + i]) T(*insert_begin);
                _size += count;
            }
            else
            {
                if (_size + count > _capacity)
                {
                    auto target_capacity = _size + count;
                    if (target_capacity == 0)
                        target_capacity = 1;
                    if (target_capacity < N)
                        target_capacity = N;

                    while (target_capacity < count)
                        target_capacity <<= 1u;

                    // Need to allocate new buffer. Move everything to a new buffer.
                    T *new_buffer =
                        target_capacity > N ? static_cast<T *>(malloc(target_capacity * sizeof(T))) : _memory.data();

                    // If we actually fail this malloc, we are hosed anyways, there is no reason to attempt recovery.
                    if (!new_buffer)
                        std::terminate();

                    // First, move elements from source buffer to new buffer.
                    // We don't deal with types which can throw in move constructor.
                    auto *target_itr = new_buffer;
                    auto *original_source_itr = begin();

                    if (new_buffer != _ptr)
                    {
                        while (original_source_itr != itr)
                        {
                            new (target_itr) T(std::move(*original_source_itr));
                            original_source_itr->~T();
                            ++original_source_itr;
                            ++target_itr;
                        }
                    }

                    // Copy-construct new elements.
                    for (auto *source_itr = insert_begin; source_itr != insert_end; ++source_itr, ++target_itr)
                        new (target_itr) T(*source_itr);

                    // Move over the other half.
                    if (new_buffer != _ptr || insert_begin != insert_end)
                    {
                        while (original_source_itr != end())
                        {
                            new (target_itr) T(std::move(*original_source_itr));
                            original_source_itr->~T();
                            ++original_source_itr;
                            ++target_itr;
                        }
                    }

                    if (_ptr != _memory.data())
                        free(_ptr);
                    _ptr = new_buffer;
                    _capacity = target_capacity;
                }
                else
                {
                    // Move in place, need to be a bit careful about which elements are constructed and which are not.
                    // Move the end and construct the new elements.
                    auto *target_itr = end() + count;
                    auto *source_itr = end();
                    while (target_itr != end() && source_itr != itr)
                    {
                        --target_itr;
                        --source_itr;
                        new (target_itr) T(std::move(*source_itr));
                    }

                    // For already constructed elements we can move-assign.
                    std::move_backward(itr, source_itr, target_itr);

                    // For the inserts which go to already constructed elements, we can do a plain copy.
                    while (itr != end() && insert_begin != insert_end)
                        *itr++ = *insert_begin++;

                    // For inserts into newly allocated memory, we must copy-construct instead.
                    while (insert_begin != insert_end)
                    {
                        new (itr) T(*insert_begin);
                        ++itr;
                        ++insert_begin;
                    }
                }

                _size += count;
            }
        }
        
        void    insert(T *itr, const T &value) 
        {
            insert(itr, &value, &value + 1);
        }
        
        T*      erase(T *itr) 
        {
            std::move(itr + 1, end(), itr);
            _ptr[--_size].~T();
            return itr;
        }
        
        void    erase(T *start_erase, T *end_erase) 
        {
            if (end_erase == end())
            {
                resize(size_t(start_erase - begin()));
            }
            else
            {
                auto new_size = _size - (end_erase - start_erase);
                std::move(end_erase, end(), start_erase);
                resize(new_size);
            }
        }
        
        void    resize(size_t new_size) 
        {
            if (new_size < _size)
            {
                for (size_t i = new_size; i < _size; i++)
                    _ptr[i].~T();
            }
            else if (new_size > _size)
            {
                reserve(new_size);
                for (size_t i = _size; i < new_size; i++)
                    new (&_ptr[i]) T();
            }
            _size = new_size;
        }
    protected:
        size_t              _capacity   =    0;
        AlignedBuffer<T, N> _memory;
    };
}

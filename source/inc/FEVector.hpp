#pragma     once
#include    <memory>
#include    <algorithm>
#include    <initializer_list>
#include    <stdexcept>

namespace   FE
{
    /// <summary>
    /// 该类不通用,属于专用类
    /// 内部使用，主要目的解决内数据管理权变更,但不释放内存
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T,typename TSize = uint64_t>
    class   FEVector 
    {
    public:
        using value_type        =   T;
        using difference_type   =   std::ptrdiff_t;
        using reference         =   value_type&;
        using const_reference   =   const value_type&;
        using pointer           =   value_type*;
        using const_pointer     =   const value_type*;
        using iterator          =   pointer;
        using const_iterator    =   const_pointer;
    public:
        /// <summary>
        /// 构造函数
        /// </summary>
        FEVector() noexcept 
            : _data(nullptr)
            , _size(0)
            , _cap(0) 
        {}

        explicit FEVector(TSize count) 
            : _data(nullptr)
            , _cap(0) 
            , _value(0)
        {
            if (count > 0)
            {
                reserve(count);
                for (TSize i = 0; i < count; ++i)
                    new (_data + i) T();
                _size   =   count;
            }
        }

        FEVector(TSize count, const T& value) 
            : _data(nullptr)
            , _cap(0) 
            , _value(0)
        {
            if (count > 0) 
            {
                reserve(count);
                for (TSize i = 0; i < count; ++i)
                    new (_data + i) T(value);
                _size   =   count;
            }
        }
        template<typename InputIt>
        FEVector(InputIt first, InputIt last) 
            : _data(nullptr)
            , _cap(0) 
            , _value(0)
        {
            TSize count =   static_cast<TSize>(std::distance(first, last));
            reserve(count);
            for (; first != last; ++first, ++_size)
                new (_data + _size) T(*first);
        }
        template<class U>
        FEVector(std::initializer_list<U> init)
            : _data(nullptr)
            , _cap(0) 
            , _value(0)
        {
            reserve(init.size());
            for (const auto& val : init)
                push_back(T(val));
        }
        FEVector(std::initializer_list<T> init)
            : _data(nullptr)
            , _cap(0) 
            , _value(0)
        {
            reserve(init.size());
            for (const auto& val : init)
                push_back(T(val));
        }
        /// <summary>
        /// 拷贝构造
        /// </summary>
        /// <param name="other"></param>
        FEVector(const FEVector& other) 
            : _data(nullptr)
            , _cap(0) 
            , _value(0)
        {
            reserve(other._size);
            for (TSize i = 0; i < other._size; ++i)
                new (_data + i) T(other._data[i]);
            _size   =   other._size;
        }
        /// <summary>
        /// 移动构造
        /// </summary>
        /// <param name="other"></param>
        FEVector(FEVector&& other) noexcept
            : _data(other._data)
            , _cap(other._cap) 
            , _value(other._value)
        {
            other._data     =   nullptr;
            other._value    =   0;
            other._cap      =   0;
        }
        /// <summary>
        /// 析构
        /// </summary>
        ~FEVector() 
        {
            destroy();
        }
        /// <summary>
        /// 拷贝赋值
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        FEVector&   operator=(const FEVector& other) 
        {
            if (this != &other) 
            {
                FEVector temp(other);
                swap(temp);
            }
            return *this;
        }
        /// <summary>
        /// 移动赋值
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        FEVector&   operator=(FEVector&& other) noexcept
        {
            if (this != &other) 
            {
                destroy();
                _value          =   other._value;
                _cap            =   other._cap;
                _free           =   other._free;
                other._data     =   nullptr;
                other._value    =   0;
                other._cap      =   0;
            }
            return *this;
        }
        /// <summary>
        /// std::initializer_list<uint8_t>  val =   {1,2,3,4,5,6};
        /// FEVector    arr;
        /// arr =   val;
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="init"></param>
        /// <returns></returns>
        template<class U>
        FEVector&   operator=(std::initializer_list<U> init) 
        {
            TSize  nByte   =   init.size() * sizeof(U);
            TSize  nCap    =   nByte;
            TSize  count   =   nByte/sizeof(T);
            assert(count * sizeof(T) == nByte);
            if (count * sizeof(T) != nByte)
            {
                throw   "U & T must be an integer multiple !";
            }
            resize(count);
            memcpy(data(),init.begin(),nByte);
            return *this;
        }
        /// <summary>
        /// 比如把一个结构体给数组赋值
        /// struct  UBO
        /// {
        ///     mat4    mpv;
        ///     mat4    v;
        ///     mat4    p;
        /// };
        /// UBO         val;
        /// FEVector    arr;
        /// arr     =   val;
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="data"></param>
        /// <returns></returns>
        template<class U>
        FEVector&   operator=(const U& val) 
        {
            TSize  nByte   =   sizeof(val);
            TSize  count   =   nByte/sizeof(T);
            assert(count * sizeof(T) == nByte);
            if (count * sizeof(T) != nByte)
            {
                throw   "U & T must be an integer multiple !";
            }
            resize(count);
            memcpy(data(),&val,nByte);
            return *this;
        }
        /// <summary>
        /// int         data[4] =   {1,2,2,3};
        /// FEVector    arr;
        /// arr =   data;
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <typeparam name="N"></typeparam>
        /// <param name="arr"></param>
        /// <returns></returns>
        template<class U, size_t N>
        FEVector&   operator=(const U (&arr)[N])
        {
            TSize  nByte   =   sizeof(U) * N;
            TSize  count   =   nByte/sizeof(T);
            assert(count * sizeof(T) == nByte);
            if (count * sizeof(T) != nByte)
            {
                throw   "U & T must be an integer multiple !";
            }
            resize(count);
            memcpy(data(),(const U*)arr,nByte);
            return *this;
        }
        /// <summary>
        /// 使用该方法，内存从一个对象移动到另一个对象，数据不会发生变化，但是管理权发生变化
        /// 不会调用释放动作
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="other"></param>
        /// <returns></returns>
        template<class U>
        auto&       referenceFrom(FEVector<U>& other)
        {
            TSize  nByte   =   other.size() * sizeof(U);
            TSize  nCap    =   other.capacity()  * sizeof(U);
            TSize  count   =   nByte/sizeof(T);
            assert(count * sizeof(T) == nByte);
            if (count * sizeof(T) != nByte)
            {
                throw   "U & T must be an integer multiple !";
            }
            destroy();
            _data           =   (T*)other.data();
            _size           =   count;
            _cap            =   nCap  / sizeof(T);
            other.detach();
            return *this;
        }
        template<class U>
        auto&       copyFrom(const FEVector<U>& other)
        {
            if (this == &other) 
                return  *this;

            TSize  nByte   =   other._size * sizeof(U);
            TSize  nCap    =   other._cap  * sizeof(U);
            TSize  count   =   nByte/sizeof(T);
            assert(count * sizeof(T) == nByte);
            if (count * sizeof(T) != nByte)
            {
                throw   "U & T must be an integer multiple !";
            }
            resize(count);
            memcpy(data(),other.data(),nByte);

            return *this;
        }
        template<class U>
        auto&       copyFrom(std::initializer_list<U> init)
        {
            *this    =   init;
            return *this;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        iterator        begin() noexcept        { return _data; }
        const_iterator  begin() const noexcept  { return _data; }
        const_iterator  cbegin() const noexcept { return _data; }
        iterator        end() noexcept          { return _data + _size; }
        const_iterator  end() const noexcept    { return _data + _size; }
        const_iterator  cend() const noexcept   { return _data + _size; }
        pointer         data() noexcept         { return _data; }
        const_pointer   data() const noexcept   { return _data; }
        /// <summary>
        /// 按字节结算
        /// </summary>
        /// <returns></returns>
        inline  TSize   bytes()const noexcept   { return _size * sizeof(T); }
        /// <summary>
        /// 按元素个数计算
        /// </summary>
        /// <returns></returns>
        inline  TSize   size() const noexcept       { return _size; }
        inline  TSize   capacity() const noexcept   { return _cap;  }
        inline  bool    empty() const noexcept      { return _size == 0; }
        inline  void    reserve(TSize newCap) 
        {
            if (newCap <= _cap) 
                return;
            pointer pNew    =   allocate(newCap);
            try
            {
                for (TSize i = 0; i < _size; ++i)
                    new (pNew + i) T(std::move_if_noexcept(_data[i]));
            } 
            catch (...) 
            {
                deallocate(pNew, newCap);
                throw;
            }
            destroyAnddeallocate();
            _data   =   pNew;
            _cap    =   newCap;
            // 注意 sz_ 不变
        }
        inline  void    shrink_to_fit() 
        {
            if (_size == _cap) 
                return;
            if (_size == 0) 
            {
                destroy();
                _data   =   nullptr;
                _cap    =   0;
                _free   =   true;
                return;
            }
            pointer pNew    =   allocate(_size);
            for (TSize i = 0; i < _size; ++i)
                new (pNew + i) T(std::move_if_noexcept(_data[i]));
            destroyAnddeallocate();
            _data   =   pNew;
            _cap    =   _size;
        }
        /// <summary>
        /// 重新分配数组大小
        /// </summary>
        /// <param name="count"></param>
        inline  void    resize(TSize count) 
        {
            if (count < _size) 
            {
                for (TSize i = count; i < _size; ++i)
                    _data[i].~T();
                _size   =   count;
            } 
            else if (count > _size) 
            {
                reserve(count);
                for (TSize i = _size; i < count; ++i)
                    new (_data + i) T();
                _size   =   count;
            }
        }
        /// <summary>
        /// 重新分配数组大小
        /// </summary>
        /// <param name="count"></param>
        /// <param name="value"></param>
        inline  void    resize(TSize count, const T& value) 
        {
            if (count < _size) 
            {
                for (TSize i = count; i < _size; ++i)
                    _data[i].~T();
                _size   =   count;
            } 
            else if (count > _size) 
            {
                reserve(count);
                for (TSize i = _size; i < count; ++i)
                    new (_data + i) T(value);
                _size   =   count;
            }
        }
        /// <summary>
        /// 元素访问
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        reference       operator[](TSize pos)       { return _data[pos]; }
        const_reference operator[](TSize pos) const { return _data[pos]; }
        reference       at(TSize pos) 
        {
            if (pos >= _size) 
                throw std::out_of_range("FEVector::at");
            return _data[pos];
        }
        const_reference at(TSize pos) const 
        {
            if (pos >= _size) 
                throw std::out_of_range("FEVector::at");
            return  _data[pos];
        }
        reference       front()         { return _data[0]; }
        const_reference front() const   { return _data[0]; }
        reference       back()          { return _data[_size - 1]; }
        const_reference back() const    { return _data[_size - 1]; }
        /// <summary>
        /// 修改器
        /// </summary>
        /// <param name="value"></param>
        inline  void    push_back(const T& value) 
        {
            emplace_back(value);
        }
        /// <summary>
        /// 添加元素
        /// </summary>
        /// <param name="value"></param>
        inline  void    push_back(T&& value) 
        {
            emplace_back(std::move(value));
        }
        /// <summary>
        /// push 元素
        /// </summary>
        /// <typeparam name="...Args"></typeparam>
        /// <param name="...args"></param>
        /// <returns></returns>
        template<typename... Args>
        reference       emplace_back(Args&&... args) 
        {
            if (_size == _cap) 
            {
                reserve(_cap == 0 ? 1 : _cap * 2);
            }
            new (_data + _size) T(std::forward<Args>(args)...);
            ++_size;
            return back();
        }
        /// <summary>
        /// 弹出元素
        /// </summary>
        inline  void    pop_back() 
        {
            if (!empty()) 
            {
                _data[_size - 1].~T();
                --_size;
            }
        }
        /// <summary>
        /// 清空，不释放内存，但调用析构
        /// </summary>
        inline  void    clear() noexcept 
        {
            /// 只有非平凡析构时才真正调用
            if constexpr (!std::is_trivially_destructible_v<T>) 
            {
                for (TSize i = 0; i < _size; ++i)
                    _data[i].~T();
            }
            _size   =   0;
        }
        /// <summary>
        /// 交换对象
        /// </summary>
        /// <param name="other"></param>
        inline  void    swap(FEVector& other) noexcept 
        {
            std::swap(_data,    other._data);
            std::swap(_value,   other._value);
            std::swap(_cap,     other._cap);
        }
        /// <summary>
        /// 插入元素
        /// </summary>
        /// <param name="pos"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        inline iterator insert(const_iterator pos, const T& value) 
        {
            TSize   index    =   pos -   begin();
            if (_size == _cap) 
                reserve(_cap == 0 ? 1 : _cap * 2);
            pointer p = _data + index;
            if (_size > index) 
            {
                /// 从后往前移动元素
                new (_data + _size) T(std::move(_data[_size - 1]));
                for (TSize i = _size - 1; i > index; --i)
                    _data[i] = std::move(_data[i - 1]);
                *p = value;
            } 
            else 
            {
                new (p) T(value);
            }
            ++_size;
            return iterator(p);
        }
        /// <summary>
        /// 插入元素
        /// </summary>
        /// <param name="pos"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        inline iterator insert(const_iterator pos, T&& value) 
        {
            TSize index = pos - begin();
            if (_size == _cap) 
                reserve(_cap == 0 ? 1 : _cap * 2);
            pointer p = _data + index;
            if (_size > index) 
            {
                new (_data + _size) T(std::move(_data[_size - 1]));
                for (TSize i = _size - 1; i > index; --i)
                    _data[i] = std::move(_data[i - 1]);
                *p = std::move(value);
            } 
            else 
            {
                new (p) T(std::move(value));
            }
            ++_size;
            return  iterator(p);
        }
        /// <summary>
        /// 插入初始化列表
        /// </summary>
        /// <param name="pos"></param>
        /// <param name="iList"></param>
        /// <returns></returns>
        inline  iterator insert(const_iterator pos, std::initializer_list<T> iList)
        {
            for (auto&& val : iList)
            {
                pos =   insert(pos,val);
            }
            return  (iterator)pos;
        }
        /// <summary>
        /// 删除元素
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        inline iterator erase(const_iterator pos) 
        {
            TSize index = pos - begin();
            if (index >= _size) 
                return end();
            for (TSize i = index; i < _size - 1; ++i)
                _data[i] = std::move(_data[i + 1]);
            _data[_size - 1].~T();
            --_size;
            return iterator(_data + index);
        }
        /// <summary>
        /// 删除元素
        /// </summary>
        /// <param name="first"></param>
        /// <param name="last"></param>
        /// <returns></returns>
        inline iterator erase(const_iterator first, const_iterator last) 
        {
            TSize first_idx = first - begin();
            TSize last_idx  = last - begin();
            if (first_idx >= _size || first_idx >= last_idx) return end();
            TSize n = last_idx - first_idx;
            for (TSize i = first_idx; i + n < _size; ++i)
                _data[i] = std::move(_data[i + n]);
            for (TSize i = _size - n; i < _size; ++i)
                _data[i].~T();
            _size -= n;
            return iterator(_data + first_idx);
        }
        /// <summary>
        /// 通过外部接口关联内存
        /// 该接口与 detach 主要是为了解决外部系统内存复用，谨慎使用
        /// </summary>
        /// <param name="data">数据缓冲区</param>
        /// <param name="len">元素个数</param>
        /// <param name="bFree">是否可以被系统释放</param>
        /// <returns></returns>
        inline  auto&   attach(T* data,size_t len,bool bFree)
        {
            /// 安全检测
            assert(data != _data);
            if (data == _data)
                return  *this;
            /// 释放老数据
            destroy();
            _data   =   data;
            _size   =   len;
            _cap    =   len;
            _free   =   bFree;
            return  *this;
        }
        /// <summary>
        /// 取消数据管理,不调用释放内存，内存需要外部自行管理，否则会出现内存泄露
        /// 该接口与 attach 主要是为了解决外部系统内存复用设计，谨慎使用
        /// </summary>
        /// <returns></returns>
        inline  auto&   detach()
        {
            _data   =   nullptr;
            _size   =   0;
            _cap    =   0;
            _free   =   true;
            return  *this;
        }
    public:
        friend  bool    operator==(const FEVector<T>& left,const FEVector<T>& right)
        {
            if (left.size() != right.size())
                return  false;
            for (size_t i = 0; i < left.size(); i++)
            {
                if (left[i] != right[i])
                    return  false;
            }
            return  true;
        }
        friend  bool    operator!=(const FEVector<T>& left,const FEVector<T>& right)
        {
             return !(left == right);
        }
    protected:
        /// <summary>
        /// 销毁数据,释放内存
        /// </summary>
        void        destroy()
        {
            clear();
            if (_free)
                delete[] reinterpret_cast<char*>(_data);
        }
        /// <summary>
        /// 调用析构函数,销毁数据,释放内存
        /// </summary>
        void        destroyAnddeallocate() noexcept 
        {
            if (_data == nullptr) 
                return;
            /// 只有非平凡析构时才真正调用
            if constexpr (!std::is_trivially_destructible_v<T>) 
            {
                for (TSize i = 0; i < _size; ++i)
                    _data[i].~T();
            }
            deallocate(_data, _cap);
        }
    protected:
        pointer     _data;
        TSize       _cap;
        union
        {
            struct  
            {
                TSize       _size:56;
                TSize       _free:8;
            };
            TSize   _value;
        };
    private:
        /// <summary>
        /// 分配内存
        /// </summary>
        /// <param name="n"></param>
        /// <returns></returns>
        static pointer  allocate(TSize n) 
        {
            if (n == 0) return nullptr;
            return static_cast<pointer>(::operator new(n * sizeof(T)));
        }
        /// <summary>
        /// 释放内存
        /// </summary>
        /// <param name="p"></param>
        /// <param name="n"></param>
        static void     deallocate(pointer p, TSize n) noexcept 
        {
            (void)n;
            if (p) ::operator delete(p);
        }
        /// <summary>
        /// 交换对象数据
        /// </summary>
        /// <param name="lhs"></param>
        /// <param name="rhs"></param>
        friend  void    swap(FEVector<T>& lhs, FEVector<T>& rhs) noexcept(noexcept(lhs.swap(rhs))) 
        {
            lhs.swap(rhs);
        }
    };

    /// <summary>
    /// 强制编译器编译模板，避免错误
    /// </summary>
    template class FEVector<float>;
}

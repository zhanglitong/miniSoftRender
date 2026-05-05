
#include    "../inc/FEObject.h"
#include    <new> 
#include    <memory> 
namespace   FE
{
    const char*     FEObject::className() const          
    {                                                  
        return  "FEObject";                           
    }                                                  
    const FEUuid&   FEObject::classId() const
    {                                                  
        return  UUIDOF(FEObject);                    
    }

    void*   FEObject::operator new(std::size_t size) 
    {
        /// 调用带对齐参数的版本，默认对齐 16 字节
        return operator new(size, std::align_val_t(16));
    }
    void*   FEObject::operator new(std::size_t size, std::align_val_t align) 
    {
        /// 直接使用 C++17 提供的全局对齐分配函数
        /// 如果分配失败，将抛出 std::bad_alloc （或调用 new_handler）
        return ::operator new(size, align);
    }
    void    FEObject::operator delete(void* ptr) noexcept 
    {
        ::operator delete(ptr);
    }
    void    FEObject::operator delete(void* ptr, std::align_val_t align) noexcept 
    {
        ::operator delete(ptr, align);
    }
}
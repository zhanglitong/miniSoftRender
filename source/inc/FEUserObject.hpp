#pragma     once
#include    "FEObject.h"

namespace   FE
{
    class   FEUserObject
    {
    protected:
        Object  _userObject;
    public:
        /// <summary>
        /// 获取自定义关联的对象
        /// </summary>
        /// <returns></returns>
        inline  Object  userObject() const
        {
            return  _userObject;
        }
        inline  Object  userObject()
        {
            return  _userObject;
        }
        /// <summary>
        /// 设置自定义对象(与业务无关，辅助业务实现使用)
        /// </summary>
        /// <param name="user"></param>
        /// <returns></returns>
        inline  auto&   setUserObject(Object user)
        {
            _userObject =   user;
            return  *this;
        }
        /// <summary>
        /// 设置自定义对象(与业务无关，辅助业务实现使用)
        /// </summary>
        /// <param name="user"></param>
        /// <returns></returns>
        inline  auto&   setUserObject(FEObject* user)
        {
            _userObject =   user;
            return  *this;
        }
    };
}

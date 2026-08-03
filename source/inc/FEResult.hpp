#pragma     once

namespace   FE
{
    class    FEResult
    {
    public:
        /// <summary>
        /// Error Code
        /// </summary>
        enum    ErrorCode
        {
            ER_SUCCESS,
            ER_FAILED,
        };
    public:
        FEResult(ErrorCode code)
            :_code(code)
        {}
        ErrorCode   errorCode() const
        {
            return  _code;
        }
        bool    operator ==(ErrorCode code) const
        {
            return  _code == code;
        }
        bool    operator !=(ErrorCode code) const
        {
            return  _code != code;
        }
    protected:
        ErrorCode   _code;
    };
}
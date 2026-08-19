#pragma     once

#include    <mutex>

namespace   FE
{
    class   FEMutex
    {
    protected:
        std::recursive_mutex  _mutex;
    public:
        class ScopeLock
        {
        protected:
            FEMutex&  _mutex;
        public:
            ScopeLock(FEMutex& mutex)
                :_mutex(mutex)
            {
                _mutex.lock();
            }
            ~ScopeLock()
            {
                _mutex.unLock();
            }
        private:
            ScopeLock(const ScopeLock&);
            void operator = (const ScopeLock&);
        };
    public:
        FEMutex()
        {
        }
        virtual ~FEMutex()
        {
        }
        void    lock()
        {
            _mutex.lock();
        }

        void    unLock()
        {
            _mutex.unlock();
        }

        bool    tryLock()
        {
            return  _mutex.try_lock();
        }
    private:
        void operator = (const FEMutex&);
    };
}

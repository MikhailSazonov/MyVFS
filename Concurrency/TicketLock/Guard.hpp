#pragma once

#include "RWTicketLock.hpp"

namespace TestTask::Concurrency
{
    /*
        Создаёт защищённый тикет-локом объект
    */

    template <typename T>
    class Guard
    {
        public:
            template <typename ...Args>
            Guard(Args&&... args)
                : object_(std::forward(args)...)
            {}

            const T& ReadAccess() {
                lock_.AcquireRead();
                return object_;
            }

            T& WriteAccess() {
                lock_.AcquireWrite();
                return object_;
            }

            void ReturnAccess() {
                lock_.Release();
            }

        private:
            T object_;
            RWTicketLock lock_;
    }

    template <typename T>
    class WriteGuard
    {
        public:
            WriteGuard(Guard<T>& g)
                : g_(g), object_(g.WriteAccess()) {}

            T& get()
            {
                return object_;
            }

            ~WriteGuard()
            {
                g_.ReturnAccess();
            }

        private:
            Guard<T>& g_;
            T& object_;
    };


    template <typename T>
    class ReadGuard
    {
        public:
            ReadGuard(Guard<T>& g)
                : g_(g), object_(g.ReadAccess()) {}

            const T& get()
            {
                return object_;
            }

            ~ReadGuard()
            {
                g_.ReturnAccess();
            }

        private:
            Guard<T>& g_;
            const T& object_;
    };
}
#ifndef CONCURRENTQUEUE_H
#define CONCURRENTQUEUE_H

#include <config.h>


namespace DePlagiarism {
    
    template <typename T>    
    class ConcurrentQueue
    {
    private:
        ConcurrentQueue(const ConcurrentQueue & src){}
        ConcurrentQueue & operator=(ConcurrentQueue & src){}
        MUTEX_TYPE m_mtx;
        COND_TYPE m_condition;
        size_t m_maxLength;
        size_t m_head, m_tail;
        T * m_data;
    public:
        ConcurrentQueue(size_t size):
            m_maxLength(size), m_head(0), m_tail(0), m_data(new T[size])
        {
            COND_SETUP(m_condition);
            MUTEX_SETUP(m_mtx);
        }
        bool push(const T & newEl) {
            bool status = true;
            size_t next;
            MUTEX_LOCK(m_mtx);
            next = m_tail + 1;
            if (next >= m_maxLength)
                next = 0;
            if (next == m_head)
                status = false;
            else
            {
                m_data[m_tail] = newEl;
                m_tail = next;
            }
            COND_SIGNAL(m_condition);
            MUTEX_UNLOCK(m_mtx);
            return status;
        }
        T pop() {
            T res;
            MUTEX_LOCK(m_mtx);
            while (m_head == m_tail)
                COND_WAIT(m_condition, m_mtx);
            res = m_data[m_head++];
            if (m_head >= m_maxLength)
                m_head = 0;
            MUTEX_UNLOCK(m_mtx);
            return res;
        }
        ~ConcurrentQueue() {
            delete[] m_data;
            COND_CLEANUP(m_condition);
            MUTEX_CLEANUP(m_mtx);
        }
    };

}
#endif // CONCURRENTQUEUE_H

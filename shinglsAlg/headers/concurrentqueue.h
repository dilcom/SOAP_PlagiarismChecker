#ifndef CONCURRENTQUEUE_H
#define CONCURRENTQUEUE_H

#include <config.h>


namespace DePlagiarism {
    /*!
     * Simple implementation of concurrent locking queue.
     * May ONLY be used to store simple types.
     */
    template <typename T>    
    class ConcurrentQueue
    {
    private:
        ConcurrentQueue(const ConcurrentQueue & src){} ///< Disable intance via copy
        ConcurrentQueue & operator=(ConcurrentQueue & src){} ///< Disable instance via copy
        MUTEX_TYPE m_mtx;
        COND_TYPE m_condition;
        size_t m_maxLength;
        size_t m_head, m_tail;
        T * m_data;
    public:
        /*!
         * \brief Instancies a queue of static size
         * \param Size is max count of elements you want to be stored
         */
        ConcurrentQueue(size_t size):
            m_maxLength(size), m_head(0), m_tail(0), m_data(new T[size])
        {
            COND_SETUP(m_condition);
            MUTEX_SETUP(m_mtx);
        }
        /*!
         * \brief Adds one more element in queue
         * \param newEl is reference to new element. Type must support copy via =
         * \return true if succeessed otherwise false (for example if queue is full)
         */
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
        /*!
         * Be careful: function locks current thread while queue is empty.
         * \brief Removes one element from the queue and returns its value
         * \return value of last element
         */
        T pop() {
            MUTEX_LOCK(m_mtx);
            while (m_head == m_tail)
                COND_WAIT(m_condition, m_mtx);
            T res = m_data[m_head++];
            if (m_head >= m_maxLength)
                m_head = 0;
            MUTEX_UNLOCK(m_mtx);
            return res;
        }
        /*!
         * Deletes all the objects remaining in queue.
         * \brief Destructor
         */
        ~ConcurrentQueue() {
            delete[] m_data;
            COND_CLEANUP(m_condition);
            MUTEX_CLEANUP(m_mtx);
        }
    };

}
#endif // CONCURRENTQUEUE_H

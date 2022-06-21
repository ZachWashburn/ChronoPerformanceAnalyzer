#pragma once
#ifndef _H_CHRONOPERFOMANCE_ANALYZER
#define _H_CHRONOPERFOMANCE_ANALYZER

#include <deque>
#include <Windows.h>
#include <chrono>
#include <map>
#include <mutex>

template<class T> 
class ChronoPerfomanceAnalyzer
{
public:
    virtual void __forceinline start()
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    virtual void __forceinline end()
    {
        m_End = std::chrono::high_resolution_clock::now();
        calculate(std::chrono::duration_cast<T>(m_End - m_Start));
    }

    virtual void __forceinline calculate(T additional)
    {
        auto weighted = m_Average * m_nSamples;
        m_Average = (weighted + additional) / ++m_nSamples;
    }

    virtual long long GetAverageInt64()
    {
        return m_Average.count();
    }


    __int64 m_nSamples = 0;
    T m_Average = (T)0 ;
    std::chrono::time_point<std::chrono::steady_clock> m_Start;
    std::chrono::time_point<std::chrono::steady_clock> m_End;
};

template <class T>
class ChronoPerfomanceAnalyzerThread
{
public:
    virtual void __forceinline start()
    {
        m_PerfCounters[GetCurrentThreadId()].start();
    }

    virtual void __forceinline end()
    {
        m_PerfCounters[GetCurrentThreadId()].end();
    }

    virtual long long GetAverageInt64CurrentThread()
    {
        return m_PerfCounters[GetCurrentThreadId()].GetAverageInt64();
    }

    virtual long long GetAverageInt64()
    {
        int nTotalCounters = 0;
        long long nTotal = 0;
        for (auto& perf_counter : m_PerfCounters)
        {
            nTotal += perf_counter.second.GetAverageInt64();
            nTotalCounters++;
        }
        return nTotal / nTotalCounters;
    }


    void CullDeadThreads()
    {
        std::lock_guard<std::mutex> _(m_DeleteLock);
        std::deque<int> deletables;
        for (auto& perf_counter : m_PerfCounters)
        {
            HANDLE threadHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, perf_counter.first);
            DWORD result = WaitForSingleObject(threadHandle, 0);
            if (threadHandle == NULL || result == WAIT_OBJECT_0)
                deletables.push_back(perf_counter.first);
        }

        for (const auto& del : deletables)
            m_PerfCounters.erase(del);
    }

private:
    std::map<int, ChronoPerfomanceAnalyzer<T>> m_PerfCounters;
    std::mutex m_DeleteLock;
};


template <class T>
class ChronoPerfScope
{
public:
    __forceinline ChronoPerfScope(T& PerfAnalyzer) : m_Perf(&PerfAnalyzer)
    {
        m_Perf->start();
    }

    __forceinline ~ChronoPerfScope()
    {
        m_Perf->end();
    }
    T* m_Perf;
};

#endif
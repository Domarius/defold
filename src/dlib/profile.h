#ifndef DM_PROFILE_H
#define DM_PROFILE_H

#include <stdint.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <dlib/array.h>
#include <dlib/log.h>
#include <dlib/atomic.h>

#define DM_PROFILE_PASTE(x, y) x ## y
#define DM_PROFILE_PASTE2(x, y) DM_PROFILE_PASTE(x, y)

/**
 * Profile macro.
 * scope_name is the scope name. Must be a literal
 * name is the sample name. An arbitrary constant string
 */
#define DM_PROFILE(scope_name, name)
#undef DM_PROFILE

/**
 * Profile counter macro
 * name is the counter name
 * amount is the amount (integer) to add to the specific counter.
 */
#define DM_COUNTER(name, amount)
#undef DM_COUNTER

/**
 * Profile counter macro. Fast version of #DM_COUNTER
 * name is the counter name
 * name_hash is the prehashed name
 * amount is the amount (integer) to add to the specific counter.
 */
#define DM_COUNTER_HASH(name, name_hash, amount)
#undef DM_COUNTER_HASH

#ifdef DM_PROFILE_DISABLE
#define DM_PROFILE(scope_name, name)
#define DM_COUNTER(name, amount)
#define DM_COUNTER_HASH(name, amount)
#else
#define DM_PROFILE(scope_name, name) \
    static dmProfile::Scope* DM_PROFILE_PASTE2(scope, __LINE__) = 0; \
    if (DM_PROFILE_PASTE2(scope, __LINE__) == 0) \
    {\
        DM_PROFILE_PASTE2(scope, __LINE__) = dmProfile::AllocateScope(#scope_name);\
    }\
    dmProfile::ProfileScope DM_PROFILE_PASTE2(profile_scope, __LINE__)(DM_PROFILE_PASTE2(scope, __LINE__), name);\

    #define DM_COUNTER(name, amount) \
    dmProfile::AddCounter(name, amount);

    #define DM_COUNTER_HASH(name, name_hash, amount) \
    dmProfile::AddCounterHash(name, name_hash, amount);

#endif

namespace dmProfile
{
#ifndef DM_PROFILE_DISABLE

    /**
     * Profile scope
     */
    struct Scope
    {
        /// Scope name
        const char* m_Name;
        /// Total time spent in scope (in ticks)
        int32_t     m_Elapsed;
        /// Scope index, range [0, scopes-1]
        uint16_t    m_Index;
        /// Occurrences of this scope (nestled occurrences do not count)
        int32_t     m_Count;
    };

    /**
     * Profile sample
     */
    struct Sample
    {
        /// Sample name
        const char*  m_Name;
        /// Sampled within scope
        Scope*       m_Scope;
        /// Start time in ticks
        uint32_t     m_Start;
        /// Elasped time in ticks
        uint32_t     m_Elapsed : 28;

        uint32_t     m_Reserved : 4;
    };

    /**
     * Profile counter
     */
    struct Counter
    {
        /// Counter name
        const char*      m_Name;
        /// Counter name hash
        uint32_t         m_NameHash;
        /// Counter value
        int32_atomic_t  m_Counter;
    };

    /**
     * Initialize profiler
     * @param max_scopes Maximum scopes
     * @param max_samples Maximum samples
     * @param max_counters Maximum counters
     */
    void Initialize(uint32_t max_scopes, uint32_t max_samples, uint32_t max_counters);

    /**
     * Finalize profiler
     */
    void Finalize();

    /**
     * Begin profiling, eg start of frame
     */
    void Begin();

    /**
     * End profiling, eg end of frame
     */
    void End();

    /**
     * Get ticks per second
     * @return Ticks per second
     */
    uint64_t GetTicksPerSecond();

    /**
     * Iterate over all scopes
     * @param context User context
     * @param call_back Call-back function pointer
     */
    void IterateScopes(void* context, void (*call_back)(void* context, const Scope* sample));

    /**
     * Iterate over all samples
     * @param context User context
     * @param call_back Call-back function pointer
     */
    void IterateSamples(void* context, void (*call_back)(void* context, const Sample* sample));

    /**
     * Iterate over all counters
     * @param context User context
     * @param call_back Call-back function pointer
     */
    void IterateCounters(void* context, void (*call_back)(void* context, const Counter* counter));

    /**
     * Internal function
     * @param name
     * @return #Scope
     */
    Scope* AllocateScope(const char* name);

    /**
     * Internal function
     * @return #Sample
     */
    Sample* AllocateSample();

    /**
     * Add #amount to counter with #name
     * @param name Counter name
     * @param amount Amount to add
     */
    void AddCounter(const char* name, uint32_t amount);

    /**
     * Add #amount to counter with prehashed name. Faster version of #AddCounter
     * @param name Counter name
     * @param name Counter name hash
     * @param amount Amount to add
     */
    void AddCounterHash(const char* name, uint32_t name_hash, uint32_t amount);

    /**
     * Get time for the frame total
     * @return Total frame time
     */
    float GetFrameTime();

    /**
     * Get time for the frame total during the last 60 frames
     * @return Total frame time
     */
    float GetMaxFrameTime();

    /**
     * Check of out of scope resources
     * @return True if out of scope resources
     */
    bool IsOutOfScopes();

    /**
     * Check of out of sample resources
     * @return True if out of sample resources
     */
    bool IsOutOfSamples();

    /// Internal, do not use.
    extern uint32_t g_BeginTime;

    /// Internal, do not use.
    struct ProfileScope
    {
        uint64_t    m_Start;
        Sample*     m_Sample;
        Scope*      m_Scope;
        inline ProfileScope(Scope* scope, const char* name)
        {
            Sample*s = AllocateSample();
            m_Scope = scope;

            s->m_Name = name;
            s->m_Scope = scope;
            m_Sample = s;

#if defined(_WIN32)
            QueryPerformanceCounter((LARGE_INTEGER *)&m_Start);
#else
            timeval tv;
            gettimeofday(&tv, 0);
            m_Start = tv.tv_sec * 1000000 + tv.tv_usec;
#endif
        }

        inline ~ProfileScope()
        {
            uint64_t end;
#if defined(_WIN32)
            QueryPerformanceCounter((LARGE_INTEGER *) &end);
#else
            timeval tv;
            gettimeofday(&tv, 0);
            end = tv.tv_sec * 1000000 + tv.tv_usec;
#endif
            uint64_t diff = end - m_Start;
            m_Sample->m_Start = (uint32_t) (m_Start - g_BeginTime);
            m_Sample->m_Elapsed = diff;
            dmAtomicAdd32(&m_Scope->m_Elapsed, (int32_t) diff);
            dmAtomicIncrement32(&m_Scope->m_Count);
        }
    };

#endif

}

#endif

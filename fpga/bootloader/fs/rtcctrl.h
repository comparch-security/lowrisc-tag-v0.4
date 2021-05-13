#ifndef _BBL_PK_RTC_CTRL
#define _BBL_PK_RTC_CTRL

#define BBL_PK_RTC2_DELTA 500000000lu

#define BBL_PK_LIMITED_RUN

extern uint64_t start_instret;

#ifdef BBL_PK_LIMITED_RUN

// #define BBL_PK_MINSTRET_WARMUP 0         //saved for future use.
#define BBL_PK_MINSTRET_TERMINATE 10000000000lu

#else 

// #define BBL_PK_MINSTRET_WARMUP 0         //saved for future use.
// #define BBL_PK_MINSTRET_INTERVAL 0x10000 //saved for future use.
#define BBL_PK_MINSTRET_TERMINATE (~0lu)

#endif // BBL_PK_LIMITED_RUN

#endif //!_BBL_PK_RTC_CTRL

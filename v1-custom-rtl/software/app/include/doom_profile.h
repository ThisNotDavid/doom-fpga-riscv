#ifndef DOOM_PROFILE_H
#define DOOM_PROFILE_H
enum {DP_LOGIC, DP_RENDER, DP_CONVERT, DP_WAIT, DP_TRANSFER, DP_STAGE_COUNT};
#ifdef DOOM_BENCHMARK
#define DP_IS_BENCHMARK 1
void DP_TickBegin(void);
void DP_TickEnd(void);
#ifdef DOOM_PROFILE
void DP_Begin(int stage);
void DP_End(int stage);
#else
#define DP_Begin(stage) ((void)0)
#define DP_End(stage) ((void)0)
#endif
#else
#define DP_IS_BENCHMARK 0
#define DP_TickBegin() ((void)0)
#define DP_TickEnd() ((void)0)
#define DP_Begin(stage) ((void)0)
#define DP_End(stage) ((void)0)
#endif
#endif

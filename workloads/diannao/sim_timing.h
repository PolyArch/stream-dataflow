#ifdef __x86_64__
static __inline__ uint64_t rdtsc(void) {
  unsigned a, d;
  //asm("cpuid");
  asm volatile("rdtsc" : "=a" (a), "=d" (d));

  return (((uint64_t)a) | (((uint64_t)d) << 32));
}

static uint64_t ticks;
__attribute__ ((noinline))  void begin_roi() {
  ticks=rdtsc();
}
__attribute__ ((noinline))  void end_roi()   {
  ticks=(rdtsc()-ticks);
  printf("ticks: %ld\n",ticks);
}
__attribute__ ((noinline)) static void sb_stats()   {
}
__attribute__ ((noinline)) static void sb_verify()   {
}

#else
__attribute__ ((noinline)) static void begin_roi() {
    __asm__ __volatile__("add x0, x0, 1"); \
}
__attribute__ ((noinline)) static void end_roi()   {
    __asm__ __volatile__("add x0, x0, 2"); \
}
__attribute__ ((noinline)) static void sb_stats()   {
    __asm__ __volatile__("add x0, x0, 3"); \
}
__attribute__ ((noinline)) static void sb_verify()   {
    __asm__ __volatile__("add x0, x0, 4"); \
}

#endif


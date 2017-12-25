#ifndef LC_COMMON_H
#define LC_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <stdint.h>

// check weather if x is power of 2
#define POWEROF2(x) ((((x) -1) & (x)) == 0)

#define CACHE_LINE_MASK     (CACHE_LINE_SIZE - 1)

// 向上取整
#define CACHE_LINE_ROUNDUP(size) \
    (CACHE_LINE_SIZE * ((size + CACHE_LINE_SIZE - 1)/ CACHE_LINE_SIZE))

// 设置内存屏障
#define smp_rmb()   asm volatile("lfence":::"memory")
#define smp_mb()   asm volatile("mfence":::"memory")
#define smp_wmb()   asm volatile("sfence":::"memory")

// Linux内核中定义根本结构成员取结构首地址的宏，只可用于GCC
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define atomic_compare_and_swap     __sync_bool_compare_and_swap
#define atomic_add                  __sync_fetch_and_add
#define atomic_sub                  __sync_fetch_and_sub
#define compile_barrier()           asm volatile("":::"memory");

typedef int socket_t;
typedef uint32_t ip_addr_t;
typedef uint16_t port_t;
typedef uint64_t mac_addr_t;

#define UNUSED(x)   (void)(x)

// 取自 kernel.h
#define DIV_ROUND_UP(nr, d)  (((nr) + (d) -1)/(d))

#endif

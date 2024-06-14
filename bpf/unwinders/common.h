#ifndef __LINUX_PAGE_CONSTANTS_HACK__
#define __LINUX_PAGE_CONSTANTS_HACK__

// Values for x86_64 as of 6.0.18-200.
#define TOP_OF_KERNEL_STACK_PADDING 0
#define THREAD_SIZE_ORDER 2
#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define THREAD_SIZE (PAGE_SIZE << THREAD_SIZE_ORDER)

#endif

#ifndef __ERROR_CONSTANTS_HACK__
#define __ERROR_CONSTANTS_HACK__

#define EFAULT 14
#define EEXIST 17
#endif

#ifndef __AGENT_STACK_TRACE_DEFINITION__
#define __AGENT_STACK_TRACE_DEFINITION__

#include "basic_types.h"
#define MAX_STACK_DEPTH 127

typedef struct {
    u32 len;
    bool truncated;
    u64 addresses[MAX_STACK_DEPTH];
} stack_trace_t;
// NOTICE: stack_t is defined in vmlinux.h.

#define CLASS_NAME_MAXLEN 32
#define METHOD_MAXLEN 64
#define PATH_MAXLEN 128

typedef struct {
    char class_name[CLASS_NAME_MAXLEN];
    char method_name[METHOD_MAXLEN];
    char path[PATH_MAXLEN];
} symbol_t;

// Use ERROR_SAMPLE to report one stack frame of an error message as an interpreter symbol.
// class -> msg provided in macro
// function -> __FUNCTION__ from C compiler
// line     -> __LINE__ from C compiler
// file     -> __FILE__ from C compiler
// Most uses should just return after calling this.
#define ERROR_SAMPLE(unw_state, msg)                                                  \
    ({                                                                                \
        symbol_t sym;                                                                 \
        __builtin_memset((void *)&sym, 0, sizeof(symbol_t));                          \
        __builtin_memset((void *)&unw_state->stack, 0, sizeof(stack_trace_t));        \
        __builtin_strncpy(sym.path, __FILE__, sizeof(sym.path));                      \
        __builtin_strncpy(sym.method_name, __FUNCTION__, sizeof(sym.method_name));    \
        __builtin_strncpy(sym.class_name, msg, sizeof(sym.class_name));               \
        u64 id = get_symbol_id(&sym);                                                 \
        u64 lineno = __LINE__;                                                        \
        unw_state->stack.addresses[0] = (lineno << 32) | id;                          \
        unw_state->stack.len = 1;                                                     \
        u64 stack_id = hash_stack(&unw_state->stack, 0);                              \
        unw_state->stack_key.interpreter_stack_id = stack_id;                         \
        bpf_map_update_elem(&stack_traces, &stack_id, &unwind_state->stack, BPF_ANY); \
        aggregate_stacks();                                                           \
    })
#endif

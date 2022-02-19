#include "rpi.h"
struct bt {
        struct bt *next;
        uint32_t sp;
        uint32_t lr;
        uint32_t pc;
    };

//helpers for printing from test-backtrace
static void pop(const char *msg) ;
static void push(const char *msg);
static void indent(unsigned n);
static const char *name_of(uintptr_t fn_start_addr);
void backtrace();

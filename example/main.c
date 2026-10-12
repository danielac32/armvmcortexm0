#include <stdint.h>
#include "serial.h"

/* ================================================================
 * SysTick register definitions
 * ================================================================ */
#define SYSTICK_BASE ((volatile uint32_t *)0xE000E010)
#define SYSTICK_CTRL SYSTICK_BASE[0]
#define SYSTICK_LOAD SYSTICK_BASE[1]
#define SYSTICK_VAL  SYSTICK_BASE[2]

/* SCB ICSR for PendSV trigger */
#define SCB_ICSR     (*(volatile uint32_t *)0xE000ED04)
#define PENDSVSET    (1u << 28)

/* ================================================================
 * Task Control Block
 * ================================================================ */
typedef struct {
    uint32_t *sp;
    uint32_t  id;
} tcb_t;

#define NUM_TASKS 2
#define TASK_STACK_SIZE 512

tcb_t task_table[NUM_TASKS];
uint32_t task_stacks[NUM_TASKS][TASK_STACK_SIZE];

/* 0 = first PendSV hasn't fired yet (skip save) */
volatile uint32_t sched_started;

/* Index of currently running task (0 or 1) */
volatile uint32_t current_task;

/* PendSV_Handler is defined in startup.s */
extern void PendSV_Handler(void);

/* ================================================================
 * SVC test globals + handler
 * SVC #imm triggers EXC 11. Handler reads imm from stacked PC-2
 * (works from both MSP and PSP via EXC_RETURN bit 2) and counts.
 * Naked: no C prologue, otherwise push/pop corrupts the HW frame
 * that cpu_exc_return() must pop (same reason as SysTick_Handler).
 * ================================================================ */
volatile uint32_t svc_count;
volatile uint32_t svc_last_num;

__attribute__((naked))
void SVC_Handler(void)
{
    __asm volatile (
        ".syntax unified\n\t"
        "mov r0, lr\n\t"
        "movs r1, #4\n\t"
        "tst r0, r1\n\t"
        "bne 1f\n\t"
        "mrs r0, msp\n\t"
        "b 2f\n\t"
        "1:\n\t"
        "mrs r0, psp\n\t"
        "2:\n\t"
        "ldr r1, [r0, #24]\n\t"  /* stacked PC */
        "subs r1, r1, #2\n\t"    /* SVC insn address */
        "ldrb r1, [r1]\n\t"      /* imm8 */
        "ldr r2, =svc_last_num\n\t"
        "str r1, [r2]\n\t"
        "ldr r2, =svc_count\n\t"
        "ldr r3, [r2]\n\t"
        "adds r3, r3, #1\n\t"
        "str r3, [r2]\n\t"
        "bx lr\n\t"
    );
}

/* ================================================================
 * SysTick Handler - triggers PendSV for context switch
 * ================================================================ */
__attribute__((naked))
void SysTick_Handler(void)
{
    __asm volatile (
        "ldr r0, =0xe000ed04\n\t"
        "ldr r1, =0x10000000\n\t"
        "str r1, [r0]\n\t"
        "bx lr\n\t"
    );
}

/* ================================================================
 * Task A
 * ================================================================ */
void task_a(void)
{
    int count = 0;
    while (1) {
        serial_putstr("Task A: ");
        char buf[16];
        int n = count++;
        int i = 0;
        if (n == 0) buf[i++] = '0';
        else {
            char tmp[16];
            int j = 0;
            while (n > 0) { tmp[j++] = '0' + (n % 10); n /= 10; }
            while (j > 0) buf[i++] = tmp[--j];
        }
        buf[i] = '\n';
        buf[i + 1] = '\0';
        serial_putstr(buf);

        for (volatile int d = 0; d < 200000; d++);
    }
}

/* ================================================================
 * Task B
 * ================================================================ */
void task_b(void)
{
    int count = 0;
    while (1) {
        serial_putstr("  Task B: ");
        char buf[16];
        int n = count++;
        int i = 0;
        if (n == 0) buf[i++] = '0';
        else {
            char tmp[16];
            int j = 0;
            while (n > 0) { tmp[j++] = '0' + (n % 10); n /= 10; }
            while (j > 0) buf[i++] = tmp[--j];
        }
        buf[i] = '\n';
        buf[i + 1] = '\0';
        serial_putstr(buf);

        for (volatile int d = 0; d < 200000; d++);
    }
}

/* ================================================================
 * Task initialization: build exception frame on stack
 * ================================================================ */
void task_create(int id, void (*entry)(void))
{
    /* Stack grows down: reserve top 64 bytes (16 words) for
       R4-R11 save area (8 words) + exception frame (8 words). */
    uint32_t *sp = &task_stacks[id][TASK_STACK_SIZE - 16];
    uint32_t *exc = &task_stacks[id][TASK_STACK_SIZE - 8];

    sp[0] = 0; sp[1] = 0; sp[2] = 0; sp[3] = 0;
    sp[4] = 0; sp[5] = 0; sp[6] = 0; sp[7] = 0;

    exc[0] = 0; /* R0 */
    exc[1] = 0; /* R1 */
    exc[2] = 0; /* R2 */
    exc[3] = 0; /* R3 */
    exc[4] = 0; /* R12 */
    exc[5] = 0; /* LR (stacked, overwritten on return) */
    exc[6] = (uint32_t)entry; /* PC */
    exc[7] = 0x01000000; /* xPSR (Thumb bit) */

    /* Store sp and id using volatile to prevent compiler reordering */
    ((volatile tcb_t *)task_table)[id].sp = sp;
    ((volatile tcb_t *)task_table)[id].id = (uint32_t)id;
}

/* ================================================================
 * Main — runs entirely on MSP (SPSEL=0).
 *
 * WFI loop on MSP. First PendSV skips save (sched_started==0),
 * then loads task 0's context and returns via PSP.
 * ================================================================ */
int main(void)
{
    serial_init();

    serial_putstr("\n=== ARMv6-M Context Switch Demo ===\n");

    /* ---- SVC test (runs on MSP, before scheduler starts) ---- */
    serial_putstr("SVC test: svc 0,1,0...\n");
    svc_count = 0;
    svc_last_num = 0xFFFFFFFFu;
    __asm volatile ("svc 0");
    __asm volatile ("svc 1");
    __asm volatile ("svc 0");
    if (svc_count == 3 && svc_last_num == 0)
        serial_putstr("SVC test PASS (count=3, last=0)\n");
    else
        serial_putstr("SVC test FAIL\n");

    task_create(0, task_a);
    task_create(1, task_b);

    serial_putstr("task_create done\n");

    sched_started = 0;

    /* SysTick: ~1ms tick (48MHz / 1000 = 48000, minus 1) */
    SYSTICK_LOAD = 47999;
    SYSTICK_VAL  = 0;
    SYSTICK_CTRL = (1 << 2) | (1 << 1) | (1 << 0);

    serial_putstr("SysTick configured. Starting tasks...\n\n");

    /* Enable interrupts */
    __asm volatile("cpsie i");

    /* Infinite loop — SysTick preempts and triggers context switches */
    while (1) {
        __asm volatile("wfi");
    }

    return 0;
}

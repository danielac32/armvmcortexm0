    .syntax unified
    .cpu    cortex-m0
    .thumb

/* ================================================================
 * Vector table (placed at start of FLASH by linker script)
 * ================================================================ */
    .section .isr_vector, "a"
    .align 2

    .globl __isr_vector
__isr_vector:
    .word   _estack             /* 0x00: Initial Stack Pointer */
    .word   Reset_Handler       /* 0x04: Reset */
    .word   NMI_Handler         /* 0x08: NMI */
    .word   HardFault_Handler   /* 0x0C: Hard Fault */
    .word   0                   /* 0x10: Reserved */
    .word   0                   /* 0x14: Reserved */
    .word   0                   /* 0x18: Reserved */
    .word   0                   /* 0x1C: Reserved */
    .word   0                   /* 0x20: Reserved */
    .word   0                   /* 0x24: Reserved */
    .word   0                   /* 0x28: Reserved */
    .word   SVC_Handler         /* 0x2C: SVCall */
    .word   0                   /* 0x30: Reserved */
    .word   0                   /* 0x34: Reserved */
    .word   PendSV_Handler      /* 0x38: PendSV */
    .word   SysTick_Handler     /* 0x3C: SysTick */

    /* External interrupts (IRQ 0-31) */
    .word   0   /* IRQ 0  */
    .word   0   /* IRQ 1  */
    .word   0   /* IRQ 2  */
    .word   0   /* IRQ 3  */
    .word   0   /* IRQ 4  */
    .word   0   /* IRQ 5  */
    .word   0   /* IRQ 6  */
    .word   0   /* IRQ 7  */
    .word   0   /* IRQ 8  */
    .word   0   /* IRQ 9  */
    .word   0   /* IRQ 10 */
    .word   0   /* IRQ 11 */
    .word   0   /* IRQ 12 */
    .word   0   /* IRQ 13 */
    .word   0   /* IRQ 14 */
    .word   0   /* IRQ 15 */
    .word   0   /* IRQ 16 */
    .word   0   /* IRQ 17 */
    .word   0   /* IRQ 18 */
    .word   0   /* IRQ 19 */
    .word   0   /* IRQ 20 */
    .word   0   /* IRQ 21 */
    .word   0   /* IRQ 22 */
    .word   0   /* IRQ 23 */
    .word   0   /* IRQ 24 */
    .word   0   /* IRQ 25 */
    .word   0   /* IRQ 26 */
    .word   0   /* IRQ 27 */
    .word   0   /* IRQ 28 */
    .word   0   /* IRQ 29 */
    .word   0   /* IRQ 30 */
    .word   0   /* IRQ 31 */

/* ================================================================
 * Reset Handler
 * ================================================================ */
    .section .text
    .thumb_func
    .globl Reset_Handler
    .type  Reset_Handler, %function
Reset_Handler:
    /* Copy .data from FLASH to SRAM */
    ldr     r0, =_sdata
    ldr     r1, =_edata
    ldr     r2, =_etext
.Lcopy_data:
    cmp     r0, r1
    bge     .Lzero_bss
    ldr     r3, [r2]
    str     r3, [r0]
    adds    r0, r0, #4
    adds    r2, r2, #4
    b       .Lcopy_data

.Lzero_bss:
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
.Lzero_loop:
    cmp     r0, r1
    bge     .Lmain
    str     r2, [r0]
    adds    r0, r0, #4
    b       .Lzero_loop

.Lmain:
    bl      main
    b       .   /* Infinite loop if main returns */

/* ================================================================
 * PendSV Handler - Context Switch
 * Saves/restores R4-R11. The simulator handles the exception
 * frame (xPSR, PC, LR, R12, R0-R3) automatically.
 * ================================================================ */
    .thumb_func
    .globl PendSV_Handler
    .type  PendSV_Handler, %function
PendSV_Handler:
    /* ---- Check if this is the first context switch ---- */
    ldr     r1, =sched_started
    ldr     r1, [r1]            /* R1 = sched_started (0 = first time) */
    cmp     r1, #0
    beq     .Lfirst_switch      /* Skip save on first PendSV */

    /* ---- Save context of current task ---- */
    ldr     r1, =current_task
    ldr     r1, [r1]            /* R1 = current task index */

    mrs     r0, psp             /* R0 = PSP (= task_table[id].sp + 32) */
    subs    r0, r0, #32         /* R0 = task_table[id].sp (base) */
    stmia   r0!, {r4-r7}        /* Save R4-R7 at base[0..3] */
    mov     r4, r8              /* Move R8-R11 to R4-R7 */
    mov     r5, r9
    mov     r6, r10
    mov     r7, r11
    stmia   r0!, {r4-r7}        /* Save R8-R11 at base[4..7] */
    subs    r0, r0, #32         /* R0 = base */

    /* Save SP to current task's TCB */
    ldr     r2, =task_table
    lsls    r3, r1, #3          /* R3 = index * 8 (sizeof tcb_t) */
    str     r0, [r2, r3]        /* task_table[idx].sp = R0 (base) */

    /* ---- Switch to next task ---- */
    adds    r1, r1, #1
    movs    r2, #2
    cmp     r1, r2
    blt     1f
    movs    r1, #0              /* Wrap: (idx+1) % NUM_TASKS */
1:
    b       .Lload_task

.Lfirst_switch:
    /* Mark scheduler as running */
    ldr     r1, =sched_started
    movs    r2, #1
    str     r2, [r1]            /* sched_started = 1 */
    /* First task is 0 */
    movs    r1, #0

.Lload_task:
    ldr     r2, =current_task
    str     r1, [r2]            /* current_task = next */

    /* ---- Load context of new task ---- */
    ldr     r2, =task_table
    lsls    r3, r1, #3          /* R3 = index * 8 (sizeof tcb_t) */
    adds    r3, r3, r2          /* r3 = &task_table[idx] */
    ldr     r0, [r3]            /* R0 = task_table[next].sp (base) */

    /* Restore R4-R11 (load R8-R11 first to avoid clobbering R4-R7) */
    adds    r0, r0, #16         /* R0 = base + 16 (R8-R11 save area) */
    ldmia   r0!, {r4-r7}        /* Load saved R8-R11 into r4-r7 */
    mov     r8, r4              /* Restore R8-R11 */
    mov     r9, r5
    mov     r10, r6
    mov     r11, r7
    subs    r0, r0, #32         /* R0 = base */
    ldmia   r0!, {r4-r7}        /* Restore R4-R7 from base[0..3] */
    adds    r0, r0, #16         /* R0 = base + 32 = PSP */
    /* r0 = base + 32 = PSP */

    msr     psp, r0             /* Update PSP to exception frame */

    /* EXC_RETURN in LR = 0xFFFFFFFD (return to Thread, use PSP).
       0 - 3 wraps to 0xFFFFFFFD in Thumb-16, no stack access needed. */
    movs    r0, #0
    subs    r0, #3              @ R0 = 0xFFFFFFFD
    mov     lr, r0              @ Set LR register to EXC_RETURN
    bx      lr                   @ Return from exception via PSP

/* ================================================================
 * Default handler (infinite loop)
 * ================================================================ */
    .thumb_func
    .globl Default_Handler
Default_Handler:
    b       .

/* ================================================================
 * Weak aliases for exception handlers
 * User can override by defining the symbol in their code
 * ================================================================ */
    .weak   NMI_Handler
    .thumb_set NMI_Handler, Default_Handler

    .weak   HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler

    .weak   SVC_Handler
    .thumb_set SVC_Handler, Default_Handler

    .weak   SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

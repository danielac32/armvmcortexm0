#ifndef ARMV6M_H
#define ARMV6M_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ================================================================
 * Registers
 * ================================================================ */
#define REG_R0   0
#define REG_R1   1
#define REG_R2   2
#define REG_R3   3
#define REG_R4   4
#define REG_R5   5
#define REG_R6   6
#define REG_R7   7
#define REG_R8   8
#define REG_R9   9
#define REG_R10  10
#define REG_R11  11
#define REG_R12  12
#define REG_SP   13
#define REG_LR   14
#define REG_PC   15
#define NUM_REGS 16

/* ================================================================
 * APSR Flags
 * ================================================================ */
#define APSR_N  (1u << 31)
#define APSR_Z  (1u << 30)
#define APSR_C  (1u << 29)
#define APSR_V  (1u << 28)
#define ALL_FLAGS (APSR_N | APSR_Z | APSR_C | APSR_V)
#define FLAGS_NZC (APSR_N | APSR_Z | APSR_C)

/* ================================================================
 * Special register bits
 * ================================================================ */
#define PRIMASK_PM   (1u << 0)

#define CONTROL_NPRIV (1u << 0)
#define CONTROL_SPSEL (1u << 1)
#define CONTROL_MASK  (CONTROL_NPRIV | CONTROL_SPSEL)

#define EXC_RETURN   0xFFFFFFE0u

/* ================================================================
 * CPU Mode
 * ================================================================ */
#define MODE_THREAD  0
#define MODE_HANDLER 1

/* ================================================================
 * Exception numbers
 * ================================================================ */
#define EXC_RESET          1
#define EXC_NMI            2
#define EXC_HARDFAULT      3
#define EXC_SVCALL        11
#define EXC_PENDSV        14
#define EXC_SYSTICK       15

/* ================================================================
 * Trace flags
 * ================================================================ */
#define TRACE_NONE    0
#define TRACE_FETCH   (1u << 0)
#define TRACE_INST    (1u << 1)
#define TRACE_REG     (1u << 2)
#define TRACE_MEM     (1u << 3)
#define TRACE_FLAGS   (1u << 4)

/* ================================================================
 * Memory regions
 * ================================================================ */
#define MAX_MEM_REGIONS 16

typedef enum {
    MEM_RAM,
    MEM_FLASH,
    MEM_MMIO_SYPICK,
    MEM_MMIO_NVIC,
    MEM_MMIO_SCB,
    MEM_MMIO_UART,
    MEM_MMIO_PENDSV,
    MEM_MMIO_REG,
} mem_type_t;

typedef struct {
    uint32_t   base;
    uint32_t   size;
    mem_type_t type;
    uint8_t   *data;
} mem_region_t;

/* ================================================================
 * Instruction groups (decode)
 * ================================================================ */
#define IGRP0 0
#define IGRP1 1
#define IGRP2 2
#define IGRP3 3
#define IGRP4 4
#define IGRP5 5
#define IGRP6 6
#define IGRP7 7
#define IGRP8 8

/* ================================================================
 * Instruction masks and opcodes
 * ================================================================ */
#define IGRP0_MASK  0xF000
#define IGRP1_MASK  0xF800
#define IGRP2_MASK  0xFE00
#define IGRP3_MASK  0xFF00
#define IGRP4_MASK  0xFF80
#define IGRP5_MASK  0xFFC0
#define IGRP6_MASK  0xFFE0
#define IGRP7_MASK  0xFFF0
#define IGRP8_MASK  0xFFFF

#define INST_ADCS_OPCODE       0x4140
#define INST_ADDS_OPCODE       0x1C00
#define INST_ADDS_1_OPCODE     0x3000
#define INST_ADDS_2_OPCODE     0x1800
#define INST_ADD_OPCODE        0x4400
#define INST_ADD_1_OPCODE      0xA800
#define INST_ADD_2_OPCODE      0xB000
#define INST_ADR_OPCODE        0xA000
#define INST_ANDS_OPCODE       0x4000
#define INST_ASRS_OPCODE       0x1000
#define INST_ASRS_1_OPCODE     0x4100
#define INST_BCC_OPCODE        0xD000
#define INST_B_OPCODE          0xE000
#define INST_BICS_OPCODE       0x4380
#define INST_BKPT_OPCODE       0xBE00
#define INST_BL_OPCODE         0xF000
#define INST_BLX_OPCODE        0x4780
#define INST_BX_OPCODE         0x4700
#define INST_CMN_OPCODE        0x42C0
#define INST_CMP_OPCODE        0x2800
#define INST_CMP_1_OPCODE      0x4280
#define INST_CMP_2_OPCODE      0x4500
#define INST_CPS_OPCODE        0xB660
#define INST_ISB_OPCODE        0xF3B0
#define INST_EORS_OPCODE       0x4040
#define INST_LDM_OPCODE        0xC800
#define INST_LDR_OPCODE        0x6800
#define INST_LDR_1_OPCODE      0x9800
#define INST_LDR_2_OPCODE      0x4800
#define INST_LDR_3_OPCODE      0x5800
#define INST_LDRB_OPCODE       0x7800
#define INST_LDRB_1_OPCODE     0x5C00
#define INST_LDRH_OPCODE       0x8800
#define INST_LDRH_1_OPCODE     0x5A00
#define INST_LDRSB_OPCODE      0x5600
#define INST_LDRSH_OPCODE      0x5E00
#define INST_LSLS_OPCODE       0x0000
#define INST_LSLS_1_OPCODE     0x4080
#define INST_LSRS_OPCODE       0x0800
#define INST_LSRS_1_OPCODE     0x40C0
#define INST_MOVS_OPCODE       0x2000
#define INST_MOV_OPCODE        0x4600
#define INST_MRS_OPCODE        0xF3E0
#define INST_MSR_OPCODE        0xF380
#define INST_MULS_OPCODE       0x4340
#define INST_MVNS_OPCODE       0x43C0
#define INST_NOP_OPCODE        0xBF00
#define INST_ORRS_OPCODE       0x4300
#define INST_POP_OPCODE        0xBC00
#define INST_PUSH_OPCODE       0xB400
#define INST_REV_OPCODE        0xBA00
#define INST_REV16_OPCODE      0xBA40
#define INST_REVSH_OPCODE      0xBAC0
#define INST_RORS_OPCODE       0x41C0
#define INST_RSBS_OPCODE       0x4240
#define INST_SBCS_OPCODE       0x4180
#define INST_SEV_OPCODE        0xBF40
#define INST_STM_OPCODE        0xC000
#define INST_STR_OPCODE        0x6000
#define INST_STR_1_OPCODE      0x9000
#define INST_STR_2_OPCODE      0x5000
#define INST_STRB_OPCODE       0x7000
#define INST_STRB_1_OPCODE     0x5400
#define INST_STRH_OPCODE       0x8000
#define INST_STRH_1_OPCODE     0x5200
#define INST_SUBS_OPCODE       0x1E00
#define INST_SUBS_1_OPCODE     0x3800
#define INST_SUBS_2_OPCODE     0x1A00
#define INST_SUB_OPCODE        0xB080
#define INST_SVC_OPCODE        0xDF00
#define INST_SXTB_OPCODE       0xB240
#define INST_SXTH_OPCODE       0xB200
#define INST_TST_OPCODE        0x4200
#define INST_UDF_OPCODE        0xDE00
#define INST_UDF_W_OPCODE      0xF7F0
#define INST_UXTB_OPCODE       0xB2C0
#define INST_UXTH_OPCODE       0xB280
#define INST_WFE_OPCODE        0xBF20
#define INST_WFI_OPCODE        0xBF30
#define INST_YIELD_OPCODE      0xBF10

/* ================================================================
 * NVIC (matches ARM CMSIS NVIC_Type for Cortex-M0)
 * ================================================================ */
typedef struct {
    volatile uint32_t ISER[8];
    volatile uint32_t _pad0[24];
    volatile uint32_t ICER[8];
    volatile uint32_t _pad1[24];
    volatile uint32_t ISPR[8];
    volatile uint32_t _pad2[24];
    volatile uint32_t ICPR[8];
    volatile uint32_t _pad3[24];
    volatile uint32_t IABR[8];
    volatile uint32_t _pad4[56];
    volatile uint32_t IP[64];
} nvic_t;

/* ================================================================
 * SysTick (matches ARM CMSIS SysTick_Type)
 * ================================================================ */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} systick_t;

#define SYSTICK_CTRL_COUNTFLAG (1u << 16)
#define SYSTICK_CTRL_CLKSRC    (1u << 2)
#define SYSTICK_CTRL_TICKINT   (1u << 1)
#define SYSTICK_CTRL_ENABLE    (1u << 0)

/* ================================================================
 * SCB (simplified, matches ARM CMSIS SCB_Type)
 * ================================================================ */
typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHP[3];
    volatile uint32_t SHCSR;
} scb_t;

#define SCB_ICSR_PENDSVSET    (1u << 28)
#define SCB_ICSR_PENDSVCLR    (1u << 27)
#define SCB_ICSR_PENDSTSET    (1u << 26)
#define SCB_ICSR_PENDSTCLR    (1u << 25)

/* ================================================================
 * CPU state
 * ================================================================ */
typedef struct {
    /* Registers */
    uint32_t r[NUM_REGS];
    uint32_t apsr;
    uint32_t ipsr;
    uint32_t epsr;
    uint32_t primask;
    uint32_t control;
    int      mode;
    uint32_t msp;
    uint32_t psp;

    /* Decode fields (set by decode, used by execute) */
    int      igroup;
    uint32_t rd, rt, rm, rn, imm, cond, reglist;

    /* Peripherals (memory-mapped) */
    nvic_t    nvic;
    systick_t systick;
    scb_t     scb;
    uint32_t  uart_dr;
    uint32_t  mmio_val;

    /* Memory */
    mem_region_t regions[MAX_MEM_REGIONS];
    int          n_regions;

    /* Status */
    int      fault;
    int      trace;
    uint32_t entry_point;
    uint64_t cycle_count;
} armv6m_t;

/* ================================================================
 * Memory access
 * ================================================================ */
void     mem_init(armv6m_t *cpu);
bool     mem_create_region(armv6m_t *cpu, uint32_t base, uint32_t size, mem_type_t type);
bool     mem_valid_addr(armv6m_t *cpu, uint32_t addr);
uint8_t  mem_read8(armv6m_t *cpu, uint32_t addr);
void     mem_write8(armv6m_t *cpu, uint32_t addr, uint8_t data);
uint32_t mem_read32(armv6m_t *cpu, uint32_t addr);
void     mem_write32(armv6m_t *cpu, uint32_t addr, uint32_t data);
uint32_t mem_read_width(armv6m_t *cpu, uint32_t addr, int width);
void     mem_write_width(armv6m_t *cpu, uint32_t addr, uint32_t data, int width);

/* ================================================================
 * CPU core
 * ================================================================ */
void     cpu_init(armv6m_t *cpu);
void     cpu_reset(armv6m_t *cpu, uint32_t start_addr);
void     cpu_step(armv6m_t *cpu);
uint16_t cpu_fetch_inst(armv6m_t *cpu, uint32_t addr);

/* ================================================================
 * Decode / Execute
 * ================================================================ */
int  armv6m_decode(armv6m_t *cpu, uint16_t inst);
void armv6m_execute(armv6m_t *cpu, uint16_t inst, uint16_t inst2);

/* ================================================================
 * Exception handling
 * ================================================================ */
uint32_t cpu_exception(armv6m_t *cpu, uint32_t pc, uint32_t exc_num);
void     cpu_exc_return(armv6m_t *cpu, uint32_t pc);

/* ================================================================
 * Utilities
 * ================================================================ */
void     armv6m_update_sp(armv6m_t *cpu, uint32_t sp);
void     armv6m_update_nz(armv6m_t *cpu, uint32_t val);
uint32_t armv6m_add_with_carry(armv6m_t *cpu, uint32_t rn, uint32_t rm, uint32_t carry_in, uint32_t mask);
uint32_t armv6m_shift_lsl(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask);
uint32_t armv6m_shift_lsr(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask);
uint32_t armv6m_shift_asr(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask);
uint32_t armv6m_shift_ror(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask);
uint32_t armv6m_sign_extend(uint32_t val, int bits);

/* ================================================================
 * ELF loader
 * ================================================================ */
int elf_load(const char *filename, armv6m_t *cpu, uint32_t *start_addr);

#endif /* ARMV6M_H */

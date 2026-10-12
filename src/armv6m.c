#include "armv6m.h"
#include <stdio.h>
#include <unistd.h>

/* Forward declarations for 16-bit helpers */
static uint16_t mem_read16_raw(armv6m_t *cpu, uint32_t addr);
static void     mem_write16_raw(armv6m_t *cpu, uint32_t addr, uint16_t data);

/* ================================================================
 * Memory subsystem
 * ================================================================ */
void mem_init(armv6m_t *cpu)
{
    cpu->n_regions = 0;
}

bool mem_create_region(armv6m_t *cpu, uint32_t base, uint32_t size, mem_type_t type)
{
    if (cpu->n_regions >= MAX_MEM_REGIONS)
        return false;

    mem_region_t *r = &cpu->regions[cpu->n_regions];
    r->base = base;
    r->size = size;
    r->type = type;

    if (type == MEM_RAM || type == MEM_FLASH) {
        r->data = (uint8_t *)calloc(1, size);
        if (!r->data)
            return false;
    } else {
        r->data = NULL;
    }

    cpu->n_regions++;
    return true;
}

bool mem_valid_addr(armv6m_t *cpu, uint32_t addr)
{
    for (int i = 0; i < cpu->n_regions; i++) {
        mem_region_t *r = &cpu->regions[i];
        if (addr >= r->base && addr < r->base + r->size)
            return true;
    }
    return false;
}

static mem_region_t *find_region(armv6m_t *cpu, uint32_t addr)
{
    for (int i = 0; i < cpu->n_regions; i++) {
        mem_region_t *r = &cpu->regions[i];
        if (addr >= r->base && addr < r->base + r->size)
            return r;
    }
    return NULL;
}

uint8_t mem_read8(armv6m_t *cpu, uint32_t addr)
{
    mem_region_t *r = find_region(cpu, addr);
    if (!r) return 0;

    uint32_t offset = addr - r->base;

    switch (r->type) {
    case MEM_RAM:
    case MEM_FLASH:
        return r->data[offset];

    case MEM_MMIO_UART:
        return 0;

    case MEM_MMIO_SYPICK:
        switch (offset) {
        case 0x00: return (uint8_t)cpu->systick.CTRL;
        case 0x04: return (uint8_t)cpu->systick.LOAD;
        case 0x08: return (uint8_t)cpu->systick.VAL;
        case 0x0C: return (uint8_t)cpu->systick.CALIB;
        }
        return 0;

    case MEM_MMIO_NVIC:
        return 0;

    case MEM_MMIO_SCB:
        switch (offset) {
        case 0x00: return 0x41;
        case 0x04: return (uint8_t)cpu->scb.ICSR;
        }
        return 0;

    case MEM_MMIO_PENDSV:
        return (cpu->scb.ICSR & SCB_ICSR_PENDSVSET) ? 1 : 0;

    case MEM_MMIO_REG:
        return (uint8_t)cpu->mmio_val;

    default:
        return 0;
    }
}

void mem_write8(armv6m_t *cpu, uint32_t addr, uint8_t data)
{
    mem_region_t *r = find_region(cpu, addr);
    if (!r) return;

    uint32_t offset = addr - r->base;

    switch (r->type) {
    case MEM_RAM:
    case MEM_FLASH:
        r->data[offset] = data;
        return;

    case MEM_MMIO_UART:
        { char c = (char)data; fprintf(stderr, "%c", c); fflush(stderr); }
        return;

    case MEM_MMIO_SYPICK:
        switch (offset) {
        case 0x00: cpu->systick.CTRL = data; return;
        case 0x04: cpu->systick.LOAD = data; return;
        case 0x08: cpu->systick.VAL  = data; return;
        }
        return;

    case MEM_MMIO_SCB:
        switch (offset) {
        case 0x04:
            if (data & SCB_ICSR_PENDSVSET)
                cpu->scb.ICSR |= SCB_ICSR_PENDSVSET;
            else if (data & SCB_ICSR_PENDSVCLR)
                cpu->scb.ICSR &= ~SCB_ICSR_PENDSVSET;
            return;
        }
        return;

    case MEM_MMIO_PENDSV:
        if (data & 0x01)
            cpu->scb.ICSR |= SCB_ICSR_PENDSVSET;
        return;

    case MEM_MMIO_REG:
        cpu->mmio_val = data;
        return;

    default:
        return;
    }
}

static uint16_t mem_read16_raw(armv6m_t *cpu, uint32_t addr)
{
    uint8_t lo = mem_read8(cpu, addr);
    uint8_t hi = mem_read8(cpu, addr + 1);
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

static void mem_write16_raw(armv6m_t *cpu, uint32_t addr, uint16_t data)
{
    mem_write8(cpu, addr, (uint8_t)(data));
    mem_write8(cpu, addr + 1, (uint8_t)(data >> 8));
}

uint32_t mem_read32(armv6m_t *cpu, uint32_t addr)
{
    addr &= ~3u;
    mem_region_t *r = find_region(cpu, addr);
    if (!r) return 0;

    uint32_t offset = addr - r->base;

    switch (r->type) {
    case MEM_RAM:
    case MEM_FLASH:
        return (uint32_t)r->data[offset]
             | ((uint32_t)r->data[offset + 1] << 8)
             | ((uint32_t)r->data[offset + 2] << 16)
             | ((uint32_t)r->data[offset + 3] << 24);

    case MEM_MMIO_SYPICK:
        switch (offset) {
        case 0x00: {
            uint32_t v = cpu->systick.CTRL;
            cpu->systick.CTRL &= ~SYSTICK_CTRL_COUNTFLAG;
            return v;
        }
        case 0x04: return cpu->systick.LOAD;
        case 0x08: return cpu->systick.VAL;
        case 0x0C: return cpu->systick.CALIB;
        }
        return 0;

    case MEM_MMIO_NVIC: {
        uint32_t reg = offset & 0xFF;
        if (reg < 32)
            return cpu->nvic.ISER[reg / 4];
        return 0;
    }

    case MEM_MMIO_SCB:
        switch (offset) {
        case 0x00: return 0x410FC241;
        case 0x04: return cpu->scb.ICSR;
        case 0x08: return cpu->scb.VTOR;
        }
        return 0;

    case MEM_MMIO_PENDSV:
        return (cpu->scb.ICSR & SCB_ICSR_PENDSVSET) ? 1 : 0;

    case MEM_MMIO_REG:
        return cpu->mmio_val;

    default:
        return 0;
    }
}

void mem_write32(armv6m_t *cpu, uint32_t addr, uint32_t data)
{
    addr &= ~3u;
    mem_region_t *r = find_region(cpu, addr);
    if (!r) return;

    uint32_t offset = addr - r->base;

    switch (r->type) {
    case MEM_RAM:
    case MEM_FLASH:
        r->data[offset]     = (uint8_t)(data);
        r->data[offset + 1] = (uint8_t)(data >> 8);
        r->data[offset + 2] = (uint8_t)(data >> 16);
        r->data[offset + 3] = (uint8_t)(data >> 24);
        return;

    case MEM_MMIO_UART:
        { char c = (char)data; fprintf(stderr, "%c", c); fflush(stderr); }
        return;

    case MEM_MMIO_SYPICK:
        switch (offset) {
        case 0x00: cpu->systick.CTRL = data; return;
        case 0x04: cpu->systick.LOAD = data; return;
        case 0x08: cpu->systick.VAL  = data; return;
        }
        return;

    case MEM_MMIO_NVIC: {
        uint32_t reg = offset & 0xFF;
        if (reg < 32)
            cpu->nvic.ISER[reg / 4] = data;
        return;
    }

    case MEM_MMIO_SCB:
        switch (offset) {
        case 0x04:
            if (data & SCB_ICSR_PENDSVSET)
                cpu->scb.ICSR |= SCB_ICSR_PENDSVSET;
            else if (data & SCB_ICSR_PENDSVCLR)
                cpu->scb.ICSR &= ~SCB_ICSR_PENDSVSET;
            return;
        }
        return;

    case MEM_MMIO_PENDSV:
        if (data & 0x01)
            cpu->scb.ICSR |= SCB_ICSR_PENDSVSET;
        return;

    case MEM_MMIO_REG:
        cpu->mmio_val = data;
        return;

    default:
        return;
    }
}

uint32_t mem_read_width(armv6m_t *cpu, uint32_t addr, int width)
{
    switch (width) {
    case 4: return mem_read32(cpu, addr);
    case 2: return mem_read16_raw(cpu, addr);
    case 1: return mem_read8(cpu, addr);
    default: return 0;
    }
}

void mem_write_width(armv6m_t *cpu, uint32_t addr, uint32_t data, int width)
{
    switch (width) {
    case 4: mem_write32(cpu, addr, data); break;
    case 2: mem_write16_raw(cpu, addr, (uint16_t)data); break;
    case 1: mem_write8(cpu, addr, (uint8_t)data); break;
    }
}

/* ================================================================
 * Utilities
 * ================================================================ */
void armv6m_update_sp(armv6m_t *cpu, uint32_t sp)
{
    cpu->r[REG_SP] = sp;
    if ((cpu->control & CONTROL_SPSEL) && (cpu->mode == MODE_THREAD))
        cpu->psp = sp;
    else
        cpu->msp = sp;
}

void armv6m_update_nz(armv6m_t *cpu, uint32_t val)
{
    if (val == 0)
        cpu->apsr |= APSR_Z;
    else
        cpu->apsr &= ~APSR_Z;

    if (val & 0x80000000)
        cpu->apsr |= APSR_N;
    else
        cpu->apsr &= ~APSR_N;
}

uint32_t armv6m_add_with_carry(armv6m_t *cpu, uint32_t rn, uint32_t rm,
                                uint32_t carry_in, uint32_t mask)
{
    uint64_t uresult = (uint64_t)rn + (uint64_t)rm + carry_in;
    uint32_t result  = (uint32_t)uresult;

    if (mask & APSR_Z) {
        cpu->apsr = (result == 0) ? (cpu->apsr | APSR_Z) : (cpu->apsr & ~APSR_Z);
    }
    if (mask & APSR_N) {
        cpu->apsr = (result & 0x80000000) ? (cpu->apsr | APSR_N) : (cpu->apsr & ~APSR_N);
    }
    if (mask & APSR_C) {
        cpu->apsr = (uresult != (uint64_t)result) ? (cpu->apsr | APSR_C) : (cpu->apsr & ~APSR_C);
    }
    if (mask & APSR_V) {
        int64_t sresult = (int64_t)(int32_t)rn + (int64_t)(int32_t)rm + (int)carry_in;
        cpu->apsr = (sresult != (int64_t)(int32_t)result) ? (cpu->apsr | APSR_V) : (cpu->apsr & ~APSR_V);
    }

    return result;
}

uint32_t armv6m_shift_lsl(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask)
{
    uint64_t res = (uint64_t)val << shift;
    if (mask & APSR_C) {
        cpu->apsr = (res & ((uint64_t)1 << 32)) ? (cpu->apsr | APSR_C) : (cpu->apsr & ~APSR_C);
    }
    armv6m_update_nz(cpu, (uint32_t)res);
    return (uint32_t)res;
}

uint32_t armv6m_shift_lsr(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask)
{
    uint32_t res = (shift >= 32) ? 0 : (val >> shift);
    if ((mask & APSR_C) && (shift > 0) && (shift <= 32)) {
        cpu->apsr = (val & (1u << (shift - 1))) ? (cpu->apsr | APSR_C) : (cpu->apsr & ~APSR_C);
    }
    armv6m_update_nz(cpu, res);
    return res;
}

uint32_t armv6m_shift_asr(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask)
{
    int32_t sval = (int32_t)val;
    uint32_t res = (shift >= 32) ? (sval < 0 ? 0xFFFFFFFF : 0) : (uint32_t)(sval >> shift);
    if ((mask & APSR_C) && (shift > 0) && (shift <= 32)) {
        cpu->apsr = (val & (1u << (shift - 1))) ? (cpu->apsr | APSR_C) : (cpu->apsr & ~APSR_C);
    }
    armv6m_update_nz(cpu, res);
    return res;
}

uint32_t armv6m_shift_ror(armv6m_t *cpu, uint32_t val, uint32_t shift, uint32_t mask)
{
    if (shift == 0) {
        armv6m_update_nz(cpu, val);
        return val;
    }
    shift &= 0x1F;
    uint32_t res = (val >> shift) | (val << (32 - shift));
    if (mask & APSR_C) {
        cpu->apsr = (res & 0x80000000) ? (cpu->apsr | APSR_C) : (cpu->apsr & ~APSR_C);
    }
    armv6m_update_nz(cpu, res);
    return res;
}

uint32_t armv6m_sign_extend(uint32_t val, int bits)
{
    if (val & (1u << (bits - 1)))
        val |= (~0u) << bits;
    else
        val &= ~((~0u) << bits);
    return val;
}

/* ================================================================
 * Instruction fetch
 * ================================================================ */
uint16_t cpu_fetch_inst(armv6m_t *cpu, uint32_t addr)
{
    uint32_t word = mem_read32(cpu, addr & ~3u);
    return (addr & 2) ? (uint16_t)(word >> 16) : (uint16_t)word;
}

/* ================================================================
 * CPU init / reset
 * ================================================================ */
void cpu_init(armv6m_t *cpu)
{
    memset(cpu, 0, sizeof(*cpu));
}

void cpu_reset(armv6m_t *cpu, uint32_t start_addr)
{
    memset(cpu->r, 0, sizeof(cpu->r));
    cpu->apsr = 0;
    cpu->ipsr = 0;
    cpu->epsr = 0;
    cpu->primask = 0;
    cpu->control = 0;
    cpu->mode = MODE_THREAD;
    cpu->fault = 0;
    cpu->cycle_count = 0;
    cpu->entry_point = start_addr;

    cpu->r[REG_SP] = mem_read32(cpu, start_addr);
    cpu->r[REG_LR] = 0;
    cpu->r[REG_PC] = mem_read32(cpu, start_addr + 4) & ~1u;
    cpu->msp = cpu->r[REG_SP];
    cpu->psp = 0;
}

/* ================================================================
 * Exception handling
 * ================================================================ */
uint32_t cpu_exception(armv6m_t *cpu, uint32_t pc, uint32_t exc_num)
{
    uint32_t stack_sp;
    int used_psp = 0;

    if ((cpu->control & CONTROL_SPSEL) && (cpu->mode == MODE_THREAD)) {
        stack_sp = cpu->psp;
        used_psp = 1;
    } else {
        stack_sp = cpu->msp;
        used_psp = 0;
    }

    stack_sp -= 4; mem_write32(cpu, stack_sp, cpu->apsr);
    stack_sp -= 4; mem_write32(cpu, stack_sp, pc);
    stack_sp -= 4; mem_write32(cpu, stack_sp, cpu->r[REG_LR]);
    stack_sp -= 4; mem_write32(cpu, stack_sp, cpu->r[REG_R12]);
    stack_sp -= 4; mem_write32(cpu, stack_sp, cpu->r[REG_R3]);
    stack_sp -= 4; mem_write32(cpu, stack_sp, cpu->r[REG_R2]);
    stack_sp -= 4; mem_write32(cpu, stack_sp, cpu->r[REG_R1]);
    stack_sp -= 4; mem_write32(cpu, stack_sp, cpu->r[REG_R0]);

    if (used_psp)
        cpu->psp = stack_sp;
    else
        cpu->msp = stack_sp;

    cpu->ipsr = exc_num & 0x3F;
    cpu->r[REG_PC] = mem_read32(cpu, cpu->entry_point + (exc_num * 4)) & ~1u;

    if (cpu->mode == MODE_HANDLER)
        cpu->r[REG_LR] = 0xFFFFFFF1;
    else {
        if ((cpu->control & CONTROL_SPSEL) == 0)
            cpu->r[REG_LR] = 0xFFFFFFF9;
        else
            cpu->r[REG_LR] = 0xFFFFFFFD;
    }

    cpu->mode = MODE_HANDLER;
    cpu->control &= ~CONTROL_SPSEL;
    cpu->r[REG_SP] = cpu->msp;

    return cpu->r[REG_PC];
}

void cpu_exc_return(armv6m_t *cpu, uint32_t exc_ret)
{
    if (cpu->mode != MODE_HANDLER)
        return;
    if ((exc_ret & EXC_RETURN) != EXC_RETURN)
        return;

    uint32_t lo = exc_ret & 0xF;
    uint32_t unstack_sp;

    if (lo == 0xD)
        unstack_sp = cpu->psp;
    else
        unstack_sp = cpu->msp;

    uint32_t sp = unstack_sp;

    cpu->r[REG_R0]  = mem_read32(cpu, sp); sp += 4;
    cpu->r[REG_R1]  = mem_read32(cpu, sp); sp += 4;
    cpu->r[REG_R2]  = mem_read32(cpu, sp); sp += 4;
    cpu->r[REG_R3]  = mem_read32(cpu, sp); sp += 4;
    cpu->r[REG_R12] = mem_read32(cpu, sp); sp += 4;
    cpu->r[REG_LR]  = mem_read32(cpu, sp); sp += 4;
    uint32_t ret_pc = mem_read32(cpu, sp); sp += 4;
    cpu->apsr       = mem_read32(cpu, sp); sp += 4;

    switch (lo) {
    case 0x1:
        cpu->mode = MODE_HANDLER;
        cpu->control &= ~CONTROL_SPSEL;
        cpu->msp = sp;
        break;
    case 0x9:
        cpu->mode = MODE_THREAD;
        cpu->control &= ~CONTROL_SPSEL;
        cpu->msp = sp;
        break;
    case 0xD:
        cpu->mode = MODE_THREAD;
        cpu->control |= CONTROL_SPSEL;
        cpu->psp = sp;
        break;
    default:
        return;
    }

    cpu->r[REG_SP] = (lo == 0xD) ? cpu->psp : cpu->msp;
    cpu->r[REG_PC] = ret_pc;
}

/* ================================================================
 * SysTick clock (called once per step)
 * ================================================================ */
static void systick_clock(armv6m_t *cpu)
{
    if (!(cpu->systick.CTRL & SYSTICK_CTRL_ENABLE))
        return;

    if (cpu->systick.VAL == 0) {
        cpu->systick.VAL = cpu->systick.LOAD;
        cpu->systick.CTRL |= SYSTICK_CTRL_COUNTFLAG;
        if (cpu->systick.CTRL & SYSTICK_CTRL_TICKINT)
            cpu->scb.ICSR |= SCB_ICSR_PENDSTSET;
    } else {
        cpu->systick.VAL--;
    }
}

/* ================================================================
 * Step
 * ================================================================ */
void cpu_step(armv6m_t *cpu)
{
    uint16_t inst, inst2;
    int is_32bit;

    if (cpu->trace & TRACE_INST)
        fprintf(stderr, "PC=%08x SP=%08x ", cpu->r[REG_PC], cpu->r[REG_SP]);

    /* Check EXC_RETURN in PC */
    if ((cpu->r[REG_PC] & EXC_RETURN) == EXC_RETURN)
        cpu_exc_return(cpu, cpu->r[REG_PC]);

    /* Fetch */
    inst = cpu_fetch_inst(cpu, cpu->r[REG_PC]);

    /* Decode */
    is_32bit = armv6m_decode(cpu, inst);

    /* 32-bit: fetch second halfword */
    inst2 = is_32bit ? cpu_fetch_inst(cpu, cpu->r[REG_PC] + 2) : 0;

    if (cpu->trace & TRACE_INST) {
        if (is_32bit)
            fprintf(stderr, "  %04x %04x  mode=%d primask=%d\n", inst, inst2, cpu->mode, cpu->primask);
        else
            fprintf(stderr, "  %04x        mode=%d primask=%d\n", inst, cpu->mode, cpu->primask);
    }

    /* Execute */
    armv6m_execute(cpu, inst, inst2);

    if (cpu->trace & TRACE_INST)
        fprintf(stderr, "  -> PC=%08x SP=%08x R0=%08x R1=%08x LR=%08x\n",
                cpu->r[REG_PC], cpu->r[REG_SP], cpu->r[0], cpu->r[1], cpu->r[REG_LR]);

    /* Peripheral clocks */
    systick_clock(cpu);

    /* PendSV exception */
    if ((cpu->scb.ICSR & SCB_ICSR_PENDSVSET) &&
        (cpu->mode == MODE_THREAD) &&
        !(cpu->primask & PRIMASK_PM)) {
        cpu->scb.ICSR &= ~SCB_ICSR_PENDSVSET;
        cpu->r[REG_PC] = cpu_exception(cpu, cpu->r[REG_PC], EXC_PENDSV);
    }

    /* SysTick exception */
    if ((cpu->scb.ICSR & SCB_ICSR_PENDSTSET) &&
        (cpu->mode == MODE_THREAD) &&
        !(cpu->primask & PRIMASK_PM)) {
        cpu->scb.ICSR &= ~SCB_ICSR_PENDSTSET;
        cpu->r[REG_PC] = cpu_exception(cpu, cpu->r[REG_PC], EXC_SYSTICK);
    }

    cpu->cycle_count++;
}

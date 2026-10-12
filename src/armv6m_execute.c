#include "armv6m.h"

void armv6m_execute(armv6m_t *cpu, uint16_t inst, uint16_t inst2)
{
    uint32_t reg_rm = cpu->r[cpu->rm];
    uint32_t reg_rn = cpu->r[cpu->rn];
    uint32_t reg_rd = 0;
    uint32_t pc     = cpu->r[REG_PC];
    uint32_t offset = 0;
    int write_rd    = 0;

    pc += 2; /* Advance PC to next instruction */

    switch (cpu->igroup) {

    /* ============================================================ */
    /* Group 0: Conditional branch                                  */
    /* ============================================================ */
    case IGRP0:
        switch (inst & IGRP0_MASK) {
        case INST_BCC_OPCODE:
            offset = armv6m_sign_extend(cpu->imm, 8) << 1;
            offset += pc + 2;

            switch (cpu->cond) {
            case 0:  if (cpu->apsr & APSR_Z)  pc = offset; break;
            case 1:  if (!(cpu->apsr & APSR_Z)) pc = offset; break;
            case 2:  if (cpu->apsr & APSR_C)  pc = offset; break;
            case 3:  if (!(cpu->apsr & APSR_C)) pc = offset; break;
            case 4:  if (cpu->apsr & APSR_N)  pc = offset; break;
            case 5:  if (!(cpu->apsr & APSR_N)) pc = offset; break;
            case 6:  if (cpu->apsr & APSR_V)  pc = offset; break;
            case 7:  if (!(cpu->apsr & APSR_V)) pc = offset; break;
            case 8:  if ((cpu->apsr & APSR_C) && !(cpu->apsr & APSR_Z)) pc = offset; break;
            case 9:  if (!(cpu->apsr & APSR_C) || (cpu->apsr & APSR_Z)) pc = offset; break;
            case 10: if (((cpu->apsr >> 31) & 1) == ((cpu->apsr >> 28) & 1)) pc = offset; break;
            case 11: if (((cpu->apsr >> 31) & 1) != ((cpu->apsr >> 28) & 1)) pc = offset; break;
            case 12: if (!(cpu->apsr & APSR_Z) && ((cpu->apsr >> 31) & 1) == ((cpu->apsr >> 28) & 1)) pc = offset; break;
            case 13: if ((cpu->apsr & APSR_Z) || ((cpu->apsr >> 31) & 1) != ((cpu->apsr >> 28) & 1)) pc = offset; break;
            case 14: pc = offset; break;
            case 15: pc = cpu_exception(cpu, pc, EXC_SVCALL); break;
            }
            break;
        }
        break;

    /* ============================================================ */
    /* Group 1: Data processing, loads/stores, branches              */
    /* ============================================================ */
    case IGRP1:
        switch (inst & IGRP1_MASK) {
        case INST_ADDS_1_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, cpu->imm, 0, ALL_FLAGS);
            write_rd = 1;
            break;

        case INST_ADD_1_OPCODE:
            reg_rd = reg_rn + (cpu->imm << 2);
            write_rd = 1;
            break;

        case INST_ADR_OPCODE:
            reg_rd = pc + cpu->imm + 2;
            write_rd = 1;
            break;

        case INST_ASRS_OPCODE:
            if (cpu->imm == 0) cpu->imm = 32;
            reg_rd = armv6m_shift_asr(cpu, reg_rm, cpu->imm, FLAGS_NZC);
            write_rd = 1;
            break;

        case INST_B_OPCODE:
            offset = armv6m_sign_extend(cpu->imm, 11) << 1;
            offset += pc + 2;
            pc = offset;
            break;

        case INST_BL_OPCODE: {
            offset = armv6m_sign_extend(cpu->imm, 11) << 11;
            offset |= (inst2 & 0x7FF);
            offset <<= 1;
            offset += pc;
            reg_rd = (pc + 2) | 1;
            write_rd = 1;
            pc = offset + 2;
            break;
        }

        case INST_CMP_OPCODE:
            armv6m_add_with_carry(cpu, reg_rn, ~cpu->imm, 1, ALL_FLAGS);
            break;

        case INST_LDM_OPCODE: {
            uint32_t addr = reg_rn;
            for (int i = 0; i < NUM_REGS && cpu->reglist != 0; i++) {
                if (cpu->reglist & (1 << i)) {
                    cpu->r[i] = mem_read32(cpu, addr);
                    if (i == REG_PC) {
                        if ((cpu->r[i] & EXC_RETURN) != EXC_RETURN)
                            cpu->r[i] &= ~1;
                        pc = cpu->r[i];
                    }
                    addr += 4;
                    cpu->reglist &= ~(1 << i);
                }
            }
            cpu->r[cpu->rd] = addr;
            break;
        }

        case INST_LDR_OPCODE:
            cpu->r[cpu->rt] = mem_read32(cpu, reg_rn + (cpu->imm << 2));
            break;

        case INST_LDR_1_OPCODE:
            cpu->r[cpu->rt] = mem_read32(cpu, reg_rn + (cpu->imm << 2));
            break;

        case INST_LDR_2_OPCODE:
            cpu->r[cpu->rt] = mem_read32(cpu, (cpu->r[REG_PC] & 0xFFFFFFFC) + (cpu->imm << 2) + 4);
            break;

        case INST_LDRB_OPCODE:
            cpu->r[cpu->rt] = mem_read_width(cpu, reg_rn + cpu->imm, 1);
            break;

        case INST_LDRH_OPCODE:
            cpu->r[cpu->rt] = mem_read_width(cpu, reg_rn + (cpu->imm << 1), 2);
            break;

        case INST_LSLS_OPCODE:
            if (cpu->imm == 0) {
                reg_rd = reg_rm;
                write_rd = 1;
                armv6m_update_nz(cpu, reg_rd);
            } else {
                reg_rd = armv6m_shift_lsl(cpu, reg_rm, cpu->imm, FLAGS_NZC);
                write_rd = 1;
            }
            break;

        case INST_LSRS_OPCODE:
            if (cpu->imm == 0) cpu->imm = 32;
            reg_rd = armv6m_shift_lsr(cpu, reg_rm, cpu->imm, FLAGS_NZC);
            write_rd = 1;
            break;

        case INST_MOVS_OPCODE:
            reg_rd = cpu->imm;
            write_rd = 1;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_STM_OPCODE: {
            uint32_t addr = reg_rn;
            for (int i = 0; i < NUM_REGS && cpu->reglist != 0; i++) {
                if (cpu->reglist & (1 << i)) {
                    mem_write32(cpu, addr, cpu->r[i]);
                    addr += 4;
                    cpu->reglist &= ~(1 << i);
                }
            }
            reg_rd = addr;
            write_rd = 1;
            break;
        }

        case INST_STR_OPCODE:
            mem_write32(cpu, reg_rn + (cpu->imm << 2), cpu->r[cpu->rt]);
            break;

        case INST_STR_1_OPCODE:
            mem_write32(cpu, reg_rn + (cpu->imm << 2), cpu->r[cpu->rt]);
            break;

        case INST_STRB_OPCODE:
            mem_write_width(cpu, reg_rn + cpu->imm, cpu->r[cpu->rt], 1);
            break;

        case INST_STRH_OPCODE:
            mem_write_width(cpu, reg_rn + (cpu->imm << 1), cpu->r[cpu->rt], 2);
            break;

        case INST_SUBS_1_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, ~cpu->imm, 1, ALL_FLAGS);
            write_rd = 1;
            break;
        }
        break;

    /* ============================================================ */
    /* Group 2: ALU reg, loads/stores reg offset, push/pop          */
    /* ============================================================ */
    case IGRP2:
        switch (inst & IGRP2_MASK) {
        case INST_ADDS_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, cpu->imm, 0, ALL_FLAGS);
            write_rd = 1;
            break;

        case INST_ADDS_2_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, reg_rm, 0, ALL_FLAGS);
            write_rd = 1;
            break;

        case INST_LDR_3_OPCODE:
            cpu->r[cpu->rt] = mem_read32(cpu, reg_rn + reg_rm);
            break;

        case INST_LDRB_1_OPCODE:
            cpu->r[cpu->rt] = mem_read_width(cpu, reg_rn + reg_rm, 1);
            break;

        case INST_LDRH_1_OPCODE:
            cpu->r[cpu->rt] = mem_read_width(cpu, reg_rn + reg_rm, 2);
            break;

        case INST_LDRSB_OPCODE: {
            uint32_t val = mem_read_width(cpu, reg_rn + reg_rm, 1);
            cpu->r[cpu->rt] = armv6m_sign_extend(val, 8);
            break;
        }

        case INST_LDRSH_OPCODE: {
            uint32_t val = mem_read_width(cpu, reg_rn + reg_rm, 2);
            cpu->r[cpu->rt] = armv6m_sign_extend(val, 16);
            break;
        }

        case INST_POP_OPCODE: {
            uint32_t sp = cpu->r[REG_SP];
            for (int i = 0; i < NUM_REGS && cpu->reglist != 0; i++) {
                if (cpu->reglist & (1 << i)) {
                    cpu->r[i] = mem_read32(cpu, sp);
                    sp += 4;
                    if (i == REG_PC) {
                        if ((cpu->r[i] & EXC_RETURN) != EXC_RETURN)
                            cpu->r[i] &= ~1;
                        pc = cpu->r[i];
                    }
                    cpu->reglist &= ~(1 << i);
                }
            }
            armv6m_update_sp(cpu, sp);
            break;
        }

        case INST_PUSH_OPCODE: {
            uint32_t sp = cpu->r[REG_SP];
            int bits = 0;
            for (int i = 0; i < NUM_REGS; i++)
                if (cpu->reglist & (1 << i)) bits++;
            sp -= (4 * bits);
            uint32_t new_sp = sp;
            for (int i = 0; i < NUM_REGS && cpu->reglist != 0; i++) {
                if (cpu->reglist & (1 << i)) {
                    mem_write32(cpu, sp, cpu->r[i]);
                    sp += 4;
                    cpu->reglist &= ~(1 << i);
                }
            }
            armv6m_update_sp(cpu, new_sp);
            break;
        }

        case INST_STR_2_OPCODE:
            mem_write32(cpu, reg_rn + reg_rm, cpu->r[cpu->rt]);
            break;

        case INST_STRB_1_OPCODE:
            mem_write_width(cpu, reg_rn + reg_rm, cpu->r[cpu->rt], 1);
            break;

        case INST_STRH_1_OPCODE:
            mem_write_width(cpu, reg_rn + reg_rm, cpu->r[cpu->rt], 2);
            break;

        case INST_SUBS_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, ~cpu->imm, 1, ALL_FLAGS);
            write_rd = 1;
            break;

        case INST_SUBS_2_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, ~reg_rm, 1, ALL_FLAGS);
            write_rd = 1;
            break;
        }
        break;

    /* ============================================================ */
    /* Group 3: Special data processing, BKPT, SVC                   */
    /* ============================================================ */
    case IGRP3:
        switch (inst & IGRP3_MASK) {
        case INST_ADD_OPCODE:
            reg_rd = reg_rn + reg_rm;
            write_rd = 1;
            break;

        case INST_BKPT_OPCODE:
            printf("\nBKPT #%d (exit code=%d)\n", cpu->imm, cpu->imm);
            cpu->fault = 1;
            break;

        case INST_CMP_2_OPCODE:
            armv6m_add_with_carry(cpu, reg_rn, ~reg_rm, 1, ALL_FLAGS);
            break;

        case INST_MOV_OPCODE:
            if (cpu->rd == REG_PC) {
                pc = reg_rm & ~1;
                write_rd = 0;
            } else {
                reg_rd = reg_rm;
                write_rd = 1;
            }
            break;

        case INST_SVC_OPCODE:
            pc = cpu_exception(cpu, pc, EXC_SVCALL);
            break;

        case INST_UDF_OPCODE:
            fprintf(stderr, "UDF at %08x\n", cpu->r[REG_PC]);
            cpu->fault = 1;
            break;
        }
        break;

    /* ============================================================ */
    /* Group 4: SP adjust, BLX, BX                                  */
    /* ============================================================ */
    case IGRP4:
        switch (inst & IGRP4_MASK) {
        case INST_ADD_2_OPCODE:
            reg_rd = reg_rn + (cpu->imm << 2);
            write_rd = 1;
            break;

        case INST_BLX_OPCODE:
            reg_rd = pc | 1;
            write_rd = 1;
            pc = reg_rm & ~1;
            break;

        case INST_BX_OPCODE:
            if ((reg_rm & EXC_RETURN) == EXC_RETURN)
                pc = reg_rm;
            else
                pc = reg_rm & ~1;
            break;

        case INST_SUB_OPCODE:
            reg_rd = reg_rn - (cpu->imm << 2);
            write_rd = 1;
            break;
        }
        break;

    /* ============================================================ */
    /* Group 5: ALU operations                                      */
    /* ============================================================ */
    case IGRP5:
        switch (inst & IGRP5_MASK) {
        case INST_ADCS_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, reg_rm,
                        (cpu->apsr & APSR_C) ? 1 : 0, ALL_FLAGS);
            write_rd = 1;
            break;

        case INST_ANDS_OPCODE:
            reg_rd = reg_rn & reg_rm;
            write_rd = 1;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_ASRS_1_OPCODE:
            reg_rd = armv6m_shift_asr(cpu, reg_rn, reg_rm, FLAGS_NZC);
            write_rd = 1;
            break;

        case INST_BICS_OPCODE:
            reg_rd = reg_rn & ~reg_rm;
            write_rd = 1;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_CMN_OPCODE:
            armv6m_add_with_carry(cpu, reg_rn, reg_rm, 0, ALL_FLAGS);
            break;

        case INST_CMP_1_OPCODE:
            armv6m_add_with_carry(cpu, reg_rn, ~reg_rm, 1, ALL_FLAGS);
            break;

        case INST_EORS_OPCODE:
            reg_rd = reg_rn ^ reg_rm;
            write_rd = 1;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_LSLS_1_OPCODE:
            if (reg_rm == 0) {
                reg_rd = reg_rn;
                write_rd = 1;
                armv6m_update_nz(cpu, reg_rd);
            } else {
                reg_rd = armv6m_shift_lsl(cpu, reg_rn, reg_rm, FLAGS_NZC);
                write_rd = 1;
            }
            break;

        case INST_LSRS_1_OPCODE:
            reg_rd = armv6m_shift_lsr(cpu, reg_rn, reg_rm & 0xFF, FLAGS_NZC);
            write_rd = 1;
            break;

        case INST_MULS_OPCODE:
            reg_rd = reg_rn * reg_rm;
            write_rd = 1;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_MVNS_OPCODE:
            reg_rd = ~reg_rm;
            write_rd = 1;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_ORRS_OPCODE:
            reg_rd = reg_rn | reg_rm;
            write_rd = 1;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_REV_OPCODE:
            reg_rd = ((reg_rm >>  0) & 0xFF) << 24
                   | ((reg_rm >>  8) & 0xFF) << 16
                   | ((reg_rm >> 16) & 0xFF) <<  8
                   | ((reg_rm >> 24) & 0xFF);
            write_rd = 1;
            break;

        case INST_REV16_OPCODE:
            reg_rd = ((reg_rm >>  0) & 0xFF) <<  8
                   | ((reg_rm >>  8) & 0xFF) <<  0
                   | ((reg_rm >> 16) & 0xFF) << 24
                   | ((reg_rm >> 24) & 0xFF) << 16;
            write_rd = 1;
            break;

        case INST_REVSH_OPCODE:
            reg_rd = ((reg_rm >>  0) & 0xFF) <<  8
                   | ((reg_rm >>  8) & 0xFF);
            reg_rd = armv6m_sign_extend(reg_rd, 16);
            write_rd = 1;
            break;

        case INST_RORS_OPCODE:
            reg_rd = armv6m_shift_ror(cpu, reg_rn, reg_rm & 0xFF, FLAGS_NZC);
            write_rd = 1;
            break;

        case INST_RSBS_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, ~reg_rn, 0, 1, ALL_FLAGS);
            write_rd = 1;
            break;

        case INST_SBCS_OPCODE:
            reg_rd = armv6m_add_with_carry(cpu, reg_rn, ~reg_rm,
                        (cpu->apsr & APSR_C) ? 1 : 0, ALL_FLAGS);
            write_rd = 1;
            break;

        case INST_SXTB_OPCODE:
            reg_rd = armv6m_sign_extend(reg_rm & 0xFF, 8);
            write_rd = 1;
            break;

        case INST_SXTH_OPCODE:
            reg_rd = armv6m_sign_extend(reg_rm & 0xFFFF, 16);
            write_rd = 1;
            break;

        case INST_TST_OPCODE:
            reg_rd = reg_rn & reg_rm;
            armv6m_update_nz(cpu, reg_rd);
            break;

        case INST_UXTB_OPCODE:
            reg_rd = reg_rm & 0xFF;
            write_rd = 1;
            break;

        case INST_UXTH_OPCODE:
            reg_rd = reg_rm & 0xFFFF;
            write_rd = 1;
            break;
        }
        break;

    /* ============================================================ */
    /* Group 6: MRS, MSR, CPS                                       */
    /* ============================================================ */
    case IGRP6:
        switch (inst & IGRP6_MASK) {
        case INST_MRS_OPCODE: {
            uint32_t sysm = inst2 & 0xFF;
            cpu->rd = (inst2 >> 8) & 0xF;
            pc += 2;

            switch ((sysm >> 3) & 0x1F) {
            case 0: {
                uint32_t val = 0;
                if (sysm & 0x1) val |= cpu->ipsr & 0x1FF;
                if (!(sysm & 0x4)) val |= cpu->apsr & 0xF8000000;
                val |= cpu->ipsr;
                val |= cpu->epsr;
                reg_rd = val;
                write_rd = 1;
                break;
            }
            case 1:
                switch (sysm & 0x7) {
                case 0: reg_rd = cpu->msp; write_rd = 1; break;
                case 1: reg_rd = cpu->psp; write_rd = 1; break;
                }
                break;
            case 2:
                switch (sysm & 0x7) {
                case 0: reg_rd = cpu->primask & PRIMASK_PM; write_rd = 1; break;
                case 4: reg_rd = cpu->control & CONTROL_MASK; write_rd = 1; break;
                }
                break;
            }
            break;
        }

        case INST_MSR_OPCODE: {
            uint32_t sysm = inst2 & 0xFF;
            pc += 2;

            switch ((sysm >> 3) & 0x1F) {
            case 0:
                if (!(sysm & 0x4))
                    cpu->apsr = reg_rn & 0xF8000000;
                break;
            case 1:
                switch (sysm & 0x7) {
                case 0: cpu->msp = reg_rn; break;
                case 1: cpu->psp = reg_rn; break;
                }
                break;
            case 2:
                switch (sysm & 0x7) {
                case 0: cpu->primask = reg_rn & PRIMASK_PM; break;
                case 4:
                    if (cpu->mode == MODE_THREAD)
                        cpu->control = reg_rn & CONTROL_MASK;
                    break;
                }
                break;
            }
            break;
        }

        case INST_CPS_OPCODE:
            if (cpu->imm == 0)
                cpu->primask &= ~PRIMASK_PM;
            else
                cpu->primask |= PRIMASK_PM;
            break;
        }
        break;

    /* ============================================================ */
    /* Group 7: ISB, UDF_W (32-bit, skip second halfword)           */
    /* ============================================================ */
    case IGRP7:
        pc += 2;
        break;

    /* ============================================================ */
    /* Group 8: NOP, SEV, WFE, WFI, YIELD                           */
    /* ============================================================ */
    case IGRP8:
        break;
    }

    /* Register writeback */
    if (write_rd) {
        if (cpu->rd == REG_SP)
            armv6m_update_sp(cpu, reg_rd);
        else
            cpu->r[cpu->rd] = reg_rd;
    }

    cpu->r[REG_PC] = pc;
}

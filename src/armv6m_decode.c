#include "armv6m.h"

int armv6m_decode(armv6m_t *cpu, uint16_t inst)
{
    int res = 0;
    int decoded = 0;

    cpu->rd = 0;
    cpu->rt = 0;
    cpu->rm = 0;
    cpu->rn = 0;
    cpu->imm = 0;
    cpu->reglist = 0;
    cpu->cond = 0;

    /* Group 0: BCC */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP0;
        switch (inst & IGRP0_MASK) {
        case INST_BCC_OPCODE:
            cpu->cond = (inst >> 8) & 0x0F;
            cpu->imm  = inst & 0xFF;
            break;
        default:
            decoded = 0;
            break;
        }
    }

    /* Group 1: misc data processing, loads/stores, branches */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP1;
        switch (inst & IGRP1_MASK) {
        case INST_ADDS_1_OPCODE:
        case INST_SUBS_1_OPCODE:
            cpu->rd   = (inst >> 8) & 7;
            cpu->rn   = cpu->rd;
            cpu->imm  = inst & 0xFF;
            break;

        case INST_ADR_OPCODE:
        case INST_MOVS_OPCODE:
            cpu->rd   = (inst >> 8) & 7;
            cpu->imm  = inst & 0xFF;
            break;

        case INST_ASRS_OPCODE:
        case INST_LSLS_OPCODE:
        case INST_LSRS_OPCODE:
            cpu->imm  = (inst >> 6) & 0x1F;
            cpu->rm   = (inst >> 3) & 7;
            cpu->rd   = inst & 7;
            break;

        case INST_B_OPCODE:
            cpu->imm  = inst & 0x7FF;
            break;

        case INST_BL_OPCODE:
            res = 1;
            cpu->imm    = inst & 0x7FF;
            cpu->rd     = REG_LR;
            /* Check if next halfword confirms BL (not MSR) */
            if ((cpu_fetch_inst(cpu, cpu->r[REG_PC] + 2) & 0xC000) != 0xC000)
                decoded = 0;
            break;

        case INST_CMP_OPCODE:
            cpu->rn   = (inst >> 8) & 7;
            cpu->imm  = inst & 0xFF;
            break;

        case INST_LDM_OPCODE:
        case INST_STM_OPCODE:
            cpu->rn      = (inst >> 8) & 7;
            cpu->rd      = cpu->rn;
            cpu->reglist = inst & 0xFF;
            break;

        case INST_LDR_OPCODE:
        case INST_LDRB_OPCODE:
        case INST_LDRH_OPCODE:
        case INST_STR_OPCODE:
        case INST_STRB_OPCODE:
        case INST_STRH_OPCODE:
            cpu->imm  = (inst >> 6) & 0x1F;
            cpu->rn   = (inst >> 3) & 7;
            cpu->rt   = inst & 7;
            cpu->rd   = cpu->rt;
            break;

        case INST_LDR_2_OPCODE:
            cpu->rt   = (inst >> 8) & 7;
            cpu->rd   = cpu->rt;
            cpu->imm  = inst & 0xFF;
            break;

        case INST_LDR_1_OPCODE:
        case INST_STR_1_OPCODE:
        case INST_ADD_1_OPCODE:
            cpu->rt   = (inst >> 8) & 7;
            cpu->rn   = REG_SP;
            cpu->rd   = cpu->rt;
            cpu->imm  = inst & 0xFF;
            break;

        default:
            decoded = 0;
            break;
        }
    }

    /* Group 2: ALU register, loads/stores with register offset, push/pop */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP2;
        switch (inst & IGRP2_MASK) {
        case INST_ADDS_OPCODE:
        case INST_SUBS_OPCODE:
            cpu->imm = (inst >> 6) & 7;
            cpu->rn  = (inst >> 3) & 7;
            cpu->rd  = inst & 7;
            break;

        case INST_ADDS_2_OPCODE:
        case INST_SUBS_2_OPCODE:
            cpu->rm  = (inst >> 6) & 7;
            cpu->rn  = (inst >> 3) & 7;
            cpu->rd  = inst & 7;
            break;

        case INST_LDR_3_OPCODE:
        case INST_LDRB_1_OPCODE:
        case INST_LDRH_1_OPCODE:
        case INST_LDRSB_OPCODE:
        case INST_LDRSH_OPCODE:
        case INST_STR_2_OPCODE:
        case INST_STRB_1_OPCODE:
        case INST_STRH_1_OPCODE:
            cpu->rm  = (inst >> 6) & 7;
            cpu->rn  = (inst >> 3) & 7;
            cpu->rt  = inst & 7;
            cpu->rd  = cpu->rt;
            break;

        case INST_POP_OPCODE:
            cpu->reglist = inst & 0xFF;
            if (inst & (1 << 8))
                cpu->reglist |= (1 << REG_PC);
            break;

        case INST_PUSH_OPCODE:
            cpu->reglist = inst & 0xFF;
            if (inst & (1 << 8))
                cpu->reglist |= (1 << REG_LR);
            break;

        default:
            decoded = 0;
            break;
        }
    }

    /* Group 3: special data processing, BKPT, SVC, UDF */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP3;
        switch (inst & IGRP3_MASK) {
        case INST_ADD_OPCODE:
            cpu->rm  = (inst >> 3) & 0xF;
            cpu->rd  = inst & 7;
            cpu->rd |= (inst >> 4) & 8;
            cpu->rn  = cpu->rd;
            break;

        case INST_BKPT_OPCODE:
        case INST_SVC_OPCODE:
        case INST_UDF_OPCODE:
            cpu->imm = inst & 0xFF;
            break;

        case INST_CMP_2_OPCODE:
            cpu->rm  = (inst >> 3) & 0xF;
            cpu->rn  = inst & 7;
            cpu->rn |= (inst >> 4) & 8;
            break;

        case INST_MOV_OPCODE:
            cpu->rm  = (inst >> 3) & 0xF;
            cpu->rd  = inst & 7;
            cpu->rd |= (inst >> 4) & 8;
            break;

        default:
            decoded = 0;
            break;
        }
    }

    /* Group 4: SP adjust, BLX, BX */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP4;
        switch (inst & IGRP4_MASK) {
        case INST_ADD_2_OPCODE:
        case INST_SUB_OPCODE:
            cpu->rn  = REG_SP;
            cpu->rd  = REG_SP;
            cpu->imm = inst & 0x7F;
            break;

        case INST_BLX_OPCODE:
            cpu->rm = (inst >> 3) & 0xF;
            cpu->rd = REG_LR;
            break;

        case INST_BX_OPCODE:
            cpu->rm = (inst >> 3) & 0xF;
            break;

        default:
            decoded = 0;
            break;
        }
    }

    /* Group 5: ALU operations */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP5;
        switch (inst & IGRP5_MASK) {
        case INST_ADCS_OPCODE:
        case INST_ANDS_OPCODE:
        case INST_ASRS_1_OPCODE:
        case INST_BICS_OPCODE:
        case INST_EORS_OPCODE:
        case INST_LSLS_1_OPCODE:
        case INST_LSRS_1_OPCODE:
        case INST_ORRS_OPCODE:
        case INST_RORS_OPCODE:
        case INST_SBCS_OPCODE:
            cpu->rm = (inst >> 3) & 7;
            cpu->rd = inst & 7;
            cpu->rn = cpu->rd;
            break;

        case INST_CMN_OPCODE:
        case INST_CMP_1_OPCODE:
        case INST_TST_OPCODE:
            cpu->rm = (inst >> 3) & 7;
            cpu->rn = inst & 7;
            break;

        case INST_MULS_OPCODE:
            cpu->rn = (inst >> 3) & 7;
            cpu->rd = inst & 7;
            cpu->rm = cpu->rd;
            break;

        case INST_MVNS_OPCODE:
        case INST_REV_OPCODE:
        case INST_REV16_OPCODE:
        case INST_REVSH_OPCODE:
        case INST_SXTB_OPCODE:
        case INST_SXTH_OPCODE:
        case INST_UXTB_OPCODE:
        case INST_UXTH_OPCODE:
            cpu->rm = (inst >> 3) & 7;
            cpu->rd = inst & 7;
            break;

        case INST_RSBS_OPCODE:
            cpu->rn = (inst >> 3) & 7;
            cpu->rd = inst & 7;
            break;

        default:
            decoded = 0;
            break;
        }
    }

    /* Group 6: MRS, MSR, CPS */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP6;
        switch (inst & IGRP6_MASK) {
        case INST_MRS_OPCODE:
            res = 1;
            break;
        case INST_MSR_OPCODE:
            cpu->rn = inst & 0xF;
            res = 1;
            break;
        case INST_CPS_OPCODE:
            cpu->imm = (inst >> 4) & 1;
            break;
        default:
            decoded = 0;
            break;
        }
    }

    /* Group 7: ISB, UDF_W (32-bit no-ops) */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP7;
        switch (inst & IGRP7_MASK) {
        case INST_ISB_OPCODE:
        case INST_UDF_W_OPCODE:
            res = 1;
            break;
        default:
            decoded = 0;
            break;
        }
    }

    /* Group 8: NOP, SEV, WFE, WFI, YIELD */
    if (!decoded) {
        decoded = 1;
        cpu->igroup = IGRP8;
        switch (inst & IGRP8_MASK) {
        case INST_NOP_OPCODE:
        case INST_SEV_OPCODE:
        case INST_WFE_OPCODE:
        case INST_WFI_OPCODE:
        case INST_YIELD_OPCODE:
            break;
        default:
            decoded = 0;
            break;
        }
    }

    if (!decoded) {
        fprintf(stderr, "Decode failed: PC=%08x INST=%04x\n", cpu->r[REG_PC], inst);
        cpu->fault = 1;
    }

    return res;
}

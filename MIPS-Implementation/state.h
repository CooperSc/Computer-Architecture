#ifndef STATE
#define STATE
#include <vector>
#include <cstdint>
#include <iostream>
#include "control.h"
//
// Pipeline registers implementation
//
// TODO:
// IFID Pipeline register, only contains instruction and pc + 1 (or 4)?
struct IFID {
    uint32_t instruction;
    uint32_t pc;
    control_t control;
};

// TODO:
// IDEX Pipeline register
// Contains: Read data 1, Read data 2, Incremented PC from IFID, 32-bit
// sign-extended immediate, IFID Reg_Rs, IFID Reg_Rt (twice?), IFID Reg_Rd,
// all the control signals
struct IDEX {
    control_t control;
    uint32_t read_data_1;
    uint32_t read_data_2;
    uint32_t pc;
    uint32_t immediate;
    uint32_t reg_Rs;
    uint32_t reg_Rt;
    uint32_t reg_Rd;
};

// TODO:
// EXMEM Pipeline register
// Contains: ALU Result, PC Branch ALU Result, Read data 2, 
// write reg (which is the mux result between Rs and Rd with signal
// RegDst)
struct EXMEM {
    control_t control;
    uint32_t read_data_2;
    uint32_t ALU_result;
    uint32_t branch_pc;
    uint32_t write_reg;
};

// TODO:
// MEMWB Pipeline register
// Contains: data memory data, ALU_result
struct MEMWB {
    control_t control;
    uint32_t data_memory_read_data;
    uint32_t ALU_result;
};

#endif

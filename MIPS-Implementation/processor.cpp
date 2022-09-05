#include <cstdint>
#include <iostream>
#include "memory.h"
#include "reg_file.h"
#include "ALU.h"
#include "BPU.h"
#include "control.h"
using namespace std;

// Sample processor main loop for a single-cycle processor
void single_cycle_main_loop(Registers &reg_file, Memory &memory, uint32_t end_pc)
{

    // Initialize ALU
    ALU alu;
    // Initialize Control
    control_t control = {
        .reg_dest = 0,
        .jump = 0,
        .branch = 0,
        .mem_read = 0,
        .mem_to_reg = 0,
        .ALU_op = 0,
        .mem_write = 0,
        .ALU_src = 0,
        .reg_write = 0,
        .branch_not_equal = 0,
        .shift = 0,
        .size = 00,
        //.slt = 00,
        .zero = 0,
    };

    uint32_t num_cycles = 0;
    uint32_t num_instrs = 0;

    while (reg_file.pc != end_pc)
    {
        cout << "reg_file pc: " << reg_file.pc << endl;
        cout << "end pc: " << end_pc << endl;
        // fetch
        uint32_t instruction;
        memory.access(reg_file.pc, instruction, 0, 1, 0);

        string instr_str = bitset<32>(instruction).to_string();
        cout << instr_str << endl;
        // increment pc
        reg_file.pc += 4;

        // TODO: fill in the function argument
        // decode into contol signals
        control.decode(instruction);
        control.print(); // used for autograding

        // break down the instruction
        int opcode = stoul(instr_str.substr(0, 6), 0, 2); // bits 32-26 or Opcode
        int reg_1 = stoul(instr_str.substr(6, 5), 0, 2);  // bits 25-21 or Rs
        int reg_2 = stoul(instr_str.substr(11, 5), 0, 2); // bits 20-16 or Rt
        cout << "reg 1: " << reg_1 << " reg 2: " << reg_2 << endl;
        int write_reg = 0;
        if (control.reg_dest == 0)
        { // rt is dst reg
            write_reg = reg_2;
        }
        else
        {                                                     // rd is dst reg
            write_reg = stoul(instr_str.substr(16, 5), 0, 2); // bits 15-11 or Rd
        }
        int shamt = stoul(instr_str.substr(21, 5), 0, 2); // bits 10-6 or shift amount
        int funct = stoul(instr_str.substr(26, 6), 0, 2); // bits 5-0 or function

        uint32_t immediate = stoul(instr_str.substr(16, 16), 0, 2); // 16 bit immediate field
        uint32_t address = stoul(instr_str.substr(6, 26), 0, 2);    // address (should it be uint32?)

        uint32_t read_data_1 = 0;
        uint32_t read_data_2 = 0;

        // TODO: fill in the function argument
        // Read from reg file

        reg_file.access(reg_1, reg_2, read_data_1, read_data_2, write_reg, false, 0);

        if (!(control.zero))
        { // shift 16 bit imm to 32-bit
            if (immediate > (pow(2, 15) - 1))
            {
                immediate = immediate + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
        }

        cout << "immediate int: " << immediate << endl;
        cout << "immediate binary: " << bitset<32>(immediate).to_string();
        // TODO: fill in the function argument
        // Execution
        alu.generate_control_inputs(control.ALU_op, funct, opcode);

        uint32_t ALU_input1 = read_data_1;
        uint32_t ALU_input2 = read_data_2;

        if (control.ALU_src == 1)
        {
            ALU_input2 = immediate;
        }

        if (control.shift == 1)
        {
            ALU_input1 = shamt;
        }

        // TODO: fill in the function argument
        uint32_t zero = 0;
        // cout << "ALU control inputs " << alu.alu_control_inputs<< endl;
        uint32_t alu_result = alu.execute(ALU_input1, ALU_input2, zero);
        cout << "ALU Result " << alu_result << endl;
        // Memory
        // TODO: fill in the function argument

        // Store word size MUX
        uint32_t memWriteData = read_data_2;
        uint32_t memMask = 0;

        if (control.size == 1)
        { // sh
            memory.access(alu_result, memMask, 0, true, false);
            memMask = (memMask >> 16) << 16;
            memWriteData = (memWriteData << 16) >> 16;
            memWriteData = memWriteData + memMask;
        }
        if (control.size == 2)
        { // sb
            memory.access(alu_result, memMask, 0, true, false);
            memMask = (memMask >> 8) << 8;
            memWriteData = (memWriteData << 24) >> 24;
            memWriteData = memWriteData + memMask;
        }
        uint32_t memReadData = 0;
        memory.access(alu_result, memReadData, memWriteData, control.mem_read, control.mem_write);
        cout << "Mem Read Data: " << memReadData << endl;
        cout << "Mem Write Data: " << memWriteData << endl;
        memory.print(28, 1);
        memory.print(7, 1);
        memory.print(0, 30);
        // Write Back
        // TODO: fill in the function argument

        // Writeback MUX
        uint32_t writeback = 0;
        if (control.mem_to_reg)
        {
            writeback = memReadData;
        }
        else
        {
            writeback = alu_result;
        }

        // Load word size MUX
        if (control.size == 1)
        { // lhu
            writeback = (writeback << 16) >> 16;
        }
        if (control.size == 2)
        { // lbu
            writeback = (writeback << 24) >> 24;
        }
        cout << "writeback " << writeback << endl;

        reg_file.access(reg_1, reg_2, read_data_1, read_data_2, write_reg, control.reg_write, writeback);
        // TODO: Update PC
        bool isBranch = false;
        if ((control.branch == 1 && zero == 1) || (control.branch_not_equal == 1 && zero == 0))
        {
            isBranch = true;
        }

        uint32_t branch_pc = (immediate << 2) + reg_file.pc;
        uint32_t jump_pc = read_data_1;

        // mux logic
        if (isBranch == true)
        {
            reg_file.pc = branch_pc;
        }

        else if (control.jump == 1)
        {
            reg_file.pc = jump_pc;
        }

        cout << "CYCLE" << num_cycles << "\n";
        reg_file.print(); // used for automated testing

        num_cycles++;
        num_instrs++;
    }

    cout << "CPI = " << (double)num_cycles / (double)num_instrs << "\n";
}

// pipelined main loop should use similar logic to single cycle
// difference is going to be the pipelined registers, and forwarding / hazard units
void pipelined_main_loop(Registers &reg_file, Memory &memory, uint32_t end_pc)
{
    cout << end_pc << endl;
    // Initialize ALU
    ALU alu;
    // Initialize Control
    control_t control = {
        .reg_dest = 0,
        .jump = 0,
        .branch = 0,
        .mem_read = 0,
        .mem_to_reg = 0,
        .ALU_op = 0,
        .mem_write = 0,
        .ALU_src = 0,
        .reg_write = 0,
        .branch_not_equal = 0,
        .shift = 0,
        .size = 00,
        //.slt = 00,
        .zero = 0,
    };
    uint32_t num_cycles = 0;
    uint32_t num_instrs = 0;
    int committed_insts = 0;

    // Initialize Pipeline Registers as Structures
    struct IFID1
    {
        uint32_t instruction = 0;
        uint32_t pc = 0;
    } IFID;

    struct IDEX1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        int forward_read_data_1_EX = 0;
        int forward_read_data_1_MEM = 0;
        int forward_read_data_2_EX = 0;
        int forward_read_data_2_MEM = 0;
        uint32_t read_data_1 = 0;
        uint32_t read_data_2 = 0;
        uint32_t immediate = 0;
        int funct = 0;
        int opcode = 0;
        int shamt = 0;
        int write_reg = 0;
        uint32_t pc = 0;
    } IDEX;

    struct EXMEM1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        uint32_t pc = 0;
        uint32_t zero = 0;
        uint32_t alu_result = 0;
        uint32_t read_data_2 = 0;
        int write_reg = 0;
        uint32_t branch_pc = 0;
        uint32_t jump_pc = 0;
    } EXMEM;

    struct MEMWB1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        uint32_t alu_result = 0;
        uint32_t memReadData = 0;
        int write_reg = 0;
    } MEMWB;

    // Initialize Variables
    uint32_t writeback = 0;    // WB: Used to hold writeback value for register writing
    uint32_t dummyVar1 = 0;    // WB: Dummy variable used for register file access read values that we don't care about
    uint32_t memWriteData = 0; // MEM: Used to hold the write value for a store operation
    uint32_t memMask = 0;      // MEM: Used as a mask for half-word and byte memory operations
    uint32_t memReadData = 0;  // MEM: Used to hold the read data from a memory load
    bool flush = false;

    uint32_t ALU_input1 = 0; // EX: Used to hold the value of the first ALU input
    uint32_t ALU_input2 = 0; // EX: Used to hold the value of the second ALU input
    uint32_t zero = 0;       // EX: Used to hold the zero output of the ALU
    uint32_t alu_result = 0; // EX: Used to hold the 32-bit output of the ALU operation
    uint32_t branch_pc = 0;  // EX: Used to hold the branch pc if a branch is to be taken
    uint32_t jump_pc = 0;    // EX: Used to hold the jump pc if a jump is to be taken

    string instr_str;         // ID: Used to hold a casted instruction value
    int opcode = 0;           // ID: Used to hold the opcode value
    int reg_1 = 0;            // ID: Used to store the Rs index
    int reg_2 = 0;            // ID: Used to store the Rt index
    int write_reg = 0;        // ID: Used to store the write register index
    int shamt = 0;            // ID: Used to store the shift bits
    int funct = 0;            // ID: Used to store the function bits
    uint32_t immediate = 0;   // ID: Used to store the immediate value
    uint32_t address = 0;     // ID: Used to jump address
    uint32_t read_data_1 = 0; // ID: Used to hold the value of the first register read
    uint32_t read_data_2 = 0; // ID: Used to hold the value of the second register read
    bool stall = false;       // ID/IF: Used to flag when a stall is needed in the pipeline

    uint32_t instruction = 0; // IF: Used to hold the fetched instruction from memory

    // register
    int MEMWB_control_mem_read = 0;

    // Revision: structure it so that we go backwards. Start with the WB
    // stage. Perform correct writeback, and then "go back" to MEM and perform
    // right logic. Then, once we have finished WB, make sure to THEN update the pipeline
    // Then, repeat. So: to simplify:
    // 1) compute WB, reg_file.access
    // 2) compute MEM, then update  MEMWB values (branch resolution MUX here?)
    // 3) compute EX, then update EXMEM values (forwarding logic + stalling logic)
    // 4) compute ID, update IDEX pipeline values
    // 5) compute IF, update IFID pipeline

    // For stalling: stalling should happen
    while (true)
    {

        // fifth pipeline stage WB: write back -----------------------------------------------

        if (MEMWB.control.mem_to_reg)
        {
            writeback = MEMWB.memReadData;
        }
        else
        {
            writeback = MEMWB.alu_result;
        }
        cout << "writeback: " << writeback << endl;
        dummyVar1 = 0;
        reg_file.access(0, 0, dummyVar1, dummyVar1, MEMWB.write_reg, MEMWB.control.reg_write, writeback);

        // no pipeline to update (last stage)

        // fourth pipeline stage MEM: data memory access--------------------------------------------

        // Branching MUX logic/Resolution: as long as this happens before IF, PC should be correct
        flush = false;
        if ((EXMEM.control.branch == 1 && EXMEM.zero == 1) || (EXMEM.control.branch_not_equal == 1 && EXMEM.zero == 0))
        {
            cout << "Took the branch" << endl;
            reg_file.pc = EXMEM.branch_pc;

            // Flushing IDEX control values
            IDEX.control.reg_dest = 0;
            IDEX.control.jump = 0;
            IDEX.control.branch = 0;
            IDEX.control.mem_read = 0;
            IDEX.control.mem_to_reg = 0;
            IDEX.control.ALU_op = 0;
            IDEX.control.mem_write = 0;
            IDEX.control.ALU_src = 0;
            IDEX.control.reg_write = 0;
            IDEX.control.branch_not_equal = 0;
            IDEX.control.shift = 0;
            IDEX.control.size = 00;
            //.slt = 00,
            IDEX.control.zero = 0;

            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.opcode = 0;
            IDEX.funct = 0;
            IDEX.shamt = 0;
            IDEX.immediate = 0;
            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.write_reg = 0;

            IFID.instruction = 0;

            flush = true;
        }

        if (EXMEM.control.jump == 1)
        {
            reg_file.pc = EXMEM.jump_pc;

            // Flushing IDEX control values
            IDEX.control.reg_dest = 0;
            IDEX.control.jump = 0;
            IDEX.control.branch = 0;
            IDEX.control.mem_read = 0;
            IDEX.control.mem_to_reg = 0;
            IDEX.control.ALU_op = 0;
            IDEX.control.mem_write = 0;
            IDEX.control.ALU_src = 0;
            IDEX.control.reg_write = 0;
            IDEX.control.branch_not_equal = 0;
            IDEX.control.shift = 0;
            IDEX.control.size = 00;
            //.slt = 00,
            IDEX.control.zero = 0;

            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.opcode = 0;
            IDEX.funct = 0;
            IDEX.shamt = 0;
            IDEX.immediate = 0;
            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.write_reg = 0;

            IFID.instruction = 0;

            flush = true;
        }

        // Store word size MUX
        memWriteData = EXMEM.read_data_2;
        memMask = 0;
        if (EXMEM.control.size == 1)
        { // sh
            memory.access(EXMEM.alu_result, memMask, 0, true, false);
            memMask = (memMask >> 16) << 16;
            memWriteData = (memWriteData << 16) >> 16;
            memWriteData = memWriteData + memMask;
        }
        if (EXMEM.control.size == 2)
        { // sb
            memory.access(EXMEM.alu_result, memMask, 0, true, false);
            memMask = (memMask >> 8) << 8;
            memWriteData = (memWriteData << 24) >> 24;
            memWriteData = memWriteData + memMask;
        }
        memReadData = 0;
        memory.access(EXMEM.alu_result, memReadData, memWriteData, EXMEM.control.mem_read, EXMEM.control.mem_write);
        memory.print(0, 10);

        // if EXMEM control signals are all 0 due to some stall beforehand, don't update MEMWB with it
        // if (!EXMEM.control.isAllZeros()){
        //     MEMWB.control = EXMEM.control;
        //     MEMWB.alu_result = EXMEM.alu_result;
        //     MEMWB.memReadData = memReadData;
        //     MEMWB.write_reg = EXMEM.write_reg;
        // }

        // third pipeline stage EX: execution or address calculation --------------------------------------
        // forwarding unit logic goes here  ...?
        alu.generate_control_inputs(IDEX.control.ALU_op, IDEX.funct, IDEX.opcode);

        ALU_input1 = IDEX.read_data_1;
        if (IDEX.forward_read_data_1_MEM && MEMWB.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input1 = MEMWB.alu_result;
            cout << "alu1 aluRes" << endl;
        }

        if (IDEX.forward_read_data_1_MEM && MEMWB.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input1 = MEMWB.memReadData;
            cout << "alu1 memread" << endl;
        }

        if (IDEX.forward_read_data_1_EX)
        {
            ALU_input1 = EXMEM.alu_result;
        }

        ALU_input2 = IDEX.read_data_2;
        if (IDEX.forward_read_data_2_MEM && MEMWB.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input2 = MEMWB.alu_result;
            if (IDEX.control.mem_write == 1)
            {
                IDEX.read_data_2 = MEMWB.alu_result;
            }
            cout << "alu2 aluRes" << endl;
        }

        if (IDEX.forward_read_data_2_MEM && MEMWB.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input2 = MEMWB.memReadData;
            cout << "alu2 memread" << endl;
            if (IDEX.control.mem_write == 1)
            {
                IDEX.read_data_2 = MEMWB.memReadData;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX.forward_read_data_2_EX)
        {
            ALU_input2 = EXMEM.alu_result;
            if (IDEX.control.mem_write == 1)
            {
                IDEX.read_data_2 = EXMEM.alu_result;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX.control.ALU_src == 1)
        {
            ALU_input2 = IDEX.immediate;
        }

        if (IDEX.control.shift == 1)
        {
            ALU_input1 = IDEX.shamt;
        }

        alu_result = alu.execute(ALU_input1, ALU_input2, zero);
        cout << "ALU Result " << alu_result << endl;

        // Branch pc
        branch_pc = (IDEX.immediate << 2) + IDEX.pc + 4;
        jump_pc = IDEX.read_data_1;

        // second pipeline stage ID: instruction decode and reg file read --------------------------
        // hazard detection occurs here

        // decode into contol signals
        instr_str = bitset<32>(IFID.instruction).to_string();
        cout << "instr_str: " << instr_str << endl;

        // set IDEX control to the decoded IFID instruction
        control.decode(IFID.instruction);
        cout << "IDEX control sigs: " << endl;
        control.print(); // used for autograding

        // break down the instruction
        opcode = stoul(instr_str.substr(0, 6), 0, 2); // bits 32-26 or Opcode

        reg_1 = stoul(instr_str.substr(6, 5), 0, 2);  // bits 25-21 or Rs
        reg_2 = stoul(instr_str.substr(11, 5), 0, 2); // bits 20-16 or Rt
        cout << "reg 1: " << reg_1 << " reg 2: " << reg_2 << endl;

        if (control.reg_dest == 0)
        { // rt is dst reg
            write_reg = reg_2;
        }
        else if (control.mem_write == 0)
        {                                                     // rd is dst reg
            write_reg = stoul(instr_str.substr(16, 5), 0, 2); // bits 15-11 or Rd
        }
        else
        {
            write_reg = 0;
        }
        shamt = stoul(instr_str.substr(21, 5), 0, 2); // bits 10-6 or shift amount
        funct = stoul(instr_str.substr(26, 6), 0, 2); // bits 5-0 or function

        immediate = stoul(instr_str.substr(16, 16), 0, 2); // 16 bit immediate field
        address = stoul(instr_str.substr(6, 26), 0, 2);    // address (should it be uint32?)

        // TODO: fill in the function argument
        // Read from reg file

        reg_file.access(reg_1, reg_2, read_data_1, read_data_2, MEMWB.write_reg, false, 0);

        if (!(control.zero))
        { // shift 16 bit imm to 32-bit
            if (immediate > (pow(2, 15) - 1))
            {
                immediate = immediate + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
        }

        // idex update was here

        // Forwarding Logic
        IDEX.forward_read_data_1_EX = 0;
        IDEX.forward_read_data_1_MEM = 0;
        IDEX.forward_read_data_2_EX = 0;
        IDEX.forward_read_data_2_MEM = 0;
        // Stalling logic
        stall = false;
        if (((reg_1 == IDEX.write_reg && reg_1 != 0) || (reg_2 == IDEX.write_reg && control.reg_dest == 1 && reg_2 != 0)) && (IDEX.opcode == 35 || IDEX.opcode == 36 || IDEX.opcode == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall = true;
        }
        else
        {
            if (reg_1 == IDEX.write_reg && reg_1 != 0 && IDEX.control.reg_write != 0)
            {
                IDEX.forward_read_data_1_EX = 1;
            }
            if (reg_1 == EXMEM.write_reg && reg_1 != 0 && EXMEM.control.reg_write != 0)
            {
                IDEX.forward_read_data_1_MEM = 1;
            }
            if (reg_2 == IDEX.write_reg && reg_2 != 0 && IDEX.control.reg_write != 0)
            {
                IDEX.forward_read_data_2_EX = 1;
            }
            if (reg_2 == EXMEM.write_reg && reg_2 != 0 && EXMEM.control.reg_write != 0)
            {
                IDEX.forward_read_data_2_MEM = 1;
            }
        }
        // if (num_cycles == 7)
        // {

        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 31, true, opcode);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 30, true, funct);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 29, true, IFID.instruction);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 28, true, EXMEM.alu_result);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 27, true, memReadData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 26, true, EXMEM.write_reg);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 25, true, memWriteData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 24, true, EXMEM.control.mem_write);
        // }

        // First stage: IF / Fetch ----------------------------------
        if (reg_file.pc >= end_pc)
        {
            instruction = 0;
        }
        else
        {
            cout << "reg_file.pc: " << reg_file.pc << endl;
            memory.access(reg_file.pc, instruction, 0, 1, 0);
        }

        // ifid update was here

        // IFID.pc = reg_file.pc;
        cout << endl;
        MEMWB.control = EXMEM.control;
        MEMWB.alu_result = EXMEM.alu_result;
        MEMWB.memReadData = memReadData;
        MEMWB.write_reg = EXMEM.write_reg;
        // update EXMEM Pipeline
        EXMEM.control = IDEX.control;
        EXMEM.pc = IDEX.pc;
        EXMEM.zero = zero;
        EXMEM.alu_result = alu_result;
        EXMEM.read_data_2 = IDEX.read_data_2;
        EXMEM.write_reg = IDEX.write_reg;
        EXMEM.branch_pc = branch_pc;
        EXMEM.jump_pc = jump_pc;

        // Stalling logic

        if (stall)
        {
            cout << "we're stalling" << endl;
            // Flushing IDEX control values
            IDEX.control.reg_dest = 0;
            IDEX.control.jump = 0;
            IDEX.control.branch = 0;
            IDEX.control.mem_read = 0;
            IDEX.control.mem_to_reg = 0;
            IDEX.control.ALU_op = 0;
            IDEX.control.mem_write = 0;
            IDEX.control.ALU_src = 0;
            IDEX.control.reg_write = 0;
            IDEX.control.branch_not_equal = 0;
            IDEX.control.shift = 0;
            IDEX.control.size = 00;
            //.slt = 00,
            IDEX.control.zero = 0;

            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.opcode = 0;
            IDEX.funct = 0;
            IDEX.shamt = 0;
            IDEX.immediate = 0;
            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.write_reg = 0;
        }

        else
        {

            IDEX.control.reg_dest = control.reg_dest;
            IDEX.control.jump = control.jump;
            IDEX.control.branch = control.branch;
            IDEX.control.mem_read = control.mem_read;
            IDEX.control.mem_to_reg = control.mem_to_reg;
            IDEX.control.ALU_op = control.ALU_op;
            IDEX.control.mem_write = control.mem_write;
            IDEX.control.ALU_src = control.ALU_src;
            IDEX.control.reg_write = control.reg_write;
            IDEX.control.branch_not_equal = control.branch_not_equal;
            IDEX.control.shift = control.shift;
            IDEX.control.size = control.size;
            IDEX.control.zero = control.zero;

            IDEX.read_data_1 = read_data_1;
            IDEX.read_data_2 = read_data_2;
            IDEX.pc = IFID.pc;
            IDEX.opcode = opcode;
            IDEX.funct = funct;
            IDEX.shamt = shamt;
            IDEX.immediate = immediate;
            IDEX.read_data_1 = read_data_1;
            IDEX.read_data_2 = read_data_2;
            IDEX.write_reg = write_reg;
        }

        // if not stalling, just update IFID pipeline to fetched values. otherwise
        // set them to the NOP stuff
        if (!(stall))
        {
            IFID.pc = reg_file.pc;
            reg_file.pc += 4;
            IFID.instruction = instruction;
        }
        // print MEMWB values
        cout << "MEMWB Control: ";
        MEMWB.control.print();
        cout << "MEMWB alu_result: " << MEMWB.alu_result << endl;
        cout << "MEMWB mmeReadData: " << MEMWB.memReadData << endl;
        cout << "MEMWB write_reg: " << MEMWB.write_reg << endl;

        // print EXMEM values
        cout << "EXMEM control";
        EXMEM.control.print();
        cout << "EXMEM pc " << EXMEM.pc << endl;
        cout << "EXMEM zero " << EXMEM.zero << endl;
        cout << "EXMEM alu_result " << EXMEM.alu_result << endl;
        cout << "EXMEM read_data_2 " << EXMEM.read_data_2 << endl;
        cout << "EXMEM write_reg " << EXMEM.write_reg << endl;
        cout << "EXMEM branch_pc " << EXMEM.branch_pc << endl;
        cout << "EXMEM jump_pc " << EXMEM.jump_pc << endl;

        // print IDEX values
        cout << "IDEX control";
        IDEX.control.print();
        cout << "IDEX fwd_read_data_1_EX: " << IDEX.forward_read_data_1_EX << endl;
        cout << "IDEX fwd_read_data_1_MEM: " << IDEX.forward_read_data_1_MEM << endl;
        cout << "IDEX fwd_read_data_2_EX: " << IDEX.forward_read_data_2_EX << endl;
        cout << "IDEX fwd_read_data_2_MEM: " << IDEX.forward_read_data_2_MEM << endl;
        cout << "IDEX read_data_1: " << IDEX.read_data_1 << endl;
        cout << "IDEX read_data_2: " << IDEX.read_data_2 << endl;
        cout << "IDEX imm: " << IDEX.immediate << endl;
        cout << "IDEX funct: " << IDEX.funct << endl;
        cout << "IDEX opcode: " << IDEX.opcode << endl;
        cout << "IDEX shamt: " << IDEX.shamt << endl;
        cout << "IDEX write_reg: " << IDEX.write_reg << endl;
        cout << "IDEX pc: " << IDEX.pc << endl;

        // print IFID values
        cout << "IFID instruction: " << IFID.instruction << endl;
        cout << "IFID PC: " << IFID.pc << endl;

        // end of cycle stuff
        cout << "CYCLE" << num_cycles << "\n";

        reg_file.print(); // used for automated testing

        num_cycles++;

        committed_insts = 1;
        if (stall)
        {
            committed_insts = 0;
        }
        if (flush)
        {
            committed_insts = -1;
        }

        num_instrs += committed_insts;

        if (reg_file.pc >= end_pc + 20)
        { // check if end PC is reached
            break;
        }

        cout << "----------------------------" << endl;
    }

    cout << "CPI = " << (double)num_cycles / (double)num_instrs << "\n";
}

void speculative_main_loop(Registers &reg_file, Memory &memory, uint32_t end_pc)
{
    cout << end_pc << endl;

    // Initialize BPU
    BPU bpu;
    // Initialize ALU
    ALU alu;
    // Initialize Control
    control_t control = {
        .reg_dest = 0,
        .jump = 0,
        .branch = 0,
        .mem_read = 0,
        .mem_to_reg = 0,
        .ALU_op = 0,
        .mem_write = 0,
        .ALU_src = 0,
        .reg_write = 0,
        .branch_not_equal = 0,
        .shift = 0,
        .size = 00,
        //.slt = 00,
        .zero = 0,
    };
    uint32_t num_cycles = 0;
    uint32_t num_instrs = 0;
    int committed_insts = 0;

    // Initialize Pipeline Registers as Structures
    struct IFID1
    {
        uint32_t instruction = 0;
        uint32_t pc = 0;
        bool prediction = false;
    } IFID;

    struct IDEX1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        int forward_read_data_1_EX = 0;
        int forward_read_data_1_MEM = 0;
        int forward_read_data_2_EX = 0;
        int forward_read_data_2_MEM = 0;
        uint32_t read_data_1 = 0;
        uint32_t read_data_2 = 0;
        uint32_t immediate = 0;
        int funct = 0;
        int opcode = 0;
        int shamt = 0;
        int write_reg = 0;
        uint32_t pc = 0;
        bool prediction = false;
    } IDEX;

    struct EXMEM1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        uint32_t pc = 0;
        uint32_t zero = 0;
        uint32_t alu_result = 0;
        uint32_t read_data_2 = 0;
        int write_reg = 0;
        uint32_t branch_pc = 0;
        uint32_t jump_pc = 0;
        bool prediction = false;
    } EXMEM;

    struct MEMWB1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        uint32_t alu_result = 0;
        uint32_t memReadData = 0;
        int write_reg = 0;
    } MEMWB;

    // Initialize Variables
    uint32_t writeback = 0;    // WB: Used to hold writeback value for register writing
    uint32_t dummyVar1 = 0;    // WB: Dummy variable used for register file access read values that we don't care about
    uint32_t memWriteData = 0; // MEM: Used to hold the write value for a store operation
    uint32_t memMask = 0;      // MEM: Used as a mask for half-word and byte memory operations
    uint32_t memReadData = 0;  // MEM: Used to hold the read data from a memory load
    bool flush = false;

    uint32_t ALU_input1 = 0; // EX: Used to hold the value of the first ALU input
    uint32_t ALU_input2 = 0; // EX: Used to hold the value of the second ALU input
    uint32_t zero = 0;       // EX: Used to hold the zero output of the ALU
    uint32_t alu_result = 0; // EX: Used to hold the 32-bit output of the ALU operation
    uint32_t branch_pc = 0;  // EX: Used to hold the branch pc if a branch is to be taken
    uint32_t jump_pc = 0;    // EX: Used to hold the jump pc if a jump is to be taken

    string instr_str;         // ID: Used to hold a casted instruction value
    int opcode = 0;           // ID: Used to hold the opcode value
    int reg_1 = 0;            // ID: Used to store the Rs index
    int reg_2 = 0;            // ID: Used to store the Rt index
    int write_reg = 0;        // ID: Used to store the write register index
    int shamt = 0;            // ID: Used to store the shift bits
    int funct = 0;            // ID: Used to store the function bits
    uint32_t immediate = 0;   // ID: Used to store the immediate value
    uint32_t address = 0;     // ID: Used to jump address
    uint32_t read_data_1 = 0; // ID: Used to hold the value of the first register read
    uint32_t read_data_2 = 0; // ID: Used to hold the value of the second register read
    bool stall = false;       // ID/IF: Used to flag when a stall is needed in the pipeline

    uint32_t instruction = 0; // IF: Used to hold the fetched instruction from memory

    // register
    int MEMWB_control_mem_read = 0;

    // Revision: structure it so that we go backwards. Start with the WB
    // stage. Perform correct writeback, and then "go back" to MEM and perform
    // right logic. Then, once we have finished WB, make sure to THEN update the pipeline
    // Then, repeat. So: to simplify:
    // 1) compute WB, reg_file.access
    // 2) compute MEM, then update  MEMWB values (branch resolution MUX here?)
    // 3) compute EX, then update EXMEM values (forwarding logic + stalling logic)
    // 4) compute ID, update IDEX pipeline values
    // 5) compute IF, update IFID pipeline

    // For stalling: stalling should happen
    while (true)
    {

        // fifth pipeline stage WB: write back -----------------------------------------------

        if (MEMWB.control.mem_to_reg)
        {
            writeback = MEMWB.memReadData;
        }
        else
        {
            writeback = MEMWB.alu_result;
        }
        cout << "writeback: " << writeback << endl;
        dummyVar1 = 0;
        reg_file.access(0, 0, dummyVar1, dummyVar1, MEMWB.write_reg, MEMWB.control.reg_write, writeback);

        // no pipeline to update (last stage)

        // fourth pipeline stage MEM: data memory access--------------------------------------------

        // Branching MUX logic/Resolution: as long as this happens before IF, PC should be correct
        flush = false;
        if ((EXMEM.control.branch == 1 && EXMEM.zero == 1 && !(EXMEM.prediction)) || (EXMEM.control.branch_not_equal == 1 && EXMEM.zero == 0 && !(EXMEM.prediction)))
        {
            cout << "Flushed" << endl;
            reg_file.pc = EXMEM.branch_pc;

            // Flushing IDEX control values
            IDEX.control.reg_dest = 0;
            IDEX.control.jump = 0;
            IDEX.control.branch = 0;
            IDEX.control.mem_read = 0;
            IDEX.control.mem_to_reg = 0;
            IDEX.control.ALU_op = 0;
            IDEX.control.mem_write = 0;
            IDEX.control.ALU_src = 0;
            IDEX.control.reg_write = 0;
            IDEX.control.branch_not_equal = 0;
            IDEX.control.shift = 0;
            IDEX.control.size = 00;
            //.slt = 00,
            IDEX.control.zero = 0;

            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.opcode = 0;
            IDEX.funct = 0;
            IDEX.shamt = 0;
            IDEX.immediate = 0;
            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.write_reg = 0;

            IFID.instruction = 0;

            flush = true;
        }

        if ((EXMEM.control.branch == 1 && EXMEM.zero == 0 && EXMEM.prediction) || (EXMEM.control.branch_not_equal == 1 && EXMEM.zero == 1 && EXMEM.prediction))
        {
            cout << "Flushed" << endl;
            reg_file.pc = EXMEM.pc + 4;

            // Flushing IDEX control values
            IDEX.control.reg_dest = 0;
            IDEX.control.jump = 0;
            IDEX.control.branch = 0;
            IDEX.control.mem_read = 0;
            IDEX.control.mem_to_reg = 0;
            IDEX.control.ALU_op = 0;
            IDEX.control.mem_write = 0;
            IDEX.control.ALU_src = 0;
            IDEX.control.reg_write = 0;
            IDEX.control.branch_not_equal = 0;
            IDEX.control.shift = 0;
            IDEX.control.size = 00;
            //.slt = 00,
            IDEX.control.zero = 0;

            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.opcode = 0;
            IDEX.funct = 0;
            IDEX.shamt = 0;
            IDEX.immediate = 0;
            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.write_reg = 0;

            IFID.instruction = 0;

            flush = true;
        }
        bool taken = false;
        if (EXMEM.prediction != flush)
        {
            taken = true;
        }
        cout << "taken: " << taken << endl;
        cout << "flush: " << flush << endl;
        if (EXMEM.control.branch || EXMEM.control.branch_not_equal)
        {
            cout << "in here branch conditional" << endl;
            bpu.update(EXMEM.pc, taken);
        }

        // if (EXMEM.control.jump == 1)
        // {
        //     reg_file.pc = EXMEM.jump_pc;

        //     // Flushing IDEX control values
        //     IDEX.control.reg_dest = 0;
        //     IDEX.control.jump = 0;
        //     IDEX.control.branch = 0;
        //     IDEX.control.mem_read = 0;
        //     IDEX.control.mem_to_reg = 0;
        //     IDEX.control.ALU_op = 0;
        //     IDEX.control.mem_write = 0;
        //     IDEX.control.ALU_src = 0;
        //     IDEX.control.reg_write = 0;
        //     IDEX.control.branch_not_equal = 0;
        //     IDEX.control.shift = 0;
        //     IDEX.control.size = 00;
        //     //.slt = 00,
        //     IDEX.control.zero = 0;

        //     IDEX.read_data_1 = 0;
        //     IDEX.read_data_2 = 0;
        //     IDEX.opcode = 0;
        //     IDEX.funct = 0;
        //     IDEX.shamt = 0;
        //     IDEX.immediate = 0;
        //     IDEX.read_data_1 = 0;
        //     IDEX.read_data_2 = 0;
        //     IDEX.write_reg = 0;

        //     IFID.instruction = 0;

        //     flush = true;
        // }

        // Store word size MUX
        memWriteData = EXMEM.read_data_2;
        memMask = 0;
        if (EXMEM.control.size == 1)
        { // sh
            memory.access(EXMEM.alu_result, memMask, 0, true, false);
            memMask = (memMask >> 16) << 16;
            memWriteData = (memWriteData << 16) >> 16;
            memWriteData = memWriteData + memMask;
        }
        if (EXMEM.control.size == 2)
        { // sb
            memory.access(EXMEM.alu_result, memMask, 0, true, false);
            memMask = (memMask >> 8) << 8;
            memWriteData = (memWriteData << 24) >> 24;
            memWriteData = memWriteData + memMask;
        }
        memReadData = 0;
        memory.access(EXMEM.alu_result, memReadData, memWriteData, EXMEM.control.mem_read, EXMEM.control.mem_write);
        // memory.print(0, 10);

        // if EXMEM control signals are all 0 due to some stall beforehand, don't update MEMWB with it
        // if (!EXMEM.control.isAllZeros()){
        //     MEMWB.control = EXMEM.control;
        //     MEMWB.alu_result = EXMEM.alu_result;
        //     MEMWB.memReadData = memReadData;
        //     MEMWB.write_reg = EXMEM.write_reg;
        // }

        // third pipeline stage EX: execution or address calculation --------------------------------------
        // forwarding unit logic goes here  ...?
        alu.generate_control_inputs(IDEX.control.ALU_op, IDEX.funct, IDEX.opcode);

        ALU_input1 = IDEX.read_data_1;
        if (IDEX.forward_read_data_1_MEM && MEMWB.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input1 = MEMWB.alu_result;
            cout << "alu1 aluRes" << endl;
        }
        if (IDEX.forward_read_data_1_EX)
        {
            ALU_input1 = EXMEM.alu_result;
        }

        if (IDEX.forward_read_data_1_MEM && MEMWB.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input1 = MEMWB.memReadData;
            cout << "alu1 memread" << endl;
        }

        ALU_input2 = IDEX.read_data_2;
        if (IDEX.forward_read_data_2_MEM && MEMWB.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input2 = MEMWB.alu_result;
            if (IDEX.control.mem_write == 1)
            {
                IDEX.read_data_2 = MEMWB.alu_result;
            }
            cout << "alu2 aluRes" << endl;
        }
        if (IDEX.forward_read_data_2_EX)
        {
            ALU_input2 = EXMEM.alu_result;
            if (IDEX.control.mem_write == 1)
            {
                IDEX.read_data_2 = EXMEM.alu_result;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX.forward_read_data_2_MEM && MEMWB.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input2 = MEMWB.memReadData;
            cout << "alu2 memread" << endl;
            if (IDEX.control.mem_write == 1)
            {
                IDEX.read_data_2 = MEMWB.memReadData;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX.control.ALU_src == 1)
        {
            ALU_input2 = IDEX.immediate;
        }

        if (IDEX.control.shift == 1)
        {
            ALU_input1 = IDEX.shamt;
        }

        alu_result = alu.execute(ALU_input1, ALU_input2, zero);
        cout << "ALU Result " << alu_result << endl;

        // Branch pc
        branch_pc = (IDEX.immediate << 2) + IDEX.pc + 4;
        cout << "(IDEX.immediate << 2)" << (IDEX.immediate << 2) << endl;
        jump_pc = IDEX.read_data_1;

        // second pipeline stage ID: instruction decode and reg file read --------------------------
        // hazard detection occurs here

        // decode into contol signals
        instr_str = bitset<32>(IFID.instruction).to_string();
        cout << "instr_str: " << instr_str << endl;

        // set IDEX control to the decoded IFID instruction
        control.decode(IFID.instruction);
        // cout << "IDEX control sigs: " << endl;
        // control.print(); // used for autograding

        // break down the instruction
        opcode = stoul(instr_str.substr(0, 6), 0, 2); // bits 32-26 or Opcode

        reg_1 = stoul(instr_str.substr(6, 5), 0, 2);  // bits 25-21 or Rs
        reg_2 = stoul(instr_str.substr(11, 5), 0, 2); // bits 20-16 or Rt
        cout << "reg 1: " << reg_1 << " reg 2: " << reg_2 << endl;

        if (control.reg_dest == 0)
        { // rt is dst reg
            write_reg = reg_2;
        }
        else if (control.mem_write == 0)
        {                                                     // rd is dst reg
            write_reg = stoul(instr_str.substr(16, 5), 0, 2); // bits 15-11 or Rd
        }
        else
        {
            write_reg = 0;
        }
        shamt = stoul(instr_str.substr(21, 5), 0, 2); // bits 10-6 or shift amount
        funct = stoul(instr_str.substr(26, 6), 0, 2); // bits 5-0 or function

        immediate = stoul(instr_str.substr(16, 16), 0, 2); // 16 bit immediate field
        address = stoul(instr_str.substr(6, 26), 0, 2);    // address (should it be uint32?)

        // TODO: fill in the function argument
        // Read from reg file

        reg_file.access(reg_1, reg_2, read_data_1, read_data_2, MEMWB.write_reg, false, 0);

        if (!(control.zero))
        { // shift 16 bit imm to 32-bit
            if (immediate > (pow(2, 15) - 1))
            {
                immediate = immediate + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
        }

        // idex update was here

        // Forwarding Logic
        IDEX.forward_read_data_1_EX = 0;
        IDEX.forward_read_data_1_MEM = 0;
        IDEX.forward_read_data_2_EX = 0;
        IDEX.forward_read_data_2_MEM = 0;
        // Stalling logic
        stall = false;
        if (((reg_1 == IDEX.write_reg && reg_1 != 0) || (reg_2 == IDEX.write_reg && control.reg_dest == 1 && reg_2 != 0)) && (IDEX.opcode == 35 || IDEX.opcode == 36 || IDEX.opcode == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall = true;
        }
        else
        {
            if (reg_1 == IDEX.write_reg && reg_1 != 0 && IDEX.control.reg_write != 0)
            {
                IDEX.forward_read_data_1_EX = 1;
            }
            if (reg_1 == EXMEM.write_reg && reg_1 != 0 && EXMEM.control.reg_write != 0)
            {
                IDEX.forward_read_data_1_MEM = 1;
            }
            if (reg_2 == IDEX.write_reg && reg_2 != 0 && IDEX.control.reg_write != 0)
            {
                IDEX.forward_read_data_2_EX = 1;
            }
            if (reg_2 == EXMEM.write_reg && reg_2 != 0 && EXMEM.control.reg_write != 0)
            {
                IDEX.forward_read_data_2_MEM = 1;
            }
        }
        // if (num_cycles == 7)
        // {

        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 31, true, opcode);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 30, true, funct);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 29, true, IFID.instruction);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 28, true, EXMEM.alu_result);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 27, true, memReadData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 26, true, EXMEM.write_reg);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 25, true, memWriteData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 24, true, EXMEM.control.mem_write);
        // }

        // First stage: IF / Fetch ----------------------------------
        if (reg_file.pc >= end_pc)
        {
            instruction = 0;
        }
        else
        {
            cout << "reg_file.pc: " << reg_file.pc << endl;
            memory.access(reg_file.pc, instruction, 0, 1, 0);
        }

        // ifid update was here

        // IFID.pc = reg_file.pc;
        cout << endl;
        MEMWB.control = EXMEM.control;
        MEMWB.alu_result = EXMEM.alu_result;
        MEMWB.memReadData = memReadData;
        MEMWB.write_reg = EXMEM.write_reg;
        // update EXMEM Pipeline
        EXMEM.control = IDEX.control;
        EXMEM.pc = IDEX.pc;
        EXMEM.zero = zero;
        EXMEM.alu_result = alu_result;
        EXMEM.read_data_2 = IDEX.read_data_2;
        EXMEM.write_reg = IDEX.write_reg;
        EXMEM.branch_pc = branch_pc;
        EXMEM.jump_pc = jump_pc;
        EXMEM.prediction = IDEX.prediction;

        // Stalling logic

        if (stall)
        {
            cout << "we're stalling" << endl;
            // Flushing IDEX control values
            IDEX.control.reg_dest = 0;
            IDEX.control.jump = 0;
            IDEX.control.branch = 0;
            IDEX.control.mem_read = 0;
            IDEX.control.mem_to_reg = 0;
            IDEX.control.ALU_op = 0;
            IDEX.control.mem_write = 0;
            IDEX.control.ALU_src = 0;
            IDEX.control.reg_write = 0;
            IDEX.control.branch_not_equal = 0;
            IDEX.control.shift = 0;
            IDEX.control.size = 00;
            //.slt = 00,
            IDEX.control.zero = 0;

            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.opcode = 0;
            IDEX.funct = 0;
            IDEX.shamt = 0;
            IDEX.immediate = 0;
            IDEX.read_data_1 = 0;
            IDEX.read_data_2 = 0;
            IDEX.write_reg = 0;
        }

        else
        {

            IDEX.control.reg_dest = control.reg_dest;
            IDEX.control.jump = control.jump;
            IDEX.control.branch = control.branch;
            IDEX.control.mem_read = control.mem_read;
            IDEX.control.mem_to_reg = control.mem_to_reg;
            IDEX.control.ALU_op = control.ALU_op;
            IDEX.control.mem_write = control.mem_write;
            IDEX.control.ALU_src = control.ALU_src;
            IDEX.control.reg_write = control.reg_write;
            IDEX.control.branch_not_equal = control.branch_not_equal;
            IDEX.control.shift = control.shift;
            IDEX.control.size = control.size;
            IDEX.control.zero = control.zero;

            IDEX.read_data_1 = read_data_1;
            IDEX.read_data_2 = read_data_2;
            IDEX.pc = IFID.pc;
            IDEX.opcode = opcode;
            IDEX.funct = funct;
            IDEX.shamt = shamt;
            IDEX.immediate = immediate;
            IDEX.read_data_1 = read_data_1;
            IDEX.read_data_2 = read_data_2;
            IDEX.write_reg = write_reg;
            IDEX.prediction = IFID.prediction;
        }

        // if not stalling, just update IFID pipeline to fetched values. otherwise
        // set them to the NOP stuff
        if (!(stall))
        {
            IFID.pc = reg_file.pc;
            reg_file.pc += 4;
            IFID.instruction = instruction;

            string instr_str1 = bitset<32>(IFID.instruction).to_string();
            IFID.prediction = false;
            int opcode1 = stoul(instr_str1.substr(0, 6), 0, 2);
            int funct1 = stoul(instr_str1.substr(26, 6), 0, 2);
            uint32_t immediate1 = stoul(instr_str1.substr(16, 16), 0, 2); // 16 bit immediate field
            int reg_11 = stoul(instr_str1.substr(6, 5), 0, 2);            // bits 25-21 or Rs
            uint32_t read_data_11 = 0;
            reg_file.access(reg_11, 0, read_data_11, dummyVar1, 0, false, 0);
            if (immediate1 > (pow(2, 15) - 1))
            {
                immediate1 = immediate1 + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
            cout << "imm1: " << immediate1 << endl;
            if (opcode1 == 4 || opcode1 == 5)
            {
                IFID.prediction = bpu.predict(IFID.pc);
                if (IFID.prediction)
                {
                    reg_file.pc = (immediate1 << 2) + reg_file.pc;
                }
            }
            if (opcode == 0 && funct == 8)
            {
                reg_file.pc = read_data_11;
            }
            cout << "reg file pc after IFID: " << reg_file.pc << endl;
        }

        // print MEMWB values
        // cout << "MEMWB Control: ";
        // MEMWB.control.print();
        // cout << "MEMWB alu_result: " << MEMWB.alu_result << endl;
        // cout << "MEMWB mmeReadData: " << MEMWB.memReadData << endl;
        // cout << "MEMWB write_reg: " << MEMWB.write_reg << endl;

        // // print EXMEM values
        // cout << "EXMEM control";
        // EXMEM.control.print();
        // cout << "EXMEM pc " << EXMEM.pc << endl;
        // cout << "EXMEM zero " << EXMEM.zero << endl;
        // cout << "EXMEM alu_result " << EXMEM.alu_result << endl;
        // cout << "EXMEM read_data_2 " << EXMEM.read_data_2 << endl;
        // cout << "EXMEM write_reg " << EXMEM.write_reg << endl;
        // cout << "EXMEM branch_pc " << EXMEM.branch_pc << endl;
        // cout << "EXMEM jump_pc " << EXMEM.jump_pc << endl;
        cout << "EXMEM prediction " << EXMEM.prediction << endl;
        // print IDEX values
        // cout << "IDEX control";
        // IDEX.control.print();
        // cout << "IDEX fwd_read_data_1_EX: " << IDEX.forward_read_data_1_EX << endl;
        // cout << "IDEX fwd_read_data_1_MEM: " << IDEX.forward_read_data_1_MEM << endl;
        // cout << "IDEX fwd_read_data_2_EX: " << IDEX.forward_read_data_2_EX << endl;
        // cout << "IDEX fwd_read_data_2_MEM: " << IDEX.forward_read_data_2_MEM << endl;
        // cout << "IDEX read_data_1: " << IDEX.read_data_1 << endl;
        // cout << "IDEX read_data_2: " << IDEX.read_data_2 << endl;
        // cout << "IDEX imm: " << IDEX.immediate << endl;
        // cout << "IDEX funct: " << IDEX.funct << endl;
        // cout << "IDEX opcode: " << IDEX.opcode << endl;
        // cout << "IDEX shamt: " << IDEX.shamt << endl;
        // cout << "IDEX write_reg: " << IDEX.write_reg << endl;
        // cout << "IDEX pc: " << IDEX.pc << endl;
        cout << "IDEX prediction " << IDEX.prediction << endl;
        // print IFID values
        // cout << "IFID instruction: " << IFID.instruction << endl;
        // cout << "IFID PC: " << IFID.pc << endl;
        cout << "IFID prediction " << IFID.prediction << endl;
        // bpu.print();
        //   end of cycle stuff
        cout << "CYCLE" << num_cycles << "\n";

        reg_file.print(); // used for automated testing

        num_cycles++;
        committed_insts = 1;
        if (stall)
        {
            committed_insts = 0;
        }
        if (flush)
        {
            committed_insts = -1;
        }

        num_instrs += committed_insts;

        if (reg_file.pc >= end_pc + 20)
        { // check if end PC is reached
            break;
        }
        // if (num_cycles == 250)
        // {
        //     break;
        // }
        cout << "----------------------------" << endl;
    }

    cout << "CPI = " << (double)num_cycles / (double)num_instrs << "\n";
}

void io_superscalar_main_loop(Registers &reg_file, Memory &memory, uint32_t end_pc)
// 2-way in-order SUPERSCALAR. From the instructions:
// Again, start with the pipelined machine (which is already inorder) and convert it to a dual-issue superscalar
// design. For the most part, the implementation would simply involve replicating pipeline registers, so two instructions can be processed simultaneously.
// Note that you might have to deal with concurrent execution hazards and add additional forwarding paths as discussed in class
{
    cout << end_pc << endl;
    // Read in two instructions at IFID, then basically have INDEPENDENT execution of each, with the exceptions of hazards and dependencies.

    // Initialize BPU
    BPU bpu1;
    BPU bpu2;
    // Initialize ALU
    ALU alu1;
    ALU alu2;
    // Initialize Control
    control_t control1 = {
        .reg_dest = 0,
        .jump = 0,
        .branch = 0,
        .mem_read = 0,
        .mem_to_reg = 0,
        .ALU_op = 0,
        .mem_write = 0,
        .ALU_src = 0,
        .reg_write = 0,
        .branch_not_equal = 0,
        .shift = 0,
        .size = 00,
        //.slt = 00,
        .zero = 0,
    };
    control_t control2 = {
        .reg_dest = 0,
        .jump = 0,
        .branch = 0,
        .mem_read = 0,
        .mem_to_reg = 0,
        .ALU_op = 0,
        .mem_write = 0,
        .ALU_src = 0,
        .reg_write = 0,
        .branch_not_equal = 0,
        .shift = 0,
        .size = 00,
        //.slt = 00,
        .zero = 0,
    };
    uint32_t num_cycles = 0;
    uint32_t num_instrs = 0;
    int committed_insts = 0;

    // Initialize Pipeline Registers as Structures
    // For 2-way, create 2 seperate pipelines, for the two instructions executing. Naming scheme: pipelinename1 for the first instruction (in a pipeline diagram, this would be the one "on top"). pipelinename2 for the second instruction (one on the bottom)
    struct IFID1
    {
        uint32_t instruction = 0;
        uint32_t pc = 0;
        bool prediction = false;
    } IFID1, IFID2;

    struct IDEX1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        int forward_read_data_1_EX1 = 0;
        int forward_read_data_1_MEM1 = 0;
        int forward_read_data_2_EX1 = 0;
        int forward_read_data_2_MEM1 = 0;
        int forward_read_data_1_EX2 = 0;
        int forward_read_data_1_MEM2 = 0;
        int forward_read_data_2_EX2 = 0;
        int forward_read_data_2_MEM2 = 0;
        uint32_t read_data_1 = 0;
        uint32_t read_data_2 = 0;
        uint32_t immediate = 0;
        int funct = 0;
        int opcode = 0;
        int shamt = 0;
        int write_reg = 0;
        uint32_t pc = 0;
        bool prediction = false;
    } IDEX1, IDEX2;


    struct EXMEM1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        uint32_t pc = 0;
        uint32_t zero = 0;
        uint32_t alu_result = 0;
        uint32_t read_data_2 = 0;
        int write_reg = 0;
        uint32_t branch_pc = 0;
        uint32_t jump_pc = 0;
        bool prediction = false;
    } EXMEM1, EXMEM2;


    struct MEMWB1
    {
        control_t control = {
            .reg_dest = 0,
            .jump = 0,
            .branch = 0,
            .mem_read = 0,
            .mem_to_reg = 0,
            .ALU_op = 0,
            .mem_write = 0,
            .ALU_src = 0,
            .reg_write = 0,
            .branch_not_equal = 0,
            .shift = 0,
            .size = 00,
            //.slt = 00,
            .zero = 0,
        };
        uint32_t alu_result = 0;
        uint32_t memReadData = 0;
        int write_reg = 0;
    } MEMWB1,MEMWB2;

    // pipeline 1 variables for WB
    uint32_t writeback1 = 0; // WB: Used to hold writeback value for register writing
    uint32_t dummyVar1 = 0;  // WB: Dummy variable used for register file access read values that we don't care about. dummyVar wil be used for both pipelines, since we never save / actually use it

    // pipeline 2 variables for WB
    uint32_t writeback2 = 0; // WB: Used to hold writeback value for register writing

    // pipeline 1 variables for MEM
    uint32_t memWriteData1 = 0; // MEM: Used to hold the write value for a store operation
    uint32_t memMask1 = 0;      // MEM: Used as a mask for half-word and byte memory operations
    uint32_t memReadData1 = 0;  // MEM: Used to hold the read data from a memory load
    bool flush1 = false;

    // pipeline 2 variables for MEM
    uint32_t memWriteData2 = 0; // MEM: Used to hold the write value for a store operation
    uint32_t memMask2 = 0;      // MEM: Used as a mask for half-word and byte memory operations
    uint32_t memReadData2 = 0;  // MEM: Used to hold the read data from a memory load
    bool flush2 = false;

    // pipeline 1 variables for EX
    uint32_t ALU_input1_1 = 0; // EX: Used to hold the value of the first ALU input
    uint32_t ALU_input2_1 = 0; // EX: Used to hold the value of the second ALU input
    uint32_t zero1 = 0;        // EX: Used to hold the zero output of the ALU
    uint32_t alu_result1 = 0;  // EX: Used to hold the 32-bit output of the ALU operation
    uint32_t branch_pc1 = 0;   // EX: Used to hold the branch pc if a branch is to be taken
    uint32_t jump_pc1 = 0;     // EX: Used to hold the jump pc if a jump is to be taken

    // pipeline 2 variables for EX
    uint32_t ALU_input1_2 = 0;
    uint32_t ALU_input2_2 = 0;
    uint32_t zero2 = 0;
    uint32_t alu_result2 = 0;
    uint32_t branch_pc2 = 0;
    uint32_t jump_pc2 = 0;

    // pipeline 1 variables for ID
    string instr_str1;          // ID: Used to hold a casted instruction value
    int opcode1 = 0;            // ID: Used to hold the opcode value
    int reg_1_1 = 0;            // ID: Used to store the Rs index
    int reg_2_1 = 0;            // ID: Used to store the Rt index
    int write_reg1 = 0;         // ID: Used to store the write register index
    int shamt1 = 0;             // ID: Used to store the shift bits
    int funct1 = 0;             // ID: Used to store the function bits
    uint32_t immediate1 = 0;    // ID: Used to store the immediate value
    uint32_t address1 = 0;      // ID: Used to jump address
    uint32_t read_data_1_1 = 0; // ID: Used to hold the value of the first register read
    uint32_t read_data_2_1 = 0; // ID: Used to hold the value of the second register read
    bool stall1 = false;        // ID/IF: Used to flag when a stall is needed in the pipeline

    // pipeline 2 variables for ID
    string instr_str2;
    int opcode2 = 0;
    int reg_1_2 = 0;
    int reg_2_2 = 0;
    int write_reg2 = 0;
    int shamt2 = 0;
    int funct2 = 0;
    uint32_t immediate2 = 0;
    uint32_t address2 = 0;
    uint32_t read_data_1_2 = 0;
    uint32_t read_data_2_2 = 0;
    bool stall2 = false;

    uint32_t instruction1 = 0; // IF: Used to hold the fetched instruction from memory
    uint32_t instruction2 = 0;

    // Revision: structure it so that we go backwards. Start with the WB
    // stage. Perform correct writeback, and then "go back" to MEM and perform
    // right logic. Then, once we have finished WB, make sure to THEN update the pipeline
    // Then, repeat. So: to simplify:
    // 1) compute WB, reg_file.access
    // 2) compute MEM, then update  MEMWB values (branch resolution MUX here?)
    // 3) compute EX, then update EXMEM values (forwarding logic + stalling logic)
    // 4) compute ID, update IDEX pipeline values
    // 5) compute IF, update IFID pipeline

    while (true)
    {

        // fifth pipeline stage WB: #1 write back -----------------------------------------------

        if (MEMWB1.control.mem_to_reg)
        {
            writeback1 = MEMWB1.memReadData;
        }
        else
        {
            writeback1 = MEMWB1.alu_result;
        }
        cout << "writeback1: " << writeback1 << endl;
        dummyVar1 = 0;
        reg_file.access(0, 0, dummyVar1, dummyVar1, MEMWB1.write_reg, MEMWB1.control.reg_write, writeback1);

        // fifth pipeline stage WB: #2 write back -----------------------------------------------

        if (MEMWB2.control.mem_to_reg)
        {
            writeback2 = MEMWB2.memReadData;
        }
        else
        {
            writeback2 = MEMWB2.alu_result;
        }
        cout << "writeback2: " << writeback2 << endl;
        dummyVar1 = 0;
        reg_file.access(0, 0, dummyVar1, dummyVar1, MEMWB2.write_reg, MEMWB2.control.reg_write, writeback2);

        // no pipeline to update (last stage)

        // fourth pipeline stage MEM: #1 data memory access--------------------------------------------

        // Branching MUX logic/Resolution: as long as this happens before IF, PC should be correct
        flush1 = false;
        if ((EXMEM1.control.branch == 1 && EXMEM1.zero == 1 && !(EXMEM1.prediction)) || (EXMEM1.control.branch_not_equal == 1 && EXMEM1.zero == 0 && !(EXMEM1.prediction)))
        {
            cout << "Flushed" << endl;
            reg_file.pc = EXMEM1.branch_pc;

            // Flushing IDEX control values
            IDEX1.control.reg_dest = 0;
            IDEX1.control.jump = 0;
            IDEX1.control.branch = 0;
            IDEX1.control.mem_read = 0;
            IDEX1.control.mem_to_reg = 0;
            IDEX1.control.ALU_op = 0;
            IDEX1.control.mem_write = 0;
            IDEX1.control.ALU_src = 0;
            IDEX1.control.reg_write = 0;
            IDEX1.control.branch_not_equal = 0;
            IDEX1.control.shift = 0;
            IDEX1.control.size = 00;
            //.slt = 00,
            IDEX1.control.zero = 0;

            IDEX1.read_data_1 = 0;
            IDEX1.read_data_2 = 0;
            IDEX1.opcode = 0;
            IDEX1.funct = 0;
            IDEX1.shamt = 0;
            IDEX1.immediate = 0;
            IDEX1.read_data_1 = 0;
            IDEX1.read_data_2 = 0;
            IDEX1.write_reg = 0;

            IFID1.instruction = 0;

            flush1 = true;
        }

        if ((EXMEM1.control.branch == 1 && EXMEM1.zero == 0 && EXMEM1.prediction) || (EXMEM1.control.branch_not_equal == 1 && EXMEM1.zero == 1 && EXMEM1.prediction))
        {
            cout << "Flushed" << endl;
            reg_file.pc = EXMEM1.pc + 4;

            // Flushing IDEX control values
            IDEX1.control.reg_dest = 0;
            IDEX1.control.jump = 0;
            IDEX1.control.branch = 0;
            IDEX1.control.mem_read = 0;
            IDEX1.control.mem_to_reg = 0;
            IDEX1.control.ALU_op = 0;
            IDEX1.control.mem_write = 0;
            IDEX1.control.ALU_src = 0;
            IDEX1.control.reg_write = 0;
            IDEX1.control.branch_not_equal = 0;
            IDEX1.control.shift = 0;
            IDEX1.control.size = 00;
            //.slt = 00,
            IDEX1.control.zero = 0;

            IDEX1.read_data_1 = 0;
            IDEX1.read_data_2 = 0;
            IDEX1.opcode = 0;
            IDEX1.funct = 0;
            IDEX1.shamt = 0;
            IDEX1.immediate = 0;
            IDEX1.read_data_1 = 0;
            IDEX1.read_data_2 = 0;
            IDEX1.write_reg = 0;

            IFID1.instruction = 0;

            flush1 = true;
        }
        bool taken1 = false;
        if (EXMEM1.prediction != flush1)
        {
            taken1 = true;
        }
        cout << "taken1: " << taken1 << endl;
        cout << "flush1: " << flush1 << endl;
        if (EXMEM1.control.branch || EXMEM1.control.branch_not_equal)
        {
            cout << "in here branch conditional 1" << endl;
            bpu1.update(EXMEM1.pc, taken1);
        }

        // Store word size MUX
        memWriteData1 = EXMEM1.read_data_2;
        memMask1 = 0;
        if (EXMEM1.control.size == 1)
        { // sh
            memory.access(EXMEM1.alu_result, memMask1, 0, true, false);
            memMask1 = (memMask1 >> 16) << 16;
            memWriteData1 = (memWriteData1 << 16) >> 16;
            memWriteData1 = memWriteData1 + memMask1;
        }
        if (EXMEM1.control.size == 2)
        { // sb
            memory.access(EXMEM1.alu_result, memMask1, 0, true, false);
            memMask1 = (memMask1 >> 8) << 8;
            memWriteData1 = (memWriteData1 << 24) >> 24;
            memWriteData1 = memWriteData1 + memMask1;
        }
        memReadData1 = 0;
        memory.access(EXMEM1.alu_result, memReadData1, memWriteData1, EXMEM1.control.mem_read, EXMEM1.control.mem_write);
        // memory.print(0, 10);

        // fourth pipeline stage MEM: #2--------------------------------------------

        // Branching MUX logic/Resolution: as long as this happens before IF, PC should be correct
        flush2 = false;
        if ((EXMEM2.control.branch == 1 && EXMEM2.zero == 1 && !(EXMEM2.prediction)) || (EXMEM2.control.branch_not_equal == 1 && EXMEM2.zero == 0 && !(EXMEM2.prediction)))
        {
            cout << "Flushed" << endl;
            reg_file.pc = EXMEM2.branch_pc;

            // Flushing IDEX control values
            IDEX2.control.reg_dest = 0;
            IDEX2.control.jump = 0;
            IDEX2.control.branch = 0;
            IDEX2.control.mem_read = 0;
            IDEX2.control.mem_to_reg = 0;
            IDEX2.control.ALU_op = 0;
            IDEX2.control.mem_write = 0;
            IDEX2.control.ALU_src = 0;
            IDEX2.control.reg_write = 0;
            IDEX2.control.branch_not_equal = 0;
            IDEX2.control.shift = 0;
            IDEX2.control.size = 00;
            //.slt = 00,
            IDEX2.control.zero = 0;

            IDEX2.read_data_1 = 0;
            IDEX2.read_data_2 = 0;
            IDEX2.opcode = 0;
            IDEX2.funct = 0;
            IDEX2.shamt = 0;
            IDEX2.immediate = 0;
            IDEX2.read_data_1 = 0;
            IDEX2.read_data_2 = 0;
            IDEX2.write_reg = 0;

            IFID2.instruction = 0;

            if ()
            flush2 = true;
        }

        if ((EXMEM2.control.branch == 1 && EXMEM2.zero == 0 && EXMEM2.prediction) || (EXMEM2.control.branch_not_equal == 1 && EXMEM2.zero == 1 && EXMEM2.prediction))
        {
            cout << "Flushed" << endl;
            reg_file.pc = EXMEM2.pc + 4;

            // Flushing IDEX control values
            IDEX2.control.reg_dest = 0;
            IDEX2.control.jump = 0;
            IDEX2.control.branch = 0;
            IDEX2.control.mem_read = 0;
            IDEX2.control.mem_to_reg = 0;
            IDEX2.control.ALU_op = 0;
            IDEX2.control.mem_write = 0;
            IDEX2.control.ALU_src = 0;
            IDEX2.control.reg_write = 0;
            IDEX2.control.branch_not_equal = 0;
            IDEX2.control.shift = 0;
            IDEX2.control.size = 00;
            //.slt = 00,
            IDEX2.control.zero = 0;

            IDEX2.read_data_1 = 0;
            IDEX2.read_data_2 = 0;
            IDEX2.opcode = 0;
            IDEX2.funct = 0;
            IDEX2.shamt = 0;
            IDEX2.immediate = 0;
            IDEX2.read_data_1 = 0;
            IDEX2.read_data_2 = 0;
            IDEX2.write_reg = 0;

            IFID2.instruction = 0;

            flush2 = true;
        }
        bool taken2 = false;
        if (EXMEM2.prediction != flush2)
        {
            taken2 = true;
        }
        cout << "taken2: " << taken2 << endl;
        cout << "flush2: " << flush2 << endl;
        if (EXMEM2.control.branch || EXMEM2.control.branch_not_equal)
        {
            cout << "in here branch conditional 2" << endl;
            bpu2.update(EXMEM2.pc, taken2);
        }

        // Store word size MUX
        memWriteData2 = EXMEM2.read_data_2;
        memMask2 = 0;
        if (EXMEM2.control.size == 1)
        { // sh
            memory.access(EXMEM2.alu_result, memMask2, 0, true, false);
            memMask2 = (memMask2 >> 16) << 16;
            memWriteData2 = (memWriteData2 << 16) >> 16;
            memWriteData2 = memWriteData2 + memMask2;
        }
        if (EXMEM2.control.size == 2)
        { // sb
            memory.access(EXMEM2.alu_result, memMask2, 0, true, false);
            memMask2 = (memMask2 >> 8) << 8;
            memWriteData2 = (memWriteData2 << 24) >> 24;
            memWriteData2 = memWriteData2 + memMask2;
        }
        memReadData2 = 0;
        memory.access(EXMEM2.alu_result, memReadData2, memWriteData2, EXMEM2.control.mem_read, EXMEM2.control.mem_write);
        // memory.print(0, 10);

        // third pipeline stage EX: #1 execution or address calculation --------------------------------------
        // forwarding unit logic goes here  ...?
        alu1.generate_control_inputs(IDEX1.control.ALU_op, IDEX1.funct, IDEX1.opcode);

        ALU_input1_1 = IDEX1.read_data_1;
        if (IDEX1.forward_read_data_1_MEM1 && MEMWB1.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input1_1 = MEMWB1.alu_result;
            cout << "alu1 aluRes" << endl;
        }
        if (IDEX1.forward_read_data_1_EX1)
        {
            ALU_input1_1 = EXMEM1.alu_result;
        }

        if (IDEX1.forward_read_data_1_MEM1 && MEMWB1.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input1_1 = MEMWB1.memReadData;
            cout << "alu1 memread" << endl;
        }

        ALU_input2_1 = IDEX1.read_data_2;
        if (IDEX1.forward_read_data_2_MEM1 && MEMWB1.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input2_1 = MEMWB1.alu_result;
            if (IDEX1.control.mem_write == 1)
            {
                IDEX1.read_data_2 = MEMWB1.alu_result;
            }
            cout << "alu2 aluRes" << endl;
        }
        if (IDEX1.forward_read_data_2_EX1)
        {
            ALU_input2_1 = EXMEM1.alu_result;
            if (IDEX1.control.mem_write == 1)
            {
                IDEX1.read_data_2 = EXMEM1.alu_result;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX1.forward_read_data_2_MEM1 && MEMWB1.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input2_1 = MEMWB1.memReadData;
            cout << "alu2 memread" << endl;
            if (IDEX1.control.mem_write == 1)
            {
                IDEX1.read_data_2 = MEMWB1.memReadData;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX1.control.ALU_src == 1)
        {
            ALU_input2_1 = IDEX1.immediate;
        }

        if (IDEX1.control.shift == 1)
        {
            ALU_input1_1 = IDEX1.shamt;
        }

        alu_result1 = alu1.execute(ALU_input1_1, ALU_input2_1, zero1);
        cout << "ALU Result " << alu_result1 << endl;

        // Branch pc
        branch_pc1 = (IDEX1.immediate << 2) + IDEX1.pc + 4;
        cout << "(IDEX1.immediate << 2)" << (IDEX1.immediate << 2) << endl;
        jump_pc1 = IDEX1.read_data_1;

        // third pipeline stage EX: #2 --------------------------------------

        alu2.generate_control_inputs(IDEX2.control.ALU_op, IDEX2.funct, IDEX2.opcode);

        ALU_input1_2 = IDEX2.read_data_1;
        if (IDEX2.forward_read_data_1_MEM1 && MEMWB2.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input1_2 = MEMWB2.alu_result;
            cout << "alu1 aluRes" << endl;
        }
        if (IDEX2.forward_read_data_1_EX1)
        {
            ALU_input1_2 = EXMEM2.alu_result;
        }

        if (IDEX2.forward_read_data_1_MEM1 && MEMWB2.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input1_2 = MEMWB2.memReadData;
            cout << "alu1 memread" << endl;
        }

        ALU_input2_2 = IDEX2.read_data_2;
        if (IDEX2.forward_read_data_2_MEM1 && MEMWB2.control.mem_read == 0)
        { // if not a LW instruction, forward the ALU result
            ALU_input2_2 = MEMWB2.alu_result;
            if (IDEX2.control.mem_write == 1)
            {
                IDEX2.read_data_2 = MEMWB2.alu_result;
            }
            cout << "alu2 aluRes" << endl;
        }
        if (IDEX2.forward_read_data_2_EX1)
        {
            ALU_input2_2 = EXMEM2.alu_result;
            if (IDEX2.control.mem_write == 1)
            {
                IDEX2.read_data_2 = EXMEM2.alu_result;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX2.forward_read_data_2_MEM1 && MEMWB2.control.mem_read == 1)
        { // if it is a LW instruction, forward the memReadData
            ALU_input2_2 = MEMWB2.memReadData;
            cout << "alu2 memread" << endl;
            if (IDEX2.control.mem_write == 1)
            {
                IDEX2.read_data_2 = MEMWB2.memReadData;
            } // need this extra thing here because read_data_2 also takes in forwarding but for SW doesn't go to alu input
        }

        if (IDEX2.control.ALU_src == 1)
        {
            ALU_input2_2 = IDEX2.immediate;
        }

        if (IDEX2.control.shift == 1)
        {
            ALU_input1_2 = IDEX2.shamt;
        }

        alu_result2 = alu2.execute(ALU_input1_2, ALU_input2_2, zero2);
        cout << "ALU Result " << alu_result2 << endl;

        // Branch pc
        branch_pc2 = (IDEX2.immediate << 2) + IDEX2.pc + 8;
        cout << "(IDEX2.immediate << 2)" << (IDEX2.immediate << 2) << endl;
        jump_pc2 = IDEX2.read_data_1;

        // second pipeline stage ID #1 --------------------------
        // hazard detection occurs here
        // original variables were instr_str for example, but since we can have two possible instructions that each need synchronous decoding, we're gonna have to distinguish instr_str1 from instr_str2 since we need to push each of those values into a pipeline

        // decode into contol signals
        instr_str1 = bitset<32>(IFID1.instruction).to_string();
        cout << "instr_str: " << instr_str1 << endl;

        // set IDEX control to the decoded IFID instruction
        control1.decode(IFID1.instruction);
        // cout << "IDEX control sigs: " << endl;
        // control.print(); // used for autograding

        // break down the instruction
        opcode1 = stoul(instr_str1.substr(0, 6), 0, 2); // bits 32-26 or Opcode

        reg_1_1 = stoul(instr_str1.substr(6, 5), 0, 2);  // bits 25-21 or Rs
        reg_2_1 = stoul(instr_str1.substr(11, 5), 0, 2); // bits 20-16 or Rt
        cout << "reg 1: " << reg_1_1 << " reg 2: " << reg_2_1 << endl;

        if (control1.reg_dest == 0)
        { // rt is dst reg
            write_reg1 = reg_2_1;
        }
        else if (control1.mem_write == 0)
        {                                                       // rd is dst reg
            write_reg1 = stoul(instr_str1.substr(16, 5), 0, 2); // bits 15-11 or Rd
        }
        else
        {
            write_reg1 = 0;
        }
        shamt1 = stoul(instr_str1.substr(21, 5), 0, 2); // bits 10-6 or shift amount
        funct1 = stoul(instr_str1.substr(26, 6), 0, 2); // bits 5-0 or function

        immediate1 = stoul(instr_str1.substr(16, 16), 0, 2); // 16 bit immediate field
        address1 = stoul(instr_str1.substr(6, 26), 0, 2);    // address (should it be uint32?)

        // TODO: fill in the function argument
        // Read from reg file

        reg_file.access(reg_1_1, reg_2_1, read_data_1_1, read_data_2_1, MEMWB1.write_reg, false, 0);

        if (!(control1.zero))
        { // shift 16 bit imm to 32-bit
            if (immediate1 > (pow(2, 15) - 1))
            {
                immediate1 = immediate1 + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
        }

        // idex update was here

        // Nothing done for the stalling / forwarding logic
        // Forwarding Logic
        IDEX1.forward_read_data_1_EX1 = 0;
        IDEX1.forward_read_data_1_MEM1 = 0;
        IDEX1.forward_read_data_2_EX1 = 0;
        IDEX1.forward_read_data_2_MEM1 = 0;
        IDEX1.forward_read_data_1_EX2 = 0;
        IDEX1.forward_read_data_1_MEM2 = 0;
        IDEX1.forward_read_data_2_EX2 = 0;
        IDEX1.forward_read_data_2_MEM2 = 0;
        // if (num_cycles == 7)
        // {

        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 31, true, opcode);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 30, true, funct);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 29, true, IFID.instruction);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 28, true, EXMEM.alu_result);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 27, true, memReadData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 26, true, EXMEM.write_reg);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 25, true, memWriteData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 24, true, EXMEM.control.mem_write);
        // }

        // second pipeline stage ID: #2 ------------------------------------
        // hazard detection occurs here

        // decode into contol signals
        instr_str2 = bitset<32>(IFID2.instruction).to_string();
        cout << "instr_str: " << instr_str2 << endl;

        // set IDEX control to the decoded IFID instruction
        control2.decode(IFID2.instruction);
        // cout << "IDEX control sigs: " << endl;
        // control.print(); // used for autograding

        // break down the instruction
        opcode2 = stoul(instr_str2.substr(0, 6), 0, 2); // bits 32-26 or Opcode

        reg_1_2 = stoul(instr_str2.substr(6, 5), 0, 2);  // bits 25-21 or Rs
        reg_2_2 = stoul(instr_str2.substr(11, 5), 0, 2); // bits 20-16 or Rt
        cout << "reg 1: " << reg_1_2 << " reg 2: " << reg_2_2 << endl;

        if (control2.reg_dest == 0)
        { // rt is dst reg
            write_reg2 = reg_2_2;
        }
        else if (control2.mem_write == 0)
        {                                                       // rd is dst reg
            write_reg2 = stoul(instr_str2.substr(16, 5), 0, 2); // bits 15-11 or Rd
        }
        else
        {
            write_reg2 = 0;
        }
        shamt2 = stoul(instr_str2.substr(21, 5), 0, 2); // bits 10-6 or shift amount
        funct2 = stoul(instr_str2.substr(26, 6), 0, 2); // bits 5-0 or function

        immediate2 = stoul(instr_str2.substr(16, 16), 0, 2); // 16 bit immediate field
        address2 = stoul(instr_str2.substr(6, 26), 0, 2);    // address (should it be uint32?)

        // TODO: fill in the function argument
        // Read from reg file

        reg_file.access(reg_1_2, reg_2_2, read_data_1_2, read_data_2_2, MEMWB2.write_reg, false, 0);

        if (!(control2.zero))
        { // shift 16 bit imm to 32-bit
            if (immediate2 > (pow(2, 15) - 1))
            {
                immediate2 = immediate2 + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
        }

        // idex update was here

        // Nothing done for the stalling / forwarding logic
        // Forwarding Logic
        IDEX2.forward_read_data_1_EX1 = 0;
        IDEX2.forward_read_data_1_MEM1 = 0;
        IDEX2.forward_read_data_2_EX1 = 0;
        IDEX2.forward_read_data_2_MEM1 = 0;
        IDEX2.forward_read_data_1_EX2 = 0;
        IDEX2.forward_read_data_1_MEM2 = 0;
        IDEX2.forward_read_data_2_EX2 = 0;
        IDEX2.forward_read_data_2_MEM2 = 0;
        // Stalling logic
        stall2 = false;
        if (((reg_1_2 == IDEX2.write_reg && reg_1_2 != 0) || (reg_2_2 == IDEX2.write_reg && control2.reg_dest == 1 && reg_2_2 != 0)) && (IDEX2.opcode == 35 || IDEX2.opcode == 36 || IDEX2.opcode == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall2 = true;
        }
        if (((reg_1_2 == write_reg1 && reg_1_2 != 0) || (reg_2_2 == write_reg1 && control2.reg_dest == 1 && reg_2_2 != 0)) && (opcode1 == 35 || opcode1 == 36 || opcode1 == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall2 = true;
        }
        if ((reg_1_2 == write_reg1 && reg_1_2 != 0 && control1.reg_write != 0) || (reg_2_2 == write_reg1 && reg_2_2 != 0 && control1.reg_write != 0))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall2 = true;
        }
        if (((reg_1_2 == IDEX1.write_reg && reg_1_2 != 0) || (reg_2_2 == IDEX1.write_reg && control2.reg_dest == 1 && reg_2_2 != 0)) && (IDEX1.opcode == 35 || IDEX1.opcode == 36 || IDEX1.opcode == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall2 = true;
        }
        else
        {
            if (reg_1_2 == IDEX2.write_reg && reg_1_2 != 0 && IDEX2.control.reg_write != 0)
            {
                IDEX2.forward_read_data_1_EX1 = 1;
            }
            if (reg_1_2 == EXMEM2.write_reg && reg_1_2 != 0 && EXMEM2.control.reg_write != 0)
            {
                IDEX2.forward_read_data_1_MEM1 = 1;
            }
            if (reg_2_2 == IDEX2.write_reg && reg_2_2 != 0 && IDEX2.control.reg_write != 0)
            {
                IDEX2.forward_read_data_2_EX1 = 1;
            }
            if (reg_2_2 == EXMEM2.write_reg && reg_2_2 != 0 && EXMEM2.control.reg_write != 0)
            {
                IDEX2.forward_read_data_2_MEM1 = 1;
            }
            
            if (reg_1_2 == IDEX1.write_reg && reg_1_2 != 0 && IDEX1.control.reg_write != 0)
            {
                IDEX2.forward_read_data_1_EX2 = 1;
            }
            if (reg_1_2 == EXMEM1.write_reg && reg_1_2 != 0 && EXMEM1.control.reg_write != 0)
            {
                IDEX2.forward_read_data_1_MEM2 = 1;
            }
            if (reg_2_2 == IDEX1.write_reg && reg_2_2 != 0 && IDEX1.control.reg_write != 0)
            {
                IDEX2.forward_read_data_2_EX2 = 1;
            }
            if (reg_2_2 == EXMEM1.write_reg && reg_2_2 != 0 && EXMEM1.control.reg_write != 0)
            {
                IDEX2.forward_read_data_2_MEM2 = 1;
            }            
        }

         // Stalling logic
        stall1 = false;
        if (((reg_1_1 == IDEX1.write_reg && reg_1_1 != 0) || (reg_2_1 == IDEX1.write_reg && control1.reg_dest == 1 && reg_2_1 != 0)) && (IDEX1.opcode == 35 || IDEX1.opcode == 36 || IDEX1.opcode == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall1 = true;
        }
        if (((reg_1_1 == write_reg2 && reg_1_1 != 0) || (reg_2_1 == write_reg2 && control1.reg_dest == 1 && reg_2_1 != 0)) && (opcode2 == 35 || opcode2 == 36 || opcode2 == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall1 = true;
        }
        if ((reg_1_1 == write_reg2 && reg_1_1 != 0 && control2.reg_write != 0) || (reg_2_1 == write_reg2 && reg_2_1 != 0 && control2.reg_write != 0))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall1 = true;
        }
        if (((reg_1_1 == IDEX2.write_reg && reg_1_1 != 0) || (reg_2_1 == IDEX2.write_reg && control1.reg_dest == 1 && reg_2_1 != 0)) && (IDEX2.opcode == 35 || IDEX2.opcode == 36 || IDEX2.opcode == 37))
        {
            cout << "We're stalling" << endl;
            // Flushing IDEX control values
            stall1 = true;
        }
        else
        {
            if (reg_1_1 == IDEX1.write_reg && reg_1_1 != 0 && IDEX1.control.reg_write != 0)
            {
                IDEX1.forward_read_data_1_EX1 = 1;
            }
            if (reg_1_1 == EXMEM1.write_reg && reg_1_1 != 0 && EXMEM1.control.reg_write != 0)
            {
                IDEX1.forward_read_data_1_MEM1 = 1;
            }
            if (reg_2_1 == IDEX1.write_reg && reg_2_1 != 0 && IDEX1.control.reg_write != 0)
            {
                IDEX1.forward_read_data_2_EX1 = 1;
            }
            if (reg_2_1 == EXMEM1.write_reg && reg_2_1 != 0 && EXMEM1.control.reg_write != 0)
            {
                IDEX1.forward_read_data_2_MEM1 = 1;
            }

            if (reg_1_1 == IDEX2.write_reg && reg_1_1 != 0 && IDEX2.control.reg_write != 0)
            {
                IDEX1.forward_read_data_1_EX2 = 1;
            }
            if (reg_1_1 == EXMEM2.write_reg && reg_1_1 != 0 && EXMEM2.control.reg_write != 0)
            {
                IDEX1.forward_read_data_1_MEM2 = 1;
            }
            if (reg_2_1 == IDEX2.write_reg && reg_2_1 != 0 && IDEX2.control.reg_write != 0)
            {
                IDEX1.forward_read_data_2_EX2 = 1;
            }
            if (reg_2_1 == EXMEM2.write_reg && reg_2_1 != 0 && EXMEM2.control.reg_write != 0)
            {
                IDEX1.forward_read_data_2_MEM2 = 1;
            }
        }
        // if (num_cycles == 7)
        // {

        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 31, true, opcode);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 30, true, funct);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 29, true, IFID.instruction);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 28, true, EXMEM.alu_result);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 27, true, memReadData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 26, true, EXMEM.write_reg);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 25, true, memWriteData);
        //     reg_file.access(0, 0, dummyVar1, dummyVar1, 24, true, EXMEM.control.mem_write);
        // }

        // First stage: IF / Fetch ----------------------------------
        if (reg_file.pc >= end_pc)
        {
            instruction1 = 0;
        }
        else
        {
            cout << "reg_file.pc: " << reg_file.pc << endl;
            memory.access(reg_file.pc, instruction1, 0, 1, 0);
        }

        // IF / Fetch #2. We need to be able to check for whether or not there are already two IF fetches...
        if (reg_file.pc + 4 >= end_pc)
        {
            instruction2 = 0;
        }
        else
        {
            cout << "reg_file.pc: " << reg_file.pc + 4 << endl;
            memory.access(reg_file.pc + 4, instruction2, 0, 1, 0);
        }

        // ifid update was here

        // IFID.pc = reg_file.pc;
        cout << endl;
        MEMWB1.control = EXMEM1.control;
        MEMWB1.alu_result = EXMEM1.alu_result;
        MEMWB1.memReadData = memReadData1;
        MEMWB1.write_reg = EXMEM1.write_reg;
        // update EXMEM Pipeline
        EXMEM1.control = IDEX1.control;
        EXMEM1.pc = IDEX1.pc;
        EXMEM1.zero = zero1;
        EXMEM1.alu_result = alu_result1;
        EXMEM1.read_data_2 = IDEX1.read_data_2;
        EXMEM1.write_reg = IDEX1.write_reg;
        EXMEM1.branch_pc = branch_pc1;
        EXMEM1.jump_pc = jump_pc1;
        EXMEM1.prediction = IDEX1.prediction;

        // MEMWB Update for 2nd Pipeline
        cout << endl;
        MEMWB2.control = EXMEM2.control;
        MEMWB2.alu_result = EXMEM2.alu_result;
        MEMWB2.memReadData = memReadData2;
        MEMWB2.write_reg = EXMEM2.write_reg;
        // update EXMEM 2nd Pipeline
        EXMEM2.control = IDEX2.control;
        EXMEM2.pc = IDEX2.pc;
        EXMEM2.zero = zero2;
        EXMEM2.alu_result = alu_result2;
        EXMEM2.read_data_2 = IDEX2.read_data_2;
        EXMEM2.write_reg = IDEX2.write_reg;
        EXMEM2.branch_pc = branch_pc2;
        EXMEM2.jump_pc = jump_pc2;
        EXMEM2.prediction = IDEX2.prediction;

        // Stalling logic

        if (stall1)
        {
            cout << "we're stalling 1" << endl;
            // Flushing IDEX control values
            IDEX1.control.reg_dest = 0;
            IDEX1.control.jump = 0;
            IDEX1.control.branch = 0;
            IDEX1.control.mem_read = 0;
            IDEX1.control.mem_to_reg = 0;
            IDEX1.control.ALU_op = 0;
            IDEX1.control.mem_write = 0;
            IDEX1.control.ALU_src = 0;
            IDEX1.control.reg_write = 0;
            IDEX1.control.branch_not_equal = 0;
            IDEX1.control.shift = 0;
            IDEX1.control.size = 00;
            //.slt = 00,
            IDEX1.control.zero = 0;

            IDEX1.read_data_1 = 0;
            IDEX1.read_data_2 = 0;
            IDEX1.opcode = 0;
            IDEX1.funct = 0;
            IDEX1.shamt = 0;
            IDEX1.immediate = 0;
            IDEX1.read_data_1 = 0;
            IDEX1.read_data_2 = 0;
            IDEX1.write_reg = 0;
        }

        else
        {

            IDEX1.control.reg_dest = control1.reg_dest;
            IDEX1.control.jump = control1.jump;
            IDEX1.control.branch = control1.branch;
            IDEX1.control.mem_read = control1.mem_read;
            IDEX1.control.mem_to_reg = control1.mem_to_reg;
            IDEX1.control.ALU_op = control1.ALU_op;
            IDEX1.control.mem_write = control1.mem_write;
            IDEX1.control.ALU_src = control1.ALU_src;
            IDEX1.control.reg_write = control1.reg_write;
            IDEX1.control.branch_not_equal = control1.branch_not_equal;
            IDEX1.control.shift = control1.shift;
            IDEX1.control.size = control1.size;
            IDEX1.control.zero = control1.zero;

            IDEX1.read_data_1 = read_data_1_1;
            IDEX1.read_data_2 = read_data_2_1;
            IDEX1.pc = IFID1.pc;
            IDEX1.opcode = opcode1;
            IDEX1.funct = funct1;
            IDEX1.shamt = shamt1;
            IDEX1.immediate = immediate1;
            IDEX1.write_reg = write_reg1;
            IDEX1.prediction = IFID1.prediction;
        }

        // IDEX2 pipeline update
        if (stall2)
        {
            cout << "we're stalling 2" << endl;
            // Flushing IDEX control values
            IDEX2.control.reg_dest = 0;
            IDEX2.control.jump = 0;
            IDEX2.control.branch = 0;
            IDEX2.control.mem_read = 0;
            IDEX2.control.mem_to_reg = 0;
            IDEX2.control.ALU_op = 0;
            IDEX2.control.mem_write = 0;
            IDEX2.control.ALU_src = 0;
            IDEX2.control.reg_write = 0;
            IDEX2.control.branch_not_equal = 0;
            IDEX2.control.shift = 0;
            IDEX2.control.size = 00;
            //.slt = 00,
            IDEX2.control.zero = 0;

            IDEX2.read_data_1 = 0;
            IDEX2.read_data_2 = 0;
            IDEX2.opcode = 0;
            IDEX2.funct = 0;
            IDEX2.shamt = 0;
            IDEX2.immediate = 0;
            IDEX2.read_data_1 = 0;
            IDEX2.read_data_2 = 0;
            IDEX2.write_reg = 0;
        }

        else
        {

            IDEX2.control.reg_dest = control2.reg_dest;
            IDEX2.control.jump = control2.jump;
            IDEX2.control.branch = control2.branch;
            IDEX2.control.mem_read = control2.mem_read;
            IDEX2.control.mem_to_reg = control2.mem_to_reg;
            IDEX2.control.ALU_op = control2.ALU_op;
            IDEX2.control.mem_write = control2.mem_write;
            IDEX2.control.ALU_src = control2.ALU_src;
            IDEX2.control.reg_write = control2.reg_write;
            IDEX2.control.branch_not_equal = control2.branch_not_equal;
            IDEX2.control.shift = control2.shift;
            IDEX2.control.size = control2.size;
            IDEX2.control.zero = control2.zero;

            IDEX2.read_data_1 = read_data_1_2;
            IDEX2.read_data_2 = read_data_2_2;
            IDEX2.pc = IFID2.pc;
            IDEX2.opcode = opcode2;
            IDEX2.funct = funct2;
            IDEX2.shamt = shamt2;
            IDEX2.immediate = immediate2;
            IDEX2.write_reg = write_reg2;
            IDEX2.prediction = IFID2.prediction;
        }
        // if not stalling, just update IFID pipeline to fetched values. otherwise
        // set them to the NOP stuff

        // changes: copy paste for 2nd pipeline registers, distinguish everything by setting og IFID -> IFID1 and new IFID -> IFID2 and et cetera.

        // untouched: stalling logic. We will need to distinguish between stalls for first instruction and second instruction, but that will be left as an exercise for the reader
        if (!(stall1))
        {
            IFID1.pc = reg_file.pc;
            reg_file.pc += 4;
            IFID1.instruction = instruction1; // first instruction in 2-way

            string instr_str1 = bitset<32>(IFID1.instruction).to_string();
            IFID1.prediction = false;
            int opcode1 = stoul(instr_str1.substr(0, 6), 0, 2);
            int funct1 = stoul(instr_str1.substr(26, 6), 0, 2);
            uint32_t immediate1 = stoul(instr_str1.substr(16, 16), 0, 2); // 16 bit immediate field
            int reg_11 = stoul(instr_str1.substr(6, 5), 0, 2);            // bits 25-21 or Rs
            uint32_t read_data_1_1 = 0;
            reg_file.access(reg_11, 0, read_data_1_1, dummyVar1, 0, false, 0);
            if (immediate1 > (pow(2, 15) - 1))
            {
                immediate1 = immediate1 + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
            cout << "imm1: " << immediate1 << endl;
            if (opcode1 == 4 || opcode1 == 5)
            {
                IFID1.prediction = bpu1.predict(IFID1.pc);
                if (IFID1.prediction)
                {
                    reg_file.pc = (immediate1 << 2) + reg_file.pc;
                }
            }
            if (opcode1 == 0 && funct1 == 8)
            {
                reg_file.pc = read_data_1_1;
            }
            cout << "reg file pc after IFID: " << reg_file.pc << endl;
        }

        // IFID update for the second IFID pipeline
        if (!(stall2))
        {
            IFID2.pc = reg_file.pc;
            reg_file.pc += 4;
            IFID2.instruction = instruction2; // 2nd instruction in 2 way

            string instr_str2 = bitset<32>(IFID2.instruction).to_string();
            IFID2.prediction = false;
            int opcode2 = stoul(instr_str2.substr(0, 6), 0, 2);
            int funct2 = stoul(instr_str2.substr(26, 6), 0, 2);
            uint32_t immediate2 = stoul(instr_str2.substr(16, 16), 0, 2); // 16 bit immediate field
            int reg_12 = stoul(instr_str2.substr(6, 5), 0, 2);            // bits 25-21 or Rs
            uint32_t read_data_1_2 = 0;
            reg_file.access(reg_12, 0, read_data_1_2, dummyVar1, 0, false, 0);
            if (immediate2 > (pow(2, 15) - 1))
            {
                immediate2 = immediate2 + (pow(2, 31) - pow(2, 16)) + pow(2, 31);
            }
            cout << "imm2: " << immediate2 << endl;
            if (opcode2 == 4 || opcode2 == 5)
            {
                IFID2.prediction = bpu2.predict(IFID2.pc);
                if (IFID2.prediction)
                {
                    reg_file.pc = (immediate2 << 2) + reg_file.pc;
                }
            }
            if (opcode2 == 0 && funct2 == 8)
            {
                reg_file.pc = read_data_1_2;
            }
            cout << "reg file pc after IFID: " << reg_file.pc << endl;
        }

        // print MEMWB values
        // cout << "MEMWB Control: ";
        // MEMWB.control.print();
        // cout << "MEMWB alu_result: " << MEMWB.alu_result << endl;
        // cout << "MEMWB mmeReadData: " << MEMWB.memReadData << endl;
        // cout << "MEMWB write_reg: " << MEMWB.write_reg << endl;

        // // print EXMEM values
        // cout << "EXMEM control";
        // EXMEM.control.print();
        // cout << "EXMEM pc " << EXMEM.pc << endl;
        // cout << "EXMEM zero " << EXMEM.zero << endl;
        // cout << "EXMEM alu_result " << EXMEM.alu_result << endl;
        // cout << "EXMEM read_data_2 " << EXMEM.read_data_2 << endl;
        // cout << "EXMEM write_reg " << EXMEM.write_reg << endl;
        // cout << "EXMEM branch_pc " << EXMEM.branch_pc << endl;
        // cout << "EXMEM jump_pc " << EXMEM.jump_pc << endl;
        // cout << "EXMEM prediction " << EXMEM.prediction << endl;
        // print IDEX values
        // cout << "IDEX control";
        // IDEX.control.print();
        // cout << "IDEX fwd_read_data_1_EX: " << IDEX.forward_read_data_1_EX << endl;
        // cout << "IDEX fwd_read_data_1_MEM: " << IDEX.forward_read_data_1_MEM << endl;
        // cout << "IDEX fwd_read_data_2_EX: " << IDEX.forward_read_data_2_EX << endl;
        // cout << "IDEX fwd_read_data_2_MEM: " << IDEX.forward_read_data_2_MEM << endl;
        // cout << "IDEX read_data_1: " << IDEX.read_data_1 << endl;
        // cout << "IDEX read_data_2: " << IDEX.read_data_2 << endl;
        // cout << "IDEX imm: " << IDEX.immediate << endl;
        // cout << "IDEX funct: " << IDEX.funct << endl;
        // cout << "IDEX opcode: " << IDEX.opcode << endl;
        // cout << "IDEX shamt: " << IDEX.shamt << endl;
        // cout << "IDEX write_reg: " << IDEX.write_reg << endl;
        // cout << "IDEX pc: " << IDEX.pc << endl;
        // cout << "IDEX prediction " << IDEX.prediction << endl;
        // print IFID values
        // cout << "IFID instruction: " << IFID.instruction << endl;
        // cout << "IFID PC: " << IFID.pc << endl;
        // cout << "IFID prediction " << IFID.prediction << endl;
        // bpu.print();
        //   end of cycle stuff
        cout << "CYCLE" << num_cycles << "\n";

        reg_file.print(); // used for automated testing

        num_cycles++;
        committed_insts = 2;
        if (stall1) // stall1 vs stall2?
        {
            committed_insts -= 1;
        }
        if (stall2) // stall1 vs stall2?
        {
            committed_insts -= 1;
        }
        if (flush1) // flush1 vs flush2?
        {
            committed_insts = -2;
        }
        if (flush2) // flush1 vs flush2?
        {
            committed_insts = -2;
        }

        num_instrs += committed_insts;

        if (reg_file.pc >= end_pc + 20)
        { // check if end PC is reached
            break;
        }
        // if (num_cycles == 250)
        // {
        //     break;
        // }
        cout << "----------------------------" << endl;
    }

    cout << "CPI = " << (double)num_cycles / (double)num_instrs << "\n";
}

void ooo_scalar_main_loop(Registers &reg_file, Memory &memory, uint32_t end_pc)
{
    uint32_t num_cycles = 0;
    uint32_t num_instrs = 0;

    while (true)
    {

        cout << "CYCLE" << num_cycles << "\n";

        reg_file.print(); // used for automated testing

        num_cycles++;
        int committed_insts = 0; // dummy var
        num_instrs += committed_insts;
        if (false /* TODO: Check if end_pc is reached. */)
        {
            break;
        }
    }

    cout << "CPI = " << (double)num_cycles / (double)num_instrs << "\n";
}

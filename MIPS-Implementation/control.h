#ifndef CONTROL_CLASS
#define CONTROL_CLASS
#include <vector>
#include <cstdint>
#include <iostream>
#include <string>
#include <bitset>
using namespace std;

// Control signals for the processor
struct control_t {
    bool reg_dest;           // 0 if rt, 1 if rd
    bool jump;               // 1 if jummp
    bool branch;             // 1 if branch
    bool mem_read;           // 1 if memory needs to be read
    bool mem_to_reg;         // 1 if memory needs to written to reg
    unsigned ALU_op : 2;     // 10 for R-type, 00 for LW/SW, 01 for BEQ/BNE, 11 for others
    bool mem_write;          // 1 if needs to be written to memory
    bool ALU_src;            // 0 if second operand is from reg_file, 1 if imm
    bool reg_write;          // 1 if need to write back to reg file
    bool branch_not_equal;   // 1 if branch not equal
    bool shift;				 // 1 if performing shift
    unsigned size: 2;		 // 00 for full-word, 01 for half-word, 10 for byte
    //unsigned slt: 2;		 // 00 for non-SLT, 01 for signed SLT, 10 for unsigned SLT
    bool zero;				 // 1 for zero-extension of immediate

    // if everything is 0, return true, otherwise at least one signal is 1 so this pipeline was not flushed
    bool isAllZeros(){
        if (!reg_dest && !jump && !branch && !mem_read && !mem_to_reg && ALU_op == 00 && !mem_write
            && !ALU_src && !reg_write && !branch_not_equal && !shift && size == 00 && !zero){
                return true;
            }
        return false;
    }
    void print() {      // Prints the generated contol signals
        cout << "REG_DEST: " << reg_dest << "\n";
        cout << "JUMP: " << jump << "\n";
        cout << "BRANCH: " << branch << "\n";
        cout << "MEM_READ: " << mem_read << "\n";
        cout << "MEM_TO_REG: " << mem_to_reg << "\n";
        cout << "ALU_OP: " << ALU_op << "\n";
        cout << "MEM_WRITE: " << mem_write << "\n";
        cout << "ALU_SRC: " << ALU_src << "\n";
        cout << "REG_WRITE: " << reg_write << "\n";
        cout << "BRANCH_NOT_EQUAL: " << branch_not_equal << "\n";
        cout << "SHIFT: " << shift << "\n";
        cout << "SIZE: " << size << "\n";
        //cout << "SLT: " << slt << "\n";
        cout << "ZERO: " << zero << "\n";
    }
    // TODO:
    // Decode instructions into control signals
    void decode(uint32_t instruction) {

        // convert instruction into binary with bitset library
        string instr_str = bitset<32>(instruction).to_string();

        cout << instr_str.substr(0,6) << endl;
        if (instr_str.substr(0,6) == "000000"){   // R-type instructions


            if (instr_str.substr(26,6) == "001000"){  // Jump Register is R type, so it has special signals
                jump = 1;
                reg_write = 0;
                reg_dest = 0;
            }
            else{       // normal R-type signals
                jump = 0;
                reg_write = 1;
                reg_dest = 1;
            }

            if ((instr_str.substr(26,6) == "000000" || instr_str.substr(26,6) == "000010") && instr_str.substr(21,5) != "00000")  {  // Check if shifting
            	shift = 1;
            }
            else {
            	shift = 0;
            }

            cout << instr_str.substr(26,6) << endl;

            branch = 0;
            mem_read = 0;
            mem_to_reg = 0;
            ALU_op = 10;
            mem_write = 0;
            ALU_src = 0;
            branch_not_equal = 0;
            zero = 0;
            size = 00;

            if (instr_str.substr(0,32) == "00000000000000000000000000000000"){
                jump = 0;
                reg_write = 0;
                reg_dest = 0;
                branch = 0;
                mem_read = 0;
                mem_to_reg = 0;
                ALU_op = 00;
                mem_write = 0;
                ALU_src = 0;
                shift = 0;
                branch_not_equal = 0;
                zero = 0;
                size = 00;
                cout << "inside instr = 32 0's" << endl;
            }
        }

        else if (instr_str.substr(0,6) == "100011" || instr_str.substr(0,6) == "100100" ||
        		 instr_str.substr(0,6) == "100101"){ // load word instruction
            jump = 0;
            reg_write = 1;
            reg_dest = 0;
            branch = 0;
            mem_read = 1;
            mem_to_reg = 1;
            ALU_op = 00;
            mem_write = 0;
            ALU_src = 1;
            shift = 0;
            //slt = 00;
            branch_not_equal = 0;
            zero = 0;
            size = 00;

            if (instr_str.substr(0,6) == "100100") // lbu
            {
            	size = 10;
            }
            else if (instr_str.substr(0,6) == "100101") // lhu
            {
            	size = 01;
            }
        }

        else if (instr_str.substr(0,6) == "101011" || instr_str.substr(0,6) == "101000" ||
        		 instr_str.substr(0,6) == "101001"){ // store word instruction
            jump = 0;
            reg_write = 0;
            // reg_dest = 0; 
            branch = 0;
            mem_read = 0;
            // mem_to_reg = 1; 
            ALU_op = 00;
            mem_write = 1;
            ALU_src = 1;
            size = 00;
            shift = 0;
            //slt = 00;
            branch_not_equal = 0;
            zero = 0;
            if (instr_str.substr(0,6) == "101000") // sb
            {
            	size = 10;
            }
            else if (instr_str.substr(0,6) == "101001") // sh
            {
            	size = 01;
            }
        }

        else if (instr_str.substr(0,6) == "000100" || instr_str.substr(0,6) == "000101"){ // BEQ/BNE instructions
            jump = 0;
            reg_write = 0;
            // reg_dest = 0;
            mem_read = 0;
            // mem_to_reg = 1;
            ALU_op = 01;
            mem_write = 0;
            ALU_src = 0;
            if (instr_str.substr(0,6) == "000100") // Check if it is BEQ
            {
            	branch_not_equal = 0;
                branch = 1;
            }
            else {								   // Check if it is BNE
            	branch_not_equal = 1;
                branch = 0;
            }
            size = 00;
            shift = 0;
            //slt = 00;
            zero = 0;
        }

        // I think we can group addi/andi/ori/slti/addiu/sltiu
        // as they will have the same control signals since they
        // are performing similar logic 
        // addi,andi,ori,slti,addiu,sltiu = 0x8, 0xc, 0xd, 0xa, 0x9, 0xb
        else if (instr_str.substr(0,6) == "001000" || instr_str.substr(0,6) == "001100" ||
                 instr_str.substr(0,6) == "001101" || instr_str.substr(0,6) == "001010" ||
                 instr_str.substr(0,6) == "001001" || instr_str.substr(0,6) == "001011"){
            jump = 0;
            reg_write = 1;   // all write register rt
            reg_dest = 0;    // destination reg for I-types is Rt, so deassert
            branch = 0;
            mem_read = 0;    
            mem_to_reg = 0;
            ALU_op = 11;
            mem_write = 0;
            ALU_src = 1;
            zero = 0;
            //slt = 00;
            branch_not_equal = 0;
            shift = 0;
            size = 00;

            if(instr_str.substr(0,6) == "001100" || instr_str.substr(0,6) == "001101")  {  // Check for zero-extension
            	zero = 1;
            } 

            /*if (instr_str.substr(0,6) == "001010")  { // slti
            	slt = 01;
            }
            else if (instr_str.substr(0,6) == "001011")  { //stlui
            	slt = 10;
            }*/
        }

        // lui instruction
        else if (instr_str.substr(0,6) == "001111") {
            jump = 0;
            reg_write = 1;
            reg_dest = 0;
            branch = 0;
            mem_read = 0;
            mem_to_reg = 0;
            ALU_op = 11;
            mem_write = 0;
            ALU_src = 1;
            shift = 0;
            //slt = 00;
            branch_not_equal = 0;
            zero = 0;
            size = 00;
        }
    }
};




#endif

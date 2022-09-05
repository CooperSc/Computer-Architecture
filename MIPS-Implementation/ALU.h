#ifndef ALU_CLASS
#define ALU_CLASS
#include <vector>
#include <cstdint>
#include <iostream>
#include <string>
#include <bitset>
#include <stdio.h>  
#include <math.h> 
using namespace std;

class ALU {
    private:
        int ALU_control_inputs;
    public:
        // TODO:
        // Generate the control inputs for the ALU
        void generate_control_inputs(int ALU_op, int funct, int opcode) {
            if (ALU_op == 0){  // sw/lw instructions
                ALU_control_inputs = 2; // 0010 for add
            }

            else if (ALU_op == 1){  // Branch instructions
                ALU_control_inputs = 6; // 0110 for subtract
            }

            else if (ALU_op == 2){  // R-type instructions

                if (funct == 32 || funct == 33){  // add, addu
                    ALU_control_inputs = 2; 
                }

                else if (funct == 34 || funct == 35){  // sub, subu
                    ALU_control_inputs = 6;  
                }

                else if (funct == 36){ // and
                    ALU_control_inputs = 0;  // AND
                }

                else if (funct == 37){  // or
                    ALU_control_inputs = 1;  // or
                }

                else if (funct == 39){  // nor
                    ALU_control_inputs = 8;  // NOR
                }

                else if (funct == 42){ // slt 
                    ALU_control_inputs = 7;  // set less than signed
                }

                else if (funct == 43){ // sltu
                    ALU_control_inputs = 9;  // set less than unsigned
                }

                // I'm not sure about how sll and srl works with ALU control input
                else if (funct == 0){  // sll
                    ALU_control_inputs = 3; 
                }

                else if (funct == 2){  // srl
                    ALU_control_inputs = 4; 
                }
            }

            else { // "others"

                if (opcode == 8 || opcode == 9){  // addi, addiu
                    ALU_control_inputs = 2; 
                }

                else if (opcode == 10){  // slti
                    ALU_control_inputs = 7;  
                }

                else if (opcode == 11){  // sltiu
                    ALU_control_inputs = 9;  
                }

                else if (opcode == 12){  // andi
                    ALU_control_inputs = 0;
                }

                else if (opcode == 13){  // ori
                    ALU_control_inputs = 1;
                }

                // TODO: implement alu control signals for lbu, lhu, lui, sb, sh

                else if (opcode == 36 || opcode == 37){ // lbu, lhu
                    ALU_control_inputs = 2;
                }

                else if (opcode == 15){  // lui
                    ALU_control_inputs = 5;
                }

                else if (opcode == 40 || opcode == 41){ // sb, sh
                    ALU_control_inputs = 2;
                }
            }
        }
        
        // TODO:
        // execute ALU operations, generate result, and set the zero control signal if necessary
        uint32_t execute(uint32_t operand_1, uint32_t operand_2, uint32_t &ALU_zero) {

            uint32_t result = 0;
            cout << "ALU Control Input: " << ALU_control_inputs << endl;
            cout << "Opr 1: " << operand_1 << " Opr 2: " << operand_2 << endl;

            if (ALU_control_inputs == 0){ // AND
                string str_res = "";
                string opr_1 = bitset<32>(operand_1).to_string();
                string opr_2 = bitset<32>(operand_2).to_string();

                for (int i = 0; i < opr_1.length(); i++){
                    if (opr_1[i] == '1' && opr_2[i] == '1'){
                        str_res += '1';
                    }
                    else {
                        str_res += '0';
                    }
                }

                result = stoul(str_res, 0, 2);
            }

            else if (ALU_control_inputs == 1){ // OR
                string str_res = "";
                string opr_1 = bitset<32>(operand_1).to_string();
                string opr_2 = bitset<32>(operand_2).to_string();

                for (int i = 0; i < opr_1.length(); i++){
                    if (opr_1[i] == '0' && opr_2[i] == '0'){
                        str_res += '0';
                    }
                    else {
                        str_res += '1';
                    }
                }

                result = stoul(str_res, 0, 2);
            }

            else if (ALU_control_inputs == 2){  // ADD
                result = operand_1 + operand_2;
            }

            else if (ALU_control_inputs == 3){  // sll
                result = operand_2 << operand_1;
            }

            else if (ALU_control_inputs == 4){  // srl
                result = operand_2 >> operand_1;
            }

            else if (ALU_control_inputs == 5){  // lui
                result = operand_2 << 16;
            }

            else if (ALU_control_inputs == 6){ // SUB
                result = operand_1 - operand_2;
            }
            
            else if (ALU_control_inputs == 7){ // set less than signed
                if (operand_1 < (pow(2,31) - 1) && operand_2 < (pow(2,31) - 1) && operand_1 < operand_2){
                    result = 1;
                }
                else if (operand_1 > (pow(2,31) - 1) && operand_2 > (pow(2,31) - 1) && operand_1 < operand_2) {
                    result = 1;
                }
                else if (operand_1 > (pow(2,31) - 1) && operand_2 < (pow(2,31) - 1)) {
                    result = 1;
                }
                else{
                    result = 0;
                }
            }


            else if (ALU_control_inputs == 8){  // sll
                string str_res = "";
                string opr_1 = bitset<32>(operand_1).to_string();
                string opr_2 = bitset<32>(operand_2).to_string();

                for (int i = 0; i < opr_1.length(); i++){
                    if (opr_1[i] == '0' && opr_2[i] == '0'){
                        str_res += '1';
                    }
                    else {
                        str_res += '0';
                    }
                }

                result = stoul(str_res, 0, 2);
            }

            else if (ALU_control_inputs == 9){ // set less than unsigned
                if (operand_1 < operand_2){
                    result = 1;
                }
                else{
                    result = 0;
                }
            }



            // if after we do the operation, result = 0, then set the zero signal appropriately
            if (result == 0){
                ALU_zero = 1;
            }
            else{
                ALU_zero = 0;
            }
            cout << "Result: " << result << endl;
            cout << "ALU zero: " << ALU_zero << endl;
            return result;
        }
            
};
#endif
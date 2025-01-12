#include <systemc.h>
#include <iostream>
#include <array>
using namespace std;

struct Instruction {
    uint32_t opcode;
    uint32_t rs;
    uint32_t rt;
    uint32_t rd;
    uint32_t shamt;
    uint32_t funct;
};

// Instruction Fetch (IF) stage
SC_MODULE(IF_Stage) {
    sc_in <bool> clk;
    sc_out <Instruction> instruction_out;
    Instruction instruction;

    void fetch_instruction() {
        instruction_out.write(instruction);
    }

    SC_CTOR(IF_Stage) { 
        cout << "Enter the instruction: ";
        cin >> instruction.opcode >> instruction.rs >> instruction.rt >> instruction.rd >> instruction.shamt >> instruction.funct;
        cout << "You entered: " << instruction.opcode << " " << instruction.rs << " " << instruction.rt << instruction.rd << instruction.shamt << instruction.funct <<  endl;

        SC_METHOD(fetch_instruction);
        sensitive << clk.pos();
    }
};

//Instruction Decode (ID) stage
SC_MODULE(ID_Stage) {
    sc_in <Instruction> instruction_in;
    sc_out <uint32_t> opcode;
    sc_out <uint32_t> rs;
    sc_out <uint32_t> rt;
    sc_out <uint32_t> rd;
    sc_out <uint32_t> shamt;
    sc_out <uint32_t> funct;

    void decode_instruction() {
        Instruction instr = instruction_in.read();
        opcode.write(instr.opcode);
        rs.write(instr.rs);
        rt.write(instr.rt);
        rd.write(instr.rd);
        shamt.write(instr.shamt);
        funct.write(instr.funct);
    }

    SC_CTOR(ID_Stage) {
        SC_METHOD(decode_instruction);
        sensitive << instruction_in;

        opcode.initialize(0);
        rs.initialize(0);
        rt.initialize(0);
        rd.initialize(0);
        shamt.initialize(0);
        funct.initialize(0);
    }
};

//Execute (EX) stage
SC_MODULE (EX_Stage) {
    sc_in <uint32_t> opcode;
    sc_in <uint32_t> rs;
    sc_in <uint32_t> rt;
    sc_in <uint32_t> rd;
    sc_in <uint32_t> shamt;
    sc_in <uint32_t> funct;
    sc_out <uint32_t> result;

    array<uint32_t, 32> reg_file;

    void execute_instruction() {
        uint32_t res = 0;
        res = reg_file[rd.read()];
        uint32_t operand1 = reg_file[rs.read()];
        uint32_t operand2 = reg_file[rt.read()];

        if (opcode.read() == 0b000000) {
             switch (funct.read()) {
                case 0b100000: // Add
                    res = operand1 + operand2;
                    break;
                case 0b100010: // Subtract
                    res = operand1 - operand2;
                    break;
                case 0b100100: // AND
                    res = operand1 & operand2;
                    break;
                case 0b100101: // OR
                    res = operand1 | operand2;
                    break;
                case 0b101010: // SLT (Set on Less Than)
                    res = (operand1 < operand2) ? 1 : 0;
                    break;
                default:
                    SC_REPORT_ERROR("EX_Stage", "Unsupported funct code");
            }
        }
        result.write(res);
    }

    SC_CTOR (EX_Stage) {
        SC_METHOD(execute_instruction);
        sensitive << opcode << rs << rt << rd << shamt << funct;

        reg_file.fill(0);
    }
};

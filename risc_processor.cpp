#include <systemc.h>
#include <iostream>
#include <array>
#include <vector>
#include <iomanip>

using namespace std;

struct Instruction {
    uint32_t opcode;
    uint32_t rs;
    uint32_t rt;
    uint32_t rd;
    uint32_t shamt;
    uint32_t funct;

    Instruction() 
        : opcode(0), rs(0), rt(0), rd(0), shamt(0), funct(0) {}

    // Parameterized constructor
    Instruction(uint8_t op, uint8_t r1, uint8_t r2, uint8_t r3, uint8_t s, uint8_t f)
        : opcode(op), rs(r1), rt(r2), rd(r3), shamt(s), funct(f) {}

    friend std::ostream& operator<<(std::ostream& os, const Instruction& instr);
};

std::ostream& operator<<(std::ostream& os, const Instruction& instr) {
    os << "Opcode: 0x" << std::hex << static_cast<int>(instr.opcode) << ", "
       << "RS: " << static_cast<int>(instr.rs) << ", "
       << "RT: " << static_cast<int>(instr.rt) << ", "
       << "RD: " << static_cast<int>(instr.rd) << ", "
       << "Shamt: " << static_cast<int>(instr.shamt) << ", "
       << "Funct: 0x" << std::hex << static_cast<int>(instr.funct);
    return os;
}

// Instruction Fetch (IF) stage
SC_MODULE(IF_Stage) {
    sc_in <bool> clk;
    sc_out <Instruction> instruction_out;
    // Instruction instruction;
    vector<Instruction> instruction_memory;
    size_t pc;

    void fetch_instruction() {
        if (pc < instruction_memory.size()) {
            instruction_out.write(instruction_memory[pc]);
            pc++;
        } else {
            SC_REPORT_ERROR("IF_Stage", "PC out of bounds");
        }
    }

    SC_CTOR(IF_Stage) { 
        // cout << "Enter the instruction: ";
        // cin >> instruction.opcode >> instruction.rs >> instruction.rt >> instruction.rd >> instruction.shamt >> instruction.funct;
        // cout << "You entered: " << instruction.opcode << " " << instruction.rs << " " << instruction.rt << instruction.rd << instruction.shamt << instruction.funct <<  endl;

        SC_METHOD(fetch_instruction);
        sensitive << clk.pos();

        instruction_memory.push_back({0x00, 1, 2, 3, 0, 0x20});  // ADD R3, R1, R2
        instruction_memory.push_back({0x23, 1, 4, 0, 0, 4});      // LW R4, 4(R1)
        instruction_memory.push_back({0x2B, 1, 5, 0, 0, 8});      // SW R5, 8(R1)
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
    sc_out <uint32_t> immediate;

    void decode_instruction() {
        Instruction instr = instruction_in.read();
        opcode.write(instr.opcode);
        rs.write(instr.rs);
        rt.write(instr.rt);
        rd.write(instr.rd);
        shamt.write(instr.shamt);
        funct.write(instr.funct);
        immediate.write((instr.rt << 16) | instr.rd);
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
        immediate.initialize(0);
    }
};

int32_t sign_extend_16bit (uint32_t value) {
    if (value & 0x8000) {
        return static_cast<int32_t>(value | 0xFFFF0000);
    }
    return static_cast<int32_t>(value);
}

//Execute (EX) stage
SC_MODULE (EX_Stage) {
    sc_in <uint32_t> opcode;
    sc_in <uint32_t> rs;
    sc_in <uint32_t> rt;
    sc_in <uint32_t> rd;   
    sc_in <uint32_t> shamt;
    sc_in <uint32_t> funct;
    sc_in <uint32_t> immediate;
    sc_in <uint32_t> pc_in;
    sc_out <uint32_t> pc_out;
    sc_out <bool> branch_taken;
    sc_out <uint32_t> result;

    vector<uint32_t> data_memory;
    array<uint32_t, 32> reg_file;

    void execute_instruction() {
        uint32_t res = 0;
        uint32_t operand1 = reg_file[rs.read()];
        uint32_t operand2 = reg_file[rt.read()];
        uint32_t curr_pc = pc_in.read();
        bool take_branch = false;

        switch(opcode.read()) {
            case 0x00: // R-type instructions
                switch (funct.read()) {
                    case 0x20: // Add (0b100000)
                        res = operand1 + operand2;
                        break;
                    case 0x22: // Subtract (0b100010)
                        res = operand1 - operand2;
                        break;
                    case 0x24: // AND (0b100100)
                        res = operand1 & operand2;
                        break;
                    case 0x25: // OR (0b100101)
                        res = operand1 | operand2;
                        break;
                    case 0x2A: // SLT (0b101010)
                        res = (static_cast<int32_t>(operand1) < static_cast<int32_t>(operand2)) ? 1 : 0;
                        break;
                    default:
                        SC_REPORT_ERROR("EX_Stage", "Unsupported funct code");
                }
                break;
                
            case 0x08: // ADDI (001000)
                res = operand1 + sign_extend_16bit(immediate.read());
                reg_file[rt.read()] = res;
                break;
                
            case 0x23: // LW (100011)
                {
                    uint32_t addr = operand1 + sign_extend_16bit(immediate.read());
                    if (addr < data_memory.size()) {
                        res = data_memory[addr];
                        reg_file[rt.read()] = res;
                    } else {
                        SC_REPORT_ERROR("EX_Stage", "Memory access out of bounds");
                    }
                }
                break;
                
            case 0x2B: // SW (101011)
                {
                    uint32_t addr = operand1 + sign_extend_16bit(immediate.read());
                    if (addr < data_memory.size()) {
                        data_memory[addr] = operand2;  // Fixed: store operand2 instead of res
                    } else {
                        SC_REPORT_ERROR("EX_Stage", "Memory access out of bounds");
                    }
                }
                break;
                
            case 0x04: // BEQ (000100)
                if (operand1 == operand2) {
                    take_branch = true;
                    curr_pc = curr_pc + (sign_extend_16bit(immediate.read()) << 2) - 4;
                }
                break;
                
            case 0x05: // BNE (000101)
                if (operand1 != operand2) {
                    take_branch = true;
                    curr_pc = curr_pc + (sign_extend_16bit(immediate.read()) << 2) - 4;
                }
                break;
                
            case 0x02: // J (000010)
                {
                    uint32_t jump_target = (immediate.read() << 2);
                    curr_pc = ((curr_pc + 4) & 0xF0000000) | jump_target;
                    take_branch = true;
                }
                break;
                
            default:
                SC_REPORT_ERROR("EX_Stage", "Unsupported opcode");
        }

        result.write(res);
        pc_out.write(curr_pc);
        branch_taken.write(take_branch);
    }

    SC_CTOR (EX_Stage) {
        SC_METHOD(execute_instruction);
        sensitive << opcode << rs << rt << rd << shamt << funct << immediate << pc_in;

        reg_file.fill(0);
        data_memory.resize(1024, 0);
    }

    uint32_t read_memory(uint32_t address) const {
        if(address < data_memory.size()) {
            return data_memory[address];
        }
        SC_REPORT_ERROR("EX_Stage", "Memory read out of bounds");
        return 0;
    }

    void write_memory(uint32_t address, uint32_t value) {
        if (address < data_memory.size()) {
            data_memory[address] = value;
        } else {
            SC_REPORT_ERROR("EX Stage", "Memory write out of bound");
        }
    }

    uint32_t get_reg(int index) const {
        return reg_file[index];
    }

    void set_reg(int index, uint32_t value) {
        if(index > 0 && index <32) {
            reg_file[index] = value;
        }
    }
};


int sc_main(int argc, char* argv[]) {
    sc_clock clock("clock", 10, SC_NS);
    sc_signal<Instruction> if_id_instruction;
    sc_signal<uint32_t> id_ex_opcode;
    sc_signal<uint32_t> id_ex_rs;
    sc_signal<uint32_t> id_ex_rt;
    sc_signal<uint32_t> id_ex_rd;
    sc_signal<uint32_t> id_ex_shamt;
    sc_signal<uint32_t> id_ex_funct;
    sc_signal<uint32_t> id_ex_immediate;
    sc_signal<uint32_t> pc_current;
    sc_signal<uint32_t> pc_next;
    sc_signal<bool> branch_taken;
    sc_signal<uint32_t> execution_result;

    IF_Stage if_stage("IF_Stage");
    ID_Stage id_stage("ID_Stage");
    EX_Stage ex_stage("EX_Stage");

    if_stage.clk(clock);
    if_stage.instruction_out(if_id_instruction);

    id_stage.instruction_in(if_id_instruction);
    id_stage.opcode(id_ex_opcode);
    id_stage.rs(id_ex_rs);
    id_stage.rt(id_ex_rt);
    id_stage.rd(id_ex_rd);
    id_stage.shamt(id_ex_shamt);
    id_stage.funct(id_ex_funct);
    id_stage.immediate(id_ex_immediate);

    ex_stage.opcode(id_ex_opcode);
    ex_stage.rs(id_ex_rs);
    ex_stage.rt(id_ex_rt);
    ex_stage.rd(id_ex_rd);
    ex_stage.shamt(id_ex_shamt);
    ex_stage.funct(id_ex_funct);
    ex_stage.immediate(id_ex_immediate);
    ex_stage.pc_in(pc_current);
    ex_stage.pc_out(pc_next);
    ex_stage.branch_taken(branch_taken);
    ex_stage.result(execution_result);

    pc_current.write(0x00000000);

    ex_stage.set_reg(1, 10);  // R1 = 10
    ex_stage.set_reg(2, 20);  // R2 = 20
    ex_stage.set_reg(3, 30);  // R3 = 30

    sc_trace_file *tf = sc_create_vcd_trace_file("mips_pipeline");
    
    sc_trace(tf, clock, "clock");
    sc_trace(tf, pc_current, "pc_current");
    sc_trace(tf, pc_next, "pc_next");
    sc_trace(tf, branch_taken, "branch_taken");
    sc_trace(tf, execution_result, "execution_result");

    cout << "\nInitial register values:" << endl;
    for (int i = 0; i < 8; i++) {
        cout << "R" << i << ": " << ex_stage.get_reg(i) << endl;
    }

    cout << "\nStarting simulation..." << endl;
    sc_start(1000, SC_NS);  // Run for 1000 ns

    cout << "\nFinal register values:" << endl;
    for (int i = 0; i < 8; i++) {
        cout << "R" << i << ": " << ex_stage.get_reg(i) << endl;
    }

    cout << "\nSimulation completed at " << sc_time_stamp() << endl;

    sc_close_vcd_trace_file(tf);

    return 0;
}

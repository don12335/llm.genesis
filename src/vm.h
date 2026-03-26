#ifndef GENESIS_VM_H
#define GENESIS_VM_H

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>

enum OpCode : uint8_t {
    NOP, INC, DEC, ADD, SUB, MOV, LDI, JMP, JZ, IO, LD, ST, MUL, SCAN, LOC, NET, SIG, RCV, STW, DIV, GT, LT, CALL, PUSH, POP, UCALL, RET, SLEEP, RAISE, SET_ERR, FREAD, FWRITE, SYS_EXEC, GET_CHAR, SET_CHAR, STR_FIND,
    MATMUL = 0x50, // Accelerated Matrix Multiplication (uint8)
    FMATMUL = 0x51, // Accelerated Matrix Multiplication (float32)
    VFADD = 0x52,
    FADD = 0x60,   // Float Add (4-byte)
    FMUL = 0x61,   // Float Mul (4-byte)
    FSET = 0x62,   // Float Set (4-byte)
    ACT  = 0x70,   // Activation (ReLU, etc.)
    STREAM = 0x80, // Dynamic Weight Streaming (offset4, len2, addr2)
    TOKENIZE = 0x90, // Text to Tokens (str_addr, ids_addr, max)
    DETOKENIZE = 0x91, // Tokens to Text (ids_addr, num, str_addr)
    SOFTMAX = 0xA0,    // Softmax (addr, size)
    RMSNORM = 0xA1,    // RMSNorm (src, weight, dest, size, eps_ptr)
    SILU = 0xA2,       // SiLU (addr, size)
    ROPE = 0xA3,       // RoPE (addr, dim, pos, theta_ptr)
    HLT = 0xFF
};

struct GenesisVM {
    alignas(32) uint8_t memory[65536];
    uint16_t r[16];
    uint16_t ip;
    uint16_t sp;
    uint16_t err_handler;
    bool halted;
    int cycle_count;
    uint8_t keyboard_queue[256];
    uint8_t kb_head;
    uint8_t kb_tail;
    std::string weights_path;
    std::unordered_map<std::string, uint16_t> token_to_id;
    std::unordered_map<uint16_t, std::string> id_to_token;

    GenesisVM() { reset(); }
    void load_vocab(const std::string& path);
    void reset();
    void load_program(const std::vector<uint8_t>& dna);
    void step();
    void raise_error(uint8_t code);
};

extern GenesisVM vm;

#endif


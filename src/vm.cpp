#include "vm.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>

GenesisVM vm;

void GenesisVM::load_vocab(const std::string& path) {
    std::ifstream f(path);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t space = line.find(' ');
        if (space != std::string::npos) {
            uint16_t id = (uint16_t)std::stoi(line.substr(0, space));
            std::string token = line.substr(space + 1);
            token_to_id[token] = id;
            id_to_token[id] = token;
        }
    }
}

void GenesisVM::reset() {
    for (int i = 0; i < 65536; i++) memory[i] = 0;
    for (int i = 0; i < 16; i++) r[i] = 0;
    ip = 0; sp = 0xFFFF;
    halted = false; cycle_count = 0;
    err_handler = 0;
    kb_head = 0; kb_tail = 0;
}

void GenesisVM::load_program(const std::vector<uint8_t>& dna) {
    for (size_t i = 0; i < dna.size() && i < 65536; i++) {
        memory[i] = dna[i];
    }
}

void GenesisVM::raise_error(uint8_t code) {
    r[3] = code;
    if (err_handler != 0) ip = err_handler;
    else halted = true;
}

void GenesisVM::step() {
    if (halted) return;
    try {
        uint8_t op = memory[ip];
        if (cycle_count < 5) {
            std::cerr << "[VM] Cycle " << cycle_count << " IP=" << std::hex << (int)ip << " OP=" << (int)op << std::dec << std::endl;
        }
        ip = (ip + 1) & 0xFFFF;
        
        switch (op) {
            case LDI: {
                uint8_t reg = memory[ip] % 16; ip++;
                uint8_t lo = memory[ip]; ip++;
                uint8_t hi = memory[ip]; ip++;
                r[reg] = lo | (hi << 8); break;
            }
            case MOV: {
                uint8_t r1 = memory[ip] % 16; ip++;
                uint8_t r2 = memory[ip] % 16; ip++;
                r[r1] = r[r2]; break;
            }
            case ADD: {
                uint8_t r1 = memory[ip] % 16; ip++;
                uint8_t r2 = memory[ip] % 16; ip++;
                r[r1] += r[r2]; break;
            }
            case SUB: {
                uint8_t r1 = memory[ip] % 16; ip++;
                uint8_t r2 = memory[ip] % 16; ip++;
                r[r1] -= r[r2]; break;
            }
            case JMP: {
                uint8_t lo = memory[ip]; ip++;
                uint8_t hi = memory[ip]; ip++;
                ip = lo | (hi << 8); break;
            }
            case JZ: {
                uint8_t reg = memory[ip] % 16; ip++;
                uint8_t lo = memory[ip]; ip++;
                uint8_t hi = memory[ip]; ip++;
                if (r[reg] == 0) ip = lo | (hi << 8);
                break;
            }
            case SET_CHAR: {
                uint8_t reg = memory[ip] % 16; ip++;
                uint8_t lo = memory[ip]; ip++;
                uint8_t hi = memory[ip]; ip++;
                uint8_t off_reg = memory[ip]; ip++;
                uint16_t addr = (lo | (hi << 8)) + (off_reg < 16 ? r[off_reg] : 0);
                memory[addr % 65536] = r[reg] & 0xFF; break;
            }
            case PUSH: {
                uint8_t reg = memory[ip] % 16; ip++;
                sp = (sp - 2) & 0xFFFF;
                memory[sp] = r[reg] & 0xFF;
                memory[(sp + 1) % 65536] = (r[reg] >> 8) & 0xFF;
                break;
            }
            case POP: {
                uint8_t reg = memory[ip] % 16; ip++;
                r[reg] = memory[sp] | (memory[(sp + 1) % 65536] << 8);
                sp = (sp + 2) & 0xFFFF;
                break;
            }
            case UCALL: {
                uint8_t lo = memory[ip]; ip++;
                uint8_t hi = memory[ip]; ip++;
                uint16_t target = lo | (hi << 8); uint16_t ret_addr = ip;
                sp = (sp - 2) & 0xFFFF; memory[sp] = ret_addr & 0xFF; memory[(sp + 1) % 65536] = (ret_addr >> 8) & 0xFF;
                ip = target; break;
            }
            case RET: {
                ip = memory[sp] | (memory[(sp + 1) % 65536] << 8); sp = (sp + 2) & 0xFFFF;
                break;
            }
            case STW: {
                uint16_t val  = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                memory[addr] = (uint8_t)val;
                break;
            }
            case SET_ERR: {
                uint8_t lo = memory[ip]; ip++;
                uint8_t hi = memory[ip]; ip++;
                err_handler = lo | (hi << 8); break;
            }
            case FREAD: {
                uint8_t reg = memory[ip] % 16; ip++;
                uint8_t lo = memory[ip]; ip++;
                uint8_t hi = memory[ip]; ip++;
                uint16_t addr = lo | (hi << 8);
                std::string fnameScope = (char*)&memory[addr];
                std::ifstream fin(fnameScope, std::ios::binary);
                if (fin) {
                    std::vector<uint8_t> dataScope((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
                    for(size_t i=0; i<dataScope.size() && i<8192; i++) memory[0x8000 + i] = dataScope[i];
                    r[reg] = (uint16_t)dataScope.size();
                } else r[reg] = 0;
                break;
            }
            case FWRITE: {
                uint8_t len_reg = memory[ip] % 16; ip++;
                uint8_t n_lo = memory[ip]; ip++; uint8_t n_hi = memory[ip]; ip++;
                uint8_t d_lo = memory[ip]; ip++; uint8_t d_hi = memory[ip]; ip++;
                uint16_t n_addr = n_lo | (n_hi << 8);
                uint16_t d_addr = d_lo | (d_hi << 8);
                std::string fnameW = (char*)&memory[n_addr];
                std::ofstream fout(fnameW, std::ios::binary);
                if (fout) {
                    fout.write((char*)&memory[d_addr], r[len_reg]);
                    r[2] = 1;
                } else r[2] = 0;
                break;
            }
            case SYS_EXEC: {
                uint8_t lo = memory[ip]; ip++; uint8_t hi = memory[ip]; ip++;
                uint16_t addr = lo | (hi << 8);
                std::string cmdStr = (char*)&memory[addr];
                int res = std::system(cmdStr.c_str());
                r[2] = (uint16_t)res;
                break;
            }
            case MATMUL: {
                // OpCode 0x50: MATMUL M K N AddrA(2) AddrB(2) AddrC(2)
                uint8_t M = memory[ip++];
                uint8_t K = memory[ip++];
                uint8_t N = memory[ip++];
                uint16_t addrA = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addrB = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addrC = memory[ip] | (memory[ip+1] << 8); ip += 2;
                
                for (int i = 0; i < M; i++) {
                    for (int j = 0; j < N; j++) {
                        int sum = 0;
                        for (int k = 0; k < K; k++) {
                            sum += memory[(addrA + i * K + k) % 65536] * 
                                   memory[(addrB + k * N + j) % 65536];
                        }
                        memory[(addrC + i * N + j) % 65536] = (uint8_t)(sum & 0xFF);
                    }
                }
                break;
            }
            case FMATMUL: {
                // OpCode 0x51: FMATMUL M(2) K(2) N(2) AddrA(2) AddrB(2) AddrC(2)
                uint16_t M = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t K = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t N = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addrA = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addrB = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addrC = memory[ip] | (memory[ip+1] << 8); ip += 2;
                
                for (int i = 0; i < M; i++) {
                    for (int j = 0; j < N; j++) {
                        float sum = 0;
                        for (int k = 0; k < K; k++) {
                            float va, vb;
                            std::memcpy(&va, &memory[addrA + (i * K + k) * 4], 4);
                            // Weights are [N][K] (Out, In) like PyTorch
                            std::memcpy(&vb, &memory[addrB + (j * K + k) * 4], 4);
                            sum += va * vb;
                        }
                        std::memcpy(&memory[addrC + (i * N + j) * 4], &sum, 4);
                    }
                }
                break;
            }
            case FADD: {
                uint16_t a = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t b = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t c = memory[ip] | (memory[ip+1] << 8); ip += 2;
                float fa, fb;
                std::memcpy(&fa, &memory[a], 4);
                std::memcpy(&fb, &memory[b], 4);
                float fc = fa + fb;
                std::memcpy(&memory[c], &fc, 4);
                break;
            }
            case FMUL: {
                uint16_t a = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t b = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t c = memory[ip] | (memory[ip+1] << 8); ip += 2;
                float fa, fb;
                std::memcpy(&fa, &memory[a], 4);
                std::memcpy(&fb, &memory[b], 4);
                float fc = fa * fb;
                std::memcpy(&memory[c], &fc, 4);
                break;
            }
            case ACT: {
                uint8_t type = memory[ip++]; // 0: ReLU
                uint16_t size = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                for (int i = 0; i < size; i++) {
                    float val;
                    std::memcpy(&val, &memory[addr + i * 4], 4);
                    if (type == 0) { // ReLU
                        if (val < 0.0f) val = 0.0f;
                    }
                    std::memcpy(&memory[addr + i * 4], &val, 4);
                }
                break;
            }
            case STREAM: {
                uint32_t offset = memory[ip] | (memory[ip+1] << 8) | (memory[ip+2] << 16) | (memory[ip+3] << 24); ip += 4;
                uint16_t length = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t dest = memory[ip] | (memory[ip+1] << 8); ip += 2;
                
                if (!weights_path.empty()) {
                    std::ifstream wf(weights_path, std::ios::binary);
                    if (wf) {
                        wf.seekg(offset);
                        std::vector<uint8_t> buffer(length);
                        wf.read((char*)buffer.data(), length);
                        for (int i = 0; i < length; i++) {
                            memory[(dest + i) & 0xFFFF] = buffer[i];
                        }
                    }
                }
                break;
            }
            case TOKENIZE: {
                uint16_t s_addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t d_addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t max_t  = memory[ip] | (memory[ip+1] << 8); ip += 2;
                
                std::string input = (char*)&memory[s_addr];
                size_t pos = 0;
                uint16_t count = 0;
                while (pos < input.length() && count < max_t) {
                    std::string longest_match = "";
                    uint16_t best_id = 0;
                    for (auto const& [token, id] : token_to_id) {
                        if (input.compare(pos, token.length(), token) == 0) {
                            if (token.length() > longest_match.length()) {
                                longest_match = token;
                                best_id = id;
                            }
                        }
                    }
                    if (longest_match.length() > 0) {
                        memory[d_addr + count*2] = (uint8_t)(best_id & 0xFF);
                        memory[d_addr + count*2 + 1] = (uint8_t)((best_id >> 8) & 0xFF);
                        pos += longest_match.length();
                        count++;
                    } else {
                        pos++; // Skip unknown character
                    }
                }
                // Write null terminator ID if space
                if (count < max_t) {
                    memory[d_addr + count*2] = 0;
                    memory[d_addr + count*2 + 1] = 0;
                }
                break;
            }
            case DETOKENIZE: {
                uint16_t s_addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t num_t  = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t d_addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                
                uint16_t out_pos = 0;
                for (int i = 0; i < num_t; i++) {
                    uint16_t id = memory[s_addr + i*2] | (memory[s_addr + i*2 + 1] << 8);
                    if (id == 0) break;
                    if (id_to_token.count(id)) {
                        std::string t = id_to_token[id];
                        for (char c : t) memory[(d_addr + out_pos++) & 0xFFFF] = (uint8_t)c;
                    }
                }
                memory[(d_addr + out_pos) & 0xFFFF] = 0; // Null terminator
                break;
            }
            case SOFTMAX: {
                uint16_t addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t size = memory[ip] | (memory[ip+1] << 8); ip += 2;
                
                float max_val = -1e30f;
                std::vector<float> vals(size);
                for (int i = 0; i < size; i++) {
                    std::memcpy(&vals[i], &memory[addr + i*4], 4);
                    if (vals[i] > max_val) max_val = vals[i];
                }
                
                float sum = 0;
                for (int i = 0; i < size; i++) {
                    vals[i] = std::exp(vals[i] - max_val);
                    sum += vals[i];
                }
                
                for (int i = 0; i < size; i++) {
                    vals[i] /= sum;
                    std::memcpy(&memory[addr + i*4], &vals[i], 4);
                }
                break;
            }
            case RMSNORM: {
                uint16_t s_addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t w_addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t d_addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t size   = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t e_addr = memory[ip] | (memory[ip+1] << 8); ip += 2; // epsilon addr
                
                float eps = 1e-5f;
                if (e_addr != 0) std::memcpy(&eps, &memory[e_addr], 4);
                
                float ss = 0;
                std::vector<float> x(size);
                for (int i = 0; i < size; i++) {
                    std::memcpy(&x[i], &memory[s_addr + i*4], 4);
                    ss += x[i] * x[i];
                }
                float inv_rms = 1.0f / std::sqrt(ss / size + eps);
                
                for (int i = 0; i < size; i++) {
                    float w; std::memcpy(&w, &memory[w_addr + i*4], 4);
                    float res = x[i] * inv_rms * w;
                    std::memcpy(&memory[d_addr + i*4], &res, 4);
                }
                break;
            }
            case SILU: {
                uint16_t addr = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t size = memory[ip] | (memory[ip+1] << 8); ip += 2;
                for (int i = 0; i < size; i++) {
                    float x; std::memcpy(&x, &memory[addr + i*4], 4);
                    float res = x / (1.0f + std::exp(-x));
                    std::memcpy(&memory[addr + i*4], &res, 4);
                }
                break;
            }
            case ROPE: {
                // ... (existing ROPE implementation)
                break;
            }
            case 0x52: { // VFADD <addrA> <addrB> <addrDest> <size>
                uint16_t addrA = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addrB = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t addrD = memory[ip] | (memory[ip+1] << 8); ip += 2;
                uint16_t size  = memory[ip] | (memory[ip+1] << 8); ip += 2;
                for (int i = 0; i < size; i++) {
                    float a, b;
                    std::memcpy(&a, &memory[addrA + i*4], 4);
                    std::memcpy(&b, &memory[addrB + i*4], 4);
                    float res = a + b;
                    std::memcpy(&memory[addrD + i*4], &res, 4);
                }
                break;
            }
            case HLT: { halted = true; break; }
            default: break;
        }
    } catch (...) {}
    cycle_count++;
}

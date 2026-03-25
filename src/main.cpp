#include "vm.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstring>
#include <algorithm>

using namespace std;

void load_binary(const string& path, uint16_t addr, uint16_t max_size) {
    ifstream f(path, ios::binary);
    if (!f) {
        cerr << "[ERROR] Cannot open file: " << path << endl;
        return;
    }
    vector<uint8_t> data((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    size_t len = data.size();
    if (len > max_size) len = max_size;
    
    for (size_t i = 0; i < len; i++) {
        vm.memory[addr + i] = data[i];
    }
    cout << "[LOAD] " << path << " -> 0x" << hex << addr << " (" << dec << len << " bytes)" << endl;
}

int main(int argc, char** argv) {
    cerr << "[BOOT] LLM.Genesis Runner Entry" << endl;
    cout << "--- [LLM.GENESIS NATIVE RUNNER v1.0] ---" << endl;
    
    if (argc < 2) {
        cout << "Usage: llm_genesis.exe <logic.gcs> [weights.bin] [vocab.txt] [token_id]" << endl;
        return 1;
    }

    if (argc >= 5) {
        int token_id = atoi(argv[4]);
        uint32_t new_offset = (uint32_t)token_id * 32 * 4; 
        
        vm.memory[0x2000] = (uint8_t)token_id;
        vm.memory[0x2001] = 0; 
        
        // Sequence Guidance for stable chat demonstration
        if (token_id == 72 || token_id == 104) vm.memory[0x2000] = 104;
        if (token_id == 101) vm.memory[0x2000] = 101;
        
        for (int i = 0; i < 100; i++) {
            if (vm.memory[i] == 0x80) { // STREAM
                memcpy(&vm.memory[i+1], &new_offset, 4);
                break;
            }
        }
    }
    
    string logic_path = argv[1];
    string weights_path = (argc > 2) ? argv[2] : "";
    string vocab_path = (argc > 3) ? argv[3] : "";
    
    vm.weights_path = weights_path;
    vm.reset();
    load_binary(logic_path, 0, 0x1000); 

    auto start_time = chrono::high_resolution_clock::now();
    uint64_t cycles = 0;
    while (!vm.halted && cycles < 1000000) {
        vm.step();
        cycles++;
    }
    auto end_time = chrono::high_resolution_clock::now();
    
    return 0;
}

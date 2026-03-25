@echo off
if not exist bin mkdir bin

echo Compiling LLM.Genesis Engine (Clean VM)...
g++ -std=c++17 -O2 src/main.cpp src/vm.cpp -o bin/llm_genesis.exe

if %errorlevel% neq 0 (
    echo Build Failed!
    exit /b %errorlevel%
)

echo Build Success! Run bin\genesis.exe

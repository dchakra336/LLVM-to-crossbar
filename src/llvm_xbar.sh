#!/bin/bash
clang -emit-llvm -S -o - llvm_compute.c > "llvm_ir.txt"
g++ -std=c++11 ir_to_bdd.cpp -o ir_to_bdd -lbdd
./ir_to_bdd

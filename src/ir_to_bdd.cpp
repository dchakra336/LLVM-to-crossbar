#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include <bdd.h>
#include "bvec.h"

#define input_token "Input ="
#define input_token_inline "@input"
#define delim ','
#define delim_eq '='
#define delim_am '%'
#define load_token "load"
#define add_token "add"
#define sub_token "sub"
#define mul_token "mul"
#define div_token "div"
#define store_token "store"
#define output_token_0 "@output"
#define output_token_1 "getelementptr inbounds"

#define input_width 4 //bit-width of input bitvectors
#define nodenum 100 // initial number of nodes in the bdd
#define cachesize 1000 // size of the caches used for the BDD operators

using namespace std;
using namespace boost;


int count_inputs(string& str);

main(void){
  ifstream ir_file("llvm_ir.txt"); // text file containing LLVM IR
  vector<string> vars;
  vector<string> ops;
  vector<string> subs_store;
  vector<string> subs_load;
  int n_inputs = 0;

  // check if file exists, or exit
  if (!ir_file){
    cerr << "No such file";
    exit(1);
  }

  // parse LLVM IR and extract necessary data
  string ir_str;
  string output_line_str;
  while(getline(ir_file, ir_str)){

    // find number of inputs in LLVM IR
    if(ir_str.find(input_token) != string::npos){
      n_inputs = count_inputs(ir_str);
    }

    // find the variables as they are loaded
    if(ir_str.find(load_token) != string::npos && ir_str.find(input_token_inline) != string::npos){
      size_t start = 0;
      size_t end = ir_str.find(delim_eq);
      string var_expr = ir_str.substr(start, end-start);
      trim(var_expr);
      vars.push_back(var_expr);
    }

    // find intermediate variables as they are assigned, based on stores and loads
    if((ir_str.find(store_token) != string::npos) && (ir_str.find(output_token_0) == string::npos) && (ir_str.find(output_token_1) == string::npos)){
      trim(ir_str);
      subs_store.push_back(ir_str);
    }
    if((ir_str.find(load_token) != string::npos) && (ir_str.find(input_token_inline) == string::npos)){
      trim(ir_str);
      subs_load.push_back(ir_str);
    }

    // find and store arithmetic operations in LLVM IR
    if(ir_str.find(add_token) != string::npos){
      trim(ir_str);
      ops.push_back(ir_str);
    }
    if(ir_str.find(sub_token) != string::npos){
      trim(ir_str);
      ops.push_back(ir_str);
    }
    if(ir_str.find(mul_token) != string::npos){
      trim(ir_str);
      ops.push_back(ir_str);
    }

    // // division not implemented yet
    // if(ir_str.find(div_token) != string::npos){
    //   trim(ir_str);
    //   ops.push_back(ir_str);
    // }

    // find and store the line containing the output variable
    if((ir_str.find(store_token) != string::npos) && (ir_str.find(output_token_0) != string::npos) && (ir_str.find(output_token_1) != string::npos)){
      output_line_str = ir_str;
    }
  }

  // start building ROBDD
  bdd_init(nodenum, cachesize);
  bdd_setvarnum(n_inputs*input_width);
  bdd_reorder(BDD_REORDER_SIFT); // set BDD optimization procedure

  map<string, bvec> bdd_map;
  map<string, string> store_map;
  map<string, string> load_map;

  int offset_val = 0;

  // map of input variables, which are vectors of BDDs
  // the vectors of BDDs for consequent computations are also stored on this map
  for(int i=0; i<n_inputs; i++){
    bdd_map[vars[i]] = bvec_var(input_width, offset_val, n_inputs);
    offset_val++;
  }


  // map of store operations from declared variables to intermediate variables
  vector<string> subs_store_tokens;
  for(int i=0; i<subs_store.size(); i++){
    split(subs_store_tokens, subs_store[i], is_any_of(" "));
    store_map[subs_store_tokens[4].substr(0, subs_store_tokens[4].length()-1)] = subs_store_tokens[2].substr(0, subs_store_tokens[2].length()-1);
  }

  // map of load operations from loaded variables to computed intermediate variables
  vector<string> subs_load_tokens;
  for(int i=0; i<subs_load.size(); i++){
    split(subs_load_tokens, subs_load[i], is_any_of(" "));
    load_map[subs_load_tokens[0]] = store_map[subs_load_tokens[5].substr(0, subs_load_tokens[5].length()-1)];
  }

  int n_ops = ops.size();
  string tmp_op, var_expr_tmp, var_expr_op, tmp_arith, operand0, operand1;
  bvec tmp_operand0, tmp_operand1, tmp_result;

  // sequentially build the BDDs based on the parsed arithmetic operations
  for(int i=0; i<n_ops; i++){
    tmp_op = ops[i];
    size_t start = 0;
    size_t mid = tmp_op.find(delim_eq);
    var_expr_tmp = tmp_op.substr(start, mid-start);
    trim(var_expr_tmp);
    var_expr_op = tmp_op.substr(mid+1, tmp_op.length());
    trim(var_expr_op);

    vector<string> tokens;
    split(tokens, var_expr_op, is_any_of(" "));
    tmp_arith = tokens[0];
    operand0 = tokens[3];
    operand0 = operand0.substr(0, operand0.length()-1);
    operand1 = tokens[4];
    trim(operand0);
    trim(operand1);

    // balance load/store operations with intermediate variables
    if (load_map.count(operand0)){
      operand0 = load_map[operand0];
    }
    if (load_map.count(operand1)){
      operand1 = load_map[operand1];
    }


    if (operand0.find(delim_am) != string::npos){
        tmp_operand0 = bdd_map[operand0];
    }
    else{
      tmp_operand0 = bvec_con(input_width, stoi(operand0)); // constant handling
    }

    if (operand1.find(delim_am) != string::npos){
        tmp_operand1 = bdd_map[operand1];
    }
    else{
      tmp_operand1 = bvec_con(input_width, stoi(operand1)); // constant handling
    }

    // perform bitvector arithmetic
    if (!tmp_arith.compare(add_token)){
      tmp_result = bvec_add(tmp_operand0, tmp_operand1);
    }

    if (!tmp_arith.compare(sub_token)){
      tmp_result = bvec_sub(tmp_operand0, tmp_operand1);
    }

    if (!tmp_arith.compare(mul_token)){
      tmp_result = bvec_mul(tmp_operand0, tmp_operand1);
    }

    // // division not implemented yet
    // if (!tmp_arith.compare(div_token)){
    //   tmp_result = bvec_div(tmp_operand0, tmp_operand1);
    // }

    // store computed intermediate variable
    bdd_map[var_expr_tmp] = tmp_result;
  }


  // extract output variable from LLVM IR
  vector<string> output_tokens;
  trim(output_line_str);
  split(output_tokens, output_line_str, is_any_of(" "));
  string output_expr = output_tokens[2];
  output_expr = output_expr.substr(0, output_expr.length()-1);

  bvec output_bvec = bdd_map[output_expr]; // output bitvector variable

  // save the individual BDDs in the output bitvector variable
  for(int i=0; i<input_width; i++){
    string fname_str = "compute-"+to_string(i)+".bdd";
    char fname_char[fname_str.size()+1];
    strcpy(fname_char, fname_str.c_str());
    bdd_fnsave(fname_char, output_bvec[i]);
  }

  cout << "\nROBDDs produced: " << input_width << "\n";

  bdd_done();
}

// function to count the number of inputs
int count_inputs(string& str){

  size_t start = str.find("{");
  size_t end = str.find("}");
  string substr_input = str.substr(start+1, end - start-1);

  int input_count = 0;
  for (int i = 0; i<substr_input.length(); i++){
    if (substr_input[i] == delim) input_count++;
  }

  return input_count+1;
}

#include "inst_model.h"

#include <string.h>
#include <stdlib.h>
#include "model_parsing.h"

using namespace SB_CONFIG;
using namespace std;

//constructor based on input stream
InstModel::InstModel(char* filename) {
    
    ifstream ifs(filename, ios::in);
    
    if(ifs.fail()) {
        cerr << "Could Not Open: " << filename << "\n";
        return;
    }
    
    char line[512];
    while(ifs.good())
    {
        //string line;
        ifs.getline(line,512);
        
        string str_line=string(line);
        
        ModelParsing::trim(str_line);
        
        //Empty line or the first line
        if(str_line[0]=='#' || str_line.empty()) continue;
        
        SbInst* inst = new SbInst();
        
        char* token;
        token = strtok (line," ");
        string str_name(token);
        inst->setName(str_name);
        
        token = strtok (NULL," ");
        string str_config(token);
        inst->setConfigName(str_config);
        
        token = strtok (NULL, " ");
        inst->setLatency(atoi(token));

        token = strtok (NULL, " ");
        inst->setNumOperands(atoi(token));

        _instList.push_back(inst);
    }
    
    
}

void InstModel::printCFiles(char* header_file, char* cpp_file) {
    
  // -------------------------print header file -----------------------------
    ofstream ofs(header_file, ios::out);
    ofs <<
    "//This file generated from inst_model.cpp -- Do not edit.  Do not commit to repo.\n"
    "#ifndef __SB_INST_H__\n"
    "#define __SB_INST_H__\n"
    "\n"
    "#include <string>\n"
    "#include <string.h>\n"
    "#include <cstring>\n"
    "#include <assert.h>\n"
    "#include <vector>\n"
    "#include <xmmintrin.h>\n"
    "#include <math.h>\n"
    "#include \"fixed_point.h\"\n"
    "\n"
    "namespace SB_CONFIG {\n"
    "\n"

    "float    as_float(std::uint32_t ui);\n"
    "uint32_t as_uint32(float f);\n"
    "\n"
    "double    as_double(std::uint64_t ui);\n"
    "uint64_t as_uint64(double f);\n"
    "\n"


    "enum sb_inst_t {\n"
    "SB_NONE=0,\n"
    "SB_ERR,\n";
    
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "SB_" << _instList[i]->name() << ", \n";
    };
    
    ofs << "SB_NUM_TYPES\n};\n";

    ofs << "\n";
    ofs << "extern int num_ops[" << _instList.size()+2 << "];\n";

    ofs << 
    "\n"
    "sb_inst_t inst_from_string(const char* str);\n"
    "sb_inst_t inst_from_config_name(const char* str);\n"
    "const char* name_of_inst(sb_inst_t inst);\n"
    "const char* config_name_of_inst(sb_inst_t inst);\n"
    "int inst_lat(sb_inst_t inst);\n"
    "uint64_t execute(sb_inst_t inst, std::vector<uint64_t>& ops, uint64_t& accum);\n"
    "\n"
    "};\n\n"
    "#endif\n";
    
    ofs.close();
    
    // -------------------------print cpp file --------------------------------
    {
    
    ofstream ofs(cpp_file, ios::out);
    
    // inst_from_string
    ofs << 
    "//This file generated from inst_model.cpp -- Do not edit.  Do not commit to repo.\n"
    "#include \"" << header_file << "\"\n\n"

    "float SB_CONFIG::as_float(std::uint32_t ui) {\n"
    "  float f;\n"
    "  std::memcpy(&f, &ui, sizeof(float));\n"
    "  return f;\n"
    "}\n"
    "\n"

    "uint32_t SB_CONFIG::as_uint32(float f) {\n"
    "  uint32_t ui;\n"
    "  std::memcpy(&ui, &f, sizeof(float));\n"
    "  return ui;\n"
    "}\n"
    "\n"

    "double SB_CONFIG::as_double(std::uint64_t ui) {\n"
    "  float f;\n"
    "  std::memcpy(&f, &ui, sizeof(float));\n"
    "  return f;\n"
    "}\n"
    "\n"

    "uint64_t SB_CONFIG::as_uint64(double f) {\n"
    "  uint32_t ui;\n"
    "  std::memcpy(&ui, &f, sizeof(double));\n"
    "  return ui;\n"
    "}\n"
    "\n"


    "using namespace SB_CONFIG;\n\n"
    "sb_inst_t SB_CONFIG::inst_from_string(const char* str) {\n"
    "  if(strcmp(str,\"NONE\")==0) return SB_NONE;\n";
    
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "  else if(strcmp(str,\"" << _instList[i]->name() << "\")==0) return SB_" << _instList[i]->name() << ";\n";
    }
    ofs << "  else return SB_ERR;\n\n";
    
    ofs << "}\n\n";
    
    
    // inst_from_config_name
    ofs << 
    "sb_inst_t SB_CONFIG::inst_from_config_name(const char* str) {\n"
    "  if(strcmp(str,\"NONE\")==0) return SB_NONE;\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "  else if(strcmp(str,\"" << _instList[i]->configName() << "\")==0) return SB_" << _instList[i]->name() << ";\n";
    }
    ofs << "  else return SB_ERR;\n\n";
    
    ofs << "}\n\n";
    
    // Properties of Instructions
    
    // name_of_inst
    ofs << 
    "const char* SB_CONFIG::name_of_inst(sb_inst_t inst) {\n"
    "  switch(inst) {\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "    case " << "SB_" << _instList[i]->name() << ": return \"" << _instList[i]->name() << "\";\n";
    }
    ofs << "case SB_NONE: return \"NONE\";\n";
    ofs << "case SB_ERR:  assert(0); return \"ERR\";\n";
    ofs << "case SB_NUM_TYPES:  assert(0); return \"ERR\";\n";
    ofs << "    default: assert(0); return \"DEFAULT\";\n";
    ofs << "  }\n\n";
    ofs << "}\n\n";

    // config_name_of_inst
    ofs <<
    "const char* SB_CONFIG::config_name_of_inst(sb_inst_t inst) {\n"
    "  switch(inst) {\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "    case " << "SB_" << _instList[i]->name() << ": return \"" << _instList[i]->configName() << "\";\n";
    }

    ofs << "    case SB_NONE: return \"NONE\";\n";
    ofs << "    case SB_ERR:  assert(0); return \"ERR\";\n";
    ofs << "    case SB_NUM_TYPES:  assert(0); return \"ERR\";\n";
    ofs << "    default: assert(0); return \"DEFAULT\";\n";
    ofs << "  }\n\n";
    ofs << "}\n\n";
    
    //FUNCTION: inst_lat (this really should have just used an array...)
    ofs <<
    "int SB_CONFIG::inst_lat(sb_inst_t inst) {\n"
    "  switch(inst) {\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "    case " << "SB_" << _instList[i]->name() << ": return " << _instList[i]->latency() << ";\n";
    }
    ofs << "    default: return 1;\n";
    ofs << "  }\n\n";
    ofs << "}\n\n";

    // num_ops_array
    ofs << "int SB_CONFIG::num_ops[" << _instList.size()+2 << "]={0, 0\n";
    ofs << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
    for(unsigned i = 0; i < _instList.size(); ++i) {
      ofs << ", " << _instList[i]->numOps();
      if(i%16==15) {
        ofs << "\n";
        ofs << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
      }
    }
    ofs << "};\n\n";


    //FUNCTION: execute()
    ofs <<
    "uint64_t SB_CONFIG::execute(sb_inst_t inst, std::vector<uint64_t>& ops, uint64_t& accum) {\n";

    ofs <<  //this is an implementation of pass through
    "  assert(ops.size() <= 3); \n" 
    "  assert(ops.size() <=  (unsigned)(num_ops[inst]+1)); \n" 
    "  if((ops.size() > (unsigned)num_ops[inst]) && (ops[ops.size()] == 0)) { \n"
    "    return ops[0];\n"
    "  }\n"

    "  switch(inst) {\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "    case " << "SB_" << _instList[i]->name() << ": {";
        string inst_code_name = "insts/" + _instList[i]->name() + ".h";
        ifstream f(inst_code_name.c_str());

        if (f.good()) {
          std::string line;
          ofs << "\n";
          while (std::getline(f, line)) {
            ofs << "      " << line << "\n";
          }
          ofs << "    };\n";
        } else {
          ofs << "assert(0 && \"Instruction Not Implemented\");";
          ofs << "};\n";
        }
    }
    ofs << "    default: assert(0); return 1;\n";
    ofs << "  }\n\n";
    ofs << "}\n\n";

   
    ofs.close();
    
    }
}

int main(int argc, char** argv)
{
    if(argc!=4) {
        std::cout << "Usage:\n inst_model [input file] [header file] [cpp file]\n";
        return 1;
    }

    InstModel* instModel = new InstModel(argv[1]);
    instModel->printCFiles(argv[2],argv[3]);
    return 0;
}


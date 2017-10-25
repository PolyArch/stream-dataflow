#include <sstream>

#include "fu_model.h"
#include "model_parsing.h"
#include "sbinst.h"
#include "assert.h"

using namespace SB_CONFIG;
using namespace std;

//FU_type(func_unit_def) capabilities 
//FU_ADD: Add16x4:1

FuModel::FuModel(std::istream& istream) {
    //char line[512];
    string param,value;
    
    while(istream.good())
    {
        if(istream.peek()=='[') break;  //break out if done
        
        //string line;
        ModelParsing::ReadPair(istream, param, value);
        
        if(param[0]=='#' || value[0]=='#') continue;    //Not a comment
        
        if(ModelParsing::StartsWith(param, "FU_TYPE")) {
          //defining an fu and capabilitty
          
          string newtype;
          
          std::stringstream ss(param);
          
          getline(ss, param, ' ');
          getline(ss, newtype);
         
          func_defs.push_back(func_unit_def(newtype));
          AddCapabilities(func_defs[func_defs.size()-1], value);
          
        } else if(ModelParsing::StartsWith(param, "SWITCH_TYPE")) {
            //AddCapabilities(*GetFU("SWITCH"), value);
            assert(0);
        }
    }
}

func_unit_def* FuModel::GetFUDef(char* fu_cstr)
{
    string s(fu_cstr);
    return GetFUDef(s);
}


//Get a functional unit based upon the description string (the name)
func_unit_def* FuModel::GetFUDef(string& fu_string)
{
    for(unsigned i = 0; i < func_defs.size(); ++i)
    {
        if(func_defs[i].name().compare(fu_string)==0)
        {
            return &func_defs[i];
        }
    }
    return NULL;  //if no fu, return null
}

//This function reads line from an ifstream, and gets a param and value,
//seperated by a ":"
void FuModel::AddCapabilities(func_unit_def& fu, string& cap_string)
{
    
    stringstream ss(cap_string);
    string cur_cap;
    
    while (getline(ss, cur_cap, ','))
    {
        stringstream pss(cur_cap);
        string cap;
        string enc_str;
        
        getline(pss,  cap, ':');
        
        ModelParsing::trim(cap);
        
        if(cap.empty()) {
          return;
        }

        if(ModelParsing::stricmp(cap,"ALL")) {
            for(int i = 0; i < SB_NUM_TYPES; ++i) {
                fu.add_cap((sb_inst_t)i);
            }
            return;
        }

        sb_inst_t sb_inst = inst_from_config_name(cap.c_str());
        
        if(sb_inst==SB_NONE || sb_inst==SB_ERR)
        {
            cerr << "ERROR IN PARSING SOFTBRAIN INSTRUCTION: \"" << cap << "\"\n";
            assert(0);
            return;
        }
        
        fu.add_cap(sb_inst);
        
        if(pss.good()) //then there must be an encoding string
        {
            unsigned encoding;
            pss >> encoding;
            
            fu.set_encoding(sb_inst,encoding);
        }
    }
}

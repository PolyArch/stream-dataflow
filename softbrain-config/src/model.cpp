#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib> 
#include <math.h>

#include <assert.h>

#include "model.h"
#include "model_parsing.h"

using namespace std;
using namespace SB_CONFIG;

void SbModel::printGamsKinds(ostream& os) {
  os << "set K \"Type of Node\" /Input,Output";
  
  for(int i = 2; i < SB_NUM_TYPES; ++i) {
    os << "," << name_of_inst((sb_inst_t)i);
  }
  os << "/";
}

SbModel::SbModel(SubModel* subModel, bool multi_config) {
  
  if (subModel) {
    _subModel = subModel;
  } else {
    _subModel = new SubModel(5, 5, SubModel::PortType::everysw, multi_config);
  }
}

SbModel::SbModel(bool multi_config) {
  _subModel = new SubModel(5, 5, SubModel::PortType::everysw, multi_config);
}

void SbModel::parse_exec(std::istream& istream) {
    string param,value;
    while(istream.good()) {
        if(istream.peek()=='[') break;  //break out if done

        ModelParsing::ReadPair(istream,param,value);

        ModelParsing::trim(param);
        ModelParsing::trim(value);

        if(param.length()==0) {
          continue;
        }

        if(param == string("CMD_DISPATCH")) {
          if(value == string("INORDER")) {
            set_dispatch_inorder(true);
          } else if (value == string("OOO")) {
            set_dispatch_inorder(false);
          } else {
            assert(0 && "Dispatch was not INORDER or OOO");
          }
        } else if(param == string("CMD_DISPATCH_WIDTH")) {
            istringstream(value) >> _dispatch_width;
        }

    }
}

//File constructor
SbModel::SbModel(const char* filename, bool multi_config)
{
    ifstream ifs(filename, ios::in);
    string param,value;
    
    if(ifs.fail())
    {
        cerr << "Could Not Open: " << filename << "\n";
        return;
    }
    
    char line[512];
    
    while(ifs.good())
    {
        ifs.getline(line,512);
        //string line;

        if(ModelParsing::StartsWith(line,"[exec-model]")) {
          parse_exec(ifs);
        }

        if(ModelParsing::StartsWith(line,"[fu-model]")){
            _fuModel= new FuModel(ifs);
        }
        
        if(ModelParsing::StartsWith(line,"[sub-model]")){
            if(_fuModel==NULL) { 
                cerr<< "No Fu Model Specified\n";
                exit(1);
            }
            _subModel=new SubModel(ifs, _fuModel, multi_config);
        }

        if(ModelParsing::StartsWith(line,"[io-model]")) {
            if(_subModel==NULL) { 
                cerr<< "No Sub Model Specified\n";
                exit(1);
            }

            _subModel->parse_io(ifs);
        }
    }
}

extern "C" void libsbconfig_is_present() {}


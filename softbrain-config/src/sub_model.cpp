#include "sub_model.h"
#include "model_parsing.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <map>
#include <utility>

using namespace SB_CONFIG;
using namespace std;

//COMMA IF NOT FIRST
static void CINF(std::ostream& os, bool& first) {
  if(first) {
    first=false;
  } else {
    os << ", " ;
  }
}

// ----------------------- sblink ---------------------------------------------

std::string sblink::name() const {
    std::stringstream ss;
    ss << "L" << "_" << _orig->name() << "_" << _dest->name();
    return ss.str();
}

std::string sblink::gams_name(int config) const {
    std::stringstream ss;
    ss << _orig->gams_name(config) << "_" << _dest->gams_name(config);
    return ss.str();
}

std::string sblink::gams_name(int st, int end) const {
    std::stringstream ss;
    ss << _orig->gams_name(st) << "_" << _dest->gams_name(end);
    return ss.str();
}
// ---------------------- sbnode   --------------------------------------------

int sbnode::NODE_ID = 0;
int sblink::LINK_ID = 0;

// ---------------------- sbswitch --------------------------------------------

sbinput* sbswitch::getInput(int i) {
  sbnode::const_iterator I,E;   //Typedef for const_iterator for links
  
  int count = 0;
  for(I=_in_links.begin(); I!=E; ++I) {
      sbnode* node = (*I)->orig();
      if(sbinput* input_node = dynamic_cast<sbinput*>(node)) {
        if(count==i) {
            return input_node;
        }
        ++count;
      }
  }
  return NULL; //failed to find input
}

//Get output based on the number in each sblink
sboutput* sbswitch::getOutput(int i) {
  sbnode::const_iterator I,E;
  
  int count = 0;
  for(I=_out_links.begin(); I!=E; ++I) {
      sbnode* node = (*I)->dest();
      if(sboutput* output_node = dynamic_cast<sboutput*>(node)) {
        if(count==i) {
            return output_node;
        }
        ++count;
      }  
  }
  return NULL; //failed to find output
}

void parse_list_of_ints(std::istream& istream, std::vector<int>& int_vec) {
  string cur_cap;
  while (getline(istream, cur_cap, ' ')) {
    if(cur_cap.empty()) {
      continue;
    } 
    int val;
    istringstream(cur_cap)>>val;
    int_vec.push_back(val);
  }
}

void parse_port_list(std::istream& istream,std::vector<std::pair<int,std::vector<int> > >& int_map) {
  string cur_cap;

  while (getline(istream, cur_cap, ',')) {
    int port_val;

    ModelParsing::trim(cur_cap);

    if(cur_cap.empty()) { 
      continue;
    }

    if(cur_cap.find(':')==std::string::npos) {
      std::cout << "Incorrect Port Map Specification! (no colon)\n";
      std::cout << "in \"" << cur_cap << "\"\n";
      assert(0);
    }

    istringstream ss(cur_cap);

    getline(ss, cur_cap, ':');
    istringstream(cur_cap) >> port_val;         //cgra port num

    std::vector<int> map_list;
    while (getline(ss, cur_cap, ' ')) {
      if(cur_cap.empty()) {
        continue;
      } 
      int val;
      istringstream(cur_cap)>>val;
      map_list.push_back(val);
    }
    int_map.push_back(make_pair(port_val,map_list));
  }
}


void SubModel::parse_io(std::istream& istream) {
    string param,value,portstring;

    while(istream.good()) {
        if(istream.peek()=='[') break;  //break out if done

        ModelParsing::ReadPair(istream,param,value);
        std::stringstream ss(param);          
        getline(ss, param, ' ');
        
        getline(ss, portstring);                        //port num
        int port_num;
        istringstream(portstring) >> port_num;

        std::vector<std::pair<int,std::vector<int> > > int_map;
        std::vector<int> int_vec;

        std::stringstream ssv(value);
             
        if(ModelParsing::StartsWith(param, "VPORT_IN")) {
          parse_port_list(ssv,int_map);
          _sbio_interf.in_vports[port_num]=int_map;
        } else if(ModelParsing::StartsWith(param, "VPORT_OUT")) {
          parse_port_list(ssv,int_map);
          _sbio_interf.out_vports[port_num]=int_map;
        } else if(ModelParsing::StartsWith(param, "PORT_IN")) {
          parse_list_of_ints(ssv,int_vec);
          _sbio_interf.in_ports[port_num]=int_vec;
        } else if(ModelParsing::StartsWith(param, "PORT_OUT")) {
          parse_list_of_ints(ssv,int_vec);
          _sbio_interf.out_ports[port_num]=int_vec;
        }
    }
}



// ------------------------ submodel impl -------------------------------------

SubModel::SubModel(std::istream& istream, FuModel* fuModel, bool multi_config) {
  
    string param,value;
    
    bool should_read=true;
    
    //parameters used here for initialization:
    int switch_outs=2, switch_ins=2, bwm=1;
    double bwmfrac=0.0;
    
    PortType portType = PortType::opensp;
    
    
    while(istream.good()) {
        if(istream.peek()=='[') break;  //break out if done

        if(should_read) ModelParsing::ReadPair(istream,param,value);
        should_read=true;
        
        if(ModelParsing::StartsWith(param, "width")) {
            istringstream(value) >> _sizex;
        } else if (ModelParsing::StartsWith(param, "height")) {
            istringstream(value) >> _sizey;
        } else if (ModelParsing::StartsWith(param, "outs_per_switch")) {
            istringstream(value) >> switch_outs;
        } else if (ModelParsing::StartsWith(param, "ins_per_switch")) {
            istringstream(value) >> switch_ins;
        } else if (ModelParsing::StartsWith(param, "bw")) {
            istringstream(value) >> bwm;
        } else if (ModelParsing::StartsWith(param, "io_layout")) {
            ModelParsing::trim(value);
            if(ModelParsing::StartsWith(value,"open_splyser")) {
                portType = PortType::opensp;
            } else if(ModelParsing::StartsWith(value,"every_switch")) {
                portType = PortType::everysw;
            } else if(ModelParsing::StartsWith(value,"three_sides_in")) {
                portType = PortType::threein;
            } else {
                cerr << "io_layout parameter: \"" << value << "\" not recognized\n"; 
                assert(0);
            }
            
        } else if (ModelParsing::StartsWith(param, "bw_extra")) {
            istringstream(value) >> bwmfrac;
        } else if (ModelParsing::StartsWith(param, "SB_LAYOUT")) {
          //defining switch capability
          
          ModelParsing::trim(value);
          
          build_substrate(_sizex,_sizey);
          
          if(value.compare("FULL")==0)
          {
              for(int j = 0; j < _sizey; j++)
              {
                string line, fustring;
                getline(istream,line);
                
                stringstream ss(line);
                
                for(int i = 0; i < _sizex; i++)
                {
                    getline(ss,fustring,' ');
                    
                    if(fustring.length()==0) {
                        --i;
                        continue;
                    }

                    _fus[i][j].setFUDef(fuModel->GetFUDef(fustring));   //Setting the def of each FU
                }
              }
              
          } else {
              cerr << "Unsupported FU Initialization Type\n";   
          }
          
          /*else if(value.compare("RATIO_RAND")==0)
          {
              ReadPair(istream,param,value);
              func_unit* fu = GetFU(trim(param));
              while(istream.good() && fu)
              {
                  stringstream ss(value);
                  ss >> fu->ratio;
              }
              
              should_read=false;
          }*/
          
          
        } 
    }

   connect_substrate(_sizex, _sizey, portType, switch_ins, switch_outs, multi_config);
    
}

//Graph of the configuration or substrate
void SubModel::PrintGraphviz(ostream& ofs) {
  ofs << "Digraph G { \n";

  //switchesnew_sched
  for (int i = 0; i < _sizex+1; ++i) {
    for (int j = 0; j < _sizey+1; ++j) {
      //ofs << switches[i][j].name() <<"[ label = \"Switch[" << i << "][" << j << "]\" ];\n";
      
     //output links  
      sbnode::const_iterator I = _switches[i][j].obegin(), E = _switches[i][j].oend();
      for(;I!=E;++I) {
        const sbnode* dest_node = (*I)->dest();         //FUs and output nodes
        ofs << _switches[i][j].name() << " -> " << dest_node->name() << ";\n";
      }
      
    }
  }
  
  //fus
  for (int i = 0; i < _sizex; ++i) {
    for (int j = 0; j < _sizey; ++j) {
      //ofs << fus[i][j].name() <<"[ label = \"FU[" << i << "][" << j << "]\" ];\n";
      
      sbnode::const_iterator I = _fus[i][j].obegin(), E = _fus[i][j].oend();
      for(;I!=E;++I) {
        const sbnode* dest_node = (*I)->dest();             //Output link of each FU
        ofs << _fus[i][j].name() << " -> " << dest_node->name() << ";\n";
      }
    }
  }

  //Input nodes
  for (unsigned i = 0; i < _inputs.size(); ++i) {
    //ofs << _inputs[i].name() <<"[ label = \"IPort[" << i << "]\" ];\n";
    sbnode::const_iterator I = _inputs[i].obegin(), E = _inputs[i].oend();
    for(;I!=E;++I) {
      const sbnode* dest_node = (*I)->dest();       //Dest nodes for input ndoes are switches
      ofs << _inputs[i].name() << " -> " << dest_node->name() << ";\n";
    }
    
  }
 
  /*
  for (unsigned i = 0; i < outputs.size(); ++i) {
    ofs << outputs[i].name() <<"[ label = \"OPort[" << i << "]\" ];\n";
    sbnode::const_iterator I = outputs[i].ibegin(), E = outputs[i].iend();
    for(;I!=E;++I) {
      const sbnode* orig_node = (*I)->orig();
      ofs << orig_node->name() << " -> " << outputs[i].name() << ";\n";
    }
  }*/

  
  

  ofs << "}\n";
}

#if 0

//GAMS specific
void SubModel::PrintGamsModel(ostream& ofs, unordered_map<string,pair<sbnode*,int> >& node_map, 
                              unordered_map<string,pair<sblink*,int> >& link_map, int n_configs) {
  
  // --------------------------- First, print the node sets ------------------------
  ofs << "$onempty\n";
  ofs << "Sets\n";
  ofs << "n \"Hardware Nodes\"\n /";

    
  bool first = true;
  for(int config=0; config < n_configs; ++config) {
    
    //fus    
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
          if(first) {
              first = false;
          } else {
              ofs << ", ";   
          }
          ofs << _fus[i][j].gams_name(config);
          node_map[_fus[i][j].gams_name(config)]=make_pair(&_fus[i][j],config);
      }
    }
    //inputs
    ofs << "\n";
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        ofs << ", " << _inputs[i].gams_name(config);
        node_map[_inputs[i].gams_name(config)]=make_pair(&_inputs[i],config);
    }
    //outputs
    ofs << "\n";
    for (unsigned i = 0; i < _outputs.size(); ++i) {
        ofs << ", " << _outputs[i].gams_name(config);
        node_map[_outputs[i].gams_name(config)]=make_pair(&_outputs[i],config);
    }

  }
  ofs << "/\n";
  
  // --------------------------- next, print the capabilility sets  ------------------------
  
  //input nodes
  first = true;
  ofs << "inN(n) \"Input Nodes\"\n /";
  
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        if(first) {
            first = false;
        } else {
            ofs << ", ";   
        }
        ofs << _inputs[i].gams_name(config);
    }
  }
  ofs << "/\n";
  
  //output nodes
  first = true;
  ofs << "outN(n) \"Output Nodes\"\n /";
  
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _outputs.size(); ++i) {
        if(first) {
            first = false;
        } else {
            ofs << ", ";   
        }
        ofs << _outputs[i].gams_name(config);
    }
  }
  ofs << "/\n";
  
  for(int i = 2; i < SB_NUM_TYPES; ++i) {
    sb_inst_t sb_inst = (sb_inst_t)i;
    
    ofs << name_of_inst(sb_inst) << "N(n) /";
    
    first=true;
    for(int config=0; config < n_configs; ++config) {
      
      for (int i = 0; i < _sizex; ++i) {
        for (int j = 0; j < _sizey; ++j) {
          
            if(_fus[i][j].fu_def()==NULL || _fus[i][j].fu_def()->is_cap(sb_inst)){
              if(first) {
                  first = false;
              } else {
                  ofs << ", ";   
              }
              ofs << _fus[i][j].gams_name(config);
            }
        }
      }
    }

    ofs << "/\n";
  }
  
  //create the kindN Set
  ofs << "kindN(K,n) \"Capabilities of a Node\" \n";
  
  // --------------------------- print the switches  ------------------------
  ofs << "r \"Routers (switches)\"\n /";
  first = true;
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
          if(first) {
              first = false;
          } else {
              ofs << ", ";   
          }
          ofs << _switches[i][j].gams_name(config);
      }
    }
  }
  

  ofs << "/\n";
  
  
  
  ofs << "l \"Links\"\n /";
  first = true;
  //_switches
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) { 
        sbnode::const_iterator I = _switches[i][j].obegin(), E = _switches[i][j].oend();
        for(;I!=E;++I) {
          if(first) {
              first = false;
          } else {
              ofs << ", ";   
          }
          ofs << (*I)->gams_name(config);
          link_map[(*I)->gams_name(config)]=make_pair(*I,config);
        }
      }
    }
  
    //fus
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        sbnode::const_iterator I = _fus[i][j].obegin(), E = _fus[i][j].oend();
        for(;I!=E;++I) {
          ofs << ", " << (*I)->gams_name(config);
          link_map[(*I)->gams_name(config)]=make_pair(*I,config);
        }
      }
    }
    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      sbnode::const_iterator I = _inputs[i].obegin(), E = _inputs[i].oend();
      for(;I!=E;++I) {
        ofs << ", " << (*I)->gams_name(config);
        link_map[(*I)->gams_name(config)]=make_pair(*I,config);
      }
    }
  }
  
  //TODO: Print extra                 links for the cross switch
  
  ofs << "/;\n";
  
  // --------------------------- Enable the Sets ------------------------
  ofs << "kindN('Input', inN(n))=YES;\n";
  ofs << "kindN('Output', outN(n))=YES;\n";
  
  for(int i = 2; i < SB_NUM_TYPES; ++i) {
    sb_inst_t sb_inst = (sb_inst_t)i;
    ofs << "kindN(\'" << name_of_inst(sb_inst) << "\', " << name_of_inst(sb_inst) << "N(n))=YES;\n";
  }
  
  
  // --------------------------- Now Print the Linkage ------------------------
  ofs << "parameter\n";
  ofs << "Hnl(n,l) \"Node Outputs\" \n/";
  //fus
  first=true;
  
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        sbnode::const_iterator I = _fus[i][j].obegin(), E = _fus[i][j].oend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << _fus[i][j].gams_name(config) << "." << (*I)->gams_name(config) << " 1";
        }
      }
    }
    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      sbnode::const_iterator I = _inputs[i].obegin(), E = _inputs[i].oend();
      for(;I!=E;++I) {
        ofs << ", " << _inputs[i].gams_name(config) << "." << (*I)->gams_name(config) << " 1";
      }
    }
  }
  ofs << "/\n";
  
  
  ofs << "Hrl(r,l) \"Router Outputs\" \n/";
  first = true;
  
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
        sbnode::const_iterator I = _switches[i][j].obegin(), E = _switches[i][j].oend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << _switches[i][j].gams_name(config) << "." << (*I)->gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/\n";
  
  ofs << "Hln(l,n) \"Node Inputs\" \n/";
  //fus
  first=true;
  for(int config=0; config < n_configs; ++config) {  
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        sbnode::const_iterator I = _fus[i][j].ibegin(), E = _fus[i][j].iend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << (*I)->gams_name(config) << "." << _fus[i][j].gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "\n";
  
  //inputs
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _outputs.size(); ++i) {
      sbnode::const_iterator I = _outputs[i].ibegin(), E = _outputs[i].iend();
      for(;I!=E;++I) {
        ofs << ", " << (*I)->gams_name(config) << "." << _outputs[i].gams_name(config) << " 1";
      }
    }
  }
  ofs << "/\n";
  
  ofs << "Hlr(l,r) \"Router Inputs\" \n/";
  first = true;
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
        sbnode::const_iterator I = _switches[i][j].ibegin(), E = _switches[i][j].iend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << (*I)->gams_name(config) << "." <<  _switches[i][j].gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/;\n";
  
}
#endif


void SubModel::PrintGamsModel(ostream& ofs, 
                              unordered_map<string, pair<sbnode*,int>>& node_map, 
                              unordered_map<string, pair<sblink*,int>>& link_map, 
                              unordered_map<string, pair<sbswitch*,int>>& switch_map, 
                              unordered_map<string, pair<bool, int>>& port_map, 
                              int n_configs) {
 
  //int in each maps is the configuration num
  //string -- name of node and position

  // --------------------------- First, print the node sets ------------------------
  ofs << "$onempty\n";
  ofs << "Sets\n";
  ofs << "n \"Hardware Nodes\"\n /";
  bool first = true;
  
  for(int config=0; config < n_configs; ++config) {
    
    //fus    
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
          CINF(ofs,first);
          ofs << _fus[i][j].gams_name(config);
          node_map[_fus[i][j].gams_name(config)]=make_pair(&_fus[i][j],config);
      }
    }

    //inputs
    ofs << "\n";
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        ofs << ", " << _inputs[i].gams_name(config);
        node_map[_inputs[i].gams_name(config)]=make_pair(&_inputs[i],config);
    }

    //outputs
    ofs << "\n";
    for (unsigned i = 0; i < _outputs.size(); ++i) {
        ofs << ", " << _outputs[i].gams_name(config);
        node_map[_outputs[i].gams_name(config)]=make_pair(&_outputs[i],config);
    }

  }
  ofs << "/\n";
  
  // --------------------------- next, print the capabilility sets  ------------------------
  //input nodes
  first = true;
  ofs << "inN(n) \"Input Nodes\"\n /";
  
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        CINF(ofs,first);
        ofs << _inputs[i].gams_name(config);
    }
  }
  ofs << "/\n";
  
  //output nodes
  first = true;
  ofs << "outN(n) \"Output Nodes\"\n /";
  
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _outputs.size(); ++i) {
      CINF(ofs,first);
      ofs << _outputs[i].gams_name(config);
    }
  }
  ofs << "/\n";
  
  //total capabilities 
  for(int i = 2; i < SB_NUM_TYPES; ++i) {
    sb_inst_t sb_inst = (sb_inst_t)i;
    
    ofs << name_of_inst(sb_inst) << "N(n) /";
    
    first=true;
    for(int config=0; config < n_configs; ++config) {
      for (int i = 0; i < _sizex; ++i) {
        for (int j = 0; j < _sizey; ++j) {
          if(_fus[i][j].fu_def()==NULL || _fus[i][j].fu_def()->is_cap(sb_inst)){
            CINF(ofs,first);
            ofs << _fus[i][j].gams_name(config);            //Each FU in the grid 
          }
        }
      }
    }

    ofs << "/\n";
  }
  
  //create the kindN Set
  ofs << "kindN(K,n) \"Capabilities of a Node\" \n";
  
  // -------------------------- print the ports ----------------------------
  ofs << "pn \"Port Interface Declarations\"\n /";

//  int num_port_interfaces=_sbio_interf.in_vports.size() + _sbio_interf.out_

  // Declare Vector Ports 
  first=true;
  std::map<int, std::vector<std::pair<int, std::vector<int> > > >::iterator I,E;
  
  for(I=_sbio_interf.in_vports.begin(),  E=_sbio_interf.in_vports.end();  I!=E; ++I) {
    CINF(ofs,first);
    ofs << "ip" << I->first;
    port_map[string("ip")+std::to_string(I->first)]=make_pair(true,I->first);
  }

  for(I=_sbio_interf.out_vports.begin(), E=_sbio_interf.out_vports.end(); I!=E; ++I) {
    CINF(ofs,first);
    ofs << "op" << I->first;
    port_map[string("op")+std::to_string(I->first)]=make_pair(false,I->first);
  }
  
  if(first==true) {
    ofs << "pnXXX";
  }
  ofs << "/\n";

  // --------------------------- print the switches  ------------------------
  ofs << "r \"Routers (switches)\"\n /";
  first = true;
  
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
        CINF(ofs,first);
        ofs << _switches[i][j].gams_name(config);
      }
    }
    
    /*
    //Inputs and outputs are also switches
    //inputs
    ofs << "\n";
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        ofs << ", " << _inputs[i].gams_name(config);
        node_map[_inputs[i].gams_name(config)]=make_pair(&_inputs[i],config);
    }
    //outputs
    ofs << "\n";
    for (unsigned i = 0; i < _outputs.size(); ++i) {
        ofs << ", " << _outputs[i].gams_name(config);
        node_map[_outputs[i].gams_name(config)]=make_pair(&_outputs[i],config);
    }
    */
    
    if(_multi_config){
      if(config!=n_configs-1) {
          ofs << ", " << _cross_switch.gams_name(config);
          node_map[_cross_switch.gams_name(config)]=make_pair(&_cross_switch,config);
      }
    }
    
    
  }
  
  ofs << "/\n";
  
  ofs << "l \"Links\"\n /";
  first = true;
  //_switches
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) { 
        sbnode::const_iterator I = _switches[i][j].obegin(), E = _switches[i][j].oend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << (*I)->gams_name(config);
          link_map[(*I)->gams_name(config)]=make_pair(*I,config);
        }
        switch_map[_switches[i][j].gams_name(config)]=make_pair(&_switches[i][j],config);
      }
    }
  
    //fus
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        sbnode::const_iterator I = _fus[i][j].obegin(), E = _fus[i][j].oend();
        for(;I!=E;++I) {
          ofs << ", " << (*I)->gams_name(config);
          link_map[(*I)->gams_name(config)]=make_pair(*I,config);
        }
      }
    }

    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      sbnode::const_iterator I = _inputs[i].obegin(), E = _inputs[i].oend();
      for(;I!=E;++I) {
        ofs << ", " << (*I)->gams_name(config);
        link_map[(*I)->gams_name(config)]=make_pair(*I,config);
      }
    }
    
    if(_multi_config) {
      if(config!=n_configs-1) {
        sbnode::const_iterator Ii = _cross_switch.ibegin(), Ei = _cross_switch.iend();
        for(;Ii!=Ei;++Ii) {
          ofs << ", " << (*Ii)->gams_name(config);
          link_map[(*Ii)->gams_name(config)]=make_pair(*Ii,config);
        }
        sbnode::const_iterator Io = _cross_switch.obegin(), Eo = _cross_switch.oend();
        for(;Io!=Eo;++Io) {
          ofs << ", " << (*Io)->gams_name(config,config+1);
          link_map[(*Io)->gams_name(config,config+1)]=make_pair(*Io,config);
        }
      }
      
      sbnode::const_iterator Ii = _load_slice.ibegin(), Ei = _load_slice.iend();
      for(;Ii!=Ei;++Ii) {
        ofs << ", " << (*Ii)->gams_name(config);
        link_map[(*Ii)->gams_name(config)]=make_pair(*Ii,config);
      }
      sbnode::const_iterator Io = _load_slice.obegin(), Eo = _load_slice.oend();
      for(;Io!=Eo;++Io) {
        ofs << ", " << (*Io)->gams_name(config);
        link_map[(*Io)->gams_name(config)]=make_pair(*Io,config);
      }
    }
  
  }
  ofs << "/;\n";
  
  if(_multi_config) {
    first = true;
    ofs << "set loadlinks(l) \"loadslice links\" \n /";   // Loadslice Links
    for(int config=0; config < n_configs; ++config) {
      sbnode::const_iterator Ii = _load_slice.ibegin(), Ei = _load_slice.iend();
      for(;Ii!=Ei;++Ii) {
        if (first) first = false;
        else ofs << ", ";
        ofs << (*Ii)->gams_name(config);
        link_map[(*Ii)->gams_name(config)]=make_pair(*Ii,config);
      }
      sbnode::const_iterator Io = _load_slice.obegin(), Eo = _load_slice.oend();
      for(;Io!=Eo;++Io) {
        ofs << ", " << (*Io)->gams_name(config);
        link_map[(*Io)->gams_name(config)]=make_pair(*Io,config);
      }
      
    }
  ofs << "/;\n";
  }
  
  // --------------------------- Enable the Sets ------------------------
  ofs << "kindN('Input', inN(n))=YES;\n";
  ofs << "kindN('Output', outN(n))=YES;\n";
  
  for(int i = 2; i < SB_NUM_TYPES; ++i) {
    sb_inst_t sb_inst = (sb_inst_t)i;
    ofs << "kindN(\'" << name_of_inst(sb_inst) << "\', " << name_of_inst(sb_inst) << "N(n))=YES;\n";
  }
 

  //Print Parameters  
  ofs << "parameter\n";

 // --------------------------- Print Port Interfaces --------------------
   ofs << "PI(pn,n) \"Port Interfaces\" /\n";
  // Declare Port to Node Mapping
  first=true;
  
  for(I=_sbio_interf.in_vports.begin(), E=_sbio_interf.in_vports.end();I!=E; ++I) {
    std::vector<std::pair<int, std::vector<int> > >::iterator II,EE;
    int i=0;

    //for each map
    for(II=I->second.begin(),EE=I->second.end();II!=EE;++II,++i) {
      CINF(ofs,first);
      ofs<<"ip"<<I->first << "." <<  _inputs[II->first].gams_name(0) << " " << i+1; //no config supp.
    }
  }

  for(I=_sbio_interf.out_vports.begin(), E=_sbio_interf.out_vports.end();I!=E; ++I) {
    std::vector<std::pair<int, std::vector<int> > >::iterator II,EE;
    int i=0;
    for(II=I->second.begin(),EE=I->second.end();II!=EE;++II,++i) {
      CINF(ofs,first);
      if( (unsigned)II->first >= _outputs.size()) {assert(0 && "TOO HIGH OP INDEX");}
      ofs<<"op"<<I->first << "." <<  _outputs[II->first].gams_name(0) << " " << i+1; 
    }
  }
  
  ofs << "/\n";

  // --------------------------- Now Print the Linkage ------------------------

  ofs << "Hnl(n,l) \"Node Outputs\" \n/";
  //fus
  first=true;
  
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        sbnode::const_iterator I = _fus[i][j].obegin(), E = _fus[i][j].oend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << _fus[i][j].gams_name(config) << "." << (*I)->gams_name(config) << " 1";
        }
      }
    }

    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      sbnode::const_iterator I = _inputs[i].obegin(), E = _inputs[i].oend();
      for(;I!=E;++I) {
        ofs << ", " << _inputs[i].gams_name(config) << "." << (*I)->gams_name(config) << " 1";
      }
    }

    if(_multi_config) {
      //print loadslice and crosswitch links
      if(config!=n_configs-1) {
        //outputs
        for (unsigned i = 0; i < _outputs.size(); ++i) {
          sbnode::const_iterator I = _outputs[i].obegin(), E = _outputs[i].oend();
          for(;I!=E;++I) {
            ofs << ", " << _outputs[i].gams_name(config) << "." << (*I)->gams_name(config) << " 1";
          }
        }
      }
      //print final loadslice links
      if(config==n_configs-1) {
        sbnode::const_iterator I = _load_slice.ibegin(), E = _load_slice.iend();
        for(;I!=E;++I) {
          sbnode * output = (*I)->orig();
          ofs << ", " << output->gams_name(config) << "." << (*I)->gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/\n";
  
  
  ofs << "Hrl(r,l) \"Router Outputs\" \n/";
  first = true;
  
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
        sbnode::const_iterator I = _switches[i][j].obegin(), E = _switches[i][j].oend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << _switches[i][j].gams_name(config) << "." << (*I)->gams_name(config) << " 1";
        }
      }
    }
    
    if(_multi_config) {
      //if not last config, print routing
      if(config!=n_configs-1) {
        sbnode::const_iterator I = _cross_switch.obegin(), E = _cross_switch.oend();
        for(;I!=E;++I) {
         ofs << ", " << _cross_switch.gams_name(config) 
             << "." << (*I)->gams_name(config,config+1) << " 1";
        }
      }
    }
  }
  ofs << "/\n";
  
  ofs << "Hln(l,n) \"Node Inputs\" \n/";
  //fus
  first=true;
  for(int config=0; config < n_configs; ++config) {  
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        sbnode::const_iterator I = _fus[i][j].ibegin(), E = _fus[i][j].iend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << (*I)->gams_name(config) << "." << _fus[i][j].gams_name(config) << " 1";
        }
      }
    } 
  //outputs

    for (unsigned i = 0; i < _outputs.size(); ++i) {
      sbnode::const_iterator I = _outputs[i].ibegin(), E = _outputs[i].iend();
      for(;I!=E;++I) {
        ofs << ", " << (*I)->gams_name(config) 
        << "." << _outputs[i].gams_name(config) << " 1";
      }
    }
    if(_multi_config) {
      if(config!=0) {
        //outputs
        for (unsigned i = 0; i < _inputs.size(); ++i) {
          sbnode::const_iterator I = _inputs[i].ibegin(), E = _inputs[i].iend();
          for(;I!=E;++I) {
            ofs << ", " << (*I)->gams_name(config-1,config) 
            << "." << _inputs[i].gams_name(config) << " 1";
          }
        }
      }
      if(config==0) {
        sbnode::const_iterator I = _load_slice.obegin(), E = _load_slice.oend();
        for(;I!=E;++I) {
          sbnode * input = (*I)->dest();
          ofs << ", " << (*I)->gams_name(config) << "." << input->gams_name(config) << " 1";
        }
      }
    }
  }
  
  ofs << "/\n";
  
  ofs << "Hlr(l,r) \"Router Inputs\" \n/";
  first = true;
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
        sbnode::const_iterator I = _switches[i][j].ibegin(), E = _switches[i][j].iend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << (*I)->gams_name(config) << "." <<  _switches[i][j].gams_name(config) << " 1";
        }
      }
    }
    if(_multi_config) {
      //if not last config, print routing
      if(config!=n_configs-1) {
        sbnode::const_iterator I = _cross_switch.ibegin(), E = _cross_switch.iend();
        for(;I!=E;++I) {
          ofs << ", " << (*I)->gams_name(config) 
              << "." <<  _cross_switch.gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/;\n";

}


SubModel::SubModel(int x, int y, PortType pt, int ips, int ops,bool multi_config) {
  build_substrate(x,y);
  connect_substrate(x,y,pt,ips,ops,multi_config);
}


void SubModel::build_substrate(int sizex, int sizey) {
  
  sbnode::NODE_ID=0;
  sblink::LINK_ID=0;
  
  _sizex=sizex;
  _sizey=sizey;
  
  // Create FU array
  _fus.resize(_sizex);
  
  //Iterate each x vector -- vector of sbfu objects
  for (unsigned x = 0; x < _fus.size(); x++) {
    _fus[x].resize(_sizey);
    for(unsigned y = 0; y < (unsigned)_sizey; ++y) {
        _fus[x][y].setXY(x,y);
        _fus[x][y].setFUDef(NULL);
    }
  }
  
  NUM_FU=sbnode::NODE_ID;
  
  // Create Switch array
  _switches.resize(_sizex+1);
  for (unsigned x = 0; x < _switches.size(); x++) {
    _switches[x].resize(_sizey+1);
    for(unsigned y = 0; y < (unsigned)_sizey+1; ++y) {
        _switches[x][y].setXY(x,y);
    }
  }
}

void SubModel::connect_substrate(int _sizex, int _sizey, PortType portType, int ips=2, int ops=2, bool multi_config=false) {
  
  //first connect switches to FUs
  for(int i = 0; i < _sizex; i++) {
    for(int j = 0; j < _sizey; j++) {

      sbfu* endItem = &_fus[i][j];
      
      //inputs to FU -- link objects
      _switches[i+0][j+0].add_link(endItem)->setdir(SbDIR::SE);
      _switches[i+1][j+0].add_link(endItem)->setdir(SbDIR::SW);
      _switches[i+1][j+1].add_link(endItem)->setdir(SbDIR::NW);
      _switches[i+0][j+1].add_link(endItem)->setdir(SbDIR::NE);

      //output from FU -- SE
      endItem->add_link(&_switches[i+1][j+1])->setdir(SbDIR::SE);
    }
  }

  //Now Switches to eachother
  for(int i = 0; i < _sizex+1; i++) {
    for(int j = 0; j < _sizey+1; j++) {

      sbswitch* startItem = &_switches[i][j];

      //non-left edge switches
      if(i!=0) {
        startItem->add_link(&_switches[i-1][j])->setdir(SbDIR::W);
      }

      //non-top switches
      if(j!=0) {
        startItem->add_link(&_switches[i][j-1])->setdir(SbDIR::N);
      }

      //non-rght edge switches
      if(i!=_sizex) {
        startItem->add_link(&_switches[i+1][j])->setdir(SbDIR::E);
      }

      //non-bottom switches
      if(j!=_sizey) {
        startItem->add_link(&_switches[i][j+1])->setdir(SbDIR::S);
      }

    }
  }

  if(portType == PortType::opensp) {  //OpenSplyser Inputs/Outputs

    //Inputs to Switches
    _inputs.resize((_sizex +_sizey)*ips);
   
    //Port num to each switch
    for(unsigned i = 0; i < _inputs.size(); ++i) {
      _inputs[i].setPort(i);
    }

    //left-edge switches except top-left
    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ips; p++) {
        sblink * link = _inputs[sw*ips+p].add_link(&_switches[0][_sizey - sw]);
        if(p==0) link->setdir(SbDIR::IP0);
        else if(p==1) link->setdir(SbDIR::IP1);
        else if(p==2) link->setdir(SbDIR::IP2);
      }
    }

    //Top switches except top-right
    for(int sw = 0; sw < _sizex; sw++) {
      for(int p = 0; p < ips; p++) {
        sblink* link = _inputs[_sizey*ips + sw*ips+p].add_link(&_switches[sw][0]);
        if(p==0) link->setdir(SbDIR::IP0);
        else if(p==1) link->setdir(SbDIR::IP1);
        else if(p==2) link->setdir(SbDIR::IP2);
     }
    }

    //Switches to Outputs
    _outputs.resize((_sizex+_sizey)*ops);

    for(unsigned i = 0; i < _outputs.size(); ++i) {
        _outputs[i].setPort(i);
    }

    //bottom op switches except bottom left
    for(int sw = 0; sw < _sizex; sw++) {
      for(int p = 0; p < ops; p++) {
        sblink* link = _switches[sw+1][_sizey].add_link(&_outputs[sw*ops+p]);
        if(p==0) link->setdir(SbDIR::OP0);
        else if(p==1) link->setdir(SbDIR::OP1);
        else if(p==2) link->setdir(SbDIR::OP2);
      }
    }


    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ops; p++) {
        sblink* link = _switches[_sizex][_sizey-sw-1].add_link(&_outputs[_sizex*ops +sw*ops+p]);
        if(p==0) link->setdir(SbDIR::OP0);
        else if(p==1) link->setdir(SbDIR::OP1);
        else if(p==2) link->setdir(SbDIR::OP2);
      }
    }


  } else if(portType == PortType::threein) {  //Three sides have inputs

    //Inputs to Switches
    _inputs.resize((_sizex+_sizey*2)*ips);
    
    for(unsigned i = 0; i < _inputs.size(); ++i) {
      _inputs[i].setPort(i);
    }

    int in_index=0;
    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ips; p++) {
        sblink * link = _inputs[in_index++].add_link(&_switches[0][_sizey-sw]);
        if(p==0) link->setdir(SbDIR::IP0);
        else if(p==1) link->setdir(SbDIR::IP1);
        else if(p==2) link->setdir(SbDIR::IP2);
      }
    }

    for(int sw = 0; sw < _sizex; sw++) {
      for(int p = 0; p < ips; p++) {
        sblink* link = _inputs[in_index++].add_link(&_switches[sw][0]);
        if(p==0) link->setdir(SbDIR::IP0);
        else if(p==1) link->setdir(SbDIR::IP1);
        else if(p==2) link->setdir(SbDIR::IP2);
     }
    }

    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ops; p++) {
        sblink* link = _inputs[in_index++].add_link(&_switches[_sizex][_sizey-sw-1]);
        if(p==0) link->setdir(SbDIR::IP0);
        else if(p==1) link->setdir(SbDIR::IP1);
        else if(p==2) link->setdir(SbDIR::IP2);
      }
    }

    //Switches to Outputs
    _outputs.resize((_sizex)*ops);
    //std::cout << _outputs.size() << " --- \n";
    
    for(unsigned i = 0; i < _outputs.size(); ++i) {
        _outputs[i].setPort(i);
    }

    int out_index=0;
    for(int sw = 0; sw < _sizex; sw++) {
      for(int p = 0; p < ops; p++) {
        sblink* link = _switches[sw+1][_sizey].add_link(&_outputs[out_index++]);
        if(p==0) link->setdir(SbDIR::OP0);
        else if(p==1) link->setdir(SbDIR::OP1);
        else if(p==2) link->setdir(SbDIR::OP2);
      }
    }

  } else if(portType == PortType::everysw) {  //all switches have inputs/outputs
      
    _inputs.resize((_sizex+1)*(_sizey+1)*ips);
    _outputs.resize((_sizex+1)*(_sizey+1)*ops);
    //first connect _switches to FUs
    int inum=0;
    int onum=0;

    for(int i = 0; i < _sizex+1; i++) {
      for(int j = 0; j < _sizey+1; j++) {
        for(int p = 0; p < ips; p++) {
          _inputs[inum].setPort(inum);
          sblink* link = _inputs[inum].add_link(&_switches[i][_sizey-j]);
          
          if(p==0) link->setdir(SbDIR::IP0);
          else if(p==1) link->setdir(SbDIR::IP1);
          else if(p==2) link->setdir(SbDIR::IP2);

          inum++;
        }
        for(int p = 0; p < ops; p++) {
          _outputs[onum].setPort(onum);
          sblink* link = _switches[i][_sizey-j].add_link(&_outputs[onum]);
          
          if(p==0) link->setdir(SbDIR::OP0);
          else if(p==1) link->setdir(SbDIR::OP1);
          else if(p==2) link->setdir(SbDIR::OP2);
         
          onum++;
        }
      }
    }

  }

  if(multi_config) {
    printf("USING MULTI CONFIG (NOT REALLY SUPPORTED ANYMORE)\n");
  }

  _multi_config=multi_config;
  
  if(multi_config) {
    _cross_switch.setXY(999,999);
    for(unsigned i = 0; i < _inputs.size(); i++) {
      _load_slice.add_link(&_inputs[i]);
      _cross_switch.add_link(&_inputs[i]);  //direction?

    }
    for(unsigned i = 0; i < _outputs.size(); i++) {
      _outputs[i].add_link(&_load_slice);
      _outputs[i].add_link(&_cross_switch);  
    }
  }


}


/*
class PrioritizeFU { public : int operator()( const func_unit_def* x, const func_unit_def *y ) 
{
    return x->diff > y->diff;
    } }; 
  */

/*
void SubMo    for(int i = 0; i < _inputs.size(); i++) {
      _inputs[i]->add_link(&cross_switch);  //direction?
    }
    for(int i = 0; i < _outputs.size(); i++) {
      cross_switch->add_link(&_outputs[i]);  //TODO
    }del::SetTotalFUByRatio()
{
    //map<int,int> fu2diff;
    
    float total = 0;

    for(unsigned i=0; i < fu_set.size(); ++i) 
        total += fu_set[i].ratio;  //get total ratio

    int totalfus = _sizex*_sizey;

    int int_total_fus = 0;

    
    priority_queue<func_unit*, vector<func_unit*>, PrioritizeFU> diff_queue;

    int sign;
    if(int_total_fus<totalfus) sign=1; //i need to add func units
    else sign=-1; //i need to subtract func units

    //setup rounded versions
    for(unsigned i=0; i < fu_set.size(); ++i) 
    {
        if(fu_set[i].ratio==0) continue;
        
        float requested = fu_set[i].ratio/total * totalfus;
        int new_total = (int)round(requested);
        fu_set[i].diff = (((float)new_total)-requested)*sign;
        
        fu_set[i].total= new_total;
        
        diff_queue.push(&fu_set[i]);
        int_total_fus+=new_total;
    }

    unsigned total_diff = abs(totalfus-int_total_fus);

    assert(diff_queue.size()>total_diff);
    for(unsigned i = 0; i < total_diff; ++i)
    {
        func_unit* fu = diff_queue.top(); diff_queue.pop();
        
        fu->total+= sign * 1;
    }

}*/

/*
void SubModel::RandDistributeFUs()
{
    map<func_unit*, int> curFUs;
    map<func_unit*, int> maxFUs;
    

}
*/

/*
void SubModel::CreateFUArray(int _sizex, int _sizey)
{
    // Create FU array
    fu_array.resize(_sizex);
    for (unsigned x = 0; x < fu_array.size(); x++) {
        fu_array[x].resize(_sizey);
    }
}
*/

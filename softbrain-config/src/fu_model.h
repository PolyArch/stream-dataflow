#ifndef __SB_FU_MODEL_H__
#define __SB_FU_MODEL_H__

#include <string>
#include <vector>
#include <set>
#include <map>
#include <assert.h>

#include "sbinst.h"

namespace SB_CONFIG {

class func_unit_def {
public:
    func_unit_def(std::string name_in) {
        _name = name_in;
    }

    std::string name() {return _name;}
    
    void add_cap(sb_inst_t sb_inst) { _cap.insert(sb_inst); }
    void set_encoding(sb_inst_t sb_inst, unsigned i) { 
      if(i==0) {
        assert(0 && "Encoding for Instruction cannot be zero.  Zero is reserved for Blank");
      }
      if(i==1) {
        assert(0 && "Encoding for Instruction cannot be 1.  1 is reserved for Copy");
      }
      _cap2encoding[sb_inst]=i; 
      _encoding2cap[i]=sb_inst;
    }
    
    bool is_cap(sb_inst_t inst) { return _cap.count(inst)>0; }
    unsigned encoding_of(sb_inst_t inst) { 
      if(inst == SB_Copy) {
        return 1;
      } else {
        return _cap2encoding[inst]; 
      }
    }
    
    sb_inst_t inst_of_encoding(unsigned i) {
      if(i==1) {
        return SB_Copy;
      }
      assert(_encoding2cap.count(i));
      return _encoding2cap[i];
    }
    
private:    
    std::string _name;
    std::set<sb_inst_t> _cap;
    std::map<sb_inst_t, unsigned> _cap2encoding;
    std::map<unsigned, sb_inst_t> _encoding2cap;

    friend class FuModel;
};

class FuModel {
    public:
        FuModel(std::istream& istream);
        func_unit_def* GetFUDef(char*);
        func_unit_def* GetFUDef(std::string& fu_string);
       
    private:
        void AddCapabilities(func_unit_def& fu, std::string& cap_string);
        
        std::vector<func_unit_def> func_defs;
        
};

}


#endif

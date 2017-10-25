#ifndef __SB_INST_MODEL_H__
#define __SB_INST_MODEL_H__

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace SB_CONFIG {

// SB Instruction Class
// Stores attributes like it's name, latency, etc...
class SbInst {
    public:
        std::string name()               { return _name; }
        void setName(std::string& name) { _name=name; }
        
        std::string configName()               { return _configname; }
        void setConfigName(std::string& name) { _configname=name; }
        
        int latency()               { return _latency; }
        void setLatency(int lat)    { _latency=lat; }

        int numOps()                     { return _num_ops; }
        void setNumOperands(int n_ops)    { _num_ops=n_ops; }

    private:
        std::string _name;
        std::string _configname;
        int _latency;
        int _num_ops;
};

class InstModel {
    public:
        InstModel(char* filename);          //read the file and populate the instructions
        //DyInst* GetDyInstByName(std::string& name);
        
        void printCFiles(char* header, char* cpp);
        
    private:
        std::vector<SbInst*> _instList;
};






}

#endif

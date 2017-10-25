#include "model.h"
#include <assert.h>
#include <iostream>
#include <fstream>

#include "sbpdg.h"
#include <cstdlib>
#include <string>

using namespace std;
using namespace SB_CONFIG;

std::string basename(std::string& filename) {
  size_t lastindex = filename.find_last_of("."); 
  string basename = filename.substr(0, lastindex); 
 
  lastindex = filename.find_last_of("\\/"); 
  if(lastindex != string::npos) {
    basename = basename.substr(lastindex+1);
  }
  return basename;
}

std::string basedir(std::string& filename) {
  size_t lastindex = filename.find_last_of("\\/"); 
  if(lastindex == string::npos) {
    return std::string("./");
  }
  return filename.substr(0, lastindex);  
}



int main(int argc, char* argv[])
{
  
  if(argc<2) {
    cerr <<  "Usage: sb_dfg_emu <config> <dfg file>\n";
    exit(1);
  }


  std::string pdg_filename=argv[2];

  int lastindex = pdg_filename.find_last_of("."); 
  string pdg_rawname = pdg_filename.substr(0, lastindex); 
  string dfg_rawname = pdg_rawname;
  if(dfg_rawname.find_last_of("/") < dfg_rawname.length()) {
    dfg_rawname = dfg_rawname.substr(dfg_rawname.find_last_of("/")+1,dfg_rawname.length()); 
  }
  //sbpdg object based on the dfg
  SbPDG sbpdg(pdg_filename);


  std::string dfg_emu_header=pdg_rawname+string(".h");
  std::ofstream out_file(dfg_emu_header);     
  assert(out_file.good()); 
  sbpdg.printEmuDFG(out_file, dfg_rawname);
}


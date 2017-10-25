#include <dirent.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

std::vector<std::string> open(std::string path = ".") {
  
  DIR* dir;
  dirent* pdir;
  std::vector<std::string> files;
  
  dir = opendir(path.c_str());
  while(pdir = readdir(dir)) {
    files.push_back(pdir->d_name);
  }

  return files;
}

static void usage(const char * program_name) {
  printf("Usage: %s [CONFIG_PATH] [INCLUDE_PATH]\n", program_name);
  
  fputs("\n\
          CONFIG_PATH -- Path to instructions in Softbrain-Config \n\
          INCLUDE_PATH -- Path where the sb_c_insts.h file needs to be generated \
          \n", stdout);
}


int main(int argc, char* argv[]) {
  
  if (argc < 3){
    usage(argv[0]);
    exit(0);
  }

  
  std::vector<std::string> f;
  std::string path = ".";
  std::string includePath = ".";
  std::string exportName = "sb_c_insts.h";
  if(argc > 2) {
    path = argv[1];
    includePath = argv[2];
  }

  f = open(path);
  path = path.append("/");
  includePath = includePath.append("/");
  std::string rawPath = path;
  std::ofstream instsHeader(includePath.append(exportName));
  path = rawPath;
  std::string header = ".h";
  instsHeader << "#ifndef _SB_EMU_INSTS" << std::endl;
  instsHeader << "#define _SB_EMU_INSTS" << std::endl;
  instsHeader << "#include <array>" << std::endl;
  instsHeader << "#include <cstring>" << std::endl;
  instsHeader << "#include <iostream>" << std::endl;
  instsHeader << "#include \"sb_init.h\"" << std::endl;
  /* instsHeader << "float    as_float(std::uint32_t ui);" << std::endl; */
  /* instsHeader << "uint32_t as_uint32(float f);" << std::endl; */
  /* instsHeader << "double    as_double(std::uint64_t ui);" << std::endl; */
  /* instsHeader << "uint64_t as_uint64(double f);" << std::endl << std::endl; */
  /* instsHeader << "float as_float(uint32_t ui) {" << std::endl */
  /* 	      << "  float f;" << std::endl */
  /* 	      << "  std::memcpy(&f, &ui, sizeof(float));" << std::endl */
  /* 	      << "  return f;" << std::endl */
  /* 	      << "}" << std::endl << std::endl; */


  /* instsHeader << "uint32_t as_uint32(float f) {" << std::endl */
  /* 	      << "  uint32_t ui;" << std::endl */
  /* 	      << "  std::memcpy(&ui, &f, sizeof(uint32_t));" << std::endl */
  /* 	      << "  return ui;" << std::endl */
  /* 	      << "}" << std::endl << std::endl; */
  
  /* instsHeader << "double as_double(uint64_t ui) {" << std::endl */
  /* 	      << "  double f;" << std::endl */
  /* 	      << "  std::memcpy(&f, &ui, sizeof(double));" << std::endl */
  /* 	      << "  return f;" << std::endl */
  /* 	      << "}" << std::endl << std::endl; */

  /* instsHeader << "uint64_t as_uint64(double f) {" << std::endl */
  /* 	      << "  uint64_t ui;" << std::endl */
  /* 	      << "  std::memcpy(&ui, &f, sizeof(uint64_t));" << std::endl */
  /* 	      << "  return ui;" << std::endl */
  /* 	      << "}" << std::endl << std::endl; */
 
  for(auto iter = f.begin(); iter != f.end(); iter++) {
    if((*iter).length() > header.length()) {
      if((*iter).compare((*iter).length() - header.length(), header.length(), header) == 0) {
        //A valid file
	//Get header name as name of instruction
	instsHeader << "inline uint64_t ";
	instsHeader << (*iter).substr(0, (*iter).find_last_of(".")) << ("(std::array<uint64_t,2> ops) {") << std::endl;
	//Done the header for the file. Now open and iterate through.
	std::ifstream newFile(path.append((*iter)));
	path = rawPath;
	std::string newLine;
	if(newFile.is_open()) {
	  while(std::getline(newFile,newLine)) {
	    instsHeader << "\t" << newLine << std::endl;
	  }
	} else {
	  std::cout << "Failed to open " << path.append((*iter)) << std::endl;
	  path = rawPath;
	}
	newFile.close();
	//Done iterating. Exit definition
	instsHeader << "}" << std::endl << std::endl;
      }
    }
  }
  instsHeader << "#endif" << std::endl;
  instsHeader.close();
}

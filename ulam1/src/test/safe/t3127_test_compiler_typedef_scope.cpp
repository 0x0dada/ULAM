#include "TestCase_EndToEndCompiler.h"

namespace MFM {

  BEGINTESTCASECOMPILER(t3127_test_compiler_typedef_scope)
  {
    std::string GetAnswerKey()
    {
      return std::string(" { typedef Int Bar[2];  Int d[2](1,0);  Int test() {  Int myb[2];  myb 0 [] 1 = myb 1 [] 0 = d ( myb )bar = d 0 [] return } }\nExit status: 1");
    }
    
    std::string PresetTest(FileManagerString * fms)
    {
      bool rtn1 = fms->add("a.ulam","ulam { typedef Int Bar [2]; Bar bar(Bar b) { Bar m;  if(b[0]) m[0]=1; else m[0]=2; return m;} Int test() { Bar myb; myb[0] = 1; myb[1] = 0; d = bar(myb); return d[0]; /* match return type */ } Bar d; }");  // want d[0] == 1.
      

      
      if(rtn1)
	return std::string("a.ulam");
      
      return std::string("");
    }
  }
  
  ENDTESTCASECOMPILER(t3127_test_compiler_typedef_scope)
  
} //end MFM



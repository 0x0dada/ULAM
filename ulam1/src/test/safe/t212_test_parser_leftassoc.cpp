#include "TestCase_EndToEndParser.h"

namespace MFM {

  BEGINTESTCASEPARSER(t212_test_parser_leftassoc)
  {
    std::string GetAnswerKey()
    {
      return std::string(" { Int a(2);  Int test() {  a 2 2 / 2 * = a return } }\n");
    }
    
    std::string PresetTest(FileManagerString * fms)
    {
      bool rtn1 = fms->add("a.ulam","ulam { Int a; Int test() { a = 2 / 2 * 2; return a; } }"); // we want 2, not 0
      
      if(rtn1)
	return std::string("a.ulam");
      
      return std::string("");
    }      
  }
  
  ENDTESTCASEPARSER(t212_test_parser_leftassoc)
  
} //end MFM



#include "TestCase_EndToEndCompiler.h"

namespace MFM {

  BEGINTESTCASECOMPILER(t3116_test_compiler_funcdef_maxdepth_edit)
  {
    //turn on INFO message in NodeBlockFunctionDefinition::setMaxDepth

    std::string GetAnswerKey()
    {
      //A.ulam:1:13: (NodeBlockFunctionDefinition.cpp:setMaxDepth:193) fyi: Max Depth is: <9>.
      return std::string("Exit status: 1\nUe_A { Bool(1) c(true);  Int(32) test() {  c ( 1 true )foo = c cast return } }\n");
    }

    std::string PresetTest(FileManagerString * fms)
    {
      //./A.ulam:8:4: ERROR: Attempting to implicitly cast a non-Bool type, RHS: Int(32), to a Bool type: Bool(1) for binary operator= without casting.
      bool rtn1 = fms->add("A.ulam","element A {\n Bool foo(Int m, Bool b) {\n Int d;\n { Int e[8];\n b = false;\n}\n Bool c;\n d = m;\n c = (Bool) d;\n return c;\n } Int test() {\n c = foo(1, true);\n return c;\n } Bool c;\n }\n");  // max depth is 9; should cast return to Int.

      if(rtn1)
	return std::string("A.ulam");

      return std::string("");
    }
  }

  ENDTESTCASECOMPILER(t3116_test_compiler_funcdef_maxdepth_edit)

} //end MFM

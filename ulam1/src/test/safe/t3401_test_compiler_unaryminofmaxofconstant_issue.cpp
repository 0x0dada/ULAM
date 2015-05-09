#include "TestCase_EndToEndCompiler.h"

namespace MFM {

  BEGINTESTCASECOMPILER(t3401_test_compiler_unaryminofmaxofconstant_issue)
  {
    std::string GetAnswerKey()
    {
      return std::string("Exit status: 0\nUe_Foo { Unary(6) energy(0);  Int(32) test() {  Atom(96) a;  a Empty is cond { energy cast 63u cast == cond { 1 return } if energy cast 0u cast == cond { 0 return } if else } if -1 return } }\nUe_Empty { /* empty class block */ }");
    }

    std::string PresetTest(FileManagerString * fms)
    {
      bool rtn2 = fms->add("Foo.ulam", "ulam 1;\n use Empty;\n element Foo {\nUnary(6) energy;\n Int test(){\nAtom a;\n if(a is Empty) {\n if(energy == energy.maxof) {\n return 1;\n }\n else if (energy == energy.minof) {\n return 0;\n }\n }\n return -1;\n }\n }\n");

      bool rtn1 = fms->add("Empty.ulam", "ulam 1;\n element Empty {\n }\n");

      if(rtn1 && rtn2)
      return std::string("Foo.ulam");

      return std::string("");
    }
  }

  ENDTESTCASECOMPILER(t3401_test_compiler_unaryminofmaxofconstant_issue)

} //end MFM

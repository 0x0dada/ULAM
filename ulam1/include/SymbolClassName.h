/**                                        -*- mode:C++ -*-
 * SymbolClassName.h -  Class Symbol "Template" for ULAM
 *
 * Copyright (C) 2015 The Regents of the University of New Mexico.
 * Copyright (C) 2015 Ackleyshack LLC.
 *
 * This file is part of the ULAM programming language compilation system.
 *
 * The ULAM programming language compilation system is free software:
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * The ULAM programming language compilation system is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the ULAM programming language compilation system
 * software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

/**
  \file SymbolClassName.h -  Class Symbol "Template" for ULAM
  \author Elenas S. Ackley.
  \author David H. Ackley.
  \date (C) 2015 All rights reserved.
  \gpl
*/

#ifndef SYMBOLCLASSNAME_H
#define SYMBOLCLASSNAME_H

#include "SymbolClass.h"
#include "SymbolConstantValue.h"

namespace MFM{

  class SymbolClassName : public SymbolClass
  {
  public:
    SymbolClassName(u32 id, UTI utype, NodeBlockClass * classblock, CompilerState& state);
    virtual ~SymbolClassName();

    void addParameterSymbol(SymbolConstantValue * argSym);
    u32 getNumberOfParameters();
    u32 getTotalSizeOfParameters();
    Symbol * getParameterSymbolPtr(u32 n);

    virtual bool isClassTemplate(UTI cuti);
    bool findClassInstanceByUTI(UTI uti, SymbolClass * & symptrref);
    bool findClassInstanceByArgString(UTI cuti, SymbolClass *& csymptr);

    void addClassInstanceUTI(UTI uti, SymbolClass * symptr);
    void addClassInstanceByArgString(UTI uti, SymbolClass * symptr);

    bool pendingClassArgumentsForShallowClassInstance(UTI instance);

    SymbolClass * makeAShallowClassInstance(Token typeTok, UTI cuti); //to hold class args, and cUTI
    void copyAShallowClassInstance(UTI instance, UTI newuti);

    /** replaces temporary class argument names, updates the ST, and the class type */
    void fixAnyClassInstances();

    void linkUnknownBitsizeConstantExpression(UTI auti, NodeTypeBitsize * ceNode);
    void linkUnknownBitsizeConstantExpression(UTI fromtype, UTI totype); // for decllist
    void linkUnknownArraysizeConstantExpression(UTI auti, NodeSquareBracket * ceNode);
    void linkUnknownNamedConstantExpression(NodeConstantDef * ceNode);

    bool statusUnknownConstantExpressionsInClassInstances();
    bool statusNonreadyClassArgumentsInShallowClassInstances();
    bool constantFoldClassArgumentsInAShallowClassInstance(UTI instance);

    std::string formatAnInstancesArgValuesAsAString(UTI instance);

    //helpers while deep instantiation
    bool hasInstanceMappedUTI(UTI instance, UTI auti, UTI& mappedUTI);
    void mapInstanceUTI(UTI instance, UTI auti, UTI mappeduti);

    bool cloneInstances(); //i.e. instantiate
    Node * findNodeNoInAClassInstance(UTI instance, NNO n);
    void constantFoldIncompleteUTIOfClassInstance(UTI instance, UTI auti);

    void updateLineageOfClassInstanceUTI(UTI cuti);
    void checkCustomArraysOfClassInstances();

    void checkAndLabelClassInstances();

    u32 countNavNodesInClassInstances();
    bool setBitSizeOfClassInstances();
    void printBitSizeOfClassInstances();
    void packBitsForClassInstances();

    void testForClassInstances(File * fp);

    void generateCodeForClassInstances(FileManager * fm);

    void generateIncludesForClassInstances(File * fp);

    void generateForwardDefsForClassInstances(File * fp);

    void generateTestInstanceForClassInstances(File * fp, bool runtest);

   protected:

  private:
    //ordered class parameters
    std::vector<SymbolConstantValue *> m_parameterSymbols;  // like named constants; symbols owned by m_ST.
    std::map<UTI, SymbolClass* > m_scalarClassInstanceIdxToSymbolPtr;
    std::map<std::string, SymbolClass* > m_scalarClassArgStringsToSymbolPtr; //merged set
    std::map<UTI, std::map<UTI,UTI> > m_mapOfTemplateUTIToInstanceUTIPerClassInstance;

    bool takeAnInstancesArgValues(SymbolClass * fm, SymbolClass * to);
    void cloneResolverForClassInstance(SymbolClass * csym);
  };

}

#endif //end SYMBOLCLASSNAME_H

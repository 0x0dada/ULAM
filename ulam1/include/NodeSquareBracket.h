/**                                        -*- mode:C++ -*-
 * NodeSquareBracket.h - Basic Node for handling 
 *                               Array Subscripts for ULAM
 *
 * Copyright (C) 2014 The Regents of the University of New Mexico.
 * Copyright (C) 2014 Ackleyshack LLC.
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
  \file NodeSquareBracket.h - Basic Node for handling Array Subscripts for ULAM
  \author Elenas S. Ackley.
  \author David H. Ackley.
  \date (C) 2014 All rights reserved.
  \gpl
*/


#ifndef NODEBINARYOPSQUAREBRACKET_H
#define NODEBINARYOPSQUAREBRACKET_H

#include "NodeBinaryOp.h"

namespace MFM{

  class NodeSquareBracket : public NodeBinaryOp
  {
  public:
    
    NodeSquareBracket(Node * left, Node * right, CompilerState & state);
    ~NodeSquareBracket();

    virtual void printOp(File * fp);

    virtual UTI checkAndLabelType();

    virtual EvalStatus eval();
    virtual EvalStatus evalToStoreInto();

    virtual bool getSymbolPtr(Symbol *& symptrref);

    virtual bool installSymbolTypedef(Token atok, u32 bitsize, u32 arraysize, Symbol *& asymptr);
    virtual bool installSymbolVariable(Token atok, u32 bitsize, u32 arraysize, Symbol *& asymptr);

    virtual const char * getName();

    virtual const std::string prettyNodeName();

    virtual void genCode(File * fp);

  protected:

  private:
    //helper method to install symbol
    bool getArraysizeInBracket(u32 & rtnArraySize);

    virtual void doBinaryOperation(s32 lslot, s32 rslot, u32 slots){}
    virtual UlamValue makeImmediateBinaryOp(UTI type, u32 ldata, u32 rdata, u32 len);
    virtual void appendBinaryOp(UlamValue& refUV, u32 ldata, u32 rdata, u32 pos, u32 len){}

  };

}

#endif //end NODEBINARYOPSQUAREBRACKET_H

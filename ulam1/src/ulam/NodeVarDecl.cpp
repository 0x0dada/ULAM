#include <stdlib.h>
#include "NodeVarDecl.h"
#include "Token.h"
#include "CompilerState.h"
#include "SymbolVariableDataMember.h"
#include "SymbolVariableStack.h"
#include "NodeIdent.h"

namespace MFM {

  NodeVarDecl::NodeVarDecl(SymbolVariable * sym, NodeTypeDescriptor * nodetype, CompilerState & state) : Node(state), m_varSymbol(sym), m_vid(0), m_currBlockNo(0), m_nodeTypeDesc(nodetype)
  {
    if(sym)
      {
	m_vid = sym->getId();
	m_currBlockNo = sym->getBlockNoOfST();
      }
  }

  NodeVarDecl::NodeVarDecl(const NodeVarDecl& ref) : Node(ref), m_varSymbol(NULL), m_vid(ref.m_vid), m_currBlockNo(ref.m_currBlockNo), m_nodeTypeDesc(NULL)
  {
    if(ref.m_nodeTypeDesc)
      m_nodeTypeDesc = (NodeTypeDescriptor *) ref.m_nodeTypeDesc->instantiate();
  }

  NodeVarDecl::~NodeVarDecl()
  {
    delete m_nodeTypeDesc;
    m_nodeTypeDesc = NULL;
  }

  Node * NodeVarDecl::instantiate()
  {
    return new NodeVarDecl(*this);
  }

  void NodeVarDecl::updateLineage(NNO pno)
  {
    Node::updateLineage(pno);
    if(m_nodeTypeDesc)
      m_nodeTypeDesc->updateLineage(getNodeNo());
  } //updateLineage

  bool NodeVarDecl::findNodeNo(NNO n, Node *& foundNode)
  {
    if(Node::findNodeNo(n, foundNode))
      return true;
    if(m_nodeTypeDesc && m_nodeTypeDesc->findNodeNo(n, foundNode))
      return true;
    return false;
  } //findNodeNo

  // see also SymbolVariable: printPostfixValuesOfVariableDeclarations via ST.
  void NodeVarDecl::printPostfix(File * fp)
  {
    printTypeAndName(fp);
    fp->write("; ");
  } //printPostfix

  void NodeVarDecl::printTypeAndName(File * fp)
  {
    UTI vuti = m_varSymbol->getUlamTypeIdx();
    UlamKeyTypeSignature vkey = m_state.getUlamKeyTypeSignatureByIndex(vuti);
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);

    fp->write(" ");
    if(vut->getUlamTypeEnum() != Class)
      fp->write(vkey.getUlamKeyTypeSignatureNameAndBitSize(&m_state).c_str());
    else
      fp->write(vut->getUlamTypeNameBrief().c_str());

    fp->write(" ");
    fp->write(getName());

    s32 arraysize = m_state.getArraySize(vuti);
    if(arraysize > NONARRAYSIZE)
      {
	fp->write("[");
	fp->write_decimal(arraysize);
	fp->write("]");
      }
    else if(arraysize == UNKNOWNSIZE)
      {
	fp->write("[UNKNOWN]");
      }
  } //printTypeAndName

  const char * NodeVarDecl::getName()
  {
    return m_state.m_pool.getDataAsString(m_varSymbol->getId()).c_str();
  }

  const std::string NodeVarDecl::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }

  bool NodeVarDecl::getSymbolPtr(Symbol *& symptrref)
  {
    symptrref = m_varSymbol;
    return true;
  }

  UTI NodeVarDecl::checkAndLabelType()
  {
    UTI it = Nav;

    // instantiate, look up in current block
    if(m_varSymbol == NULL)
      checkForSymbol();

    if(m_varSymbol)
      {
	it = m_varSymbol->getUlamTypeIdx(); //base type has arraysize

	UTI cuti = m_state.getCompileThisIdx();
	if(m_nodeTypeDesc)
	  {
	    UTI duti = m_nodeTypeDesc->checkAndLabelType(); //sets goagain
	    if(duti != Nav && duti != it)
	      {
		std::ostringstream msg;
		msg << "REPLACING Symbol UTI" << it;
		msg << ", " << m_state.getUlamTypeNameBriefByIndex(it).c_str();
		msg << " used with variable symbol name '" << getName();
		msg << "' with node type descriptor type: ";
		msg << m_state.getUlamTypeNameBriefByIndex(duti).c_str();
		msg << " UTI" << duti << " while labeling class: ";
		msg << m_state.getUlamTypeNameBriefByIndex(cuti).c_str();
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
		m_varSymbol->resetUlamType(duti); //consistent!
		m_state.mapTypesInCurrentClass(it, duti);
		it = duti;
	      }
	  }

	  if(!m_state.isComplete(it))
	  {
	    std::ostringstream msg;
	    msg << "Incomplete Variable Decl for type: ";
	    msg << m_state.getUlamTypeNameBriefByIndex(it).c_str();
	    msg << " used with variable symbol name '" << getName();
	    msg << "' UTI" << it << " while labeling class: ";
	    msg << m_state.getUlamTypeNameBriefByIndex(cuti).c_str();
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
	    it = Nav;
	    m_state.setGoAgain(); //since not error
	  }
      } //end var_symbol

    ULAMTYPE etyp = m_state.getUlamTypeByIndex(it)->getUlamTypeEnum();
    if(etyp == Void)
      {
	//void only valid use is as a func return type
	std::ostringstream msg;
	msg << "Invalid use of type ";
	msg << m_state.getUlamTypeNameBriefByIndex(it).c_str();
	msg << " with variable symbol name '" << getName() << "'";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	it = Nav;
      }

    setNodeType(it);
    return getNodeType();
  } //checkAndLabelType

  void NodeVarDecl::checkForSymbol()
  {
    //in case of a cloned unknown
    NodeBlock * currBlock = getBlock();
    m_state.pushCurrentBlockAndDontUseMemberBlock(currBlock);

    Symbol * asymptr = NULL;
    bool hazyKin = false;
    if(m_state.alreadyDefinedSymbol(m_vid, asymptr, hazyKin) && !hazyKin)
      {
	if(!asymptr->isTypedef() && !asymptr->isConstant() && !asymptr->isModelParameter() && !asymptr->isFunction())
	  {
	    m_varSymbol = (SymbolVariable *) asymptr;
	  }
	else
	  {
	    std::ostringstream msg;
	    msg << "(1) <" << m_state.m_pool.getDataAsString(m_vid).c_str();
	    msg << "> is not a variable, and cannot be used as one";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	  }
      }
    else
      {
	std::ostringstream msg;
	msg << "(2) Variable <" << m_state.m_pool.getDataAsString(m_vid).c_str();
	msg << "> is not defined, and cannot be used";
	if(!hazyKin)
	  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	else
	  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
      } //alreadyDefined

    m_state.popClassContext(); //restore
  } //checkForSymbol

  NNO NodeVarDecl::getBlockNo()
  {
    return m_currBlockNo;
  }

  NodeBlock * NodeVarDecl::getBlock()
  {
    assert(m_currBlockNo);
    NodeBlock * currBlock = (NodeBlock *) m_state.findNodeNoInThisClass(m_currBlockNo);
    assert(currBlock);
    return currBlock;
  }

  void NodeVarDecl::packBitsInOrderOfDeclaration(u32& offset)
  {
    assert((s32) offset >= 0); //neg is invalid
    assert(m_varSymbol);

    m_varSymbol->setPosOffset(offset);
    UTI it = m_varSymbol->getUlamTypeIdx();

    if(!m_state.isComplete(it))
      {
	UTI cuti = m_state.getCompileThisIdx();
	UTI mappedUTI = Nav;
	if(m_state.mappedIncompleteUTI(cuti, it, mappedUTI))
	  {
	    std::ostringstream msg;
	    msg << "Substituting Mapped UTI" << mappedUTI;
	    msg << ", " << m_state.getUlamTypeNameBriefByIndex(mappedUTI).c_str();
	    msg << " for incomplete Variable Decl for type: ";
	    msg << m_state.getUlamTypeNameBriefByIndex(it).c_str();
	    msg << " used with variable symbol name '" << getName();
	    msg << "' (UTI" << it << ") while bit packing class: ";
	    msg << m_state.getUlamTypeNameBriefByIndex(cuti).c_str();
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
	    it = mappedUTI;
	    m_varSymbol->resetUlamType(it); //consistent!
	  }

	if(!m_state.isComplete(it)) //reloads
	  {
	    std::ostringstream msg;
	    msg << "Unresolved type <";
	    msg << m_state.getUlamTypeNameBriefByIndex(it).c_str();
	    msg << "> used with variable symbol name '" << getName();
	    msg << "' found while bit packing class ";
	    msg << m_state.getUlamTypeNameBriefByIndex(cuti).c_str();
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	  }
      } //not complete

    ULAMCLASSTYPE classtype = m_state.getUlamTypeByIndex(it)->getUlamClass();
    if(m_state.getTotalBitSize(it) > MAXBITSPERLONG && classtype == UC_NOTACLASS)
      {
	std::ostringstream msg;
	msg << "Data member <" << getName() << "> of type: ";
	msg << m_state.getUlamTypeNameBriefByIndex(it).c_str();
	msg << ", total size: " << (s32) m_state.getTotalBitSize(it);
	msg << " MUST fit into " << MAXBITSPERLONG << " bits;";
	msg << " Local variables do not have this restriction";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
      }

    if(m_state.getTotalBitSize(it) > MAXBITSPERINT && classtype == UC_QUARK)
      {
	std::ostringstream msg;
	msg << "Data member <" << getName() << "> of type: ";
	msg << m_state.getUlamTypeNameBriefByIndex(it).c_str();
	msg << ", total size: " << (s32) m_state.getTotalBitSize(it);
	msg << " MUST fit into " << MAXBITSPERINT << " bits;";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
      }

    if(classtype == UC_ELEMENT)
      {
	std::ostringstream msg;
	msg << "Data member <" << getName() << "> of type: ";
	msg << m_state.getUlamTypeNameBriefByIndex(it).c_str();
	msg << ", is an element, and is NOT permitted; Local variables, quarks, ";
	msg << "and Model Parameters do not have this restriction";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
      }

    offset += m_state.getTotalBitSize(m_varSymbol->getUlamTypeIdx());
  } //packBitsInOrderOfDeclaration

  void NodeVarDecl::calcMaxDepth(u32& depth, u32& maxdepth, s32 base)
  {
    assert(m_varSymbol);
    s32 newslot = depth + base;
    s32 oldslot = ((SymbolVariable *) m_varSymbol)->getStackFrameSlotIndex();
    if(oldslot != newslot)
      {
	std::ostringstream msg;
	msg << "'" << m_state.m_pool.getDataAsString(m_vid).c_str();
	msg << "' was at slot: " << oldslot << ", new slot is " << newslot;
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
	((SymbolVariable *) m_varSymbol)->setStackFrameSlotIndex(newslot);
      }
    depth += m_state.slotsNeeded(getNodeType());
  } //calcMaxDepth

  void NodeVarDecl::countNavNodes(u32& cnt)
  {
    if(m_nodeTypeDesc)
      m_nodeTypeDesc->countNavNodes(cnt);

    Node::countNavNodes(cnt);
  } //countNavNodes

  EvalStatus NodeVarDecl::eval()
  {
    assert(m_varSymbol);

    UTI nuti = getNodeType();
    if(nuti == Nav)
      return ERROR;

    assert(m_varSymbol->getUlamTypeIdx() == nuti); //is it so? if so, some cleanup needed

    if(m_varSymbol->isAutoLocal())
      return evalAutoLocal();

    ULAMCLASSTYPE classtype = m_state.getUlamTypeByIndex(nuti)->getUlamClass();
    if(nuti == UAtom)
      {
	UlamValue atomUV = UlamValue::makeAtom(m_varSymbol->getUlamTypeIdx());
	m_state.m_funcCallStack.storeUlamValueInSlot(atomUV, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex());
      }
    else if(classtype == UC_ELEMENT)
      {
	UlamValue atomUV = UlamValue::makeDefaultAtom(m_varSymbol->getUlamTypeIdx(), m_state);
	m_state.m_funcCallStack.storeUlamValueInSlot(atomUV, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex());
      }
    else if(!m_varSymbol->isDataMember())
      {
	if(classtype == UC_NOTACLASS)
	  {
	    //local variable to a function;
	    // t.f. must be SymbolVariableStack, not SymbolVariableDataMember
	    u32 len = m_state.getTotalBitSize(nuti);
	    UlamValue immUV;
	    if(len <= MAXBITSPERINT)
	      immUV = UlamValue::makeImmediate(m_varSymbol->getUlamTypeIdx(), 0, m_state);
	    else if(len <= MAXBITSPERLONG)
	      immUV = UlamValue::makeImmediateLong(m_varSymbol->getUlamTypeIdx(), 0, m_state);
	    else
	      immUV = UlamValue::makePtr(((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex(), STACK, getNodeType(), m_state.determinePackable(getNodeType()), m_state, 0, m_varSymbol->getId()); //array ptr

	    m_state.m_funcCallStack.storeUlamValueInSlot(immUV, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex());
	  }
	else
	  {
	    //must be a local quark!
	    u32 dq = 0;
	    assert(m_state.getDefaultQuark(nuti, dq));
	    if(m_state.isScalar(nuti))
	      {
		UlamValue immUV = UlamValue::makeImmediate(nuti, dq, m_state);
		m_state.m_funcCallStack.storeUlamValueInSlot(immUV, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex());
	      }
	    else
	      {
		u32 len = m_state.getTotalBitSize(nuti);
		u32 bitsize = m_state.getBitSize(nuti);
		u32 pos = 0;
		u32 arraysize = m_state.getArraySize(nuti);
		u32 mask = _GetNOnes32((u32) bitsize);
		dq &= mask;
		if(len <= MAXBITSPERINT)
		  {
		    u32 dqarrval = 0;
		    //similar to build default quark value in NodeVarDeclDM
		    for(u32 j = 1; j <= arraysize; j++)
		      {
			dqarrval |= (dq << (len - (pos + (j * bitsize))));
		      }

		    UlamValue immUV = UlamValue::makeImmediate(nuti, dqarrval, m_state);
		    m_state.m_funcCallStack.storeUlamValueInSlot(immUV, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex());
		  }
		else if(len <= MAXBITSPERLONG)
		  {
		    u64 dqarrval = 0;
		    //similar to build default quark value in NodeVarDeclDM
		    for(u32 j = 1; j <= arraysize; j++)
		      {
			dqarrval |= (dq << (len - (pos + (j * bitsize))));
		      }

		    UlamValue immUV = UlamValue::makeImmediateLong(nuti, dqarrval, m_state);
		    m_state.m_funcCallStack.storeUlamValueInSlot(immUV, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex());
		  }
		else
		  {
		    //unpacked array of quarks is unsupported at this time!!
		    assert(0);
		  }
	      }
	  }
      }
    else
      {
	//see NodeVarDeclDM for data members..
	assert(0);
      }
    return NORMAL;
  } //eval

  EvalStatus NodeVarDecl::evalAutoLocal()
  {
    assert(m_varSymbol);

    UTI nuti = getNodeType();
    if(nuti == Nav)
      return ERROR;

    assert(m_varSymbol->getUlamTypeIdx() == nuti);
    assert(nuti != UAtom); //rhs type of conditional as/has can't be an atom

    UlamValue pluv = m_state.m_currentAutoObjPtr;
    ((SymbolVariableStack *) m_varSymbol)->setAutoPtrForEval(pluv); //for future ident eval uses
    ((SymbolVariableStack *) m_varSymbol)->setAutoStorageTypeForEval(m_state.m_currentAutoStorageType); //for future virtual function call eval uses

    m_state.m_funcCallStack.storeUlamValueInSlot(pluv, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex()); //doesn't seem to matter..

    return NORMAL;
  } //evalAutoLocal

  EvalStatus  NodeVarDecl::evalToStoreInto()
  {
    assert(0); //no way to get here!
    return ERROR;
  }

  // parse tree in order declared, unlike the ST.
  void NodeVarDecl::genCode(File * fp, UlamValue& uvpass)
  {
    assert(m_varSymbol);
    assert(getNodeType() != Nav);

    if(m_varSymbol->isDataMember())
      {
	return genCodedBitFieldTypedef(fp, uvpass);
      }

    if(m_varSymbol->isAutoLocal())
      {
	return genCodedAutoLocal(fp, uvpass);
      }

    UTI vuti = m_varSymbol->getUlamTypeIdx();
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);

    m_state.indent(fp);
    if(!m_varSymbol->isDataMember())
      {
	fp->write(vut->getImmediateStorageTypeAsString().c_str()); //for C++ local vars
      }
    else
      {
	fp->write(vut->getUlamTypeMangledName().c_str()); //for C++
	assert(0); //doesn't happen anymore..
      }

    fp->write(" ");
    fp->write(m_varSymbol->getMangledName().c_str());

    ULAMCLASSTYPE vclasstype = vut->getUlamClass();

    //initialize T to default atom (might need "OurAtom" if data member ?)
    if(vclasstype == UC_ELEMENT)
      {
	fp->write(" = ");
	fp->write(m_state.getUlamTypeByIndex(vuti)->getUlamTypeMangledName().c_str());
	fp->write("<EC>");
	fp->write("::THE_INSTANCE");
	fp->write(".GetDefaultAtom()"); //returns object of type T
      }

    if(vclasstype == UC_QUARK)
      {
	//left-justified?
      }

    fp->write(";\n"); //func call parameters aren't NodeVarDecl's
  } //genCode


  // variable is a data member; not an element
  void NodeVarDecl::genCodedBitFieldTypedef(File * fp, UlamValue& uvpass)
  {
    UTI nuti = getNodeType();
    UlamType * nut = m_state.getUlamTypeByIndex(nuti);
    ULAMCLASSTYPE nclasstype = nut->getUlamClass();
    ULAMCLASSTYPE classtype = m_state.getUlamClassForThisClass();

    m_state.indent(fp);
    if(nclasstype == UC_QUARK && nut->isScalar())
      {
	// use typedef rather than atomic parameter for quarks within elements,
	// except if an array of quarks.
	fp->write("typedef ");
	fp->write(nut->getUlamTypeMangledName().c_str()); //for C++
	fp->write("<EC, ");
	if(classtype == UC_ELEMENT)
	  {
	    fp->write_decimal_unsigned(m_varSymbol->getPosOffset() + ATOMFIRSTSTATEBITPOS);
	    fp->write("u");
	  }
	else
	  {
	    //inside a quark
	    fp->write("POS + ");
	    fp->write_decimal_unsigned(m_varSymbol->getPosOffset());
	    fp->write("u");
	  }
      }
    else
      {
	fp->write("typedef AtomicParameterType");
	fp->write("<EC"); //BITSPERATOM
	fp->write(", ");
	fp->write(nut->getUlamTypeVDAsStringForC().c_str());
	fp->write(", ");
	fp->write_decimal(nut->getTotalBitSize());   //include arraysize
	fp->write(", ");
	if(classtype == UC_QUARK)
	  {
	    fp->write("POS + ");
	    fp->write_decimal(m_varSymbol->getPosOffset());
	  }
	else
	  {
	    assert(classtype == UC_ELEMENT);
	    fp->write_decimal_unsigned(m_varSymbol->getPosOffset() + ATOMFIRSTSTATEBITPOS);
	    fp->write("u");
	  }
      }
    fp->write("> ");
    fp->write(m_varSymbol->getMangledNameForParameterType().c_str());
    fp->write(";\n"); //func call parameters aren't NodeVarDecl's
  } //genCodedBitFieldTypedef

  // this is the auto local variable's node, created at parse time,
  // for Conditional-As case.
  void NodeVarDecl::genCodedAutoLocal(File * fp, UlamValue & uvpass)
  {
    assert(!m_state.m_currentObjSymbolsForCodeGen.empty());
    // the uvpass comes from NodeControl, and still has the POS obtained
    // during the condition statement for As..unorthodox, but necessary.
    assert(uvpass.getUlamValueTypeIdx() == Ptr);
    s32 tmpVarPos = uvpass.getPtrSlotIndex();

    // before shadowing the lhs of the h/as-conditional variable with its auto,
    // let's load its storage from the currentSelfSymbol:
    s32 tmpVarStg = m_state.getNextTmpVarNumber();
    UTI stguti = m_state.m_currentObjSymbolsForCodeGen[0]->getUlamTypeIdx();
    UlamType * stgut = m_state.getUlamTypeByIndex(stguti);
    assert(stguti == UAtom || stgut->getUlamClass() == UC_ELEMENT); //not quark???

    // can't let Node::genCodeReadIntoTmpVar do this for us: it's a ref.
    assert(m_state.m_currentObjSymbolsForCodeGen.size() == 1);
    m_state.indent(fp);
    fp->write(stgut->getTmpStorageTypeAsString().c_str());
    fp->write("& "); //here it is!!
    fp->write(m_state.getTmpVarAsString(stguti, tmpVarStg, TMPBITVAL).c_str());
    fp->write(" = ");
    fp->write(m_state.m_currentObjSymbolsForCodeGen[0]->getMangledName().c_str());

    if(m_varSymbol->getId() != m_state.m_pool.getIndexForDataString("atom")) //not isSelf check; was "self"
      fp->write(".getRef()");
    fp->write(";\n");

    // now we have our pos in tmpVarPos, and our T in tmpVarStg
    // time to shadow 'self' with auto local variable:
    UTI vuti = m_varSymbol->getUlamTypeIdx();
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);
    ULAMCLASSTYPE vclasstype = vut->getUlamClass();

    m_state.indent(fp);
    fp->write(vut->getUlamTypeImmediateAutoMangledName().c_str()); //for C++ local vars, ie non-data members
    if(vclasstype == UC_ELEMENT)
      fp->write("<EC> ");
    else //QUARK
      {
	fp->write("<EC, ");
	fp->write_decimal_unsigned(m_varSymbol->getPosOffset()); //POS should be 0+25 for inheritance
	fp->write("u + T::ATOM_FIRST_STATE_BIT >");
      }

    fp->write(m_varSymbol->getMangledName().c_str());
    fp->write("(");
    fp->write(m_state.getTmpVarAsString(stguti, tmpVarStg, TMPBITVAL).c_str());

    if(vclasstype == UC_QUARK)
      {
	fp->write(", ");
	if(m_state.m_genCodingConditionalHas) //not sure this is posoffset, and not true/false???
	  fp->write(m_state.getTmpVarAsString(uvpass.getPtrTargetType(), tmpVarPos).c_str());
	else
	  {
	    assert(m_varSymbol->getPosOffset() == 0);
	    fp->write_decimal_unsigned(m_varSymbol->getPosOffset()); //should be 0!
	  }
      }
    else if(vclasstype == UC_ELEMENT)
      {
	//fp->write(", true"); //invokes 'badass' constructor
      }
    else
      assert(0);

    fp->write(");   //shadows lhs of 'h/as'\n");

    m_state.m_genCodingConditionalHas = false; // done
    m_state.m_currentObjSymbolsForCodeGen.clear(); //clear remnant of lhs ?
  } //genCodedAutoLocal

  void NodeVarDecl::generateUlamClassInfo(File * fp, bool declOnly, u32& dmcount)
  {
    assert(0); //see NodeVarDeclDM data members only
  } //generateUlamClassInfo

} //end MFM

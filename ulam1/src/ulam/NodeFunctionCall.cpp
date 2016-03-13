#include "NodeFunctionCall.h"
#include "CompilerState.h"
#include "NodeBlockFunctionDefinition.h"
#include "SymbolFunction.h"
#include "SymbolFunctionName.h"
#include "SymbolVariableDataMember.h"
#include "SymbolVariableStack.h"
#include "CallStack.h"


namespace MFM {

  NodeFunctionCall::NodeFunctionCall(Token tok, SymbolFunction * fsym, CompilerState & state) : Node(state), m_functionNameTok(tok), m_funcSymbol(fsym), m_argumentNodes(NULL)
  {
    m_argumentNodes = new NodeList(state);
    assert(m_argumentNodes);
    m_argumentNodes->setNodeLocation(tok.m_locator); //same as func call
  }

  NodeFunctionCall::NodeFunctionCall(const NodeFunctionCall& ref) : Node(ref), m_functionNameTok(ref.m_functionNameTok), m_funcSymbol(NULL), m_argumentNodes(NULL)
  {
    m_argumentNodes = (NodeList *) ref.m_argumentNodes->instantiate();
  }

  NodeFunctionCall::~NodeFunctionCall()
  {
    delete m_argumentNodes;
    m_argumentNodes = NULL;
  }

  Node * NodeFunctionCall::instantiate()
  {
    return new NodeFunctionCall(*this);
  }

  void NodeFunctionCall::updateLineage(NNO pno)
  {
    setYourParentNo(pno);
    m_argumentNodes->updateLineage(getNodeNo());
  } //updateLineage

  bool NodeFunctionCall::exchangeKids(Node * oldnptr, Node * newnptr)
  {
    return m_argumentNodes->exchangeKids(oldnptr, newnptr);
  } //exchangeKids

  bool NodeFunctionCall::findNodeNo(NNO n, Node *& foundNode)
  {
    if(Node::findNodeNo(n, foundNode))
      return true;

    if(m_argumentNodes->findNodeNo(n, foundNode))
      return true;
    return false;
  } //findNodeNo

  void NodeFunctionCall::printPostfix(File * fp)
  {
    fp->write(" (");
    m_argumentNodes->printPostfix(fp);
    fp->write(" )");
    fp->write(getName());
  } //printPostfix

  const char * NodeFunctionCall::getName()
  {
    return m_state.getTokenDataAsString(&m_functionNameTok).c_str();
  }

  const std::string NodeFunctionCall::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }

  FORECAST NodeFunctionCall::safeToCastTo(UTI newType)
  {
    UlamType * newut = m_state.getUlamTypeByIndex(newType);
    //ulamtype checks for complete, non array, and type specific rules
    return newut->safeCast(getNodeType());
  } //safeToCastTo

  UTI NodeFunctionCall::checkAndLabelType()
  {
    UTI it = Nav;  // init return type
    u32 numErrorsFound = 0;
    u32 numHazyFound = 0;

    //might be related to m_currentSelfPtr?
    //member selection doesn't apply to arguments
    //look up in class block, and match argument types to parameters
    SymbolFunction * funcSymbol = NULL;
    Symbol * fnsymptr = NULL;

    //label argument types; used to pinpoint the exact function symbol in case of overloading
    std::vector<Node *> argNodes;
    u32 constantArgs = 0;
    u32 navArgs = 0;
    u32 hzyArgs = 0;
    u32 noutiArgs = 0;
    UTI listuti = Nav;
    bool hazyKin = false;

    if(m_state.isFuncIdInClassScope(m_functionNameTok.m_dataindex, fnsymptr, hazyKin) && !hazyKin)
      {
        //use member block doesn't apply to arguments; no change to current block
	m_state.pushCurrentBlockAndDontUseMemberBlock(m_state.getCurrentBlock()); //set forall args
	listuti = m_argumentNodes->checkAndLabelType(); //plus side-effect; void return is ok

	u32 numargs = getNumberOfArguments();
	for(u32 i = 0; i < numargs; i++)
	  {
	    UTI argtype = m_argumentNodes->getNodeType(i); //plus side-effect
	    argNodes.push_back(m_argumentNodes->getNodePtr(i));
	    if(argtype == Nav)
	      navArgs++;
	    else if(argtype == Hzy)
	      hzyArgs++;
	    else if(argtype == Nouti)
	      noutiArgs++;

	    // track constants and potential casting to be handled
	    if(m_argumentNodes->isAConstant(i))
	      constantArgs++;
	  }
	m_state.popClassContext(); //restore here

	if(navArgs)
	  {
	    argNodes.clear();
	    setNodeType(Nav);
	    return Nav; //short circuit
	  }

	if(hzyArgs || noutiArgs)
	  {
	    argNodes.clear();
	    setNodeType(Hzy);
	    m_state.setGoAgain(); //for compier counts
	    return Hzy; //short circuit
	  }

	// still need to pinpoint the SymbolFunction for m_funcSymbol!
	// exact match if possible; o.w. allow safe casts to find matches
	bool hasHazyArgs = false;
	u32 numFuncs = ((SymbolFunctionName *) fnsymptr)->findMatchingFunctionWithSafeCasts(argNodes, funcSymbol, hasHazyArgs);
	if(numFuncs == 0)
	  {
	    std::ostringstream msg;
	    msg << "(1) <" << m_state.getTokenDataAsString(&m_functionNameTok).c_str();
	    msg << "> has no defined function with " << numargs;
	    msg << " matching argument type";
	    if(numargs != 1)
	      msg << "s";
	    msg << ": ";
	    for(u32 i = 0; i < argNodes.size(); i++)
	      {
		UTI auti = argNodes[i]->getNodeType();
		msg << m_state.getUlamTypeNameBriefByIndex(auti).c_str() << ", ";
	      }
	    msg << "and cannot be called";
	    if(hasHazyArgs)
	      {
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
		numHazyFound++;
	      }
	    else
	      {
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		numErrorsFound++;
	      }
	  }
	else if(numFuncs > 1)
	  {
	    std::ostringstream msg;
	    msg << "Ambiguous matches (" << numFuncs << ") of function <";
	    msg << m_state.getTokenDataAsString(&m_functionNameTok).c_str();
	    msg << "> called with " << numargs << " argument type";
	    if(numargs != 1)
	      msg << "s";
	    msg << ": ";
	    for(u32 i = 0; i < argNodes.size(); i++)
	      {
		UTI auti = argNodes[i]->getNodeType();
		msg << m_state.getUlamTypeNameBriefByIndex(auti).c_str() << ", ";
	      }
	    msg << "explicit casting is required";
	    if(hasHazyArgs)
	      {
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
		numHazyFound++;
	      }
	    else
	      {
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		numErrorsFound++;
	      }
	  }
	else //==1
	  {
	    if(hasHazyArgs)
	      numHazyFound++; //wait to cast

	    //check ref types and func calls here..care with variable args (2 pass)
	    u32 numParams = funcSymbol->getNumberOfParameters();
	    for(u32 i = 0; i < numParams; i++)
	      {
		if(m_state.isReference(funcSymbol->getParameterType(i)))
		  {
		    //if(argNodes[i]->isFunctionCall())
		    TBOOL argstor = argNodes[i]->getStoreIntoAble();
		    if(argstor != TBOOL_TRUE)
		      {
			std::ostringstream msg;
			msg << "Invalid argument " << i + 1 << " to function <";
			msg << m_state.getTokenDataAsString(&m_functionNameTok).c_str();
			//msg << "> is a function call, and cannot be used as a reference parameter";
			msg << ">; Cannot be used as a reference parameter";
			if(argstor == TBOOL_HAZY)
			  {
			    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
			    numHazyFound++;
			  }
			else
			  {
			    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
			    numErrorsFound++;
			  }
		      }
		  }
	      }
	    //don't worry about variable args in native methods; they can't be refs.
	  }
      }
    else
      {
	std::ostringstream msg;
	msg << "(2) <" << m_state.getTokenDataAsString(&m_functionNameTok).c_str();
	msg << "> is not a defined function, or cannot be safely called in this context";
	if(hazyKin)
	  {
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
	    numHazyFound++;
	  }
	else
	  {
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	    numErrorsFound++;
	  }
      }

    if(funcSymbol && m_funcSymbol != funcSymbol)
      {
	m_funcSymbol = funcSymbol;

	std::ostringstream msg;
	msg << "Substituting <" << funcSymbol->getMangledNameWithTypes().c_str() << "> ";
	if(m_funcSymbol)
	    msg << "for <" << m_funcSymbol->getMangledNameWithTypes().c_str() <<">";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
      }

    if(m_funcSymbol && m_funcSymbol != funcSymbol)
      {
	std::ostringstream msg;
	if(funcSymbol)
	  {
	    msg << "Substituting <" << funcSymbol->getMangledNameWithTypes().c_str();
	    msg << "> for <" << m_funcSymbol->getMangledNameWithTypes().c_str() <<">";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
	    m_funcSymbol = funcSymbol;
	  }
      }

    if((numErrorsFound == 0) && (numHazyFound == 0))
      {
	if(m_funcSymbol == NULL)
	  m_funcSymbol = funcSymbol;

	assert(m_funcSymbol && m_funcSymbol == funcSymbol);

	it = m_funcSymbol->getUlamTypeIdx();
	setNodeType(it);

	// insert safe casts of complete arg types, now that we have a "matching" function symbol
        //use member block doesn't apply to arguments; no change to current block
	m_state.pushCurrentBlockAndDontUseMemberBlock(m_state.getCurrentBlock()); //set forall args

	{
	  std::vector<u32> argsWithCastErr;
	  u32 argsWithCast = 0;
	  u32 numParams = m_funcSymbol->getNumberOfParameters();
	  for(u32 i = 0; i < numParams; i++)
	    {
	      Symbol * psym = m_funcSymbol->getParameterSymbolPtr(i);
	      UTI ptype = psym->getUlamTypeIdx();
	      Node * argNode = m_argumentNodes->getNodePtr(i); //constArgs[i];
	      UTI atype = argNode->getNodeType();
	      if(UlamType::compare(ptype, atype, m_state) == UTIC_NOTSAME) //o.w. known same
		{
		  Node * argCast = NULL;
		  if(!Node::makeCastingNode(argNode, ptype, argCast))
		    {
		      argsWithCastErr.push_back(i); //error!
		    }
		  m_argumentNodes->exchangeKids(argNode, argCast, i);
		  argsWithCast++;
		}
	    }

	  // do similar casting on any variable arg constants (without parameters)
	  if(m_funcSymbol->takesVariableArgs())
	    {
	      u32 numargs = getNumberOfArguments();
	      for(u32 i = numParams; i < numargs; i++)
		  {
		    if(argNodes[i]->isAConstant())
		      {
			Node * argCast = NULL;
			UTI auti = argNodes[i]->getNodeType();
			if(!Node::makeCastingNode(argNodes[i], m_state.getDefaultUlamTypeOfConstant(auti), argCast))
			  {
			    argsWithCastErr.push_back(i); //error!
			  }
			m_argumentNodes->exchangeKids(argNodes[i], argCast, i);
			argsWithCast++;
		      }
		  }
	    } //var args

	  if(!argsWithCastErr.empty())
	      {
		std::ostringstream msg;
		msg << "Casting errors for args with constants: " ;
		for(u32 i = 0; i < argsWithCastErr.size(); i++)
		  {
		    if(i > 0)
		      msg << ", ";
		    msg << "arg_" << i + 1;
		  }

		msg << " to function <";
		msg << m_state.getTokenDataAsString(&m_functionNameTok).c_str() <<">";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		argsWithCastErr.clear();
	      }
	} //constants

	m_state.popClassContext(); //restore here
      } // no errors found

    // late, important to do, but not too soon;
    // o.w. NodeIdents can't find their blocks.
    if((listuti == Nav) || (numErrorsFound > 0))
      {
	setNodeType(Nav); //happens when the arg list has erroneous types.
	it = Nav;
      }

    if((listuti == Hzy) || (numHazyFound > 0))
      {
	setNodeType(Hzy); //happens when the arg list has incomplete types.
	m_state.setGoAgain(); //for compier counts
	it = Hzy;
      }

    argNodes.clear();
    assert(m_funcSymbol || (getNodeType() == Nav) || (getNodeType() == Hzy));
    return it;
  } //checkAndLabelType

  void NodeFunctionCall::countNavHzyNoutiNodes(u32& ncnt, u32& hcnt, u32& nocnt)
  {
    Node::countNavHzyNoutiNodes(ncnt, hcnt, nocnt); //missing
    m_argumentNodes->countNavHzyNoutiNodes(ncnt, hcnt, nocnt);
  } //countNavHzyNoutiNodes

  void NodeFunctionCall::calcMaxDepth(u32& depth, u32& maxdepth, s32 base)
  {
    u32 argbase = 0;
    //allot enough stack space for the function call to another func
    u32 numargs = m_argumentNodes->getNumberOfNodes();
    for(u32 i = 0; i < numargs; i++)
      {
	u32 depthi = 0;
	u32 nomaxdepth = 0;
	u32 nobase = 0;
	m_argumentNodes->getNodePtr(i)->calcMaxDepth(depthi, nomaxdepth, nobase); //possible func call as arg
	u32 sloti = m_state.slotsNeeded(m_argumentNodes->getNodeType(i)); //just a variable or constant
	//take the greater
	argbase += depthi > sloti ? depthi : sloti;
      }

    argbase += m_state.slotsNeeded(getNodeType()); //return
    argbase += 1; //hidden
    depth += argbase;
  } //calcMaxDepth

  bool NodeFunctionCall::isFunctionCall()
  {
    return true;
  }

  // since functions are defined at the class-level; a function call
  // must be PRECEDED by a member selection (element or quark) --- a
  // local variable instance that provides the storage (i.e. atom) for
  // its data members on the STACK, as the first argument.
  EvalStatus NodeFunctionCall::eval()
  {
    UTI nuti = getNodeType();
    if(nuti == Nav)
      return ERROR;

    if(nuti == Hzy)
      return NOTREADY;

    assert(m_funcSymbol);
    NodeBlockFunctionDefinition * func = m_funcSymbol->getFunctionNode();
    assert(func);

    // before processing arguments, get the "self" atom ptr,
    // so that arguments will be relative to it, and not the possible
    // selected member instance this function body could effect.
    UlamValue saveCurrentObjectPtr = m_state.m_currentObjPtr; //*********
    m_state.m_currentObjPtr = m_state.m_currentSelfPtr;

    evalNodeProlog(0); //new current frame pointer on node eval stack
    u32 argsPushed = 0;
    EvalStatus evs;

    // for now we're going to bypass variable arguments for eval purposes
    // since our NodeFunctionDef has no way to know how many extra args to expect!
    u32 numargs = getNumberOfArguments();
    s32 diffInArgs = numargs - m_funcSymbol->getNumberOfParameters();
    assert(diffInArgs == 0 || m_funcSymbol->takesVariableArgs()); //sanity check

    // place values of arguments on call stack (reverse order) before calling function
    for(s32 i= numargs - diffInArgs - 1; i >= 0; i--)
      {
	UTI argType = m_argumentNodes->getNodeType(i);

	// extra slot for a Ptr to unpacked array;
	// arrays are handled by CS/callstack, and passed by value
	u32 slots = makeRoomForNodeType(argType); //for eval return

	UTI paramType = m_funcSymbol->getParameterType(i);
	UlamType * put = m_state.getUlamTypeByIndex(paramType);
	ALT paramreftype = put->getReferenceType();
	if(paramreftype == ALT_REF)
	  evs = m_argumentNodes->evalToStoreInto(i);
	else
	  evs = m_argumentNodes->eval(i);

	if(evs != NORMAL)
	  {
	    evalNodeEpilog();
	    return evs;
	  }

	// transfer to call stack
	if(slots==1)
	  {
	    UlamValue auv = m_state.m_nodeEvalStack.popArg();
	    if(paramreftype == ALT_REF && (auv.getPtrStorage() == STACK))
	      {
		assert(m_state.isPtr(auv.getUlamValueTypeIdx()));
		u32 absrefslot = m_state.m_funcCallStack.getAbsoluteStackIndexOfSlot(auv.getPtrSlotIndex());
		auv.setPtrSlotIndex(absrefslot);
		auv.setUlamValueTypeIdx(PtrAbs);
	      }
	    m_state.m_funcCallStack.pushArg(auv);
	    argsPushed++;
	  }
	else
	  {
	    //array
	    PACKFIT packed = m_state.determinePackable(argType);
	    assert(WritePacked(packed));

	    //array to transfer without reversing order again
	    u32 baseSlot = m_state.m_funcCallStack.getRelativeTopOfStackNextSlot();
	    argsPushed  += makeRoomForNodeType(argType, STACK);

	    //both either unpacked or packed
	    UlamValue basePtr = UlamValue::makePtr(baseSlot, STACK, argType, packed, m_state);

	    //positive to current frame pointer
	    UlamValue auvPtr = UlamValue::makePtr(1, EVALRETURN, argType, packed, m_state);

	    m_state.assignValue(basePtr, auvPtr);
	    m_state.m_nodeEvalStack.popArgs(slots);
	  }
      } //done with args

    //before pushing return slot(s) last (on both STACKS for now)
    UTI rtnType = m_funcSymbol->getUlamTypeIdx();
    s32 rtnslots = makeRoomForNodeType(rtnType);

    // insert "first" hidden arg (adjusted index pointing to atom);
    // atom index (negative) relative new frame, includes ALL the pushed args,
    // and upcoming rtnslots: current_atom_index - relative_top_index (+ returns)
    m_state.m_currentObjPtr = saveCurrentObjectPtr; // RESTORE *********
    UlamValue atomPtr = m_state.m_currentObjPtr; //*********

    //update func def (at eval time) based on class in virtual table
    // this requires symbol search like used in c&l and parsing;
    // t.f. we use the classblock stack to track the block ST's
    if(m_funcSymbol->isVirtualFunction())
      {
	u32 vtidx = m_funcSymbol->getVirtualMethodIdx();
	u32 atomid = atomPtr.getPtrNameId();
	if(atomid != 0)
	  {
	    //if auto local as, set shadowed lhs type (, and pos?)
	    Symbol * asym = NULL;
	    bool hazyKin = false;
	    if(m_state.alreadyDefinedSymbol(atomid, asym, hazyKin) && !hazyKin)
	      {
		ALT autolocaltype = asym->getAutoLocalType();
		if(autolocaltype == ALT_AS) //must be a class
		  {
		    atomPtr.setPtrTargetType(((SymbolVariableStack *) asym)->getAutoStorageTypeForEval());
		  }
		else if(autolocaltype == ALT_HAS)
		  {
		    assert(0); //deprecated
		    // auto type is the type of the data member,
		    // rather than the base (rhs)
		  }
		else if(autolocaltype == ALT_REF)
		  {
		    //unlike alt_as, alt_ref can be a primitive or a class
		    atomPtr.setPtrTargetType(((SymbolVariableStack *) asym)->getAutoStorageTypeForEval());
		  }
	      }
	  } //else can't be an autolocal

	UTI cuti = atomPtr.getPtrTargetType(); //must be a class
	SymbolClass * vcsym = NULL;
	AssertBool isDefined = m_state.alreadyDefinedSymbolClass(cuti, vcsym);
	assert(isDefined);
	UTI vtcuti = vcsym->getClassForVTableEntry(vtidx);

	//is the virtual class uti the same as what we already have?
	NNO funcstnno = m_funcSymbol->getBlockNoOfST();
	UTI funcclassuti = m_state.findAClassByNodeNo(funcstnno);
	if(funcclassuti != vtcuti)
	  {
	    SymbolClass * vtcsym = NULL;
	    AssertBool isDefined = m_state.alreadyDefinedSymbolClass(vtcuti, vtcsym);
	    assert(isDefined);

	    NodeBlockClass * memberClassNode = vtcsym->getClassBlockNode();
	    assert(memberClassNode);  //e.g. forgot the closing brace on quark definition
	    //set up compiler state to use the member class block for symbol searches
	    m_state.pushClassContextUsingMemberClassBlock(memberClassNode);

	    Symbol * fnsymptr = NULL;
	    bool hazyKin = false;
	    AssertBool isDefinedFunc = (m_state.isFuncIdInClassScope(m_functionNameTok.m_dataindex, fnsymptr, hazyKin) && !hazyKin);
	    assert(isDefinedFunc);

	    //find this func in the virtual class; get its func def.
	    std::vector<UTI> pTypes;
	    u32 numparams = m_funcSymbol->getNumberOfParameters();
	    for(u32 j = 0; j < numparams; j++)
	      {
		Symbol * psym = m_funcSymbol->getParameterSymbolPtr(j);
		assert(psym);
		pTypes.push_back(psym->getUlamTypeIdx());
	      }

	    SymbolFunction * funcSymbol = NULL;
	    u32 numFuncs = ((SymbolFunctionName *) fnsymptr)->findMatchingFunctionStrictlyByTypes(pTypes, funcSymbol);

	    if(numFuncs != 1)
	      {
		std::ostringstream msg;
		msg << "Virtual function <" << funcSymbol->getMangledNameWithTypes().c_str();
		msg << "> is ";
		if(numFuncs > 1)
		  msg << "ambiguous";
		else
		  msg << "not found";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		assert(0);
	      }

	    if(!funcSymbol->isVirtualFunction())
	      {
		std::ostringstream msg;
		msg << "Function <" << funcSymbol->getMangledNameWithTypes().c_str();
		msg << "> is not virtual";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		m_state.popClassContext(); //restore here
		evalNodeEpilog();
		return ERROR;
	      }

	    if(funcSymbol->isPureVirtualFunction())
	      {
		std::ostringstream msg;
		msg << "Virtual function <" << funcSymbol->getMangledNameWithTypes().c_str();
		msg << "> is pure; cannot be called";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		m_state.popClassContext(); //restore here
		evalNodeEpilog();
		return ERROR;
	      }

	    m_state.popClassContext(); //restore here

	    func = funcSymbol->getFunctionNode(); //replace with virtual function def!!!
	  } //end use virtual function
      } //end virtual function

    if(atomPtr.getPtrStorage() == STACK)
      {
	//adjust index if on the STACK, not for Event Window site
	s32 nextslot = m_state.m_funcCallStack.getRelativeTopOfStackNextSlot();
	s32 atomslot = atomPtr.getPtrSlotIndex();
	s32 adjustedatomslot = atomslot - (nextslot + rtnslots + 1); //negative index; 1 more for atomPtr
	atomPtr.setPtrSlotIndex(adjustedatomslot);
	if(atomPtr.getUlamValueTypeIdx() == PtrAbs)
	  atomPtr.setUlamValueTypeIdx(Ptr); //let's see..
      }
    // push the "hidden" first arg, and update the current object ptr (restore later)
    m_state.m_funcCallStack.pushArg(atomPtr); //*********
    argsPushed++;
    m_state.m_currentObjPtr = atomPtr; //*********

    UlamValue saveSelfPtr = m_state.m_currentSelfPtr; // restore upon return from func *****
    m_state.m_currentSelfPtr = m_state.m_currentObjPtr; // set for subsequent func calls ****

    //(continue) push return slot(s) last (on both STACKS for now)
    makeRoomForNodeType(rtnType, STACK);

    assert(rtnslots == m_state.slotsNeeded(rtnType));

    //********************************************
    //*  FUNC CALL HERE!!
    //*
    evs = func->eval(); //NodeBlockFunctionDefinition..
    if(evs != NORMAL)
      {
	assert(evs != RETURN);
	//drops all the args and return slots on callstack
	m_state.m_funcCallStack.popArgs(argsPushed+rtnslots);
	m_state.m_currentObjPtr = saveCurrentObjectPtr; //restore current object ptr *******
	m_state.m_currentSelfPtr = saveSelfPtr; //restore previous self *****
	evalNodeEpilog();
	return evs;
      }
    //*
    //**********************************************

    // ANY return value placed on the STACK by a Return Statement,
    // was copied to EVALRETURN by the NodeBlockFunctionDefinition
    // before arriving here! And may be ignored at this point.
    if(m_state.isAtom(rtnType))
      {
	UlamValue rtnUV = m_state.m_nodeEvalStack.loadUlamValueFromSlot(1);
	Node::assignReturnValueToStack(rtnUV); //into return space on eval stack;
      }
    else
      {
	//positive to current frame pointer; pos is (BITSPERATOM - rtnbitsize * rtnarraysize)
	UlamValue rtnPtr = UlamValue::makePtr(1, EVALRETURN, rtnType, m_state.determinePackable(rtnType), m_state);
	Node::assignReturnValueToStack(rtnPtr); //into return space on eval stack;
      }

    m_state.m_funcCallStack.popArgs(argsPushed+rtnslots); //drops all the args and return slots on callstack

    m_state.m_currentObjPtr = saveCurrentObjectPtr; //restore current object ptr *****
    m_state.m_currentSelfPtr = saveSelfPtr; //restore previous self      *************
    evalNodeEpilog(); //clears out the node eval stack
    return NORMAL;
  } //eval

  EvalStatus NodeFunctionCall::evalToStoreInto()
  {
    std::ostringstream msg;
    msg << "Eval of function calls as lefthand values is not currently supported.";
    msg << " Save the results of <";
    msg << m_state.getTokenDataAsString(&m_functionNameTok).c_str();
    msg << "> to a variable, type: ";
    msg << m_state.getUlamTypeNameBriefByIndex(getNodeType()).c_str();
    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
    assert(Node::getStoreIntoAble() == TBOOL_FALSE);
    return ERROR;
  } //evalToStoreInto

  void NodeFunctionCall::addArgument(Node * n)
  {
    //check types later
    m_argumentNodes->addNodeToList(n);
    return;
  }

  u32 NodeFunctionCall::getNumberOfArguments()
  {
    return m_argumentNodes->getNumberOfNodes();
  }

  bool NodeFunctionCall::getSymbolPtr(Symbol *& symptrref)
  {
    symptrref = m_funcSymbol;
    return true;
  }

  // during genCode of a single function body "self" doesn't change!!!
  // note: uvpass arg is not equal to m_currentObjPtr; it is blank.
  void NodeFunctionCall::genCode(File * fp, UlamValue& uvpass)
  {
    if(!m_funcSymbol || !m_state.okUTItoContinue(getNodeType()))
      {
	std::ostringstream msg;
	msg << "(3) <" << m_state.getTokenDataAsString(&m_functionNameTok).c_str();
	msg << "> is not a fully resolved function definition; ";
	msg << "A call to it cannot be generated in this context";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	m_state.m_currentObjSymbolsForCodeGen.clear();
	return;
      }

    // The Call:
    if(m_state.isPtr(uvpass.getUlamValueTypeIdx()) && (uvpass.getPtrStorage() == TMPAUTOREF))
      genCodeAReferenceIntoABitValue(fp, uvpass);
    else
      genCodeIntoABitValue(fp, uvpass);

    // Result:
    if(getNodeType() != Void)
      {
	Node::genCodeConvertABitVectorIntoATmpVar(fp, uvpass); //inc uvpass slot
      }
  } //genCode

  // during genCode of a single function body "self" doesn't change!!!
  void NodeFunctionCall::genCodeToStoreInto(File * fp, UlamValue& uvpass)
  {
    return genCodeIntoABitValue(fp,uvpass);
  } //codeGenToStoreInto

  void NodeFunctionCall::genCodeIntoABitValue(File * fp, UlamValue& uvpass)
  {
    // generate for value
    UTI nuti = getNodeType();
    UlamType * nut = m_state.getUlamTypeByIndex(nuti);

    u32 urtmpnum = 0;
    std::string hiddenarg2str = genHiddenArg2(urtmpnum);
    if(urtmpnum > 0)
      {
	m_state.indent(fp);
	fp->write(hiddenarg2str.c_str());
	fp->write("\n");
      }

    std::ostringstream arglist;
    // presumably there's no = sign.., and no open brace for tmpvars
    arglist << genHiddenArgs(urtmpnum).c_str();

    //loads any variables into tmps before used as args (needs fp)
    arglist << genRestOfFunctionArgs(fp, uvpass).c_str();

    //non-void RETURN value saved in a tmp BitValue; depends on return type
    m_state.indent(fp);
    if(nuti != Void)
      {
	u32 pos = 0; //POS 0 rightjustified;
	if(nut->getUlamClass() == UC_NOTACLASS) //includes atom too
	  {
	    u32 wordsize = nut->getTotalWordSize();
	    pos = wordsize - nut->getTotalBitSize();
	  }

	s32 rtnSlot = m_state.getNextTmpVarNumber();

	u32 selfid = 0;
	if(m_state.m_currentObjSymbolsForCodeGen.empty())
	  selfid = m_state.getCurrentSelfSymbolForCodeGen()->getId(); //a use for CSS
	else
	  selfid = m_state.m_currentObjSymbolsForCodeGen[0]->getId();

	uvpass = UlamValue::makePtr(rtnSlot, TMPBITVAL, nuti, m_state.determinePackable(nuti), m_state, pos, selfid); //POS adjusted for BitVector, justified; self id in Ptr;

	// put result of function call into a variable;
	// (C turns it into the copy constructor)
	fp->write("const ");
	fp->write(nut->getLocalStorageTypeAsString().c_str()); //e.g. BitVector<32>
	fp->write(" ");
	fp->write(m_state.getTmpVarAsString(nuti, rtnSlot, TMPBITVAL).c_str());
	fp->write(" = ");
      } //not void return

    // no longer for quarks!!
    // static functions..oh yeah..but only for quarks and virtual funcs
    // who's function is it?
    if(m_funcSymbol->isVirtualFunction())
      genCodeVirtualFunctionCall(fp, uvpass, urtmpnum); //indirect call thru func ptr
    else
      {
	if(!Node::isCurrentObjectALocalVariableOrArgument())
	  genMemberNameOfMethod(fp);
	else
	  {
	    s32 epi = Node::isCurrentObjectsContainingAModelParameter();
	    assert(epi < 0); //model parameters no longer classes
	    genLocalMemberNameOfMethod(fp);
	  }
	fp->write(m_funcSymbol->getMangledName().c_str());
      }
    // the arguments
    fp->write("(");
    fp->write(arglist.str().c_str());
    fp->write(");\n");

    m_state.m_currentObjSymbolsForCodeGen.clear();
  } //genCodeIntoABitValue

  void NodeFunctionCall::genCodeAReferenceIntoABitValue(File * fp, UlamValue& uvpass)
  {
    UlamValue rtnuvpass;
    // generate for value
    UTI nuti = getNodeType();
    UlamType * nut = m_state.getUlamTypeByIndex(nuti);

    u32 urtmpnum = 0;
    std::string hiddenarg2str = genHiddenArg2ForARef(fp, uvpass, urtmpnum);

    m_state.indent(fp);
    fp->write(hiddenarg2str.c_str());
    fp->write("\n");

    std::ostringstream arglist;
    // presumably there's no = sign.., and no open brace for tmpvars
    arglist << genHiddenArgs(urtmpnum).c_str();

    //loads any variables into tmps before used as args (needs fp)
    arglist << genRestOfFunctionArgs(fp, uvpass).c_str();

    //non-void RETURN value saved in a tmp BitValue; depends on return type
    m_state.indent(fp);
    if(nuti != Void)
      {
	u32 pos = 0; //POS 0 rightjustified;
	if(nut->getUlamClass() == UC_NOTACLASS) //includes atom too
	  {
	    u32 wordsize = nut->getTotalWordSize();
	    pos = wordsize - nut->getTotalBitSize();
	  }

	s32 rtnSlot = m_state.getNextTmpVarNumber();

	u32 selfid = 0;
	if(m_state.m_currentObjSymbolsForCodeGen.empty())
	  selfid = m_state.getCurrentSelfSymbolForCodeGen()->getId(); //a use for CSS
	else
	  selfid = m_state.m_currentObjSymbolsForCodeGen[0]->getId();

	rtnuvpass = UlamValue::makePtr(rtnSlot, TMPBITVAL, nuti, m_state.determinePackable(nuti), m_state, pos, selfid); //POS adjusted for BitVector, justified; self id in Ptr;

	// put result of function call into a variable;
	// (C turns it into the copy constructor)
	fp->write("const ");
	fp->write(nut->getLocalStorageTypeAsString().c_str()); //e.g. BitVector<32>
	fp->write(" ");
	fp->write(m_state.getTmpVarAsString(nuti, rtnSlot, TMPBITVAL).c_str());
	fp->write(" = ");
      } //not void return


    assert(uvpass.getPtrStorage() == TMPAUTOREF);
    UTI vuti = uvpass.getPtrTargetType();
    assert(m_state.getUlamTypeByIndex(vuti)->getReferenceType() != ALT_NOT);

    //use possible dereference type for mangled name
    UTI derefuti = m_state.getUlamTypeAsDeref(vuti);
    UlamType * derefut = m_state.getUlamTypeByIndex(derefuti);

    // who's function is it?
    if(m_funcSymbol->isVirtualFunction())
      genCodeVirtualFunctionCall(fp, uvpass, urtmpnum); //indirect call thru func ptr
    else
      {
	fp->write(derefut->getUlamTypeMangledName().c_str());
	fp->write("<EC>::"); //same for elements and quarks
        fp->write("THE_INSTANCE.");
	fp->write(m_funcSymbol->getMangledName().c_str());
      }

    // the arguments
    fp->write("(");
    fp->write(arglist.str().c_str());
    fp->write(");\n");

    m_state.m_currentObjSymbolsForCodeGen.clear();
    uvpass = rtnuvpass;
  } //genCodeAReferenceIntoABitValue

  void NodeFunctionCall::genCodeVirtualFunctionCall(File * fp, UlamValue & uvpass, u32 urtmpnum)
  {
    assert(m_funcSymbol);
    //requires runtime lookup for virtual function pointer
    u32 vfidx = m_funcSymbol->getVirtualMethodIdx();

    //need typedef typename for this vfunc, any vtable of any owner of this vfunc
    u32 cosSize = m_state.m_currentObjSymbolsForCodeGen.size();
    Symbol * cos = NULL; //any owner of func

    if(cosSize != 0)
      cos = m_state.m_currentObjSymbolsForCodeGen.back(); //"owner" of func
    else
      cos = m_state.getCurrentSelfSymbolForCodeGen(); //'self'

    UTI cosuti = cos->getUlamTypeIdx();
    UlamType * cosut = m_state.getUlamTypeByIndex(cosuti);
    SymbolClass * csym = NULL;
    AssertBool isDefined = m_state.alreadyDefinedSymbolClass(cosuti, csym);
    assert(isDefined);

    UTI cvfuti = csym->getClassForVTableEntry(vfidx);
    UlamType * cvfut = m_state.getUlamTypeByIndex(cvfuti);

    fp->write("((typename ");
    fp->write(cvfut->getUlamTypeMangledName().c_str());
    fp->write("<EC>::"); //same for elements and quarks
    fp->write(m_funcSymbol->getMangledNameWithTypes().c_str());
    fp->write(") ");

    //requires runtime lookup for virtual function pointer
    if(cos->isSelf() || cosSize == 0)
      {
	fp->write(m_state.getHiddenArgName()); //ur
	fp->write(".GetEffectiveSelf()->getVTableEntry(");
      }
    else if(urtmpnum > 0)
      {
	fp->write(m_state.getUlamRefTmpVarAsString(urtmpnum).c_str());
	fp->write(".GetEffectiveSelf()->getVTableEntry(");
      }
    else if(cos->getAutoLocalType() == ALT_AS)
      {
	assert(0);
	fp->write(m_state.getHiddenArgName()); //ur, should use urtmpnum!!
	fp->write(".GetEffectiveSelf()->getVTableEntry(");
      }
    else
      {
	//unless local or dm, known at compile time!
	if(cosut->getReferenceType() == ALT_REF)
	  {
	    UTI derefcos = m_state.getUlamTypeAsDeref(cosuti);
	    fp->write(m_state.getUlamTypeByIndex(derefcos)->getUlamTypeMangledName().c_str());
	  }
	else
	  fp->write(cosut->getUlamTypeMangledName().c_str());
	fp->write("<EC>::"); //same for elements and quarks
	fp->write("THE_INSTANCE.getVTableEntry(");
      }

    fp->write_decimal_unsigned(vfidx);
    fp->write(")) ");
    //args to function pointer remain
  } //genCodeVirtualFunctionCall

  // overrides Node in case of memberselect genCode
  void NodeFunctionCall::genCodeReadIntoATmpVar(File * fp, UlamValue & uvpass)
  {
    return; //no-op
  }

  void NodeFunctionCall::genMemberNameOfMethod(File * fp)
  {
    assert(!Node::isCurrentObjectALocalVariableOrArgument());
    u32 cosSize = m_state.m_currentObjSymbolsForCodeGen.size();
    Symbol * stgcos = m_state.getCurrentSelfSymbolForCodeGen();
    Symbol * cos = NULL;
    if(cosSize > 0)
      cos = m_state.m_currentObjSymbolsForCodeGen.back();
    else
      cos = stgcos;
    UTI cosuti = cos->getUlamTypeIdx();

    UTI futi = m_funcSymbol->getDataMemberClass();
    if((cosSize > 0) || (UlamType::compare(cosuti, futi, m_state) != UTIC_SAME))
      {
	UlamType * fut = m_state.getUlamTypeByIndex(futi);
	fp->write(fut->getUlamTypeMangledName().c_str());
	fp->write("<EC>::"); //same for elements and quarks
      }
    fp->write("THE_INSTANCE."); //non-static functions require an instance
  } //genMemberNameOfMethod

  void NodeFunctionCall::genModelParameterMemberNameOfMethod(File * fp, s32 epi)
  {
    assert(0);
  } //genModelParamenterMemberNameOfMethod

  std::string NodeFunctionCall::genHiddenArgs(u32 urtmpnum)
  {
    std::ostringstream hiddenargs;
    hiddenargs << m_state.getHiddenContextArgName(); //same uc
    hiddenargs << ", ";
    if(urtmpnum != 0)
       hiddenargs << m_state.getUlamRefTmpVarAsString(urtmpnum).c_str();
    else
      hiddenargs << m_state.getHiddenArgName(); //same ur;

    return hiddenargs.str();
  } //genHiddenArgs

  std::string NodeFunctionCall::genHiddenArg2(u32& urtmpnumref)
  {
    bool sameur = true;
    u32 tmpvar = m_state.getNextTmpVarNumber();

    std::ostringstream hiddenarg2;
    Symbol * cos = NULL;
    if(m_state.m_currentObjSymbolsForCodeGen.empty())
      {
	cos = m_state.getCurrentSelfSymbolForCodeGen();
      }
    else
      {
	cos = m_state.m_currentObjSymbolsForCodeGen.back();
      }
    UTI cosuti = cos->getUlamTypeIdx();
    UlamType * cosut = m_state.getUlamTypeByIndex(cosuti);

    // first "hidden" arg is the context, then
    //"hidden" ref self (ur) arg
    if(!Node::isCurrentObjectALocalVariableOrArgument())
      {
	if(m_state.m_currentObjSymbolsForCodeGen.empty())
	  hiddenarg2 << m_state.getHiddenArgName(); //same ur
	else
	  {
	    sameur = false;
	    hiddenarg2 << "UlamRef<EC> " << m_state.getUlamRefTmpVarAsString(tmpvar).c_str() << "(";
	    //update ur to reflect "effective" self for this funccall
	    hiddenarg2 << m_state.getHiddenArgName(); //ur
	    hiddenarg2 << ", " << Node::calcPosOfCurrentObjectClasses(); //relative off;
	    hiddenarg2 << "u, " << cosut->getTotalBitSize(); //len
	    hiddenarg2 << "u, &" << cosut->getUlamTypeMangledName().c_str();
	    hiddenarg2 << "<EC>::THE_INSTANCE);";
	  }
      }
    else
      {
	s32 epi = Node::isCurrentObjectsContainingAModelParameter();
	if(epi >= 0)
	  {
	    //model parameters no longer classes, deprecated
	    assert(0);
	    hiddenarg2 << genModelParameterHiddenArgs(epi).c_str();
	  }
	else //local var
	  {
	    Symbol * stgcos = NULL;
	    if(m_state.m_currentObjSymbolsForCodeGen.empty())
	      {
		hiddenarg2 << m_state.getHiddenArgName(); //same ur
	      }
	    else if(cos->getAutoLocalType() == ALT_AS)
	      {
		sameur = false;
		stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
		UTI stgcosuti = stgcos->getUlamTypeIdx();
		stgcosuti = m_state.getUlamTypeAsDeref(stgcosuti);
		UlamType * stgcosut = m_state.getUlamTypeByIndex(stgcosuti);
		//update ur to reflect "effective" self for this funccall
		hiddenarg2 << "UlamRef<EC> " << m_state.getUlamRefTmpVarAsString(tmpvar).c_str() << "(";
		hiddenarg2 << stgcos->getMangledName().c_str();
		hiddenarg2 << ", " << Node::calcPosOfCurrentObjectClasses(); //relative off;
		hiddenarg2 << "u, " << stgcosut->getTotalBitSize(); //len
		hiddenarg2 << "u, ";
		hiddenarg2 << stgcos->getMangledName().c_str(); //effself of as-variable
		hiddenarg2 << ".GetEffectiveSelf());";
	      }
	    else
	      {
		sameur = false;
		stgcos = m_state.m_currentObjSymbolsForCodeGen[0];

		//use possible dereference type for mangled name
		UTI cosderefuti = m_state.getUlamTypeAsDeref(cosuti);
		UlamType * cosderefut = m_state.getUlamTypeByIndex(cosderefuti);

		//new ur to reflect "effective" self and storage for this funccall
		hiddenarg2 << "UlamRef<EC> " << m_state.getUlamRefTmpVarAsString(tmpvar).c_str() << "(";
		hiddenarg2 << stgcos->getMangledName().c_str();
		hiddenarg2 << ", " ;

		if(cos->isDataMember()) //dm of local stgcos
		  hiddenarg2 << Node::calcPosOfCurrentObjectClasses(); //relative off;
		else
		  hiddenarg2 << "0";

		hiddenarg2 << "u, " << cosderefut->getTotalBitSize(); //len
		hiddenarg2 << "u, &";
		hiddenarg2 << cosderefut->getUlamTypeMangledName().c_str();
		hiddenarg2 << "<EC>::THE_INSTANCE);";
	      }

	  }
      }

    if(!sameur)
      urtmpnumref = tmpvar; //update arg

    return hiddenarg2.str();
  } //genHiddenArg2

  std::string NodeFunctionCall::genHiddenArg2ForARef(File * fp, UlamValue uvpass, u32& urtmpnumref)
  {
    assert(m_state.isPtr(uvpass.getUlamValueTypeIdx()));
    assert(uvpass.getPtrStorage() == TMPAUTOREF);

    UTI vuti = uvpass.getPtrTargetType();
    assert(m_state.getUlamTypeByIndex(vuti)->getReferenceType() != ALT_NOT);

    //use possible dereference type for mangled name
    UTI derefuti = m_state.getUlamTypeAsDeref(vuti);
    UlamType * derefut = m_state.getUlamTypeByIndex(derefuti);

   u32 tmpvarnum = uvpass.getPtrSlotIndex();
   u32 tmpvarur = m_state.getNextTmpVarNumber();

    std::ostringstream hiddenarg2;
    //new ur to reflect "effective" self and the ref storage, for this funccall
    hiddenarg2 << "UlamRef<EC> " << m_state.getUlamRefTmpVarAsString(tmpvarur).c_str() << "(";
    hiddenarg2 << m_state.getTmpVarAsString(derefuti, tmpvarnum, TMPAUTOREF).c_str();
    hiddenarg2 << ", 0u, "; //left-justified (uvpass.getPtrPosOffset()?)
    hiddenarg2 << derefut->getTotalBitSize(); //len
    hiddenarg2 << "u, &";
    hiddenarg2 << derefut->getUlamTypeMangledName().c_str();
    hiddenarg2 << "<EC>::THE_INSTANCE);";

    urtmpnumref = tmpvarur;
    return hiddenarg2.str();
  } //genHiddenArg2ForARef

  std::string NodeFunctionCall::genStorageType()
  {
    std::ostringstream stype;

    //"hidden" self arg
    if(!Node::isCurrentObjectALocalVariableOrArgument())
      {
	stype << m_state.getHiddenArgName();
	stype << ".GetType()";
      }
    else
      {
	s32 epi = Node::isCurrentObjectsContainingAModelParameter();
	if(epi >= 0)
	  {
	    stype << genModelParameterHiddenArgs(epi).c_str();
	  }
	else //local var
	  {
	    Symbol * stgcos = NULL;
	    if(m_state.m_currentObjSymbolsForCodeGen.empty())
	      {
		stgcos = m_state.getCurrentSelfSymbolForCodeGen();
	      }
	    else
	      {
		stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
	      }

	    stype << stgcos->getMangledName().c_str();
	    stype << ".GetType()";
	  }
      }
    return stype.str();
  } //genStorageType

  // "static" data member, a mixture of local variable and dm;
  // requires THE_INSTANCE, and local variables are superfluous.
  std::string NodeFunctionCall::genModelParameterHiddenArgs(s32 epi)
  {
    assert(!m_state.m_currentObjSymbolsForCodeGen.empty());
    assert(epi >= 0);

    std::ostringstream hiddenlist;
    Symbol * stgcos = NULL;

    if(epi == 0)
      stgcos = m_state.getCurrentSelfSymbolForCodeGen();
    else
      stgcos = m_state.m_currentObjSymbolsForCodeGen[epi - 1]; //***

    UTI stgcosuti = stgcos->getUlamTypeIdx();
    UlamType * stgcosut = m_state.getUlamTypeByIndex(stgcosuti);

    Symbol * epcos = m_state.m_currentObjSymbolsForCodeGen[epi]; //***
    UTI epcosuti = epcos->getUlamTypeIdx();
    UlamType * epcosut = m_state.getUlamTypeByIndex(epcosuti);
    ULAMCLASSTYPE epcosclasstype = epcosut->getUlamClass();

    hiddenlist << stgcosut->getUlamTypeMangledName().c_str();
    hiddenlist << "<EC>::THE_INSTANCE.";

    // the MP (only primitive, no longer an element, or quark):
    hiddenlist << epcos->getMangledName().c_str();

    if(epcosclasstype != UC_NOTACLASS)
      {
	assert(0);
	hiddenlist << ".getRef()";
      }
    return hiddenlist.str();
  } //genModelParameterHiddenArgs

  std::string NodeFunctionCall::genRestOfFunctionArgs(File * fp, UlamValue& uvpass)
  {
    std::ostringstream arglist;

    //wiped out by arg processing; needed to determine owner of called function
    std::vector<Symbol *> saveCOSVector = m_state.m_currentObjSymbolsForCodeGen;

    assert(m_funcSymbol);
    u32 numParams = m_funcSymbol->getNumberOfParameters();
    // handle any variable number of args separately
    // since non-datamember variables can modify globals, save/restore before/after each
    for(u32 i = 0; i < numParams; i++)
      {
	UlamValue auvpass;
	UTI auti;
	m_state.m_currentObjSymbolsForCodeGen.clear(); //*************

	// what if ALT_ARRAYITEM?
	if(m_state.getReferenceType(m_funcSymbol->getParameterType(i)) != ALT_NOT)
	  {
	    genCodeReferenceArg(fp, auvpass, i);
	  }
	else
	  {
	    m_argumentNodes->genCode(fp, auvpass, i);
	    Node::genCodeConvertATmpVarIntoBitVector(fp, auvpass);
	  }
	auti = auvpass.getUlamValueTypeIdx();
	if(m_state.isPtr(auti))
	  {
	    auti = auvpass.getPtrTargetType();
	  }
	arglist << ", " << m_state.getTmpVarAsString(auti, auvpass.getPtrSlotIndex(), auvpass.getPtrStorage()).c_str();
      } //next arg..

    if(m_funcSymbol->takesVariableArgs())
      {
	u32 numargs = getNumberOfArguments();
	for(u32 i = numParams; i < numargs; i++)
	  {
	    UlamValue auvpass;
	    UTI auti;
	    m_state.m_currentObjSymbolsForCodeGen.clear(); //*************

	    if(m_state.getReferenceType(m_argumentNodes->getNodeType(i)) != ALT_NOT)
	      {
		genCodeReferenceArg(fp, auvpass, i);
	      }
	    else
	      {
		m_argumentNodes->genCode(fp, auvpass, i);
		Node::genCodeConvertATmpVarIntoBitVector(fp, auvpass);
	      }

	    auti = auvpass.getUlamValueTypeIdx();
	    if(m_state.isPtr(auti))
	      {
		auti = auvpass.getPtrTargetType();
	      }

	    // use pointer for variable arg's since all the same size that way
	    arglist << ", &" << m_state.getTmpVarAsString(auti, auvpass.getPtrSlotIndex(), auvpass.getPtrStorage()).c_str();
	  } //end forloop through variable number of args

	arglist << ", (void *) 0"; //indicates end of args
      } //end of handling variable arguments

    m_state.m_currentObjSymbolsForCodeGen = saveCOSVector; //restore vector after args******

    return arglist.str();
  } //genRestOfFunctionArgs

  // should be like NodeVarRef::genCode
  void NodeFunctionCall::genCodeReferenceArg(File * fp, UlamValue & uvpass, u32 n)
  {
    // get the right?-hand side, stgcos
    // can be same type (e.g. element, quark, or primitive),
    // or ancestor quark if a class.
    m_argumentNodes->genCodeToStoreInto(fp, uvpass, n);

    //assert(!m_state.m_currentObjSymbolsForCodeGen.empty()); such as .storageof
    if(m_state.m_currentObjSymbolsForCodeGen.empty())
      {
	return genCodeAnonymousReferenceArg(fp, uvpass, n); //such as .storageof
      }

    Symbol * stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
    UTI stgcosuti = stgcos->getUlamTypeIdx();
    UlamType * stgcosut = m_state.getUlamTypeByIndex(stgcosuti);

    Symbol * cos = m_state.m_currentObjSymbolsForCodeGen.back();
    UTI cosuti = cos->getUlamTypeIdx();
    UlamType * cosut = m_state.getUlamTypeByIndex(cosuti);

    assert(m_funcSymbol);
    UTI vuti = m_funcSymbol->getParameterType(n);
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);
    ULAMCLASSTYPE vclasstype = vut->getUlamClass();

    assert(vut->getUlamTypeEnum() == cosut->getUlamTypeEnum());

    m_state.indent(fp);
    fp->write(vut->getLocalStorageTypeAsString().c_str()); //for C++ local vars, ie non-data members
    fp->write(" ");

    s32 tmpVarArgNum = m_state.getNextTmpVarNumber();
    fp->write(m_state.getTmpVarAsString(vuti, tmpVarArgNum, TMPBITVAL).c_str());
    fp->write("("); //pass ref in constructor (ref's not assigned with =)
    if(stgcos->isDataMember()) //can't be an element
      {
	fp->write(m_state.getHiddenArgName());
	fp->write(", ");
	fp->write_decimal_unsigned(Node::calcPosOfCurrentObjects()); //rel offset
	fp->write("u");
      }
    else
      {
	fp->write(stgcos->getMangledName().c_str());
	if(cos->isDataMember())
	  {
	    fp->write(", ");
	    fp->write_decimal_unsigned(Node::calcPosOfCurrentObjects()); //rel offset
	    fp->write("u");
	  }
	else
	  {
	    if(m_state.isAtom(vuti))
	      {
		//nada - copy constructor
	      }
	    else if(vclasstype == UC_NOTACLASS)
	      {
		fp->write(", ");
		fp->write_decimal_unsigned(BITSPERATOM - stgcosut->getTotalBitSize()); //right-justified
		fp->write("u");
	      }
	    else if(vclasstype == UC_QUARK)
	      fp->write(", 0u"); //left-justified
	  }
      }
    fp->write(");\n");

    uvpass.setPtrSlotIndex(tmpVarArgNum);
    uvpass.setPtrStorage(TMPBITVAL);

    m_state.m_currentObjSymbolsForCodeGen.clear(); //clear remnant of rhs ?
  } //genCodeReferenceArg

  // uses uvpass rather than stgcos, cos for classes or atoms (not primitives)
  void NodeFunctionCall::genCodeAnonymousReferenceArg(File * fp, UlamValue & uvpass, u32 n)
  {
    assert(m_state.m_currentObjSymbolsForCodeGen.empty()); //such as .storageof

    assert(m_funcSymbol);
    UTI vuti = m_funcSymbol->getParameterType(n);
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);
    ULAMCLASSTYPE vclasstype = vut->getUlamClass();

    UTI puti = uvpass.getUlamValueTypeIdx();
    assert(m_state.isPtr(puti));
    puti = uvpass.getPtrTargetType();
    UlamType * put = m_state.getUlamTypeByIndex(puti);
    STORAGE rstor = put->getUlamClass() == UC_QUARK ? TMPREGISTER : uvpass.getPtrStorage();

    assert(vut->getUlamTypeEnum() == put->getUlamTypeEnum());

    s32 tmpVarArgNum = uvpass.getPtrSlotIndex();
    s32 tmpVarArgNum2 = m_state.getNextTmpVarNumber();

    m_state.indent(fp);
    fp->write(vut->getLocalStorageTypeAsString().c_str()); //for C++ local vars, ie non-data members
    fp->write(" ");

    fp->write(m_state.getTmpVarAsString(vuti, tmpVarArgNum2, TMPBITVAL).c_str());
    fp->write("("); //pass ref in constructor (ref's not assigned with =)
    fp->write(m_state.getTmpVarAsString(puti, tmpVarArgNum, rstor).c_str());

    if(vclasstype == UC_QUARK)
      fp->write(", 0u"); //left-justified

    fp->write(");\n");

    uvpass.setPtrSlotIndex(tmpVarArgNum2);
    uvpass.setPtrStorage(TMPBITVAL);
  } //genCodeAnonymousReferenceArg

void NodeFunctionCall::genLocalMemberNameOfMethod(File * fp)
  {
    assert(Node::isCurrentObjectALocalVariableOrArgument());
    assert(!m_state.m_currentObjSymbolsForCodeGen.empty());

    UTI futi = m_funcSymbol->getDataMemberClass();
    UlamType * fut = m_state.getUlamTypeByIndex(futi);
    fp->write(fut->getLocalStorageTypeAsString().c_str());
    fp->write("::Us::THE_INSTANCE.");
  } //genLocalMemberNameOfMethod

} //end MFM

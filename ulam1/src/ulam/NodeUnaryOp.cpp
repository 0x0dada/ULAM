#include <stdio.h>
#include "NodeUnaryOp.h"
#include "NodeTerminal.h"
#include "CompilerState.h"

namespace MFM {

  NodeUnaryOp::NodeUnaryOp(Node * n, CompilerState & state): Node(state), m_node(n) {}

  NodeUnaryOp::NodeUnaryOp(const NodeUnaryOp& ref) : Node(ref)
  {
    m_node = ref.m_node->instantiate();
  }

  NodeUnaryOp::~NodeUnaryOp()
  {
    delete m_node;
    m_node = NULL;
  }

  void NodeUnaryOp::updateLineage(NNO pno)
  {
    setYourParentNo(pno);
    m_node->updateLineage(getNodeNo());
  } //updateLineage

  bool NodeUnaryOp::exchangeKids(Node * oldnptr, Node * newnptr)
  {
    if(m_node == oldnptr)
      {
	m_node = newnptr;
	return true;
      }
    return false;
  } //exhangeKids

  bool NodeUnaryOp::findNodeNo(NNO n, Node *& foundNode)
  {
    if(Node::findNodeNo(n, foundNode))
      return true;
    if(m_node && m_node->findNodeNo(n, foundNode))
      return true;
    return false;
  } //findNodeNo

  void NodeUnaryOp::print(File * fp)
  {
    printNodeLocation(fp);
    UTI myut = getNodeType();
    char id[255];
    if(myut == Nav)
      sprintf(id,"%s<NOTYPE>\n", prettyNodeName().c_str());
    else
      sprintf(id,"%s<%s>\n",prettyNodeName().c_str(), m_state.getUlamTypeNameByIndex(myut).c_str());
    fp->write(id);

    if(m_node)
      m_node->print(fp);
    else
      fp->write("<NULL>\n");
    sprintf(id,"-----------------%s\n", prettyNodeName().c_str());
    fp->write(id);
  } //print

  void NodeUnaryOp::printPostfix(File * fp)
  {
    if(m_node)
      m_node->printPostfix(fp);
    else
      fp->write("<NULL>");

    printOp(fp); //operators last
  } //printPostfix

  void NodeUnaryOp::printOp(File * fp)
  {
    char myname[16];
    sprintf(myname," %s", getName());
    fp->write(myname);
  }

  const std::string NodeUnaryOp::methodNameForCodeGen()
  {
    assert(0);
    return "_UNARY_NOOP";
  }

  bool NodeUnaryOp::isAConstant()
  {
    assert(m_node);
    return m_node->isAConstant();
  }

  bool NodeUnaryOp::isReadyConstant()
  {
    return false; //needs constant folding
  }

  FORECAST NodeUnaryOp::safeToCastTo(UTI newType)
  {
    //ulamtype checks for complete, non array, and type specific rules
    return m_state.getUlamTypeByIndex(newType)->safeCast(getNodeType());
  } //safeToCastTo

  UTI NodeUnaryOp::checkAndLabelType()
  {
    assert(m_node);
    UTI uti = m_node->checkAndLabelType();

    if(!m_state.isScalar(uti)) //array unsupported at this time
      {
	std::ostringstream msg;
	msg << "Incompatible (nonscalar) type: ";
	msg << m_state.getUlamTypeNameBriefByIndex(uti).c_str();
	msg << " for unary operator" << getName();
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	setNodeType(Nav);
	return Nav;
      }

    UTI newType = Nav;
    if(uti)
      newType = calcNodeType(uti); //does safety check

    if(newType != Nav && m_state.isComplete(newType))
      {
	if(UlamType::compare(newType, uti, m_state) != UTIC_SAME) //not same|dontknow
	  {
	    if(!Node::makeCastingNode(m_node, newType, m_node))
	      newType = Nav;
	  }
      }
    else
      m_state.setGoAgain(); //since not error


    setNodeType(newType);
    setStoreIntoAble(false);

    if(newType != Nav && isAConstant() && m_node->isReadyConstant())
      return constantFold();

    return newType;
  } //checkAndLabelType

  bool NodeUnaryOp::checkSafeToCastTo(UTI newType)
  {
    bool rtnOK = true;
    FORECAST scr = m_node->safeToCastTo(newType);
    if(scr != CAST_CLEAR)
      {
	std::ostringstream msg;
	if(m_state.getUlamTypeByIndex(newType)->getUlamTypeEnum() == Bool)
	  msg << "Use a comparison operator";
	else
	  msg << "Use explicit cast";
	msg << " to convert "; // the real converting-message
	msg << m_state.getUlamTypeNameBriefByIndex(m_node->getNodeType()).c_str();
	msg << " to ";
	msg << m_state.getUlamTypeNameBriefByIndex(newType).c_str();
	msg << " for unary operator" << getName();
	if(scr == CAST_HAZY)
	  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
	else
	  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	rtnOK = false;
      } //not safe
    return rtnOK;
  } //checkSafeToCastTo

  s32 NodeUnaryOp::resultBitsize(UTI uti)
  {
    s32 newbs = m_state.getBitSize(uti);
    ULAMCLASSTYPE ct = m_state.getUlamTypeByIndex(uti)->getUlamClass();

    if(ct == UC_QUARK)
      newbs = MAXBITSPERINT; //32

    return newbs;
  } //resultBitsize

  //no atoms, elements as operand
  bool NodeUnaryOp::checkForPrimitiveType(UTI uti)
  {
    if(!m_state.getUlamTypeByIndex(uti)->isPrimitiveType())
      {
	std::ostringstream msg;
	msg << "Non-primitive type <";
	msg << m_state.getUlamTypeNameBriefByIndex(uti).c_str();
	msg << "> is not supported for unary operator";
	msg << getName();
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	return false;
      }
    return checkNotVoidType(uti);
  } //checkForPrimitiveType

  bool NodeUnaryOp::checkNotVoidType(UTI uti)
  {
    bool rtnOK = true;
    ULAMTYPE typEnum = m_state.getUlamTypeByIndex(uti)->getUlamTypeEnum();
    if(typEnum == Void)
      {
	std::ostringstream msg;
	msg << "Void is not a supported type for unary operator";
	msg << getName();
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	rtnOK = false;
      }
    return rtnOK;
  } //checkNotVoidTypes

  bool NodeUnaryOp::checkForNumericType(UTI uti)
  {
    bool rtnOK = true;
    bool isnum = m_state.getUlamTypeByIndex(uti)->isNumericType();
    if(!isnum)
      {
	std::ostringstream msg;
	msg << "Incompatible type for unary operator";
	msg << getName() << " : ";
	msg << m_state.getUlamTypeNameBriefByIndex(uti).c_str();
	msg << "; Suggest casting to a numeric type first";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	rtnOK = false;
      }
    return rtnOK;
  } //checkForNumericType

  void NodeUnaryOp::countNavNodes(u32& cnt)
  {
    Node::countNavNodes(cnt); //missing
    m_node->countNavNodes(cnt); //no need to count self?
  }

  UTI NodeUnaryOp::constantFold()
  {
    u64 val = U64_MAX;
    UTI nuti = getNodeType();

    if(nuti == Nav) return Nav; //nothing to do yet

    // if here, must be a constant..
    assert(isAConstant());

    evalNodeProlog(0); //new current frame pointer
    makeRoomForNodeType(nuti); //offset a constant expression
    EvalStatus evs = eval();
    if( evs == NORMAL)
      {
	UlamValue cnstUV = m_state.m_nodeEvalStack.popArg();
	u32 wordsize = m_state.getTotalWordSize(nuti);
	if(wordsize <= MAXBITSPERINT)
	  val = cnstUV.getImmediateData(m_state);
	else if(wordsize <= MAXBITSPERLONG)
	  val = cnstUV.getImmediateDataLong(m_state);
	else
	  assert(0);
      }

    evalNodeEpilog();

    if(evs == ERROR)
      {
	std::ostringstream msg;
	msg << "Constant value expression for unary op" << getName();
	msg << " is not yet ready while compiling class: ";
	msg << m_state.getUlamTypeNameBriefByIndex(m_state.getCompileThisIdx()).c_str();
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
	setNodeType(Nav);
	return Nav;
      }

    //replace ourselves (and kids) with a node terminal; new NNO unlike template's
    NodeTerminal * newnode = new NodeTerminal(val, nuti, m_state);
    assert(newnode);
    newnode->setNodeLocation(getNodeLocation());

    NNO pno = Node::getYourParentNo();
    assert(pno);
    Node * parentNode = m_state.findNodeNoInThisClass(pno);
    assert(parentNode);

    assert(parentNode->exchangeKids(this, newnode));

    std::ostringstream msg;
    msg << "Exchanged kids! for unary operator" << getName();
    msg << ", with a constant == " << newnode->getName();
    msg << " while compiling class: ";
    msg << m_state.getUlamTypeNameBriefByIndex(m_state.getCompileThisIdx()).c_str();
    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);

    newnode->setYourParentNo(pno);
    newnode->resetNodeNo(getNodeNo());

    delete this; //suicide is painless..

    return newnode->checkAndLabelType();
  } //constantFold

  bool NodeUnaryOp::assignClassArgValueInStubCopy()
  {
    return m_node->assignClassArgValueInStubCopy();
  }

  EvalStatus NodeUnaryOp::eval()
  {
    assert(m_node);

    UTI nuti = getNodeType();
    if(nuti == Nav)
      return ERROR;

    evalNodeProlog(0); //new current frame pointer
    u32 slots = makeRoomForNodeType(nuti);
    EvalStatus evs = m_node->eval();

    if(evs == NORMAL)
      if(!doUnaryOperation(1,slots))
	evs = ERROR;

    evalNodeEpilog();
    return evs;
  } //eval

  bool NodeUnaryOp::doUnaryOperation(s32 slot, u32 nslots)
  {
    UTI nuti = getNodeType();
    if(m_state.isScalar(nuti))  //not an array
      {
	return doUnaryOperationImmediate(slot, nslots);
      }
    else
      {
	//arrays not supported at this time
	std::ostringstream msg;
	msg << "Unsupported unary operator" << getName();
	msg << ", with an array type <";
	msg << m_state.getUlamTypeNameBriefByIndex(nuti).c_str() << ">";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
      }
    return false;
  } //dobinaryop

  bool NodeUnaryOp::doUnaryOperationImmediate(s32 slot, u32 nslots)
  {
    assert(nslots == 1);
    UTI nuti = getNodeType();
    u32 len = m_state.getTotalBitSize(nuti);

    UlamValue uv = m_state.m_nodeEvalStack.loadUlamValueFromSlot(slot); //immediate value

    if(uv.getUlamValueTypeIdx() == Nav || nuti == Nav)
      return false;

    u32 data = uv.getImmediateData(len);
    UlamValue rtnUV = makeImmediateUnaryOp(nuti, data, len);
    m_state.m_nodeEvalStack.storeUlamValueInSlot(rtnUV, -1);
    return true;
  } //dounaryopImmediate

  void NodeUnaryOp::genCode(File * fp, UlamValue& uvpass)
  {
    assert(m_node);
    m_node->genCode(fp, uvpass);

    UTI nuti = getNodeType();
    UlamType * nut = m_state.getUlamTypeByIndex(nuti);
    s32 tmpVarNum = m_state.getNextTmpVarNumber();

    m_state.indent(fp);
    fp->write("const ");
    fp->write(nut->getTmpStorageTypeAsString().c_str()); //e.g. u32, s32, u64..
    fp->write(" ");

    fp->write(m_state.getTmpVarAsString(getNodeType(),tmpVarNum).c_str());
    fp->write(" = ");

    fp->write(methodNameForCodeGen().c_str());
    fp->write("(");

    UTI uti = uvpass.getUlamValueTypeIdx();
    assert(uti == Ptr);

    fp->write(m_state.getTmpVarAsString(uvpass.getPtrTargetType(), uvpass.getPtrSlotIndex()).c_str());

    fp->write(", ");
    fp->write_decimal(nut->getBitSize());

    fp->write(");\n");

    uvpass = UlamValue::makePtr(tmpVarNum, TMPREGISTER, nuti, m_state.determinePackable(nuti), m_state, 0); //POS 0 rightjustified.
  } //genCode

  void NodeUnaryOp::genCodeToStoreInto(File * fp, UlamValue& uvpass)
  {
    genCode(fp,uvpass);
  }

} //end MFM

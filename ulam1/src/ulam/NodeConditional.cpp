#include <stdio.h>
#include "NodeConditional.h"
#include "CompilerState.h"
#include "UlamTypeBool.h"

namespace MFM {

  NodeConditional::NodeConditional(Node * leftNode, NodeTypeDescriptor * classType, CompilerState & state): Node(state), m_nodeLeft(leftNode), m_nodeTypeDesc(classType) {}

  NodeConditional::NodeConditional(const NodeConditional& ref) : Node(ref)
  {
    m_nodeLeft = ref.m_nodeLeft->instantiate();
    m_nodeTypeDesc = (NodeTypeDescriptor *) ref.m_nodeTypeDesc->instantiate();
  }

  NodeConditional::~NodeConditional()
  {
    delete m_nodeLeft;
    m_nodeLeft = NULL;

    delete m_nodeTypeDesc;
    m_nodeTypeDesc = NULL;
  }

  void NodeConditional::updateLineage(NNO pno)
  {
    setYourParentNo(pno);
    m_nodeLeft->updateLineage(getNodeNo());
    m_nodeTypeDesc->updateLineage(getNodeNo());
  } //updateLineage

  bool NodeConditional::exchangeKids(Node * oldnptr, Node * newnptr)
  {
    if(m_nodeLeft == oldnptr)
      {
	m_nodeLeft = newnptr;
	return true;
      }
    return false;
  } //exhangeKids

  bool NodeConditional::findNodeNo(NNO n, Node *& foundNode)
  {
    if(Node::findNodeNo(n, foundNode))
      return true;
    if(m_nodeLeft->findNodeNo(n, foundNode))
      return true;
    if(m_nodeTypeDesc->findNodeNo(n, foundNode))
      return true;
    return false;
  } //findNodeNo

  void NodeConditional::print(File * fp)
  {
    printNodeLocation(fp);
    UTI myut = getNodeType();
    UTI ruti = getRightType();
    char id[255];
    if(myut == Nav)
      sprintf(id,"%s<NOTYPE>\n",prettyNodeName().c_str());
    else
      sprintf(id,"%s<%s>\n",prettyNodeName().c_str(), m_state.getUlamTypeNameByIndex(myut).c_str());
    fp->write(id);

    fp->write("conditional:\n");
    assert(m_nodeLeft);
    m_nodeLeft->print(fp);

    sprintf(id," %s ",getName());
    fp->write(id);

    //fp->write(m_typeTok.getTokenString());
    fp->write(m_state.getUlamTypeByIndex(ruti)->getUlamKeyTypeSignature().getUlamKeyTypeSignatureName(&m_state).c_str());
    fp->write("\n");
  } //print

  void NodeConditional::printPostfix(File * fp)
  {
    UTI ruti = getRightType();

    assert(m_nodeLeft);
    m_nodeLeft->printPostfix(fp);

    fp->write(" ");
    fp->write(m_state.getUlamTypeNameBriefByIndex(ruti).c_str());

    printOp(fp); //operators last
  } //printPostfix

  void NodeConditional::printOp(File * fp)
  {
    char myname[16];
    sprintf(myname," %s", getName());
    fp->write(myname);
  }

  bool NodeConditional::safeToCastTo(UTI newType)
  {
    //ulamtype checks for complete, non array, and type specific rules
    return m_state.getUlamTypeByIndex(newType)->safeCast(getNodeType());
  } //safeToCastTo

  void NodeConditional::countNavNodes(u32& cnt)
  {
    Node::countNavNodes(cnt); //missing
    m_nodeLeft->countNavNodes(cnt);
    m_nodeTypeDesc->countNavNodes(cnt);
  }

  UTI NodeConditional::getRightType()
  {
    return m_nodeTypeDesc->givenUTI();
  }

} //end MFM

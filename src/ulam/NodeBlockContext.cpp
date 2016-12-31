#include <stdio.h>
#include "NodeBlockContext.h"
#include "CompilerState.h"

namespace MFM {

  NodeBlockContext::NodeBlockContext(NodeBlock * prevBlockNode, CompilerState & state): NodeBlock(prevBlockNode, state, NULL){}

  NodeBlockContext::NodeBlockContext(const NodeBlockContext& ref) : NodeBlock(ref){}

  NodeBlockContext::~NodeBlockContext() {}

  const char * NodeBlockContext::getName()
  {
    return "context";
  }

  const std::string NodeBlockContext::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }

  StringPoolUser& NodeBlockContext::getUserStringPoolRef()
  {
    return m_upool;
  }

  void NodeBlockContext::setUserStringPoolRef(const StringPoolUser& spref)
  {
    m_upool = spref;
  }

} //end MFM

#include "SymbolVariableDataMember.h"
#include "CompilerState.h"

namespace MFM {

  SymbolVariableDataMember::SymbolVariableDataMember(u32 id, UTI utype, u32 slot, CompilerState& state) : SymbolVariable(id, utype, true, state), m_dataMemberUnpackedSlotIndex(slot)
  {
    setDataMember();
  }


  SymbolVariableDataMember::~SymbolVariableDataMember()
  {
    //   m_static.clearAllocatedMemory(); //clean up arrays,etc.
  }


  u32  SymbolVariableDataMember::getDataMemberUnpackedSlotIndex()
  {
    return m_dataMemberUnpackedSlotIndex;
  }


  s32 SymbolVariableDataMember::getBaseArrayIndex()
  {
    return (s32) getDataMemberUnpackedSlotIndex();
  }


  const std::string SymbolVariableDataMember::getMangledPrefix()
  {
    return "Um_"; 
  }


  // replaced by NodeVarDecl:genCode to leverage the declaration order preserved by the parse tree.
  void SymbolVariableDataMember::generateCodedVariableDeclarations(File * fp, ULAMCLASSTYPE classtype)
  {
    UTI vuti = getUlamTypeIdx(); 
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);
    ULAMCLASSTYPE vclasstype = vut->getUlamClass();

    m_state.indent(fp);
    fp->write(vut->getUlamTypeMangledName(&m_state).c_str()); //for C++
    
    if(vclasstype == UC_QUARK)       // called on classtype elements only
      {
	fp->write("<");
	fp->write_decimal(getPosOffset());
	fp->write(">");
      }    
    fp->write(" ");
    fp->write(getMangledName().c_str());

#if 0
    u32 arraysize = vut->getArraySize();
    if(arraysize > 0)
      {
	fp->write("[");
	fp->write_decimal(arraysize);
	fp->write("]");
      }
#endif
    fp->write(";\n");  
  }


  // replaced by NodeVarDecl:genCode to leverage the declaration order preserved by the parse tree.
  void SymbolVariableDataMember::printPostfixValuesOfVariableDeclarations(File * fp, ULAMCLASSTYPE classtype)
  {
    UTI vuti = getUlamTypeIdx(); 
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);
    ULAMCLASSTYPE vclasstype = vut->getUlamClass();

    fp->write(" ");
    fp->write(m_state.getUlamTypeNameBriefByIndex(vuti).c_str()); //short type name
    fp->write(" ");
    fp->write(m_state.m_pool.getDataAsString(getId()).c_str());

    u32 arraysize = vut->getArraySize();
    arraysize = arraysize > 0 ? arraysize : 1;
    u32 bitlen = vut->getBitSize();
    assert(m_state.determinePackable(vuti));   //has to be to fit in an atom/site

    char * valstr = new char[arraysize * 8 + 32];

    //simplifying assumption for testing purposes: center site
    Coord c0(0,0);
    u32 slot = c0.convertCoordToIndex();

    //build the string of values (for both scalar and packed array)
    UlamValue arrayPtr = UlamValue::makePtr(slot, EVENTWINDOW, vuti, true, m_state, ATOMFIRSTSTATEBITPOS + getPosOffset());
    UlamValue nextPtr = UlamValue::makeScalarPtr(arrayPtr, m_state);
    
    UlamValue atval = m_state.getPtrTarget(nextPtr);
    u32 data = atval.getImmediateData(m_state);
    sprintf(valstr,"%d", data);
    
    for(u32 i = 1; i < arraysize; i++)
      {
	char tmpstr[8];
	nextPtr.incrementPtr(m_state);
	atval = m_state.getPtrTarget(nextPtr);
	data = atval.getImmediateData(m_state);
	sprintf(tmpstr,",%d", data); 
	strcat(valstr,tmpstr);
      }
    
#if 1
    if(arraysize > 1)
      {
	fp->write("[");
	fp->write_decimal(arraysize);
	fp->write("]");
      }
#endif

    fp->write("=(");
    fp->write(valstr);
    fp->write("); ");

    delete [] valstr;
  }

} //end MFM

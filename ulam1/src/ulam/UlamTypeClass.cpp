#include <ctype.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include "UlamTypeClass.h"
#include "UlamValue.h"
#include "CompilerState.h"
#include "Util.h"

namespace MFM {

  UlamTypeClass::UlamTypeClass(const UlamKeyTypeSignature key, CompilerState & state, ULAMCLASSTYPE type) : UlamType(key, state), m_class(type), m_customArray(false)
  {
    m_wordLengthTotal = calcWordSize(getTotalBitSize());
    m_wordLengthItem = calcWordSize(getBitSize());
  }

   ULAMTYPE UlamTypeClass::getUlamTypeEnum()
   {
     return Class;
   }

  bool UlamTypeClass::isNumericType()
  {
    if(getUlamClass() == UC_QUARK)
      {
	u32 quti = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	return (m_state.quarkHasAToIntMethod(quti));
      }
    return false;
  } //isNumericType

  bool UlamTypeClass::isPrimitiveType()
  {
    return isNumericType();
  }

  bool UlamTypeClass::cast(UlamValue & val, UTI typidx)
  {
    bool brtn = true;
    assert(m_state.getUlamTypeByIndex(typidx) == this); //tobe
    UTI valtypidx = val.getUlamValueTypeIdx();
    UlamType * vut = m_state.getUlamTypeByIndex(valtypidx);
    assert(vut->isScalar() && isScalar());
    ULAMTYPE vetype = vut->getUlamTypeEnum();
    //now allowing atoms to be cast as quarks, as well as elements;
    // also allowing subclasses to be cast as their superclass (u1.2.2)
    if(getUlamClass() == UC_ELEMENT)
      {
	if(!(vetype == UAtom || UlamType::compare(valtypidx, typidx, m_state) == UTIC_SAME))
	  {
	    //no longer elements inherit from elements, only quarks.
	    brtn = false;
	  }
	else if(vetype == UAtom)
	  val.setAtomElementTypeIdx(typidx); //for testing purposes, assume ok
	//else true
      }
    else if(getUlamClass() == UC_QUARK)
      {
	if(vetype == UAtom)
	  brtn = false; //cast atom to a quark?
	else if(UlamType::compare(valtypidx, typidx, m_state) == UTIC_SAME)
	  {
	    //if same type nothing to do; if atom, shows as element in eval-land.
	    //val.setAtomElementTypeIdx(typidx); //?
	  }
	else if(m_state.isClassASubclassOf(valtypidx, typidx))
	  {
	    //2 quarks, or element (val) inherits from this quark
	    ULAMCLASSTYPE vclasstype = vut->getUlamClass();
	    if(vclasstype == UC_ELEMENT)
	      {
		s32 pos = 0; //ancestors start at first state bit pos
		s32 len = getTotalBitSize();
		assert(len != UNKNOWNSIZE);
		if(len <= MAXBITSPERINT)
		  {
		    u32 qdata = val.getDataFromAtom(pos + ATOMFIRSTSTATEBITPOS, len);
		    val = UlamValue::makeImmediateQuark(typidx, qdata, len);
		  }
		else if(len <= MAXBITSPERLONG)
		  {
		    assert(0); //quarks are max 32 bits
		    u64 qdata = val.getDataLongFromAtom(pos + ATOMFIRSTSTATEBITPOS, len);
		    val = UlamValue::makeImmediateLong(typidx, qdata, len);
		  }
		else
		  assert(0);
	      }
	    else
	      {
		// both left-justified immediate quarks
		// Coo c = (Coo) f.su; where su is a Soo : Coo
		s32 vlen = vut->getTotalBitSize();
		s32 len = getTotalBitSize();
		u32 vdata = val.getImmediateQuarkData(vlen); //not from element
		assert((vlen - len) >= 0); //sanity check
		u32 qdata = vdata >> (vlen - len); //stays left-justified
		val = UlamValue::makeImmediateQuark(typidx, qdata, len);
	      }
	  }
	else
	  brtn = false;
      } //end quark casting
    else
      assert(0); //neither element nor quark

    return brtn;
  } //end cast

  FORECAST UlamTypeClass::safeCast(UTI typidx)
  {
    FORECAST scr = UlamType::safeCast(typidx);
    if(scr != CAST_CLEAR)
      return scr;

    if(m_state.getUlamTypeByIndex(typidx) == this)
      return CAST_CLEAR; //same class, quark or element
    else
      {
	u32 cuti = m_key.getUlamKeyTypeSignatureClassInstanceIdx(); //our scalar "new"
	if(m_state.isClassASubclassOf(typidx, cuti))
	  return CAST_CLEAR; //casting to a super class
	else
	  {
	    //e.g. array item to a ref of same type
	    UTI fmderef = m_state.getUlamTypeAsDeref(typidx);
	    ULAMTYPECOMPARERESULTS cmpr1 = UlamType::compare(fmderef, cuti, m_state);
	    if(cmpr1 == UTIC_SAME)
	      return CAST_CLEAR;
	    if(cmpr1 == UTIC_DONTKNOW)
	      return CAST_HAZY;

	    if(m_state.isClassASubclassOf(fmderef, cuti))
	      return CAST_CLEAR; //casting ref to a super class (may also be ref)

	    //ref of this class, applies to entire arrays too
	    UTI anyUTI = Nouti;
	    AssertBool anyDefined = m_state.anyDefinedUTI(m_key, anyUTI);
	   assert(anyDefined);

	    ULAMTYPECOMPARERESULTS cmpr2 = m_state.isARefTypeOfUlamType(typidx, anyUTI);
	    if(cmpr2 == UTIC_SAME)
	      return CAST_CLEAR;
	    else if(cmpr2 == UTIC_DONTKNOW)
	      return CAST_HAZY;

	    ULAMTYPECOMPARERESULTS cmpr3 = m_state.isARefTypeOfUlamType(anyUTI, typidx);
	    if(cmpr3 == UTIC_SAME)
	      return CAST_CLEAR;
	    else if(cmpr3 == UTIC_DONTKNOW)
	      return CAST_HAZY;
	  }
      }
    return CAST_BAD; //e.g. (typidx == UAtom)
  } //safeCast

  const char * UlamTypeClass::getUlamTypeAsSingleLowercaseLetter()
  {
    switch(getUlamClass())
      {
      case UC_ELEMENT:
	return "e";
      case UC_QUARK:
	return "q";
      case UC_UNSEEN:
      default:
	assert(0);
      };
    return UlamType::getUlamTypeEnumCodeChar(getUlamTypeEnum());
  } //getUlamTypeAsSingleLowercaseLetter()

  const std::string UlamTypeClass::getUlamTypeVDAsStringForC()
  {
    return "VD::BITS"; //for quark use bits
  }

  const std::string UlamTypeClass::getUlamTypeMangledType()
  {
    // e.g. parsing overloaded functions, may not be complete.
    std::ostringstream mangled;
    s32 bitsize = getBitSize();
    s32 arraysize = getArraySize();

    if(isReference())
      mangled << "r";

    if(arraysize > 0)
      mangled << ToLeximitedNumber(arraysize);
    else
      mangled << 10;

    if(bitsize > 0)
      mangled << ToLeximitedNumber(bitsize);
    else
      mangled << 10;

    mangled << m_state.getDataAsStringMangled(m_key.getUlamKeyTypeSignatureNameId()).c_str();
    //without types and values of args!!
    return mangled.str();
  } //getUlamTypeMangledType

  const std::string UlamTypeClass::getUlamTypeMangledName()
  {
    std::ostringstream mangledclassname;
    mangledclassname << UlamType::getUlamTypeMangledName(); //includes Uprefix

    //or numberOfParameters followed by each digi-encoded: mangled type and value
    u32 id = m_key.getUlamKeyTypeSignatureNameId();
    UTI cuti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    SymbolClassName * cnsym = (SymbolClassName *) m_state.m_programDefST.getSymbolPtr(id);
    mangledclassname << cnsym->formatAnInstancesArgValuesAsAString(cuti);
    return mangledclassname.str();
  } //getUlamTypeMangledName

  //quarks are right-justified in an atom space
  const std::string UlamTypeClass::getUlamTypeAsStringForC(bool useref)
  {
    assert(getUlamClass() != UC_UNSEEN);
    if(getUlamClass() == UC_QUARK)
      {
	return UlamType::getUlamTypeAsStringForC(useref);
      }
    return "T"; //for elements
  } //getUlamTypeAsStringForC()

  const std::string UlamTypeClass::getUlamTypeUPrefix()
  {
    if(getArraySize() > 0)
      return "Ut_";

    switch(getUlamClass())
      {
      case UC_ELEMENT:
	return "Ue_";
      case UC_QUARK:
	return "Uq_";
      case UC_UNSEEN:
	return "U?_";
     default:
	assert(0);
      };
    return "xx";
  } //getUlamTypeUPrefix

  const std::string UlamTypeClass::getUlamTypeNameBrief()
  {
    std::ostringstream namestr;

    namestr << m_key.getUlamKeyTypeSignatureName(&m_state).c_str();

    u32 id = m_key.getUlamKeyTypeSignatureNameId();
    u32 cuti = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    SymbolClassName * cnsym = (SymbolClassName *) m_state.m_programDefST.getSymbolPtr(id);
    if(cnsym && cnsym->isClassTemplate())
      namestr << ((SymbolClassNameTemplate *) cnsym)->formatAnInstancesArgValuesAsCommaDelimitedString(cuti).c_str();

    if(getReferenceType() != ALT_NOT)
      namestr << "&";
    return namestr.str();
  } //getUlamTypeNameBrief

  //see SymbolVariableDataMember printPostfix for recursive output
  void UlamTypeClass::getDataAsString(const u32 data, char * valstr, char prefix)
  {
    if(prefix == 'z')
      sprintf(valstr,"%d", data);
    else
      sprintf(valstr,"%c%d", prefix, data);
  }

  void UlamTypeClass::getDataLongAsString(const u64 data, char * valstr, char prefix)
  {
    if(prefix == 'z')
      sprintf(valstr,"%s", ToSignedDecimal(data).c_str());
    else
      sprintf(valstr,"%c%s", prefix, ToSignedDecimal(data).c_str());
  }

  ULAMCLASSTYPE UlamTypeClass::getUlamClass()
  {
    if(m_class == UC_UNSEEN && isReference())
      {
	UTI classidx = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	if(m_state.okUTItoContinue(classidx))
	  setUlamClass(m_state.getUlamTypeByIndex(classidx)->getUlamClass());
      }
    return m_class;
  }

  void UlamTypeClass::setUlamClass(ULAMCLASSTYPE type)
  {
    m_class = type;
    if(m_class == UC_ELEMENT)
      {
	setTotalWordSize(BITSPERATOM);
	setItemWordSize(BITSPERATOM);
      }
  } //setUlamClass

  bool UlamTypeClass::isScalar()
  {
    return (m_key.getUlamKeyTypeSignatureArraySize() == NONARRAYSIZE);
  }

  bool UlamTypeClass::isCustomArray()
  {
    return m_customArray; //canonical, ignores ancestors
  }

  void UlamTypeClass::setCustomArray()
  {
    m_customArray = true;
  }

  UTI UlamTypeClass::getCustomArrayType()
  {
    u32 cuti = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    return m_state.getAClassCustomArrayType(cuti);
  } //getCustomArrayType

  u32 UlamTypeClass::getCustomArrayIndexTypeFor(Node * rnode, UTI& idxuti, bool& hasHazyArgs)
  {
    u32 cuti = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    return m_state.getAClassCustomArrayIndexType(cuti, rnode, idxuti, hasHazyArgs);
  } //getCustomArrayIndexTypeFor

  s32 UlamTypeClass::getBitSize()
  {
    s32 bitsize = m_key.getUlamKeyTypeSignatureBitSize();
    return bitsize; //could be negative "unknown"; allow for empty quarks
  }

  bool UlamTypeClass::isHolder()
  {
    std::string name = m_key.getUlamKeyTypeSignatureName(&m_state);
    s32 c = name.at(0);
    return (!isalpha(c)); //id same as UTI number, out-of-band
  }

  bool UlamTypeClass::isComplete()
  {
    if(isHolder())
      return false;

    if(getUlamClass() == UC_UNSEEN)
      return false; //forgotten?

    return UlamType::isComplete();
  }

  bool UlamTypeClass::isMinMaxAllowed()
  {
    return false;
  }

  PACKFIT UlamTypeClass::getPackable()
  {
    if(getUlamClass() == UC_ELEMENT)
      return UNPACKED; //was PACKED, now matches ATOM regardless of its bit size.
    return UlamType::getPackable(); //quarks depend their size
  }

  const std::string UlamTypeClass::readMethodForCodeGen()
  {
    std::string method;
    if(getUlamClass() == UC_ELEMENT)
      method = "ReadAtom";
    else
      method = "Read"; //quarks only 32 bits
    return method;
  } //readMethodForCodeGen

  const std::string UlamTypeClass::writeMethodForCodeGen()
  {
    std::string method;
    if(getUlamClass() == UC_ELEMENT)
      method = "WriteAtom";
    else
      method = "Write"; //quarks only 32 bits
    return method;
  } //writeMethodForCodeGen

  bool UlamTypeClass::needsImmediateType()
  {
    if(!isComplete())
      return false;

    bool rtnb = false;
    if((getUlamClass() == UC_QUARK || getUlamClass() == UC_ELEMENT) && !isReference())
      {
	rtnb = true;
	u32 id = m_key.getUlamKeyTypeSignatureNameId();
	u32 cuti = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	SymbolClassName * cnsym = (SymbolClassName *) m_state.m_programDefST.getSymbolPtr(id);
	if(cnsym->isClassTemplate() && ((SymbolClassNameTemplate *) cnsym)->isClassTemplate(cuti))
	  rtnb = false; //the "template" has no immediate, only instances
      }
    return rtnb;
  } //needsImmediateType

  const std::string UlamTypeClass::getUlamTypeImmediateMangledName()
  {
    if(needsImmediateType() || isReference())
      {
	return UlamType::getUlamTypeImmediateMangledName();
      }
    return "T";
  } //getUlamTypeImmediateMangledName

  const std::string UlamTypeClass::getUlamTypeImmediateAutoMangledName()
  {
    assert(needsImmediateType() || isReference());

    if(isReference())
      {
	assert(0); //use ImmediateMangledName
	return getUlamTypeImmediateMangledName();
      }

    //get name of a referenced UTI for this type;
    UTI myuti = Nav;
    AssertBool anyDefined = m_state.anyDefinedUTI(m_key, myuti);
    assert(anyDefined);

    UTI asRefType = m_state.getUlamTypeAsRef(myuti);
    UlamType * asRef = m_state.getUlamTypeByIndex(asRefType);
    return asRef->getUlamTypeImmediateMangledName();
  } //getUlamTypeImmediateAutoMangledName

  const std::string UlamTypeClass::getTmpStorageTypeAsString()
  {
    if(getUlamClass() == UC_QUARK)
      {
	return "u32";
      }
    else if(getUlamClass() == UC_ELEMENT)
      {
 	return "T";
      }
    else
      assert(0);
    return ""; //error!
  } //getTmpStorageTypeAsString

  const std::string UlamTypeClass::getArrayItemTmpStorageTypeAsString()
  {
    if(!isScalar())
	return getTmpStorageTypeAsString(); //return its scalar tmp storage type

    return m_state.getUlamTypeByIndex(getCustomArrayType())->getTmpStorageTypeAsString();
  } //getArrayItemTmpStorageTypeAsString

  const std::string UlamTypeClass::getLocalStorageTypeAsString()
  {
    std::ostringstream ctype;
    ctype << getUlamTypeImmediateMangledName();

    if(getUlamClass() == UC_QUARK)
      ctype << "<EC>"; //default local quarks
    else if(getUlamClass() == UC_ELEMENT)
      ctype << "<EC>";
    else
      assert(0);
    return ctype.str();
  } //getLocalStorageTypeAsString

  const std::string UlamTypeClass::castMethodForCodeGen(UTI nodetype)
  {
    std::ostringstream rtnMethod;
    UlamType * nut = m_state.getUlamTypeByIndex(nodetype);
    //base types e.g. Int, Bool, Unary, Foo, Bar..
    u32 sizeByIntBitsToBe = getTotalWordSize();
    u32 sizeByIntBits = nut->getTotalWordSize();

    if(sizeByIntBitsToBe != sizeByIntBits)
      {
	std::ostringstream msg;
	msg << "Casting different word sizes; " << sizeByIntBits;
	msg << ", Value Type and size was: ";
	msg << nut->getUlamTypeName().c_str() << ", to be: ";
	msg << sizeByIntBitsToBe << " for type: ";
	msg << getUlamTypeName().c_str();
	MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(),msg.str().c_str(), ERR);
      }

    if(getUlamClass() != UC_ELEMENT)
      {
	std::ostringstream msg;
	msg << "Quarks only cast 'toInt': value type and size was: ";
	msg << nut->getUlamTypeName().c_str();
	msg << ", to be: " << getUlamTypeName().c_str();
	MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(),msg.str().c_str(), ERR);
      }

    //e.g. casting an element to an element, redundant and not supported: Element96ToElement96?
    if(!m_state.isAtom(nodetype))
      {
	std::ostringstream msg;
	msg << "Attempting to illegally cast a non-atom type to an element: ";
	msg << "value type and size was: ";
	msg << nut->getUlamTypeName().c_str() << ", to be: " << getUlamTypeName().c_str();
	MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(),msg.str().c_str(), ERR);
      }
    rtnMethod << "_" << nut->getUlamTypeNameOnly().c_str() << sizeByIntBits;
    rtnMethod << "ToElement" << sizeByIntBitsToBe;
    return rtnMethod.str();
  } //castMethodForCodeGen

  void UlamTypeClass::genUlamTypeMangledAutoDefinitionForC(File * fp)
  {
    if(getUlamClass() == UC_QUARK)
      return genUlamTypeQuarkMangledAutoDefinitionForC(fp);
    else if(getUlamClass() == UC_ELEMENT)
      return genUlamTypeElementMangledAutoDefinitionForC(fp);
    else
      assert(0);
  } //genUlamTypeMangledAutoDefinitionForC

  void UlamTypeClass::genUlamTypeQuarkMangledAutoDefinitionForC(File * fp)
  {
    assert(getUlamClass() == UC_QUARK);

    s32 len = getTotalBitSize(); //could be 0, includes arrays
    if(len > (BITSPERATOM - ATOMFIRSTSTATEBITPOS))
      return genUlamTypeMangledUnpackedArrayAutoDefinitionForC(fp);

    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();

    m_state.m_currentIndentLevel = 0;
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    //forward declaration of quark (before struct!)
    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(scalarmangledName.c_str());
    fp->write(";  //forward\n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n"); //quark auto perPos???

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());
    fp->write(" : public UlamRef<EC>\n");
    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRef"); //was atomicparametertype
    fp->write("<EC> Up_Us;\n");

    // see UlamClass.h for AutoRefBase
    //constructor for conditional-as (auto)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(T& targ, u32 idx, const UlamClass<EC>* effself) : UlamRef<EC>");
    fp->write("(idx, "); //the real pos!!!
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u, targ, effself) { }\n");

    //constructor for chain of autorefs (e.g. memberselect with array item)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const UlamRef<EC>& arg, u32 idx, const UlamClass<EC>* effself) : UlamRef<EC>(arg, idx, ");
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u, effself) { }\n");

    //copy constructor here; pos relative to exisiting (i.e. same).
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const ");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : UlamRef<EC>(r, 0u, r.GetLen(), r.GetEffectiveSelf()) { }\n");

    //read 'entire quark' method
    genUlamTypeReadDefinitionForC(fp);

    //write 'entire quark' method
    genUlamTypeWriteDefinitionForC(fp);

    // aref/aset calls generated inline for immediates.
    if(isCustomArray())
      {
	m_state.indent(fp);
	fp->write("/* a custom array, btw ('Us' has aref, aset methods) */\n");
      }

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeQuarkMangledAutoDefinitionForC

  void UlamTypeClass::genUlamTypeReadDefinitionForC(File * fp)
  {
    if(getUlamClass() == UC_QUARK)
      return genUlamTypeQuarkReadDefinitionForC(fp);
    else if(getUlamClass() == UC_ELEMENT)
      return genUlamTypeElementReadDefinitionForC(fp);
    else
      assert(0);
  } //genUlamTypeReadDefinitionForC

  void UlamTypeClass::genUlamTypeWriteDefinitionForC(File * fp)
  {
    if(getUlamClass() == UC_QUARK)
      return genUlamTypeQuarkWriteDefinitionForC(fp);
    else if(getUlamClass() == UC_ELEMENT)
      return genUlamTypeElementWriteDefinitionForC(fp);
    else
      assert(0);
  } //genUlamTypeWriteDefinitionForC

  void UlamTypeClass::genUlamTypeQuarkReadDefinitionForC(File * fp)
  {
    //scalar and entire PACKEDLOADABLE array handled by base class read method
    if(!isScalar())
      {
	//class instance idx is always the scalar uti
	UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
	//reads an item of array
	//2nd argument generated for compatibility with underlying method
	m_state.indent(fp);
	fp->write("const ");
	fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
	fp->write(" readArrayItem(");
	fp->write("const u32 index, const u32 itemlen) const { return "); //was const after )
	fp->write("UlamRef<EC>(");
	fp->write("*this, index * itemlen, "); //const ref, rel offset
	fp->write("itemlen, &");  //itemlen,
	fp->write(scalarmangledName.c_str()); //primitive effself
	fp->write("<EC>::THE_INSTANCE).Read(); }\n");
      }
  } //genUlamTypeQuarkReadDefinitionForC

  void UlamTypeClass::genUlamTypeQuarkWriteDefinitionForC(File * fp)
  {
    //scalar and entire PACKEDLOADABLE array handled by base class write method
    if(!isScalar())
      {
	//class instance idx is always the scalar uti
	UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
	// writes an item of array
	//3rd argument generated for compatibility with underlying method
	m_state.indent(fp);
	fp->write("void writeArrayItem(const ");
	fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
	fp->write(" v, const u32 index, const u32 itemlen) { ");
	fp->write("UlamRef<EC>(");
	fp->write("*this, index * itemlen, "); //rel offset
	fp->write("itemlen, &");  //itemlen,
	fp->write(scalarmangledName.c_str()); //primitive effself
	fp->write("<EC>::THE_INSTANCE).Write(v); }\n");
      }
  } //genUlamTypeQuarkWriteDefinitionForC

  void UlamTypeClass::genUlamTypeElementMangledAutoDefinitionForC(File * fp)
  {
    //regardless of actual element size, it takes up the full atom space
    if(!isScalar())
      return genUlamTypeMangledUnpackedArrayAutoDefinitionForC(fp);

    m_state.m_currentIndentLevel = 0;
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    //forward declaration of element (before struct!)
    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(getUlamTypeMangledName().c_str());
    fp->write(";  //forward\n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());
    fp->write(" : public UlamRefAtom<EC>");
    fp->write("\n");

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    m_state.indent(fp);
    fp->write("typedef BitField<BitVector<BPA>, VD::BITS, T::ATOM_FIRST_STATE_BIT, 0> BFTYP;\n");
    fp->write("\n");

    //element typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(getUlamTypeMangledName().c_str());
    fp->write("<EC> Us;\n");
    fp->write("\n");

    //constructor for conditional-as (auto)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(T& targ, const UlamClass<EC> * effself) : UlamRefAtom<EC>(targ, effself) { }\n");

    //constructor for chain of autorefs (e.g. memberselect with array item)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const UlamRefAtom<EC>& arg, const UlamClass<EC> * effself) : UlamRefAtom<EC>(arg, effself) { }\n");

    //copy constructor here
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const ");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : UlamRefAtom<EC>(r, r.GetEffectiveSelf()) { }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(automangledName.c_str());
    fp->write("() {}\n");

    //read 'entire atom' method
    genUlamTypeReadDefinitionForC(fp);

    //write 'entire atom' method
    genUlamTypeWriteDefinitionForC(fp);

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeElementMangledAutoDefinitionForC

  void UlamTypeClass::genUlamTypeElementReadDefinitionForC(File * fp)
  {
    // arrays are handled separately
    assert(isScalar());
  } //genUlamTypeElementReadDefinitionForC

  void UlamTypeClass::genUlamTypeElementWriteDefinitionForC(File * fp)
  {
    // arrays are handled separately
    assert(isScalar());

    // write must be scalar; ref param to avoid excessive copying
    //not an array
    m_state.indent(fp);
    fp->write("void");
    fp->write(" write(");
    fp->write(getTmpStorageTypeAsString().c_str()); //T
    fp->write("& targ) { return UlamRefAtom<EC>::WriteAtom(targ); /* entire element */ }\n");

    m_state.indent(fp);
    fp->write("void writeTypeField(const u32 v)");
    fp->write("{ UlamRefAtom<EC>::SetType(v); }\n");
  } //genUlamTypeElementWriteDefinitionForC

  const std::string UlamTypeClass::readArrayItemMethodForCodeGen()
  {
    if(isCustomArray())
      return m_state.getCustomArrayGetMangledFunctionName();
    return UlamType::readArrayItemMethodForCodeGen();
  }

  const std::string UlamTypeClass::writeArrayItemMethodForCodeGen()
  {
    if(isCustomArray())
      return m_state.getCustomArraySetMangledFunctionName();
    return UlamType::writeArrayItemMethodForCodeGen();
  }

  void UlamTypeClass::genUlamTypeMangledDefinitionForC(File * fp)
  {
    if(getUlamClass() == UC_QUARK)
      return genUlamTypeQuarkMangledDefinitionForC(fp);
    else if(getUlamClass() == UC_ELEMENT)
      return genUlamTypeElementMangledDefinitionForC(fp);
    else
      assert(0);
  } //genUlamTypeMangledDefinitionForC

  //builds the immediate definition, inheriting from auto
  void UlamTypeClass::genUlamTypeQuarkMangledDefinitionForC(File * fp)
  {
    assert((getUlamClass() == UC_QUARK));

    s32 len = getTotalBitSize(); //could be 0, includes arrays
    if(len > (BITSPERATOM - ATOMFIRSTSTATEBITPOS))
      return genUlamTypeMangledUnpackedArrayDefinitionForC(fp);

    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();

    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.m_currentIndentLevel = 0;

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");
    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    fp->write(" : public ");
    //let's see if gcc can make use of these extra template args for immediate primitives
    fp->write("UlamRefFixed");
    fp->write("<EC, 0u, "); //left-just pos
    fp->write_decimal_unsigned(len);
    fp->write("u>\n");

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed"); //was atomicparametertype
    fp->write("<EC, 0u, "); //BITSPERATOM
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u> Up_Us;\n");

    //storage here in atom
    m_state.indent(fp);
    fp->write("T m_stg;  //storage here!\n");

    //default constructor (used by local vars)
    //(unlike element) call build default in case of initialized data members
    u32 dqval = 0;
    bool hasDQ = genUlamTypeDefaultQuarkConstant(fp, dqval);

    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() : ");
    fp->write("Up_Us(m_stg, &");
    fp->write("Us::THE_INSTANCE), ");
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { "); //for immediate quarks
    if(hasDQ)
      {
	if(isScalar())
	  {
	    fp->write("Up_Us::Write(DEFAULT_QUARK);");
	  }
	else
	  {
	    //very packed array
	    if(len <= MAXBITSPERINT)
	      {
		u32 bitsize = getBitSize();
		u32 dqarrval = 0;
		u32 pos = 0;
		u32 mask = _GetNOnes32((u32) bitsize);
		u32 arraysize = getArraySize();
		dqval &= mask;
		//similar to build default quark value in NodeVarDeclDM
		for(u32 j = 1; j <= arraysize; j++)
		  {
		    dqarrval |= (dqval << (len - (pos + (j * bitsize))));
		  }

		std::ostringstream qdhex;
		qdhex << "0x" << std::hex << dqarrval;
		fp->write("Up_Us::Write(");
		fp->write(qdhex.str().c_str());
		fp->write(");");
	      }
	    else
	      {
		fp->write("u32 n = ");
		fp->write_decimal(getArraySize());
		fp->write("u; while(n--) { ");
		fp->write("writeArrayItem(DEFAULT_QUARK, n, ");
		fp->write_decimal_unsigned(getBitSize());
		fp->write("u); }");
	      }
	  }
      } //hasDQ
    fp->write(" }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" d) : ");
    fp->write("Up_Us(m_stg, &");
    fp->write("Us::THE_INSTANCE), "); //effself
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { "); //for immediate quarks
    fp->write("Up_Us::Write(d); }\n");

    // assignment constructor
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(mangledName.c_str());
    fp->write("<EC> & arg) : ");
    fp->write("Up_Us(m_stg, &");
    fp->write("Us::THE_INSTANCE), "); //effself
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { "); //for immediate quarks
    fp->write("Up_Us::Write(arg.Read()); }\n");

    genUlamTypeReadDefinitionForC(fp);

    genUlamTypeWriteDefinitionForC(fp);

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeQuarkMangledDefinitionForC

  //builds the immediate definition, inheriting from auto
  void UlamTypeClass::genUlamTypeElementMangledDefinitionForC(File * fp)
  {
    assert((getUlamClass() == UC_ELEMENT));

    //regardless of actual element size, it takes up the full atom space
    if(!isScalar())
      return genUlamTypeMangledUnpackedArrayDefinitionForC(fp);

    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();

    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.m_currentIndentLevel = 0;

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    //forward declaration of quark (before struct!)
    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(scalarmangledName.c_str());
    fp->write(";  //forward\n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n");
    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    fp->write(" : public ");
    fp->write("UlamRefAtom<EC>\n");

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //element typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC> Us;\n");
    fp->write("\n");

    //storage here in atom
    m_state.indent(fp);
    fp->write("T m_stg;  //storage here!\n");

    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() : ");
    fp->write("UlamRefAtom<EC>(m_stg, &");
    fp->write("Us::THE_INSTANCE), "); //effself
    fp->write("m_stg(");
    fp->write("Us::THE_INSTANCE");
    fp->write(".GetDefaultAtom()"); //returns object of type T
    fp->write(") { }\n");

    //constructor here (used by const tmpVars); ref param to avoid excessive copying
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //T
    fp->write("& d) : ");
    fp->write("UlamRefAtom<EC>(m_stg, &");
    fp->write("Us::THE_INSTANCE), "); //effself
    fp->write("m_stg(d) ");
    fp->write("{ }\n");

    //assignment constructor
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(mangledName.c_str());
    fp->write("<EC> & arg) : ");
    fp->write("UlamRefAtom<EC>(m_stg, &");
    fp->write("Us::THE_INSTANCE), "); //effself
    fp->write("m_stg(");
    fp->write("arg.ReadAtom()) { } \n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    //read 'entire atom' method
    genUlamTypeReadDefinitionForC(fp);

    //write 'entire atom' method
    genUlamTypeWriteDefinitionForC(fp);

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeElementMangledDefinitionForC

  void UlamTypeClass::genUlamTypeMangledUnpackedArrayAutoDefinitionForC(File * fp)
  {
    if(getUlamClass() == UC_ELEMENT)
      return genUlamTypeMangledUnpackedElementArrayAutoDefinitionForC(fp);
    else if(getUlamClass() == UC_QUARK)
      return genUlamTypeMangledUnpackedQuarkArrayAutoDefinitionForC(fp);
    assert(0);
    return;
  } //genUlamTypeMangledUnpackedArrayAutoDefinitionForC

  //similar to ulamtype primitives, except left-justified per item
  void UlamTypeClass::genUlamTypeMangledUnpackedQuarkArrayAutoDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();

    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 itemlen = getBitSize();
    u32 arraysize = getArraySize();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(mangledName.c_str());
    fp->write("; //forward \n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC");
    fp->write("> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC"); //BITSPERATOM
    fp->write(", 0u, "); //left-just
    fp->write_decimal_unsigned(itemlen); //includes arraysize
    fp->write("u > Up_Us;\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("T (& m_stgarrayref)[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u];  //ref to BIG storage!\n\n");

    //copy constructor here
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : m_stgarrayref(r.m_stgarrayref) { }\n");

    //constructor here (used by tmpVars)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(");
    fp->write(mangledName.c_str());
    fp->write("<EC>& d) : m_stgarrayref(d.getStorageRef()) { }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(automangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) const { return ");
    fp->write("getBits(index).Read(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u); }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" v, const u32 index, const u32 itemlen) { ");
    fp->write("getBits(index).Write(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u, v);");
    fp->write(" }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T&, Or UlamRef??? GetStorage?
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index + T::ATOM_FIRST_STATE_BIT"); //left-justified per item
    fp->write("); }\n");

    //Unpacked, position within each item T
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write(" ) const { return ");
    fp->write("(T::ATOM_FIRST_STATE_BIT");  //left-justified per item
    fp->write("); }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedQuarkArrayAutoDefinitionForC

  //similar to ulamtypeatom array of unpacked atoms
  void UlamTypeClass::genUlamTypeMangledUnpackedElementArrayAutoDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;

    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();

    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 arraysize = getArraySize();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(mangledName.c_str());
    fp->write("; //forward \n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("T (& m_stgarrayref)[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u];  //ref to BIG storage!\n\n");

    //copy constructor here
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : m_stgarrayref(r.m_stgarrayref) { }\n");

    //constructor here (used by tmpVars)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(");
    fp->write(mangledName.c_str());
    fp->write("<EC>& d) : m_stgarrayref(d.getStorageRef()) { }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(automangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read entire Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //T
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) const { return ");
    fp->write("m_stgarrayref[index]; /* entire element item */ }\n");

    //Unpacked Write entire Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //T&
    fp->write("& v, const u32 index, const u32 itemlen) { ");
    fp->write("m_stgarrayref[index] = v; /* entire element item */ }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T&
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index); }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedElementArrayAutoDefinitionForC

  void UlamTypeClass::genUlamTypeMangledUnpackedArrayDefinitionForC(File * fp)
  {
    if(getUlamClass() == UC_ELEMENT)
      return genUlamTypeMangledUnpackedElementArrayDefinitionForC(fp);
    else if(getUlamClass() == UC_QUARK)
      return genUlamTypeMangledUnpackedQuarkArrayDefinitionForC(fp);
    assert(0);
    return;
  } //genUlamTypeMangledUnpackedArrayDefinitionForC

  //similar to ulamtype primitives, except left-justified per item
  void UlamTypeClass::genUlamTypeMangledUnpackedQuarkArrayDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();

    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 itemlen = getBitSize();
    u32 arraysize = getArraySize();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC");
    fp->write("> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC"); //BITSPERATOM
    fp->write(", 0u, "); //left-just
    fp->write_decimal_unsigned(itemlen); //includes arraysize
    fp->write("u > Up_Us;\n");

    //Unpacked, storage reference T (&) [N]
    m_state.indent(fp);
    fp->write("typedef T TARR[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("];\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("TARR m_stgarr;  //BIG storage here!\n\n");


    u32 dqval = 0;
    bool hasDQ = genUlamTypeDefaultQuarkConstant(fp, dqval);

    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) {");
    fp->write("m_stgarr[j].SetUndefinedImpl();"); //T::ATOM_UNDEFINED_TYPE
    if(hasDQ)
      {
	fp->write("writeArrayItem(j, ");
	fp->write_decimal_unsigned(itemlen);
	fp->write("u, DEFAULT_QUARK);");
      }
    fp->write(" } }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //u32
    fp->write(" d) { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) {");
    fp->write("m_stgarr[j].SetUndefinedImpl(); "); //T::ATOM_UNDEFINED_TYPE
    fp->write("writeArrayItem(j, ");
    fp->write_decimal_unsigned(itemlen);
    fp->write("u, d); } }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) const { return ");
    fp->write("getBits(index).Read(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u); }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" v, const u32 index, const u32 itemlen) { ");
    fp->write("getBits(index).Write(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u, v);");
    fp->write(" }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T&, Or UlamRef??? GetStorage?
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index + T::ATOM_FIRST_STATE_BIT"); //left-justified per item
    fp->write("); }\n");

    //Unpacked, position within each item T
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write(" ) const { return ");
    fp->write("(T::ATOM_FIRST_STATE_BIT");  //left-justified per item
    fp->write("); }\n");

    m_state.indent(fp);
    fp->write("TARR& getStorageRef(");
    fp->write(") { return ");
    fp->write("m_stgarr; }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedQuarkArrayDefinitionForC

  //similar to ulamtypeatom array of unpacked atoms
  void UlamTypeClass::genUlamTypeMangledUnpackedElementArrayDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;

    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();

    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 arraysize = getArraySize();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //Unpacked, storage reference T (&) [N]
    m_state.indent(fp);
    fp->write("typedef T TARR[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("];\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("TARR m_stgarr;  //BIG storage here!\n\n");

    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) ");
    fp->write("writeArrayItem(");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC>::THE_INSTANCE");
    fp->write(".GetDefaultAtom()"); //returns object of type T
    fp->write(", j, BPA); }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //T
    fp->write("& d) { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) {");
    fp->write("writeArrayItem(d, j, BPA);");
    fp->write(" } }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read entire Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //T
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) const { return ");
    fp->write("m_stgarr[index]; /* entire element item */ }\n");

    //Unpacked Write entire Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //T&
    fp->write("& v, const u32 index, const u32 itemlen) { ");
    fp->write("m_stgarr[index] = v; /* entire element item */ }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T&
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index); }\n");

    m_state.indent(fp);
    fp->write("TARR& getStorageRef(");
    fp->write(") { return ");
    fp->write("m_stgarr; }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedElementArrayDefinitionForC

  void UlamTypeClass::genUlamTypeMangledImmediateModelParameterDefinitionForC(File * fp)
  {
    assert(0); //only primitive types
  }

  bool UlamTypeClass::genUlamTypeDefaultQuarkConstant(File * fp, u32& dqref)
  {
    bool rtnb = false;
    if(getUlamClass() == UC_QUARK)
      {
	//always the scalar.
	if(m_state.getDefaultQuark(m_key.getUlamKeyTypeSignatureClassInstanceIdx(), dqref))
	  {
	    std::ostringstream qdhex;
	    qdhex << "0x" << std::hex << dqref;

	    m_state.indent(fp);
	    fp->write("static const u32 DEFAULT_QUARK = ");
	    fp->write(qdhex.str().c_str());
	    fp->write("; //=");
	    fp->write_decimal_unsigned(dqref);
	    fp->write("u\n\n");
	    rtnb = true;
	  }
      }
    return rtnb;
  } //genUlamTypeDefaultQuarkConstant

} //end MFM

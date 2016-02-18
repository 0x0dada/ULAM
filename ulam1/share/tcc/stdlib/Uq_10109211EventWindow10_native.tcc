/* -*- C++ -*- */
/**
   Uq_10109211EventWindow_native.tcc: EventWindow native methods implementation.
 */

namespace MFM{

  template<class EC>
  Ui_Ut_102961a<EC> Uq_10109211EventWindow10<EC>::Uf_4aref(const UlamContext<EC> & uc, UlamRef<EC>& ur,
							       Ui_Ut_10161u<EC> Uv_5index) const //native
  {
    u32 siteNumber = Uv_5index.Read();
    const EventWindow<EC> & ew = uc.GetEventWindow();
    const T & a = ew.GetAtomSym(siteNumber);
    return Ui_Ut_102961a<EC>(a, uc);
  }

  template<class EC>
  void Uq_10109211EventWindow10<EC>::Uf_4aset(const UlamContext<EC> & uc,
						  UlamRef<EC>& ur, Ui_Ut_10161u<EC> Uv_5index,
						  Ui_Ut_102961a<EC> Uv_1v) const //native
  {
    u32 siteNumber = Uv_5index.Read();
    EventWindow<EC> & ew = const_cast <UlamContext<EC> &>(uc).GetEventWindow();
    ew.SetAtomSym(siteNumber, Uv_1v.Read());
  }

  template<class EC>
  Ui_Ut_10111b<EC> Uq_10109211EventWindow10<EC>::Uf_6isLive(const UlamContext<EC> & uc,
								UlamRef<EC>& ur, Ui_Ut_10161u<EC> Uv_5index) const
  {
    u32 siteNumber = Uv_5index.Read();
    const EventWindow<EC> & ew = uc.GetEventWindow();
    return Ui_Ut_10111b<EC>(ew.IsLiveSiteSym(siteNumber));
  }

  template<class EC>
  Ui_Ut_10111b<EC> Uq_10109211EventWindow10<EC>::Uf_4swap(const UlamContext<EC> & uc, UlamRef<EC>& ur,
							      Ui_Ut_10161u<EC> Uv_6index1,
							      Ui_Ut_10161u<EC> Uv_6index2) const
  {
    u32 idx1 = Uv_6index1.Read();
    u32 idx2 = Uv_6index2.Read();
    EventWindow<EC> & ew = const_cast <UlamContext<EC> &>(uc).GetEventWindow();
    if (!ew.IsLiveSiteSym(idx1) || !ew.IsLiveSiteSym(idx2))
      return Ui_Ut_10111b<EC>(false);
    ew.SwapAtomsSym(idx1,idx2);
    return Ui_Ut_10111b<EC>(true);
  }

  template<class EC>
  Ui_Uq_102323C2D10<EC> Uq_10109211EventWindow10<EC>::Uf_8getCoord(const UlamContext<EC> & uc, UlamRef<EC>& ur, Ui_Ut_10161u<EC> Uv_7siteNum) const
  {
    //! EventWindow.ulam:21:     C2D ret;
    Ui_Uq_102323C2D10<EC> Uv_3ret;
    const EventWindow<EC> & ew = uc.GetEventWindow();
    u32 idx = Uv_7siteNum.Read();
    SPoint loc = ew.MapToPointSymValid(idx);

    //Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x::Write(Uv_3ret.getBits(), loc.GetX());
    Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x(Uv_3ret, NULL).Write(loc.GetX());
    //    Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y::Write(Uv_3ret.getBits(), loc.GetY());
    Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y(Uv_3ret, NULL).Write(loc.GetY());

    //! EventWindow.ulam:24:     return ret;
    const u32 Uh_tmpreg_loadable_240 = Uv_3ret.Read();
    const Ui_Uq_102323C2D10<EC> Uh_tmpval_loadable_241(Uh_tmpreg_loadable_240);
    return (Uh_tmpval_loadable_241);
  } // Uf_8getCoord

  template<class EC>
  Ui_Ut_10161u<EC> Uq_10109211EventWindow10<EC>::Uf_9213getSiteNumber(const UlamContext<EC> & uc, UlamRef<EC>& ur, Ui_Uq_102323C2D10<EC> Uv_5coord) const
  {
    enum { R = EC::EVENT_WINDOW_RADIUS };
    const EventWindow<EC> & ew = uc.GetEventWindow();

    //const s32 x = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x::Read(Uv_5coord.getBits()), 16);
    const s32 x = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x(Uv_5coord, NULL).Read(), 16);
    //const s32 y = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y::Read(Uv_5coord.getBits()), 16);
    const s32 y = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y(Uv_5coord, NULL).Read(), 16);
    const SPoint loc(x,y);
    u32 ret;
    if (ew.InWindow(loc))
      ret = ew.MapToIndexSymValid(loc);
    else
      ret = EventWindow<EC>::SITE_COUNT;
    return Ui_Ut_10161u<EC>(ret);
  }

  //! EventWindow.ulam:28:   SiteNum size() native;
  template<class EC>
  Ui_Ut_10161u<EC> Uq_10109211EventWindow10<EC>::Uf_4size(const UlamContext<EC> & uc,
                                                               UlamRef<EC>& ur) const
  {
    return Ui_Ut_10161u<EC>(EventWindow<EC>::SITE_COUNT);
  }

  template<class EC>
  Ui_Ut_10131u<EC> Uq_10109211EventWindow10<EC>::Uf_9214changeSymmetry(const UlamContext<EC> & uc,UlamRef<EC>& ur,
									    Ui_Ut_10131u<EC> Uv_6newSym) const
  {
    EventWindow<EC> & ew = const_cast <UlamContext<EC> &>(uc).GetEventWindow();

    const u32 oldSym = (u32) ew.GetSymmetry();
    const u32 newSym = (u32) Uv_6newSym.Read();
    if (newSym < PSYM_SYMMETRY_COUNT)
      ew.SetSymmetry((PointSymmetry) newSym);
    return Ui_Ut_10131u<EC>(oldSym);
  } // Uf_9214changeSymmetry

  //! EventWindow.ulam:34:   C2D mapSym(C2D directCoord) {
  template<class EC>
  Ui_Uq_102323C2D10<EC> Uq_10109211EventWindow10<EC>::Uf_6mapSym(const UlamContext<EC> & uc, UlamRef<EC>& ur,
								      Ui_Uq_102323C2D10<EC> Uv_9211directCoord) const
  {
    const EventWindow<EC> & ew = uc.GetEventWindow();

    //const s32 x = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x::Read(Uv_9211directCoord.getBits()), 16);
    const s32 x = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x(Uv_9211directCoord, NULL).Read(), 16);
    //const s32 y = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y::Read(Uv_9211directCoord.getBits()), 16);
    const s32 y = _SignExtend32(Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y(Uv_9211directCoord, NULL).Read(), 16);
    const SPoint direct(x,y);

    SPoint mapped = SymMap(direct,ew.GetSymmetry(),direct);

    Ui_Uq_102323C2D10<EC> Uv_3ret;

    //Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x::Write(Uv_3ret.getBits(), mapped.GetX());
    Ui_Uq_102323C2D10<EC>::Us::Up_Um_1x(Uv_3ret, NULL).Write(mapped.GetX());

    //Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y::Write(Uv_3ret.getBits(), mapped.GetY());
    Ui_Uq_102323C2D10<EC>::Us::Up_Um_1y(Uv_3ret, NULL).Write(mapped.GetY());

    //! EventWindow.ulam:38:     return ret;
    return Ui_Uq_102323C2D10<EC>(Uv_3ret.Read());
  } // Uf_6mapSym


  template<class EC>
  void Uq_10109211EventWindow10<EC>::Uf_7diffuse(const UlamContext<EC> & uc,
                                                   UlamRef<EC>& ur) const	 //native
  {
    EventWindow<EC> & ew = const_cast <UlamContext<EC> &>(uc).GetEventWindow();
    ew.Diffuse();
  }


} //MFM

/*
 * mscodecs.h
 *
 * Microsoft nonstandard codecs handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: mscodecs.h,v $
 * Revision 1.2004  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.2  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.1  2001/08/01 05:03:09  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 * Allowed codecs to be used without H.,323 being linked by using the
 *   new NO_H323 define.
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.9  2002/09/03 05:41:56  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 * Added globally accessible functions for media format names.
 *
 * Revision 1.8  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.7  2001/10/24 01:20:34  robertj
 * Added code to help with static linking of H323Capability names database.
 *
 * Revision 1.6  2001/03/08 01:42:20  robertj
 * Cosmetic changes to recently added MS IMA ADPCM codec.
 *
 * Revision 1.5  2001/03/08 00:57:46  craigs
 * Added MS-IMA codec thanks to Liu Hao. Not yet working - do not use
 *
 * Revision 1.4  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.3  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.2  2001/01/09 23:05:22  robertj
 * Fixed inability to have 2 non standard codecs in capability table.
 *
 * Revision 1.1  2000/08/23 14:23:11  craigs
 * Added prototype support for Microsoft GSM codec
 *
 *
 */

#ifndef __OPAL_MSCODECS_H
#define __OPAL_MSCODECS_H

#ifdef __GNUC__
#pragma interface
#endif


#include <codec/gsmcodec.h>
#include <h323/h323caps.h>


#define OPAL_MSGSM "MS-GSM"

extern OpalMediaFormat const OpalMSGSM;

#define OPAL_MSIMA "MS-IMA-ADPCM"

extern OpalMediaFormat const OpalMSIMA;


///////////////////////////////////////////////////////////////////////////////

class Opal_MSGSM_PCM : public Opal_GSM0610 {
  public:
    Opal_MSGSM_PCM(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_MSGSM : public Opal_GSM0610 {
  public:
    Opal_PCM_MSGSM(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_MSIMA_PCM : public OpalFramedTranscoder {
  public:
    Opal_MSIMA_PCM(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

struct adpcm_state {
  short valprev;        /* Previous output value */
  char  index;          /* Index into stepsize table */
};

class Opal_PCM_MSIMA : public OpalFramedTranscoder {
  public:
    Opal_PCM_MSIMA(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
  protected:
    adpcm_state s_adpcm;
};


///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

class MicrosoftNonStandardAudioCapability : public H323NonStandardAudioCapability
{
  PCLASSINFO(MicrosoftNonStandardAudioCapability, H323NonStandardAudioCapability);

  public:
    MicrosoftNonStandardAudioCapability(
      const OpalMediaFormat & mediaFormat,
      const BYTE * header,
      PINDEX headerSize,
      PINDEX offset,
      PINDEX len
    );
};


/////////////////////////////////////////////////////////////////////////

class MicrosoftGSMAudioCapability : public MicrosoftNonStandardAudioCapability
{
  PCLASSINFO(MicrosoftGSMAudioCapability, MicrosoftNonStandardAudioCapability);

  public:
    MicrosoftGSMAudioCapability();
    PObject * MicrosoftGSMAudioCapability::Clone() const;
};


/////////////////////////////////////////////////////////////////////////

class MicrosoftIMAAudioCapability : public MicrosoftNonStandardAudioCapability
{
  PCLASSINFO(MicrosoftIMAAudioCapability, MicrosoftNonStandardAudioCapability);

  public:
    MicrosoftIMAAudioCapability();
    PObject * MicrosoftIMAAudioCapability::Clone() const;
};

#define OPAL_REGISTER_MSCODECS_H323 \
          H323_REGISTER_CAPABILITY(MicrosoftGSMAudioCapability, OPAL_MSGSM) \
          H323_REGISTER_CAPABILITY(MicrosoftIMAAudioCapability, OPAL_MSIMA)

#ifdef H323_STATIC_LIB
H323_STATIC_LOAD_REGISTER_CAPABILITY(MicrosoftGSMAudioCapability);
H323_STATIC_LOAD_REGISTER_CAPABILITY(MicrosoftIMAAudioCapability);
#endif


#else // ifndef NO_H323

#define OPAL_REGISTER_MSCODECS_H323

#endif // ifndef NO_H323

#define OPAL_REGISTER_MSCODECS() \
          OPAL_REGISTER_MSCODECS_H323 \
          OPAL_REGISTER_TRANSCODER(Opal_MSGSM_PCM, OPAL_MSGSM, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_MSGSM, OPAL_PCM16, OPAL_MSGSM); \
          OPAL_REGISTER_TRANSCODER(Opal_MSIMA_PCM, OPAL_MSIMA, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_MSIMA, OPAL_PCM16, OPAL_MSIMA)


#endif // __OPAL_MSCODECS_H


/////////////////////////////////////////////////////////////////////////

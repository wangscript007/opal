/*
 * sippdu.cxx
 *
 * Session Initiation Protocol PDU support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_SIP

#ifdef __GNUC__
#pragma implementation "sippdu.h"
#endif

#include <sip/sippdu.h>

#include <sip/sipep.h>
#include <sip/sipcon.h>
#include <opal/call.h>
#include <opal/manager.h>
#include <opal/connection.h>
#include <opal/transports.h>

#include <ptclib/cypher.h>
#include <ptclib/pdns.h>


#define  SIP_VER_MAJOR  2
#define  SIP_VER_MINOR  0


#define new PNEW

static bool LocateFieldParameter(const PString & fieldValue, const PString & paramName, PINDEX & start, PINDEX & val, PINDEX & end);

////////////////////////////////////////////////////////////////////////////

static const char * const MethodNames[SIP_PDU::NumMethods] = {
  "INVITE",
  "ACK",
  "OPTIONS",
  "BYE",
  "CANCEL",
  "REGISTER",
  "SUBSCRIBE",
  "NOTIFY",
  "REFER",
  "MESSAGE",
  "INFO",
  "PING",
  "PUBLISH",
  "PRACK"
};

#if PTRACING
ostream & operator<<(ostream & strm, SIP_PDU::Methods method)
{
  if (method < SIP_PDU::NumMethods)
    strm << MethodNames[method];
  else
    strm << "SIP_PDU_Method<" << (unsigned)method << '>';
  return strm;
}
#endif


const char * SIP_PDU::GetStatusCodeDescription(int code)
{
  static struct {
    int code;
    const char * desc;
  } sipErrorDescriptions[] = {
    { SIP_PDU::Information_Trying,                  "Trying" },
    { SIP_PDU::Information_Ringing,                 "Ringing" },
    { SIP_PDU::Information_CallForwarded,           "Call Forwarded" },
    { SIP_PDU::Information_Queued,                  "Queued" },
    { SIP_PDU::Information_Session_Progress,        "Progress" },

    { SIP_PDU::Successful_OK,                       "OK" },
    { SIP_PDU::Successful_Accepted,                 "Accepted" },

    { SIP_PDU::Redirection_MultipleChoices,         "Multiple Choices" },
    { SIP_PDU::Redirection_MovedPermanently,        "Moved Permanently" },
    { SIP_PDU::Redirection_MovedTemporarily,        "Moved Temporarily" },
    { SIP_PDU::Redirection_UseProxy,                "Use Proxy" },
    { SIP_PDU::Redirection_AlternativeService,      "Alternative Service" },

    { SIP_PDU::Failure_BadRequest,                  "BadRequest" },
    { SIP_PDU::Failure_UnAuthorised,                "Unauthorised" },
    { SIP_PDU::Failure_PaymentRequired,             "Payment Required" },
    { SIP_PDU::Failure_Forbidden,                   "Forbidden" },
    { SIP_PDU::Failure_NotFound,                    "Not Found" },
    { SIP_PDU::Failure_MethodNotAllowed,            "Method Not Allowed" },
    { SIP_PDU::Failure_NotAcceptable,               "Not Acceptable" },
    { SIP_PDU::Failure_ProxyAuthenticationRequired, "Proxy Authentication Required" },
    { SIP_PDU::Failure_RequestTimeout,              "Request Timeout" },
    { SIP_PDU::Failure_Conflict,                    "Conflict" },
    { SIP_PDU::Failure_Gone,                        "Gone" },
    { SIP_PDU::Failure_LengthRequired,              "Length Required" },
    { SIP_PDU::Failure_RequestEntityTooLarge,       "Request Entity Too Large" },
    { SIP_PDU::Failure_RequestURITooLong,           "Request URI Too Long" },
    { SIP_PDU::Failure_UnsupportedMediaType,        "Unsupported Media Type" },
    { SIP_PDU::Failure_UnsupportedURIScheme,        "Unsupported URI Scheme" },
    { SIP_PDU::Failure_BadExtension,                "Bad Extension" },
    { SIP_PDU::Failure_ExtensionRequired,           "Extension Required" },
    { SIP_PDU::Failure_IntervalTooBrief,            "Interval Too Brief" },
    { SIP_PDU::Failure_TemporarilyUnavailable,      "Temporarily Unavailable" },
    { SIP_PDU::Failure_TransactionDoesNotExist,     "Call Leg/Transaction Does Not Exist" },
    { SIP_PDU::Failure_LoopDetected,                "Loop Detected" },
    { SIP_PDU::Failure_TooManyHops,                 "Too Many Hops" },
    { SIP_PDU::Failure_AddressIncomplete,           "Address Incomplete" },
    { SIP_PDU::Failure_Ambiguous,                   "Ambiguous" },
    { SIP_PDU::Failure_BusyHere,                    "Busy Here" },
    { SIP_PDU::Failure_RequestTerminated,           "Request Terminated" },
    { SIP_PDU::Failure_NotAcceptableHere,           "Not Acceptable Here" },
    { SIP_PDU::Failure_BadEvent,                    "Bad Event" },
    { SIP_PDU::Failure_RequestPending,              "Request Pending" },
    { SIP_PDU::Failure_Undecipherable,              "Undecipherable" },

    { SIP_PDU::Failure_InternalServerError,         "Internal Server Error" },
    { SIP_PDU::Failure_NotImplemented,              "Not Implemented" },
    { SIP_PDU::Failure_BadGateway,                  "Bad Gateway" },
    { SIP_PDU::Failure_ServiceUnavailable,          "Service Unavailable" },
    { SIP_PDU::Failure_ServerTimeout,               "Server Time-out" },
    { SIP_PDU::Failure_SIPVersionNotSupported,      "SIP Version Not Supported" },
    { SIP_PDU::Failure_MessageTooLarge,             "Message Too Large" },

    { SIP_PDU::GlobalFailure_BusyEverywhere,        "Busy Everywhere" },
    { SIP_PDU::GlobalFailure_Decline,               "Decline" },
    { SIP_PDU::GlobalFailure_DoesNotExistAnywhere,  "Does Not Exist Anywhere" },
    { SIP_PDU::GlobalFailure_NotAcceptable,         "Not Acceptable" },

    { SIP_PDU::Local_TransportError,                "Transport Error" },
    { SIP_PDU::Local_BadTransportAddress,           "Invalid Address/Hostname" },
    { SIP_PDU::Local_Timeout,                       "Timeout or retries exceeded" },

    { 0 }
  };

  for (PINDEX i = 0; sipErrorDescriptions[i].code != 0; i++) {
    if (sipErrorDescriptions[i].code == code)
      return sipErrorDescriptions[i].desc;
  }
  return "";
}


ostream & operator<<(ostream & strm, SIP_PDU::StatusCodes status)
{
  strm << (unsigned)status;
  const char * info = SIP_PDU::GetStatusCodeDescription(status);
  if (info != NULL && *info != '\0')
    strm << ' ' << info;
  return strm;
}


static struct {
  char         compact;
  const char * full;
} const CompactForms[] = {
  { 'l', "Content-Length" },
  { 'c', "Content-Type" },
  { 'e', "Content-Encoding" },
  { 'f', "From" },
  { 'i', "Call-ID" },
  { 'm', "Contact" },
  { 's', "Subject" },
  { 't', "To" },
  { 'v', "Via" },
  { 'r', "Refer-To" },
  { 'b', "Referred-By" },
  { 'k', "Supported" },
  { 'o', "Event" }
};

/////////////////////////////////////////////////////////////////////////////

SIPURL::SIPURL()
{
}


SIPURL::SIPURL(const char * str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


SIPURL::SIPURL(const PString & str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


SIPURL::SIPURL(const PString & name,
               const OpalTransportAddress & address,
               WORD listenerPort)
{
  if (strncmp(name, "sip:", 4) == 0 || strncmp(name, "sips:", 5) == 0)
    Parse(name);
  else if (address.IsEmpty() && (name.Find('$') != P_MAX_INDEX)) 
    ParseAsAddress(PString::Empty(), name, listenerPort);
  else
    ParseAsAddress(name, address, listenerPort);
}

SIPURL::SIPURL(const OpalTransportAddress & address, WORD listenerPort)
{
  ParseAsAddress(PString::Empty(), address, listenerPort);
}
  
void SIPURL::ParseAsAddress(const PString & name, const OpalTransportAddress & address, WORD listenerPort)
{
  PIPSocket::Address ip;
  WORD port;
  if (address.GetIpAndPort(ip, port)) {
    PString transProto;
    WORD defaultPort = 5060;

    PStringStream s;
    s << "sip";

    PCaselessString proto = address.GetProto();
    if (proto == "tcps") {
      defaultPort = 5061;
      s << 's';
      // use default tranport=UDP
    }
    else if (proto != "udp")
      transProto = proto; // Typically "tcp"
    // else use default UDP

    s << ':';
    if (!name.IsEmpty())
      s << name << '@';
    s << ip.AsString(true);

    if (listenerPort == 0)
      listenerPort = port;
    if (listenerPort != 0 && listenerPort != defaultPort)
      s << ':' << listenerPort;

    if (!transProto.IsEmpty())
      s << ";transport=" << transProto;

    Parse(s);
  }
}


PBoolean SIPURL::InternalParse(const char * cstr, const char * p_defaultScheme)
{
  /* This will try to parse an SIP URI according to the RFC3261 EBNF

        Contact        =  ("Contact" / "m" ) HCOLON
                          ( STAR / (contact-param *(COMMA contact-param)))
        contact-param  =  (name-addr / addr-spec) *(SEMI contact-params)

        From           =  ( "From" / "f" ) HCOLON from-spec
        from-spec      =  ( name-addr / addr-spec ) *( SEMI from-param )

        Record-Route   =  "Record-Route" HCOLON rec-route *(COMMA rec-route)
        rec-route      =  name-addr *( SEMI rr-param )

        Reply-To       =  "Reply-To" HCOLON rplyto-spec
        rplyto-spec    =  ( name-addr / addr-spec ) *( SEMI rplyto-param )

        Route          =  "Route" HCOLON route-param *(COMMA route-param)
        route-param    =  name-addr *( SEMI rr-param )

        To             =  ( "To" / "t" ) HCOLON
                          ( name-addr / addr-spec ) *( SEMI to-param )


     For all of the above the field parameters (contact-params, to-param etc)
     all degenerate into special cases of the generic-param production. The
     generic-param is always included as an option for all these fields as
     well, so for general parsing we just need to parse the EBNF:

        generic-param  =  token [ EQUAL gen-value ]
        gen-value      =  token / host / quoted-string

     The URI is then embedded in:

        name-addr      =  [ display-name ] LAQUOT addr-spec RAQUOT
        addr-spec      =  SIP-URI / SIPS-URI / absoluteURI
        display-name   =  *(token LWS) / quoted-string

        token          =  1*(alphanum / "-" / "." / "!" / "%" / "*"
                             / "_" / "+" / "`" / "'" / "~" )

        quoted-string  =  SWS DQUOTE *(qdtext / quoted-pair ) DQUOTE
        qdtext         =  LWS / %x21 / %x23-5B / %x5D-7E
                                / UTF8-NONASCII
        quoted-pair    =  "\" (%x00-09 / %x0B-0C / %x0E-7F)


     So, what all that means is we distinguish between name-addr and addr-spec
     by the '<' character, but we can't just search for it if there is a
     quoted-string before it in the optional display-name field. So check for
     that and remove it first, then look for the the '<' which tells us if
     there COULD be a generic-param later.
   */

  displayName.MakeEmpty();
  fieldParameters.MakeEmpty();

  while (isspace(*cstr))
    cstr++;
  PString str = cstr;

  PINDEX endQuote = 0;
  if (str[0] == '"') {
    do {
      endQuote = str.Find('"', endQuote+1);
      if (endQuote == P_MAX_INDEX) {
        PTRACE(1, "SIP\tNo closing double quote in URI: " << str);
        return false; // No close quote!
      }
    } while (str[endQuote-1] == '\\');

    displayName = str(1, endQuote-1);

    PINDEX backslash;
    while ((backslash = displayName.Find('\\')) != P_MAX_INDEX)
      displayName.Delete(backslash, 1);
  }

  // see if URL is just a URI or it contains a display address as well
  PINDEX startBracket = str.Find('<', endQuote);
  PINDEX endBracket = str.Find('>', startBracket);

  const char * defaultScheme = p_defaultScheme != NULL ? p_defaultScheme : "sip";

  // see if URL is just a URI or it contains a display address as well
  if (startBracket == P_MAX_INDEX || endBracket == P_MAX_INDEX) {
    if (!PURL::InternalParse(cstr, defaultScheme))
      return false;
  }
  else {
    // get the URI from between the angle brackets
    if (!PURL::InternalParse(str(startBracket+1, endBracket-1), defaultScheme))
      return false;

    fieldParameters = str.Mid(endBracket+1).Trim();

    if (endQuote == 0) {
      // There were no double quotes around the display name, take
      // everything before the start angle bracket, sans whitespace
      displayName = str.Left(startBracket).Trim();
    }
  }

  return !IsEmpty();
}


PObject::Comparison SIPURL::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, SIPURL), PInvalidCast);
  const SIPURL & other = (const SIPURL &)obj;

  // RFC3261 Section 19.1.4 matching rules, hideously complicated!

#define COMPARE_COMPONENT(component) \
  if (component != other.component) \
    return component < other.component ? LessThan : GreaterThan

  COMPARE_COMPONENT(GetScheme());
  COMPARE_COMPONENT(GetUserName());
  COMPARE_COMPONENT(GetPassword());
  COMPARE_COMPONENT(GetHostName());
  COMPARE_COMPONENT(GetPort());
  COMPARE_COMPONENT(GetPortSupplied());

  // If URI parameter exists in both then must be equal
  for (PINDEX i = 0; i < paramVars.GetSize(); i++) {
    PString param = paramVars.GetKeyAt(i);
    if (other.paramVars.Contains(param))
      COMPARE_COMPONENT(paramVars[param]);
  }
  COMPARE_COMPONENT(paramVars("user"));
  COMPARE_COMPONENT(paramVars("ttl"));
  COMPARE_COMPONENT(paramVars("method"));

  /* While RFC3261/19.1.4 does not mention transport explicitly in the same
     sectionas the above, the "not equivalent" examples do state that the
     absence of a transport is not the same as using the default. While this
     is not normative, the logic is impeccable so we add it in. */
  COMPARE_COMPONENT(paramVars("transport"));

  return EqualTo;
}


PString SIPURL::AsQuotedString() const
{
  PStringStream s;

  if (!displayName)
    s << '"' << displayName << "\" ";
  s << '<' << AsString() << '>';

  if (!fieldParameters.IsEmpty()) {
    if (fieldParameters[0] != ';')
      s << ';';
    s << fieldParameters;
  }

  return s;
}


PString SIPURL::GetDisplayName (PBoolean useDefault) const
{
  PString s;
  PINDEX tag;
    
  s = displayName;

  if (displayName.IsEmpty () && useDefault) {

    s = AsString ();
    s.Replace ("sip:", "");

    /* There could be a tag if we are using the URL,
     * remove it
     */
    tag = s.Find (';');
    if (tag != P_MAX_INDEX)
      s = s.Left (tag);
  }

  return s;
}


PString SIPURL::GetFieldParameter(const PString & name) const
{
  return SIPMIMEInfo::ExtractFieldParameter(fieldParameters, name);
}


void SIPURL::SetFieldParameter(const PString & name, const PString & value)
{
  fieldParameters = SIPMIMEInfo::InsertFieldParameter(fieldParameters, name, value);
}


OpalTransportAddress SIPURL::GetHostAddress() const
{
  if (IsEmpty())
    return PString::Empty();

  PStringStream addr;

  if (scheme *= "sips")
    addr << "tcps$";
  else
    addr << paramVars("transport", "udp") << '$';

  if (paramVars.Contains("maddr"))
    addr << paramVars["maddr"];
  else
    addr << hostname;

  if (port > 0)
    addr << ':' << port;
  else
    addr << ":5060";

  return addr;
}


void SIPURL::Sanitise(UsageContext context)
{
  PINDEX i;

  // RFC3261, 19.1.1 Table 1
  static struct {
    const char * name;
    unsigned     contexts;
  } const SanitaryFields[] = {
    { "method",    (1<<RequestURI)|(1<<ToURI)|(1<<FromURI)|(1<<ContactURI)|(1<<RouteURI)|(1<<RegisterURI)                  },
    { "maddr",                     (1<<ToURI)|(1<<FromURI)                                                                 },
    { "ttl",                       (1<<ToURI)|(1<<FromURI)                |(1<<RouteURI)|(1<<RegisterURI)                  },
    { "transport",                 (1<<ToURI)|(1<<FromURI)                                                                 },
    { "lr",                        (1<<ToURI)|(1<<FromURI)|(1<<ContactURI)|              (1<<RegisterURI)                  },
    { "expires",   (1<<RequestURI)|(1<<ToURI)|(1<<FromURI)                |(1<<RouteURI)|(1<<RegisterURI)|(1<<ExternalURI) },
    { "tag",       (1<<RequestURI)|                        (1<<ContactURI)|(1<<RouteURI)|(1<<RegisterURI)|(1<<ExternalURI) }
  };

  for (i = 0; i < PARRAYSIZE(SanitaryFields); i++) {
    if (SanitaryFields[i].contexts&(1<<context)) {
      paramVars.RemoveAt(PCaselessString(SanitaryFields[i].name));
      PINDEX start, val, end;
      if (LocateFieldParameter(fieldParameters, SanitaryFields[i].name, start, val, end))
        fieldParameters.Delete(start, end-start);
    }
  }

  if (fieldParameters == ";")
    fieldParameters.MakeEmpty();

  for (i = 0; i < paramVars.GetSize(); ++i) {
    PCaselessString key = paramVars.GetKeyAt(i);
    if (key.NumCompare("OPAL-") == EqualTo) {
      paramVars.RemoveAt(key);
      --i; // Allow for ++ in for loop
    }
  }

  if (context != ContactURI && context != ExternalURI)
    queryVars.RemoveAll();

  switch (context) {
    case RequestURI :
      SetDisplayName(PString::Empty());
      break;

    case ToURI :
    case FromURI :
      if (GetPortSupplied()) {
        // Port not allowed for To or From, RFC3261, 19.1.1
        portSupplied = false;
        port = scheme == "sips" ? 5061 : 5060;
      }
      break;

    case ContactURI :
      /* Strict application of RFC3261, 19.1.1 does not require this, but
         there are some interop problems with registrars where the ports
         presence/absence is not handled correctly. There is a distinction
         between absence and presence with the default value, that is
         "sip:fred" is not the same as "sip:fred:5060". Some registrars
         remove the default port if present, others insert it. Sheesh!

         Safest thing to do is always assume an explicit port number for
         REGISTER contact field and never send an empty port. This should be
         generally OK, though not perfect. It prevents using an SRV record
         as the contact, but as the thing doing the registering is nearly
         always the thing listening on a port, it is the 99.9% solution.

         Note INVITE contact field is the RouteURI enum, not ContactURI so
         the port may be defaulted there.
       */
      if (!GetPortSupplied())
        SetPort(scheme == "sips" ? 5061 : 5060);
      break;

    case RegisterURI :
      username.MakeEmpty();
      password.MakeEmpty();

    default:
      break;
  }

  Recalculate();
}


#if OPAL_PTLIB_DNS
PBoolean SIPURL::AdjustToDNS(PINDEX entry)
{
  // RFC3263 states we do not do lookup if explicit port mentioned
  if (GetPortSupplied()) {
    PTRACE(4, "SIP\tNo SRV lookup as has explicit port number.");
    return true;
  }

  // Or it is a valid IP address, not a domain name
  PIPSocket::Address ip = GetHostName();
  if (ip.IsValid())
    return true;

  // Do the SRV lookup, if fails, then we actually return TRUE so outer loops
  // can use the original host name value.
  PIPSocketAddressAndPortVector addrs;
  if (!PDNS::LookupSRV(GetHostName(), "_sip._" + paramVars("transport", "udp"), GetPort(), addrs)) {
    PTRACE(4, "SIP\tNo SRV record found.");
    return true;
  }

  // Got the SRV list, return FALSE if outer loop has got to the end of it
  if (entry >= (PINDEX)addrs.size()) {
    PTRACE(4, "SIP\tRan out of SRV records at entry " << entry);
    return PFalse;
  }

  PTRACE(4, "SIP\tAttempting SRV record entry " << entry << ": " << addrs[entry].AsString());

  // Adjust our host and port to what the DNS SRV record says
  SetHostName(addrs[entry].GetAddress().AsString());
  SetPort(addrs[entry].GetPort());
  return true;
}
#else
PBoolean SIPURL::AdjustToDNS(PINDEX)
{
  return true;
}
#endif


PString SIPURL::GenerateTag()
{
  return OpalGloballyUniqueID().AsString();
}


void SIPURL::SetTag(const PString & tag)
{
  if (fieldParameters.Find(";tag=") != P_MAX_INDEX)
    return;

  fieldParameters += ";tag=" + tag;
}


/////////////////////////////////////////////////////////////////////////////

SIPMIMEInfo::SIPMIMEInfo(PBoolean _compactForm)
  : compactForm(_compactForm)
{
}


void SIPMIMEInfo::PrintOn(ostream & strm) const
{
  const char * eol = strm.fill() == '\r' ? "\r\n" : "\n";

  for (PINDEX i = 0; i < GetSize(); i++) {
    PCaselessString name = GetKeyAt(i);
    PString value = GetDataAt(i);

    if (compactForm) {
      for (PINDEX i = 0; i < PARRAYSIZE(CompactForms); ++i) {
        if (name == CompactForms[i].full) {
          name = CompactForms[i].compact;
          break;
        }
      }
    }

    if (value.FindOneOf("\r\n") == P_MAX_INDEX)
      strm << name << ": " << value << eol;
    else {
      PStringArray vals = value.Lines();
      for (PINDEX j = 0; j < vals.GetSize(); j++)
        strm << name << ": " << vals[j] << eol;
    }
  }

  strm << eol;
}


void SIPMIMEInfo::ReadFrom(istream & strm)
{
  PMIMEInfo::ReadFrom(strm);

  for (PINDEX i = 0; i < PARRAYSIZE(CompactForms); ++i) {
    PCaselessString compact(CompactForms[i].compact);
    if (Contains(compact)) {
      SetAt(CompactForms[i].full, *GetAt(compact));
      RemoveAt(compact);
    }
  }
}


PINDEX SIPMIMEInfo::GetContentLength() const
{
  PString len = GetString("Content-Length");
  if (len.IsEmpty())
    return 0; //P_MAX_INDEX;
  return len.AsInteger();
}

PBoolean SIPMIMEInfo::IsContentLengthPresent() const
{
  return Contains("Content-Length");
}


PCaselessString SIPMIMEInfo::GetContentType(bool includeParameters) const
{
  PCaselessString str = GetString(PMIMEInfo::ContentTypeTag);
  return str.Left(includeParameters ? P_MAX_INDEX : str.Find(';')).Trim();
}


void SIPMIMEInfo::SetContentType(const PString & v)
{
  SetAt(PMIMEInfo::ContentTypeTag,  v);
}


PCaselessString SIPMIMEInfo::GetContentEncoding() const
{
  return GetString("Content-Encoding");
}


void SIPMIMEInfo::SetContentEncoding(const PString & v)
{
  SetAt("Content-Encoding",  v);
}


PString SIPMIMEInfo::GetFrom() const
{
  return GetString("From");
}


void SIPMIMEInfo::SetFrom(const PString & v)
{
  SetAt("From",  v);
}

PString SIPMIMEInfo::GetPAssertedIdentity() const
{
  return (*this)["P-Asserted-Identity"];
}

void SIPMIMEInfo::SetPAssertedIdentity(const PString & v)
{
  SetAt("P-Asserted-Identity", v);
}

PString SIPMIMEInfo::GetPPreferredIdentity() const
{
  return (*this)["P-Preferred-Identity"];
}

void SIPMIMEInfo::SetPPreferredIdentity(const PString & v)
{
  SetAt("P-Preferred-Identity", v);
}

PString SIPMIMEInfo::GetCallID() const
{
  return GetString("Call-ID");
}


void SIPMIMEInfo::SetCallID(const PString & v)
{
  SetAt("Call-ID",  v);
}


PString SIPMIMEInfo::GetContact() const
{
  return GetString("Contact");
}


bool SIPMIMEInfo::GetContacts(std::set<SIPURL> & contacts) const
{
  return ExtractURLs(GetString("Contact"), contacts);
}


bool SIPMIMEInfo::ExtractURLs(const PString & str, std::set<SIPURL> & urls)
{
  PStringArray lines = str.Lines();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    PString line = lines[0];

    PINDEX previousPos = (PINDEX)-1;
    PINDEX comma = previousPos;
    do {
      PINDEX pos = line.FindOneOf(",\"<", previousPos+1);
      if (pos != P_MAX_INDEX && line[pos] != ',') {
        if (line[pos] == '<')
          previousPos = line.Find('>', pos);
        else {
          PINDEX lastQuote = pos;
          do { 
            lastQuote = line.Find('"', lastQuote+1);
          } while (lastQuote != P_MAX_INDEX && line[lastQuote-1] == '\\');
          previousPos = lastQuote;
        }
        if (previousPos != P_MAX_INDEX)
          continue;
        pos = previousPos;
      }

      SIPURL uri = line(comma+1, pos-1);
      if (!uri.GetPortSupplied())
        uri.SetPort(uri.GetScheme() == "sips" ? 5061 : 5060);
      urls.insert(uri);

      comma = previousPos = pos;
    } while (comma != P_MAX_INDEX);
  }

  return !urls.empty();
}


void SIPMIMEInfo::SetContact(const PString & v)
{
  SetAt("Contact",  v);
}


PString SIPMIMEInfo::GetSubject() const
{
  return GetString("Subject");
}


void SIPMIMEInfo::SetSubject(const PString & v)
{
  SetAt("Subject",  v);
}


PString SIPMIMEInfo::GetTo() const
{
  return GetString("To");
}


void SIPMIMEInfo::SetTo(const PString & v)
{
  SetAt("To",  v);
}


PString SIPMIMEInfo::GetVia() const
{
  return GetString("Via");
}


void SIPMIMEInfo::SetVia(const PString & v)
{
  if (!v.IsEmpty())
    SetAt("Via",  v);
}


bool SIPMIMEInfo::GetViaList(PStringList & viaList) const
{
  PString s = GetVia();
  if (s.FindOneOf("\r\n") != P_MAX_INDEX)
    viaList = s.Lines();
  else
    viaList = s.Tokenise(",", PFalse);

  return !viaList.IsEmpty();
}


void SIPMIMEInfo::SetViaList(const PStringList & v)
{
  PStringStream fieldValue;
  for (PStringList::const_iterator via = v.begin(); via != v.end(); ++via) {
    if (!fieldValue.IsEmpty())
      fieldValue << '\n';
    fieldValue << *via;
  }
  SetAt("Via", fieldValue);
}


PString SIPMIMEInfo::GetFirstVia() const
{
  // Get first via
  PString via = GetVia();
  via.Delete(via.FindOneOf("\r\n"), P_MAX_INDEX);
  return via;
}

OpalTransportAddress SIPMIMEInfo::GetViaReceivedAddress() const
{
  // Get first via
  PCaselessString via = GetFirstVia();

  if (via.Find("/UDP") == P_MAX_INDEX)
    return OpalTransportAddress();

  PINDEX start, val, end;
  if (!LocateFieldParameter(via, "rport", start, val, end) || val >= end)
    return OpalTransportAddress();

  WORD port = (WORD)via(val, end).AsUnsigned();
  if (port == 0)
    return OpalTransportAddress();
  
  if (LocateFieldParameter(via, "received", start, val, end) && val < end)
    return OpalTransportAddress(via(val, end), port, "udp");

  return OpalTransportAddress(via(via.Find(' ')+1, via.Find(':')-1), port, "udp");
}


PString SIPMIMEInfo::GetReferTo() const
{
  return GetString("Refer-To");
}


void SIPMIMEInfo::SetReferTo(const PString & r)
{
  SetAt("Refer-To",  r);
}

PString SIPMIMEInfo::GetReferredBy() const
{
  // If no RFC 3892 header, try Cisco custom header
  return GetString(Contains("Referred-By") ? "Referred-By" : "Diversion");
}


void SIPMIMEInfo::SetReferredBy(const PString & r)
{
  SetAt("Referred-By",  r);
}


void SIPMIMEInfo::SetContentLength(PINDEX v)
{
  SetAt("Content-Length", PString(PString::Unsigned, v));
}


PString SIPMIMEInfo::GetCSeq() const
{
  return GetString("CSeq");       // no compact form
}


void SIPMIMEInfo::SetCSeq(const PString & v)
{
  SetAt("CSeq",  v);            // no compact form
}


PStringList SIPMIMEInfo::GetRoute() const
{
  return GetRouteList("Route", false);
}


void SIPMIMEInfo::SetRoute(const PString & v)
{
  if (!v.IsEmpty())
    SetAt("Route",  v);
}


void SIPMIMEInfo::SetRoute(const PStringList & v)
{
  if (!v.IsEmpty())
    SetRouteList("Route",  v);
}


PStringList SIPMIMEInfo::GetRecordRoute(bool reversed) const
{
  return GetRouteList("Record-Route", reversed);
}


void SIPMIMEInfo::SetRecordRoute(const PStringList & v)
{
  if (!v.IsEmpty())
    SetRouteList("Record-Route",  v);
}


PStringList SIPMIMEInfo::GetRouteList(const char * name, bool reversed) const
{
  PStringList routeSet;

  PString s = GetString(name);
  PINDEX left;
  PINDEX right = 0;
  while ((left = s.Find('<', right)) != P_MAX_INDEX &&
         (right = s.Find('>', left)) != P_MAX_INDEX &&
         (right - left) > 5) {
    PString * pstr = new PString(s(left+1, right-1));
    if (reversed)
      routeSet.InsertAt(0, pstr);
    else
      routeSet.Append(pstr);
  }

  return routeSet;
}


void SIPMIMEInfo::SetRouteList(const char * name, const PStringList & v)
{
  PStringStream s;

  for (PStringList::const_iterator via = v.begin(); via != v.end(); ++via) {
    if (!s.IsEmpty())
      s << ',';
    s << '<' << *via << '>';
  }

  SetAt(name,  s);
}


PString SIPMIMEInfo::GetAccept() const
{
  return GetString("Accept");    // no compact form
}


void SIPMIMEInfo::SetAccept(const PString & v)
{
  SetAt("Accept", v);  // no compact form
}


PString SIPMIMEInfo::GetAcceptEncoding() const
{
  return GetString("Accept-Encoding");   // no compact form
}


void SIPMIMEInfo::SetAcceptEncoding(const PString & v)
{
  SetAt("Accept-Encoding", v); // no compact form
}


PString SIPMIMEInfo::GetAcceptLanguage() const
{
  return GetString("Accept-Language");   // no compact form
}


void SIPMIMEInfo::SetAcceptLanguage(const PString & v)
{
  SetAt("Accept-Language", v); // no compact form
}


PString SIPMIMEInfo::GetAllow() const
{
  return GetString("Allow");     // no compact form
}


unsigned SIPMIMEInfo::GetAllowBitMask() const
{
  unsigned bits = 0;

  PCaselessString allowedMethods = GetAllow();
  for (unsigned i = 0; i < SIP_PDU::NumMethods; ++i) {
    if (allowedMethods.Find(MethodNames[i]) != P_MAX_INDEX)
      bits |= (1 << i);
  }

  return bits;
}


void SIPMIMEInfo::SetAllow(const PString & v)
{
  SetAt("Allow", v);   // no compact form
}



PString SIPMIMEInfo::GetDate() const
{
  return GetString("Date");      // no compact form
}


void SIPMIMEInfo::SetDate(const PString & v)
{
  SetAt("Date", v);    // no compact form
}


void SIPMIMEInfo::SetDate(const PTime & t)
{
  SetDate(t.AsString(PTime::RFC1123, PTime::GMT)) ;
}


void SIPMIMEInfo::SetDate(void) // set to current date
{
  SetDate(PTime()) ;
}

        
unsigned SIPMIMEInfo::GetExpires(unsigned dflt) const
{
  return GetInteger("Expires", dflt);      // no compact form
}


void SIPMIMEInfo::SetExpires(unsigned v)
{
  SetAt("Expires", PString(PString::Unsigned, v));      // no compact form
}


PINDEX SIPMIMEInfo::GetMaxForwards() const
{
  return GetInteger("Max-Forwards", P_MAX_INDEX);       // no compact form
}


void SIPMIMEInfo::SetMaxForwards(PINDEX v)
{
  SetAt("Max-Forwards", PString(PString::Unsigned, v)); // no compact form
}


PINDEX SIPMIMEInfo::GetMinExpires() const
{
  return GetInteger("Min-Expires", P_MAX_INDEX);        // no compact form
}


void SIPMIMEInfo::SetMinExpires(PINDEX v)
{
  SetAt("Min-Expires",  PString(PString::Unsigned, v)); // no compact form
}


PString SIPMIMEInfo::GetProxyAuthenticate() const
{
  return GetString("Proxy-Authenticate");        // no compact form
}


void SIPMIMEInfo::SetProxyAuthenticate(const PString & v)
{
  SetAt("Proxy-Authenticate",  v);      // no compact form
}


PStringSet SIPMIMEInfo::GetTokenSet(const char * field) const
{
  PStringSet set;
  PStringArray tokens = GetString(field).Tokenise(',');
  for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
    PString token = tokens[i].Trim();
    if (!token.IsEmpty())
      set += token;
  }
  return set;
}


void SIPMIMEInfo::AddTokenSet(const char * field, const PString & token)
{
  if (token.IsEmpty())
    RemoveAt(field);
  else {
    PString existing = GetString(field);
    if (existing.IsEmpty())
      SetAt(field,  token);
    else {
      existing += ',';
      existing += token;
      SetAt(field,  existing);
    }
  }
}


void SIPMIMEInfo::SetTokenSet(const char * field, const PStringSet & tokens)
{
  if (tokens.IsEmpty())
    RemoveAt(field);
  else {
    PStringStream strm;
    for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
      if (i > 0)
        strm << ',';
      strm << tokens.GetKeyAt(i);
    }
    SetAt(field,  strm);
  }
}


PStringSet SIPMIMEInfo::GetRequire() const
{
  return GetTokenSet("Require");
}


void SIPMIMEInfo::SetRequire(const PStringSet & v)
{
  SetTokenSet("Require", v);
}


void SIPMIMEInfo::AddRequire(const PString & v)
{
  SetTokenSet("Require", v);
}


PStringSet SIPMIMEInfo::GetSupported() const
{
  return GetTokenSet("Supported");
}


void SIPMIMEInfo::SetSupported(const PStringSet & v)
{
  SetTokenSet("Supported", v);
}


void SIPMIMEInfo::AddSupported(const PString & v)
{
  AddTokenSet("Supported", v);
}


PStringSet SIPMIMEInfo::GetUnsupported() const
{
  return GetTokenSet("Unsupported");
}


void SIPMIMEInfo::SetUnsupported(const PStringSet & v)
{
  SetTokenSet("Unsupported",  v);
}


void SIPMIMEInfo::AddUnsupported(const PString & v)
{
  SetTokenSet("Unsupported",  v);
}


PString SIPMIMEInfo::GetEvent() const
{
  return GetString("Event");
}


void SIPMIMEInfo::SetEvent(const PString & v)
{
  SetAt("Event",  v);
}


PCaselessString SIPMIMEInfo::GetSubscriptionState(PStringToString & info) const
{
  return GetComplex("Subscription-State", info) ? info[PString::Empty()] : PString::Empty();
}


void SIPMIMEInfo::SetSubscriptionState(const PString & v)
{
  SetAt("Subscription-State",  v);     // no compact form
}


PString SIPMIMEInfo::GetUserAgent() const
{
  return GetString("User-Agent");        // no compact form
}


void SIPMIMEInfo::SetUserAgent(const PString & v)
{
  SetAt("User-Agent",  v);     // no compact form
}


PString SIPMIMEInfo::GetOrganization() const
{
  return GetString("Organization");        // no compact form
}


void SIPMIMEInfo::SetOrganization(const PString & v)
{
  SetAt("Organization",  v);     // no compact form
}


PString SIPMIMEInfo::GetAllowEvents() const
{
  return GetString("Allow-Events");     // no compact form
}


void SIPMIMEInfo::SetAllowEvents(const PString & v)
{
  SetAt("Allow-Events", v);   // no compact form
}

static const char UserAgentTokenChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.!%*_+`'~";

void SIPMIMEInfo::GetProductInfo(OpalProductInfo & info) const
{
  PCaselessString str = GetUserAgent();
  if (str.IsEmpty()) {
    str = GetString("Server");
    if (str.IsEmpty()) {
      PTRACE(4, "SIP\tNo User-Agent or Server fields, Product Info unknown.");
      return; // Have nothing, change nothing
    }
  }

  // This is not strictly correct according to he BNF, but we cheat
  // and assume that the prod/ver tokens are first and if there is an
  // explicit comment field, it is always last. If any other prod/ver
  // tokens are present, then they will end up as the comments too.
  // All other variations just end up as one big comment

  PINDEX endFirstToken = str.FindSpan(UserAgentTokenChars);
  if (endFirstToken == 0) {
    info.name = str;
    info.vendor = info.version = PString::Empty();
    info.manufacturerCode = info.t35Extension = info.t35CountryCode = 0;
    PTRACE(4, "SIP\tProduct Info: name=\"" << str << '"');
    return;
  }

  PINDEX endSecondToken = endFirstToken;
  if (endFirstToken != P_MAX_INDEX && str[endFirstToken] == '/')
    endSecondToken = str.FindSpan(UserAgentTokenChars, endFirstToken+1);

  info.name = str.Left(endFirstToken);
  info.version = str(endFirstToken+1, endSecondToken);
  info.vendor = GetOrganization();
  info.comments = str.Mid(endSecondToken+1).Trim();
  PTRACE(4, "SIP\tProduct Info: name=\"" << info.name << "\","
                           " version=\"" << info.version << "\","
                            " vendor=\"" << info.vendor << "\","
                          " comments=\"" << info.comments << '"');
}


void SIPMIMEInfo::SetProductInfo(const PString & ua, const OpalProductInfo & info)
{
  PString userAgent = ua;
  if (userAgent.IsEmpty()) {
    PString comments;

    PINDEX pos;
    PCaselessString temp = info.name;
    if ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX) {
      comments += temp.Mid(pos);
      temp.Delete(pos, P_MAX_INDEX);
    }

    if (!temp.IsEmpty()) {
      userAgent = temp;

      temp = info.version;
      while ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX)
        temp.Delete(pos, 1);
      if (!temp.IsEmpty())
        userAgent += '/' + temp;
    }

    if (info.comments.IsEmpty() || info.comments[0] == '(')
      comments += info.comments;
    else
      comments += '(' + info.comments + ')';

    userAgent &= comments;
  }

  if (!userAgent.IsEmpty())
    SetUserAgent(userAgent);      // no compact form

  if (!info.vendor.IsEmpty())
    SetOrganization(info.vendor);      // no compact form
}


PString SIPMIMEInfo::GetWWWAuthenticate() const
{
  return GetString("WWW-Authenticate");  // no compact form
}


void SIPMIMEInfo::SetWWWAuthenticate(const PString & v)
{
  SetAt("WWW-Authenticate",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPIfMatch() const
{
  return GetString("SIP-If-Match");  // no compact form
}

void SIPMIMEInfo::SetSIPIfMatch(const PString & v)
{
  SetAt("SIP-If-Match",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPETag() const
{
  return GetString("SIP-ETag");  // no compact form
}

void SIPMIMEInfo::SetSIPETag(const PString & v)
{
  SetAt("SIP-ETag",  v);        // no compact form
}


void SIPMIMEInfo::GetAlertInfo(PString & info, int & appearance)
{
  info.MakeEmpty();
  appearance = -1;

  PString str = GetString("Alert-Info");
  if (str.IsEmpty())
    return;

  PINDEX pos = str.Find('<');
  PINDEX end = str.Find('>', pos);
  if (pos == P_MAX_INDEX || end == P_MAX_INDEX) {
    info = str;
    return;
  }

  info = str(pos+1, end-1);

  static const char appearance1[] = ";appearance=";
  pos = str.Find(appearance1, end);
  if (pos != P_MAX_INDEX) {
    appearance = str.Mid(pos+sizeof(appearance1)).AsUnsigned();
    return;
  }

  static const char appearance2[] = ";x-line-id";
  pos = str.Find(appearance2, end);
  if (pos != P_MAX_INDEX)
    appearance = str.Mid(pos+sizeof(appearance2)).AsUnsigned();
}


void SIPMIMEInfo::SetAlertInfo(const PString & info, int appearance)
{
  if (appearance < 0 && info.IsEmpty()) {
    RemoveAt("Alert-Info");
    return;
  }

  PStringStream str;
  if (info[0] == '<')
    str << info;
  else
    str << '<' << info << '>';

  if (appearance >= 0)
    str << ";appearance=" << appearance;

  SetAt("Alert-Info", str);
}


PString SIPMIMEInfo::GetCallInfo() const
{
  return GetString("Call-Info");
}


static bool LocateFieldParameter(const PString & fieldValue, const PString & paramName, PINDEX & start, PINDEX & val, PINDEX & end)
{
  PINDEX semicolon = (PINDEX)-1;
  while ((semicolon = fieldValue.Find(';', semicolon+1)) != P_MAX_INDEX) {
    start = semicolon+1;
    val = fieldValue.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!%*_+`'~", semicolon+1);
    if (val == P_MAX_INDEX) {
      end = val;
      if (fieldValue.Mid(start) *= paramName) {
        return true;
      }
      break;
    }
    else if (fieldValue[val] != '=') {
      if (fieldValue(start, val-1) *= paramName) {
        end = val-1;
        return true;
      }
    }
    else if ((fieldValue(start, val-1) *= paramName)) {
      val++;
      end = fieldValue.FindOneOf("()<>@,;:\\\"/[]?{}= \t", val)-1;
      return true;
    }
  }

  return false;
}


PString SIPMIMEInfo::ExtractFieldParameter(const PString & fieldValue,
                                           const PString & paramName,
                                           const PString & defaultValue)
{
  PINDEX start, val, end;
  if (!LocateFieldParameter(fieldValue, paramName, start, val, end))
    return PString::Empty();
  return (val == end) ? defaultValue : fieldValue(val, end);
}


PString SIPMIMEInfo::InsertFieldParameter(const PString & fieldValue,
                                          const PString & paramName,
                                          const PString & newValue)
{
  PStringStream newField;
  newField << paramName;
  if (!newValue.IsEmpty())
    newField << '=' << newValue;

  PINDEX start, val, end;
  PString str = fieldValue;
  if (LocateFieldParameter(fieldValue, paramName, start, val, end))
    str.Splice(newField, start, end-start+1);
  else
    str += ';' + newField;
  return str;
}


////////////////////////////////////////////////////////////////////////////////////

SIPAuthenticator::SIPAuthenticator(SIP_PDU & pdu)
  : m_pdu(pdu)
{
}


PMIMEInfo & SIPAuthenticator::GetMIME()
{
  return m_pdu.GetMIME();
}


PString SIPAuthenticator::GetURI()
{
  return m_pdu.GetURI().AsString();
}


PString SIPAuthenticator::GetEntityBody()
{
  m_pdu.SetEntityBody();
  return m_pdu.GetEntityBody();
}


PString SIPAuthenticator::GetMethod()
{
  return MethodNames[m_pdu.GetMethod()];
}


////////////////////////////////////////////////////////////////////////////////////

class SIPNTLMAuthentication : public PHTTPClientAuthentication
{
  public: 
    SIPNTLMAuthentication();

    virtual Comparison Compare(
      const PObject & other
    ) const;

    PBoolean Parse(
      const PString & auth,
      PBoolean proxy
    );

    PBoolean Authorise(
      SIP_PDU & pdu
    ) const;

    struct Type1MessageHdr {
       BYTE     protocol[8];     // 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'
       BYTE     type;            // 0x01
       BYTE     _zero1[3];
       WORD     flags;           // 0xb203
       BYTE     _zero2[2];

       PUInt16l dom_len;         // domain string length
       PUInt16l dom_len2;        // domain string length
       PUInt16l dom_off;         // domain string offset
       BYTE     _zero3[2];

       PUInt16l host_len;        // host string length
       PUInt16l host_len2;       // host string length
       PUInt16l host_off;        // host string offset (always 0x20)
       BYTE     _zero4[2];

       BYTE     hostAndDomain;   // host string and domain (ASCII)
    };

    void ConstructType1Message(PBYTEArray & message) const;

  public:
    PString domainName;
    PString hostName;
};

SIPNTLMAuthentication::SIPNTLMAuthentication()
{
  hostName   = "Hostname";
  domainName = "Domain";
}

PObject::Comparison SIPNTLMAuthentication::Compare(const PObject & other) const
{
  const SIPNTLMAuthentication * otherAuth = dynamic_cast<const SIPNTLMAuthentication *>(&other);
  if (otherAuth == NULL)
    return LessThan;

  Comparison result = hostName.Compare(otherAuth->hostName);
  if (result != EqualTo)
    return result;

  result = domainName.Compare(otherAuth->domainName);
  if (result != EqualTo)
    return result;

  return PHTTPClientAuthentication::Compare(other);
}

PBoolean SIPNTLMAuthentication::Parse(const PString & /*auth*/, PBoolean /*proxy*/)
{
  return false;
}

PBoolean SIPNTLMAuthentication::Authorise(SIP_PDU & pdu) const
{
  PBYTEArray type1;
  ConstructType1Message(type1);
  pdu.GetMIME().SetAt(isProxy ? "Proxy-Authorization" : "Authorization", AsHex(type1));

  return false;
}

void SIPNTLMAuthentication::ConstructType1Message(PBYTEArray & buffer) const
{
  BYTE * ptr = buffer.GetPointer(sizeof(Type1MessageHdr) + hostName.GetLength() + domainName.GetLength());

  Type1MessageHdr * hdr = (Type1MessageHdr *)ptr;
  memset(hdr, 0, sizeof(Type1MessageHdr));
  memcpy(hdr->protocol, "NTLMSSP", 7);
  hdr->flags = 0xb203;

  hdr->host_off = PUInt16l((WORD)(&hdr->hostAndDomain - (BYTE *)hdr));
  PAssert(hdr->host_off == 0x20, "NTLM auth cannot be constructed");
  hdr->host_len = hdr->host_len2 = PUInt16l((WORD)hostName.GetLength());
  memcpy(&hdr->hostAndDomain, (const char *)hostName, hdr->host_len);

  hdr->dom_off = PUInt16l((WORD)(hdr->host_off + hdr->host_len));
  hdr->dom_len = hdr->dom_len2  = PUInt16l((WORD)domainName.GetLength());
  memcpy(&hdr->hostAndDomain + hdr->dom_len - hdr->host_len, (const char *)domainName, hdr->host_len2);
}

////////////////////////////////////////////////////////////////////////////////////

SIP_PDU::SIP_PDU(Methods meth)
  : m_method(meth)
  , m_statusCode(IllegalStatusCode)
  , m_versionMajor(SIP_VER_MAJOR)
  , m_versionMinor(SIP_VER_MINOR)
  , m_SDP(NULL)
{
}


SIP_PDU::SIP_PDU(const SIP_PDU & request, 
                 StatusCodes code, 
                 const char * contact,
                 const char * extra,
                 const SDPSessionDescription * sdp)
  : m_method(NumMethods)
  , m_statusCode(code)
  , m_SDP(sdp != NULL ? new SDPSessionDescription(*sdp) : NULL)
{
  InitialiseHeaders(request);

  /* Use extra parameter as redirection URL in case of 302 */
  if (code == SIP_PDU::Redirection_MovedTemporarily) {
    m_mime.SetContact(SIPURL(extra));
    extra = NULL;
  }
  else if (contact != NULL) {
    m_mime.SetContact(PString(contact));
  }
    
  // format response
  if (extra != NULL)
    m_info = extra;
}


SIP_PDU::SIP_PDU(const SIP_PDU & pdu)
  : PSafeObject(pdu)
  , m_method(pdu.m_method)
  , m_statusCode(pdu.m_statusCode)
  , m_uri(pdu.m_uri)
  , m_versionMajor(pdu.m_versionMajor)
  , m_versionMinor(pdu.m_versionMinor)
  , m_info(pdu.m_info)
  , m_mime(pdu.m_mime)
  , m_entityBody(pdu.m_entityBody)
  , m_SDP(pdu.m_SDP != NULL ? new SDPSessionDescription(*pdu.m_SDP) : NULL)
{
}


SIP_PDU & SIP_PDU::operator=(const SIP_PDU & pdu)
{
  m_method = pdu.m_method;
  m_statusCode = pdu.m_statusCode;
  m_uri = pdu.m_uri;

  m_versionMajor = pdu.m_versionMajor;
  m_versionMinor = pdu.m_versionMinor;
  m_info = pdu.m_info;
  m_mime = pdu.m_mime;
  m_entityBody = pdu.m_entityBody;

  delete m_SDP;
  m_SDP = pdu.m_SDP != NULL ? new SDPSessionDescription(*pdu.m_SDP) : NULL;

  return *this;
}


SIP_PDU::~SIP_PDU()
{
  delete m_SDP;
}


void SIP_PDU::InitialiseHeaders(const SIPURL & dest,
                                const SIPURL & to,
                                const SIPURL & from,
                                const PString & callID,
                                unsigned cseq,
                                const PString & via)
{
  m_uri = dest;
  m_uri.Sanitise(m_method == Method_REGISTER ? SIPURL::RegisterURI : SIPURL::RequestURI);

  SIPURL tmp = to;
  tmp.Sanitise(SIPURL::ToURI);
  m_mime.SetTo(tmp.AsQuotedString());

  tmp = from;
  tmp.Sanitise(SIPURL::FromURI);
  m_mime.SetFrom(tmp.AsQuotedString());

  m_mime.SetCallID(callID);
  m_mime.SetCSeq(PString(cseq) & MethodNames[m_method]);
  m_mime.SetMaxForwards(70);
  m_mime.SetVia(via);
}


PString SIP_PDU::CreateVia(SIPEndPoint & endpoint, const OpalTransport & transport, SIPConnection * connection)
{
  PString localPartyName;

  if (connection != NULL) {
    localPartyName = connection->GetLocalPartyName();

    PINDEX pos = localPartyName.Find('@');
    if (pos != P_MAX_INDEX)
      localPartyName = localPartyName.Left(pos);

    pos = localPartyName.Find(' ');
    if (pos != P_MAX_INDEX)
      localPartyName.Replace(" ", "_", true);
  }

  OpalTransportAddress via = endpoint.GetLocalURL(transport, localPartyName).GetHostAddress();

  // construct Via:
  PINDEX dollar = via.Find('$');

  PStringStream str;
  str << "SIP/" << m_versionMajor << '.' << m_versionMinor << '/'
      << via.Left(dollar).ToUpper() << ' ';
  PIPSocket::Address ip;
  WORD port = 5060;
  if (via.GetIpAndPort(ip, port))
    str << ip.AsString(true) << ':' << port;
  else
    str << via.Mid(dollar+1);
  str << ";branch=z9hG4bK" << OpalGloballyUniqueID() << ";rport";
  return str;
}


void SIP_PDU::InitialiseHeaders(SIPDialogContext & dialog, const PString & via)
{
  InitialiseHeaders(dialog.GetRequestURI(),
                    dialog.GetRemoteURI(),
                    dialog.GetLocalURI(),
                    dialog.GetCallID(),
                    dialog.GetNextCSeq(),
                    via);
  SetRoute(dialog.GetRouteSet());
}


void SIP_PDU::InitialiseHeaders(SIPConnection & connection,
                                const OpalTransport & transport)
{
  InitialiseHeaders(connection.GetDialog(), CreateVia(connection.GetEndPoint(), transport));
  connection.GetEndPoint().AdjustToRegistration(transport, *this);
}


void SIP_PDU::InitialiseHeaders(const SIP_PDU & request)
{
  m_versionMajor = request.GetVersionMajor();
  m_versionMinor = request.GetVersionMinor();

  // add mandatory fields to response (RFC 2543, 11.2)
  const SIPMIMEInfo & requestMIME = request.GetMIME();
  m_mime.SetTo(requestMIME.GetTo());
  m_mime.SetFrom(requestMIME.GetFrom());
  m_mime.SetCallID(requestMIME.GetCallID());
  m_mime.SetCSeq(requestMIME.GetCSeq());
  m_mime.SetVia(requestMIME.GetVia());
  m_mime.SetRecordRoute(requestMIME.GetRecordRoute(false));
}


bool SIP_PDU::SetRoute(const PStringList & set)
{
  PStringList routeSet = set;

  if (routeSet.IsEmpty())
    return false;

  SIPURL firstRoute = routeSet.front();
  if (!firstRoute.GetParamVars().Contains("lr")) {
    // this procedure is specified in RFC3261:12.2.1.1 for backwards compatibility with RFC2543
    routeSet.MakeUnique();
    routeSet.RemoveAt(0);
    routeSet.AppendString(m_uri.AsString());
    m_uri = firstRoute;
    m_uri.Sanitise(SIPURL::RouteURI);
  }

  m_mime.SetRoute(routeSet);
  return true;
}


bool SIP_PDU::SetRoute(const SIPURL & proxy)
{
  if (proxy.IsEmpty())
    return false;

  PStringStream str;
  str << "<sip:" << proxy.GetHostName() << ':'  << proxy.GetPort() << ";lr>";
  m_mime.SetRoute(str);
  return true;
}


void SIP_PDU::SetAllow(unsigned bitmask)
{
  PStringStream str;
  
  for (Methods method = Method_INVITE ; method < SIP_PDU::NumMethods ; method = (Methods)(method+1)) {
    if ((bitmask&(1<<method))) {
      if (!str.IsEmpty())
        str << ',';
      str << method;
    }
  }
  
  m_mime.SetAllow(str);
}


void SIP_PDU::AdjustVia(OpalTransport & transport)
{
  // Update the VIA field following RFC3261, 18.2.1 and RFC3581
  PStringList viaList;
  if (!m_mime.GetViaList(viaList))
    return;

  PString viaFront = viaList.front();
  PString via = viaFront;
  PString port, ip;
  PINDEX j = 0;
  
  if ((j = via.FindLast (' ')) != P_MAX_INDEX)
    via = via.Mid(j+1);
  if ((j = via.Find (';')) != P_MAX_INDEX)
    via = via.Left(j);
  if ((j = via.Find (':')) != P_MAX_INDEX) {
    ip = via.Left(j);
    port = via.Mid(j+1);
  }
  else
    ip = via;

  PIPSocket::Address a (ip);
  PIPSocket::Address remoteIp;
  WORD remotePort;
  if (transport.GetLastReceivedAddress().GetIpAndPort(remoteIp, remotePort)) {
    PINDEX start, val, end;
    if (LocateFieldParameter(viaFront, "rport", start, val, end)) {
      // fill in empty rport and received for RFC 3581
      viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "rport", remotePort);
      viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "received", remoteIp);
    }
    else if (remoteIp != a ) // set received when remote transport address different from Via address 
    {
      viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "received", remoteIp);
    }
  }
  else if (!a.IsValid()) {
    // Via address given has domain name
    viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "received", via);
  }

  viaList.front() = viaFront;
  m_mime.SetViaList(viaList);
}


bool SIP_PDU::SendResponse(OpalTransport & transport,
                           StatusCodes code,
                           SIPEndPoint * endpoint,
                           const char * contact,
                           const char * extra) const
{
  SIP_PDU response(*this, code, contact, extra);
  return SendResponse(transport, response, endpoint);
}


bool SIP_PDU::SendResponse(OpalTransport & transport, SIP_PDU & response, SIPEndPoint * endpoint) const
{
  OpalTransportAddress newAddress;

  if (transport.IsReliable() && transport.IsOpen())
    newAddress = transport.GetRemoteAddress();
  else {
    WORD defaultPort = transport.GetEndPoint().GetDefaultSignalPort();

    PStringList viaList;
    if (m_mime.GetViaList(viaList)) {
      PString viaAddress = viaList.front();
      PString proto = viaList.front();
      PString viaPort = defaultPort;

      PINDEX bracket = viaAddress.FindLast(']');
      if (bracket == P_MAX_INDEX)
        bracket = 0;

      PINDEX pos = 0;
      // get the address specified in the Via
      if ((pos = viaAddress.FindLast (' ')) != P_MAX_INDEX)
        viaAddress = viaAddress.Mid(pos+1);
      if ((pos = viaAddress.Find (';')) != P_MAX_INDEX)
        viaAddress = viaAddress.Left(pos);
      if ((pos = viaAddress.Find (':')) != P_MAX_INDEX && (pos > bracket)) {
        viaPort = viaAddress.Mid(pos+1);
        viaAddress = viaAddress.Left(pos);
      }

      // get the protocol type from Via header
      if ((pos = proto.FindLast (' ')) != P_MAX_INDEX)
        proto = proto.Left(pos);
      if ((pos = proto.FindLast('/')) != P_MAX_INDEX)
        proto = proto.Mid(pos+1);

      // maddr is present, no support for multicast yet
      PString param = SIPMIMEInfo::ExtractFieldParameter(viaList.front(), "maddr");
      if (!param.IsEmpty()) 
        viaAddress = param;

      // received is present
      bool received = false;
      param = SIPMIMEInfo::ExtractFieldParameter(viaList.front(), "received");
      if (!param.IsEmpty()) {
        viaAddress = param;
        received = true;
      }

      // rport is present. be careful to distinguish between not present and empty
      PIPSocket::Address remoteIp;
      WORD remotePort;
      transport.GetLastReceivedAddress().GetIpAndPort(remoteIp, remotePort);
      {
        PINDEX start, val, end;
        if (LocateFieldParameter(viaList.front(), "rport", start, val, end)) {
          param = viaList.front()(val, end);
          if (!param.IsEmpty()) 
            viaPort = param;
          else
            viaPort = remotePort;
          if (!received) 
            viaAddress = remoteIp.AsString();
        }
      }

      newAddress = OpalTransportAddress(viaAddress+":"+viaPort, defaultPort, (proto *= "TCP") ? "tcp$" : "udp$");
    }
    else {
      // get Via from From field
      PString from = m_mime.GetFrom();
      if (!from.IsEmpty()) {
        PINDEX j = from.Find (';');
        if (j != P_MAX_INDEX)
          from = from.Left(j); // Remove all parameters
        j = from.Find ('<');
        if (j != P_MAX_INDEX && from.Find ('>') == P_MAX_INDEX)
          from += '>';

        SIPURL url(from);

        newAddress = OpalTransportAddress(url.GetHostName()+ ":" + PString(PString::Unsigned, url.GetPort()), defaultPort, "udp$");
      }
    }
  }

  if (endpoint != NULL && response.GetMIME().GetContact().IsEmpty())
    endpoint->AdjustToRegistration(transport, *const_cast<SIP_PDU *>(this));

  return response.Write(transport, newAddress);
}


void SIP_PDU::PrintOn(ostream & strm) const
{
  strm << m_mime.GetCSeq() << ' ';
  if (m_method != NumMethods)
    strm << m_uri;
  else if (m_statusCode != IllegalStatusCode)
    strm << '<' << (unsigned)m_statusCode << '>';
  else
    strm << "<<Uninitialised>>";
}


SIP_PDU::StatusCodes SIP_PDU::Read(OpalTransport & transport)
{
  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to read PDU from closed transport " << transport);
    return SIP_PDU::Local_TransportError;
  }

  PStringStream datagram;
  PBYTEArray pdu;
  bool truncated = false;

  istream * stream;
  if (transport.IsReliable())
    stream = &transport;
  else {
    stream = &datagram;

    if (!transport.ReadPDU(pdu)) {
      if (pdu.IsEmpty())
        return SIP_PDU::Local_TransportError;
      truncated = true;
    }

    datagram = PString((char *)pdu.GetPointer(), pdu.GetSize());
  }

  // get the message from transport/datagram into cmd and parse MIME
  PString cmd;
  *stream >> cmd >> m_mime;

  if (!stream->good() || cmd.IsEmpty() || m_mime.IsEmpty()) {
    if (stream == &datagram) {
      transport.setstate(ios::failbit);
      PTRACE(1, "SIP\tInvalid datagram from " << transport.GetLastReceivedAddress()
                << " - " << pdu.GetSize() << " bytes.\n" << hex << setprecision(2) << pdu << dec);
    }
    return SIP_PDU::Failure_BadRequest;
  }

  if (cmd.Left(4) *= "SIP/") {
    // parse Response version, code & reason (ie: "SIP/2.0 200 OK")
    PINDEX space = cmd.Find(' ');
    if (space == P_MAX_INDEX) {
      PTRACE(2, "SIP\tBad Status-Line \"" << cmd << "\" received on " << transport);
      return SIP_PDU::Failure_BadRequest;
    }

    m_versionMajor = cmd.Mid(4).AsUnsigned();
    m_versionMinor = cmd(cmd.Find('.')+1, space).AsUnsigned();
    m_statusCode = (StatusCodes)cmd.Mid(++space).AsUnsigned();
    m_info    = cmd.Mid(cmd.Find(' ', space));
    m_uri     = PString();
  }
  else {
    // parse the method, URI and version
    PStringArray cmds = cmd.Tokenise( ' ', PFalse);
    if (cmds.GetSize() < 3) {
      PTRACE(2, "SIP\tBad Request-Line \"" << cmd << "\" received on " << transport);
      return SIP_PDU::Failure_BadRequest;
    }

    int i = 0;
    while (!(cmds[0] *= MethodNames[i])) {
      i++;
      if (i >= NumMethods) {
        PTRACE(2, "SIP\tUnknown method name " << cmds[0] << " received on " << transport);
        return SIP_PDU::Failure_BadRequest;
      }
    }
    m_method = (Methods)i;

    m_uri = cmds[1];
    m_versionMajor = cmds[2].Mid(4).AsUnsigned();
    m_versionMinor = cmds[2].Mid(cmds[2].Find('.')+1).AsUnsigned();
    m_info.MakeEmpty();
  }

  if (m_versionMajor < 2) {
    PTRACE(2, "SIP\tInvalid version (" << m_versionMajor << ") received on " << transport);
    return SIP_PDU::Failure_BadRequest;
  }

  // get the SDP content body
  // if a content length is specified, read that length
  // if no content length is specified (which is not the same as zero length)
  // then read until end of datagram or stream
  PINDEX contentLength = m_mime.GetContentLength();
  bool contentLengthPresent = m_mime.IsContentLengthPresent();

  if (!contentLengthPresent) {
    PTRACE(2, "SIP\tNo Content-Length present from " << transport << ", reading till end of datagram/stream.");
  }
  else if (contentLength < 0) {
    PTRACE(2, "SIP\tImpossible negative Content-Length from " << transport << ", reading till end of datagram/stream.");
    contentLengthPresent = false;
  }
  else if (contentLength > (transport.IsReliable() ? 1000000 : datagram.GetLength())) {
    PTRACE(2, "SIP\tImplausibly long Content-Length " << contentLength << " received from " << transport << ", reading to end of datagram/stream.");
    contentLengthPresent = false;
  }

  // Don't worry about body if was truncated packet
  if (!truncated) {
    if (contentLengthPresent) {
      if (contentLength > 0)
        stream->read(m_entityBody.GetPointer(contentLength+1), contentLength);
    }
    else {
      contentLength = 0;
      int c;
      while ((c = stream->get()) != EOF) {
        m_entityBody.SetMinSize((++contentLength/1000+1)*1000);
        m_entityBody += (char)c;
      }
    }

    m_entityBody[contentLength] = '\0';
  }

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTrace::Begin(3, __FILE__, __LINE__);

    trace << "SIP\t";
    if (truncated)
      trace << "Truncated (EMSGSIZE) ";
    trace << "PDU ";

    if (!PTrace::CanTrace(4)) {
      if (m_method != NumMethods)
        trace << MethodNames[m_method] << ' ' << m_uri;
      else
        trace << (unsigned)m_statusCode << ' ' << m_info;
      trace << ' ';
    }

    trace << "received: rem=" << transport.GetLastReceivedAddress()
          << ",local=" << transport.GetLocalAddress()
          << ",if=" << transport.GetLastReceivedInterface();

    if (PTrace::CanTrace(4)) {
      trace << '\n' << cmd << '\n' << setfill('\n') << m_mime << setfill(' ');
      for (const char * ptr = m_entityBody; *ptr != '\0'; ++ptr) {
        if (*ptr != '\r')
          trace << *ptr;
      }
    }
    if (truncated && contentLength > 0)
      trace << "... truncated";

    trace << PTrace::End;
  }
#endif

  return truncated ? SIP_PDU::Failure_MessageTooLarge : SIP_PDU::Successful_OK;
}


PBoolean SIP_PDU::Write(OpalTransport & transport, const OpalTransportAddress & remoteAddress, const PString & localInterface)
{
  PWaitAndSignal mutex(transport.GetWriteMutex());

  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to write PDU to closed transport " << transport);
    return PFalse;
  }

  OpalTransportAddress oldRemoteAddress = transport.GetRemoteAddress();
  if (!remoteAddress.IsEmpty() && !oldRemoteAddress.IsEquivalent(remoteAddress)) {
    if (!transport.SetRemoteAddress(remoteAddress)) {
      PTRACE(1, "SIP\tCannot use remote address " << remoteAddress << " for transport " << transport);
      return false;
    }
    PTRACE(4, "SIP\tSet new remote address " << remoteAddress << " for transport " << transport);
  }

  PString oldInterface = transport.GetInterface();
  if (!localInterface.IsEmpty() && oldInterface != localInterface) {
    if (!transport.SetInterface(localInterface)) {
      PTRACE(1, "SIP\tCannot use local interface \"" << localInterface << "\" for transport " << transport);
      return false;
    }
    PTRACE(4, "SIP\tSet new interface " << localInterface << " for transport " << transport);
  }

  m_mime.SetCompactForm(false);
  PString strPDU = Build();
  if (!transport.IsReliable() && strPDU.GetLength() > 1300) {
    PTRACE(4, "SIP\tPDU is too large (" << strPDU.GetLength() << " bytes) trying compact form.");
    m_mime.SetCompactForm(true);
    strPDU = Build();
    PTRACE_IF(2, strPDU.GetLength() > PluginCodec_RTP_MaxPacketSize,
              "SIP\tPDU is likely too large (" << strPDU.GetLength() << " bytes) for UDP datagram.");
  }

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTrace::Begin(3, __FILE__, __LINE__);

    trace << "SIP\tSending PDU ";

    if (!PTrace::CanTrace(4)) {
      if (m_method != NumMethods)
        trace << MethodNames[m_method] << ' ' << m_uri;
      else
        trace << (unsigned)m_statusCode << ' ' << m_info;
      trace << ' ';
    }

    trace << '(' << strPDU.GetLength() << " bytes) to: "
             "rem=" << transport.GetRemoteAddress() << ","
             "local=" << transport.GetLocalAddress() << ","
             "if=" << transport.GetInterface();

    if (PTrace::CanTrace(4)) {
      trace << '\n';
      for (const char * ptr = strPDU; *ptr != '\0'; ++ptr) {
        if (*ptr != '\r')
          trace << *ptr;
      }
    }

    trace << PTrace::End;
  }
#endif

  bool ok = transport.WriteString(strPDU);
  PTRACE_IF(1, !ok, "SIP\tPDU Write failed: " << transport.GetErrorText(PChannel::LastWriteError));

  transport.SetInterface(oldInterface);
  transport.SetRemoteAddress(oldRemoteAddress);

  return ok;
}


void  SIP_PDU::SetEntityBody()
{
  if (m_SDP == NULL)
    return;

  if (!m_entityBody.IsEmpty())
    return;

  m_entityBody = m_SDP->Encode();
  m_mime.SetContentType("application/sdp");
  m_mime.SetContentLength(m_entityBody.GetLength());
}


PString SIP_PDU::Build()
{
  PStringStream str;

  SetEntityBody();

  if (m_method != NumMethods)
    str << MethodNames[m_method] << ' ' << m_uri << ' ';

  str << "SIP/" << m_versionMajor << '.' << m_versionMinor;

  if (m_method == NumMethods) {
    if (m_info.IsEmpty())
      m_info = GetStatusCodeDescription(m_statusCode);
    str << ' ' << (unsigned)m_statusCode << ' ' << m_info;
  }

  str << "\r\n" << setfill('\r') << m_mime << m_entityBody;
  return str;
}


PString SIP_PDU::GetTransactionID() const
{
  if (m_transactionID.IsEmpty()) {
    /* RFC3261 Sections 8.1.1.7 & 17.1.3 transactions are identified by the
       branch paranmeter in the top most Via and CSeq. We do NOT include the
       CSeq in our id as we want the CANCEL messages directed at out
       transaction structure.
     */
    m_transactionID = SIPMIMEInfo::ExtractFieldParameter(m_mime.GetFirstVia(), "branch");
    if (m_transactionID.IsEmpty()) {
      PTRACE(2, "SIP\tTransaction " << m_mime.GetCSeq() << " has no branch parameter!");
      m_transactionID = m_mime.GetCallID() + m_mime.GetCSeq(); // Fail safe ...
    }
  }

  return m_transactionID;
}


SDPSessionDescription * SIP_PDU::GetSDP(const SIPConnection & connection)
{
  if (m_SDP == NULL && m_mime.GetContentType() == "application/sdp") {
    OpalMediaFormatList mediaFormats = connection.GetEndPoint().GetMediaFormats();
    connection.AdjustMediaFormats(false, mediaFormats, NULL);

    m_SDP = new SDPSessionDescription(0, 0, OpalTransportAddress());
    if (!m_SDP->Decode(m_entityBody, mediaFormats)) {
      delete m_SDP;
      m_SDP = NULL;
    }
  }

  return m_SDP;
}


void SIP_PDU::SetSDP(SDPSessionDescription * sdp)
{
  delete m_SDP;
  m_SDP = sdp;
}


////////////////////////////////////////////////////////////////////////////////////

SIPDialogContext::SIPDialogContext()
  : m_callId(SIPTransaction::GenerateCallID())
  , m_lastSentCSeq(0)
  , m_lastReceivedCSeq(0)
  , m_forking(false)
{
}


PString SIPDialogContext::AsString() const
{
  PStringStream str;
  SIPURL url = m_requestURI;
  url.SetParamVar("call-id",   m_callId);
  url.SetParamVar("local-uri", m_localURI.AsQuotedString());
  url.SetParamVar("remote-uri",m_remoteURI.AsQuotedString());
  url.SetParamVar("tx-cseq",   m_lastSentCSeq);
  url.SetParamVar("rx-cseq",   m_lastReceivedCSeq);

  unsigned index = 0;
  for (PStringList::const_iterator it = m_routeSet.begin(); it != m_routeSet.end(); ++it)
    url.SetParamVar(psprintf("route-set-%u", ++index), *it);

  return url.AsString();
}


bool SIPDialogContext::FromString(const PString & str)
{
  SIPURL url;
  if (!url.Parse(str))
    return false;

  m_requestURI = url;
  m_requestURI.SetParamVars(PStringToString());

  const PStringToString & params = url.GetParamVars();
  SetCallID(params("call-id"));
  SetLocalURI(params("local-uri"));
  SetRemoteURI(params("remote-uri"));
  m_lastSentCSeq = params("tx-cseq").AsUnsigned();
  m_lastReceivedCSeq = params("rx-cseq").AsUnsigned();

  PString route;
  unsigned index = 0;
  while (!(route = params(psprintf("route-set-%u", ++index))).IsEmpty())
    m_routeSet.AppendString(route);

  return IsEstablished();
}


static void SetWithTag(const SIPURL & url, SIPURL & uri, PString & tag, bool local)
{
  uri = url;

  PString newTag = url.GetParamVars()("tag");
  if (newTag.IsEmpty())
    newTag = SIPMIMEInfo::ExtractFieldParameter(uri.GetFieldParameters(), "tag");
  else
    uri.SetParamVar("tag", PString::Empty());

  if (!newTag.IsEmpty() && tag != newTag) {
    PTRACE(4, "SIP\tUpdating dialog tag from \"" << tag << "\" to \"" << newTag << '"');
    tag = newTag;
  }

  if (tag.IsEmpty() && local)
    tag = SIPURL::GenerateTag();

  if (!tag.IsEmpty())
    uri.SetFieldParameters("tag="+tag);

  uri.Sanitise(local ? SIPURL::FromURI : SIPURL::ToURI);
}


static bool SetWithTag(const PString & str, SIPURL & uri, PString & tag, bool local)
{
  SIPURL url;
  if (!url.Parse(str))
    return false;

  SetWithTag(url, uri, tag, local);
  return true;
}


void SIPDialogContext::SetLocalURI(const SIPURL & url)
{
  SetWithTag(url, m_localURI, m_localTag, true);
}


bool SIPDialogContext::SetLocalURI(const PString & uri)
{
  return SetWithTag(uri, m_localURI, m_localTag, true);
}


void SIPDialogContext::SetRemoteURI(const SIPURL & url)
{
  SetWithTag(url, m_remoteURI, m_remoteTag, false);
}


bool SIPDialogContext::SetRemoteURI(const PString & uri)
{
  return SetWithTag(uri, m_remoteURI, m_remoteTag, false);
}


void SIPDialogContext::SetProxy(const SIPURL & proxy, bool addToRouteSet)
{
  PTRACE_IF(3, !proxy.IsEmpty(), "SIP\tOutbound proxy for dialog set to " << proxy);

  m_proxy = proxy;

  // Default routeSet if there is a proxy
  if (addToRouteSet && m_routeSet.IsEmpty() && !proxy.IsEmpty()) {
    PStringStream str;
    str << "sip:" << proxy.GetHostName() << ':'  << proxy.GetPort() << ";lr";
    m_routeSet += str;
  }
}


void SIPDialogContext::Update(OpalTransport & transport, const SIP_PDU & pdu)
{
  const SIPMIMEInfo & mime = pdu.GetMIME();

  SetCallID(mime.GetCallID());

  if (m_routeSet.IsEmpty()) {
    // get the route set from the Record-Route response field according to 12.1.2
    // requests in a dialog do not modify the initial route set according to 12.2
    m_routeSet = mime.GetRecordRoute(pdu.GetMethod() == SIP_PDU::NumMethods);
  }

  /* Update request URI
     Do this only if no Route and no Record-Route header is set.
     Otherwise we will send SIP Messages to the proxy.
     Problems found if Contact contains transport=tcp.
     OPAL tries to switch to tcp even if there is an upp proxy between.
     TCP/UDP Problems conversions of a proxy are not solved with this work around.
     See RFC 5658 for a description of these problems.
  */
  if (m_requestURI.IsEmpty() || pdu.GetMethod() != SIP_PDU::NumMethods || pdu.GetStatusCode()/100 == 2) {
    PString contact = mime.GetContact();
    if (!contact.IsEmpty()) {
      m_requestURI.Parse(contact);
      PTRACE(4, "SIP\tSet Request URI to " << m_requestURI);
    }
  }

  /* Update the local/remote fields */
  if (pdu.GetMethod() == SIP_PDU::NumMethods) {
    SetRemoteURI(mime.GetTo());
    SetLocalURI(mime.GetFrom());
  }
  else {
    SetLocalURI(mime.GetTo()); // Will add a tag
    SetRemoteURI(mime.GetFrom());
  }

  if (pdu.GetMethod() != SIP_PDU::NumMethods) {
    PINDEX start, val, end;
    if (LocateFieldParameter(mime.GetFirstVia(), "rport", start, val, end) && val >= end)
      m_externalTransportAddress = transport.GetLastReceivedAddress();
  }
}


unsigned SIPDialogContext::GetNextCSeq()
{
  if (m_forking && m_lastSentCSeq > 0)
    return m_lastSentCSeq;

  return ++m_lastSentCSeq;
}


bool SIPDialogContext::IsDuplicateCSeq(unsigned requestCSeq)
{
  /* See if have had a mesage before and this one is older than the last,
     with a check for server bugs where the sequence number changes
     dramatically. if older then is a retransmission so return true and do
     not process it. */
  bool duplicate = m_lastReceivedCSeq != 0 && requestCSeq <= m_lastReceivedCSeq && (m_lastReceivedCSeq - requestCSeq) < 10;

  PTRACE_IF(4, m_lastReceivedCSeq == 0, "SIP\tDialog initial sequence number " << requestCSeq);
  PTRACE_IF(3, duplicate, "SIP\tReceived duplicate sequence number " << requestCSeq);
  PTRACE_IF(2, !duplicate && m_lastReceivedCSeq != 0 && requestCSeq != m_lastReceivedCSeq+1,
            "SIP\tReceived unexpected sequence number " << requestCSeq << ", expecting " << m_lastReceivedCSeq+1);

  m_lastReceivedCSeq = requestCSeq;

  return duplicate;
}


OpalTransportAddress SIPDialogContext::GetRemoteTransportAddress() const
{
  // In order of priority ...

  if (!m_externalTransportAddress.IsEmpty())
    return m_externalTransportAddress;

  OpalTransportAddress addr = m_proxy.GetHostAddress();
  if (!addr.IsEmpty())
    return addr;

  SIPURL uri;
  if (m_routeSet.IsEmpty())
    uri = m_requestURI;
  else
    uri = m_routeSet.front();


  uri.AdjustToDNS();
  return uri.GetHostAddress();
}


////////////////////////////////////////////////////////////////////////////////////

SIPTransaction::SIPTransaction(Methods method, SIPEndPoint & ep, OpalTransport & trans)
  : SIP_PDU(method)
  , m_endpoint(ep)
  , m_transport(trans)
  , m_retryTimeoutMin(ep.GetRetryTimeoutMin())
  , m_retryTimeoutMax(ep.GetRetryTimeoutMax())
  , m_state(NotStarted)
  , m_retry(1)
{
  m_retryTimer.SetNotifier(PCREATE_NOTIFIER(OnRetry));
  m_completionTimer.SetNotifier(PCREATE_NOTIFIER(OnTimeout));

  m_mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());

  PTRACE(4, "SIP\tTransaction created.");
}


SIPTransaction::SIPTransaction(Methods meth, SIPConnection & conn)
  : SIP_PDU(meth)
  , m_endpoint(conn.GetEndPoint())
  , m_transport(conn.GetTransport())
  , m_connection(&conn)
  , m_retryTimeoutMin(m_endpoint.GetRetryTimeoutMin())
  , m_retryTimeoutMax(m_endpoint.GetRetryTimeoutMax())
  , m_state(NotStarted)
  , m_retry(1)
  , m_remoteAddress(conn.GetDialog().GetRemoteTransportAddress())
{
  m_retryTimer.SetNotifier(PCREATE_NOTIFIER(OnRetry));
  m_completionTimer.SetNotifier(PCREATE_NOTIFIER(OnTimeout));

  InitialiseHeaders(conn, m_transport);
  m_mime.SetProductInfo(m_endpoint.GetUserAgent(), conn.GetProductInfo());

  PTRACE(4, "SIP\t" << meth << " transaction id=" << GetTransactionID() << " created.");
}


SIPTransaction::~SIPTransaction()
{
  PTRACE_IF(1, m_state < Terminated_Success, "SIP\tDestroying transaction id="
            << GetTransactionID() << " which is not yet terminated.");
  PTRACE(4, "SIP\tTransaction id=" << GetTransactionID() << " destroyed.");
}


void SIPTransaction::SetParameters(const SIPParameters & params)
{
  if (params.m_minRetryTime != PMaxTimeInterval)
    m_retryTimeoutMin = params.m_minRetryTime;
  if (params.m_maxRetryTime != PMaxTimeInterval)
    m_retryTimeoutMax = params.m_maxRetryTime;

  m_mime.SetExpires(params.m_expire);

  if (!params.m_contactAddress.IsEmpty())
    m_mime.SetContact(params.m_contactAddress);

  if (!params.m_proxyAddress.IsEmpty())
    SetRoute(SIPURL(params.m_proxyAddress));

  m_mime.AddMIME(params.m_mime);
}


PBoolean SIPTransaction::Start()
{
  if (m_state == Completed)
    return true;

  m_endpoint.AddTransaction(this);

  if (m_state != NotStarted) {
    PAssertAlways(PLogicError);
    return PFalse;
  }

  if (m_connection != NULL) {
    m_connection->m_pendingTransactions.Append(this);
    m_connection->OnStartTransaction(*this);

    if (m_connection->GetAuthenticator() != NULL) {
      SIPAuthenticator auth(*this);
      m_connection->GetAuthenticator()->Authorise(auth);
    }
  }

  PSafeLockReadWrite lock(*this);

  m_state = Trying;
  m_retry = 0;

  if (m_localInterface.IsEmpty())
    m_localInterface = m_transport.GetInterface();

  /* Get the address to which the request PDU should be sent, according to
     the RFC, for a request in a dialog. 
     handle case where address changed using rport
   */
  if (m_remoteAddress.IsEmpty()) {
    SIPURL destination;
    destination = m_uri;
    PStringList routeSet = GetMIME().GetRoute();
    if (!routeSet.IsEmpty()) {
      SIPURL firstRoute = routeSet.front();
      if (firstRoute.GetParamVars().Contains("lr"))
        destination = firstRoute;
    }

    // Do a DNS SRV lookup
    destination.AdjustToDNS();
    m_remoteAddress = destination.GetHostAddress();
  }

  PTRACE(3, "SIP\tTransaction remote address is " << m_remoteAddress);

  // Use the connection transport to send the request
  if (!Write(m_transport, m_remoteAddress, m_localInterface)) {
    SetTerminated(Terminated_TransportError);
    return PFalse;
  }

  m_retryTimer = m_retryTimeoutMin;
  if (m_method == Method_INVITE)
    m_completionTimer = m_endpoint.GetInviteTimeout();
  else
    m_completionTimer = m_endpoint.GetNonInviteTimeout();

  PTRACE(4, "SIP\tTransaction timers set: retry=" << m_retryTimer << ", completion=" << m_completionTimer);
  return true;
}


void SIPTransaction::WaitForCompletion()
{
  if (IsCompleted())
    return;

  if (m_state == NotStarted)
    Start();

  m_completed.Wait();
}


PBoolean SIPTransaction::Cancel()
{
  PSafeLockReadWrite lock(*this);

  if (m_state == NotStarted || m_state >= Cancelling) {
    PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " cannot be cancelled as in state " << m_state);
    return PFalse;
  }

  PTRACE(4, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " cancelled.");
  m_state = Cancelling;
  m_retry = 0;
  m_retryTimer = m_retryTimeoutMin;
  m_completionTimer = m_endpoint.GetPduCleanUpTimeout();
  return ResendCANCEL();
}


void SIPTransaction::Abort()
{
  PTRACE(4, "SIP\tAttempting to abort " << GetMethod() << " transaction id=" << GetTransactionID());

  if (LockReadWrite()) {
    if (!IsCompleted()) {
      SetTerminated(Terminated_Aborted);
    }
    UnlockReadWrite();
  }
}


bool SIPTransaction::SendPDU(SIP_PDU & pdu)
{
  if (pdu.Write(m_transport, m_remoteAddress, m_localInterface))
    return true;

  SetTerminated(Terminated_TransportError);
  return false;
}


bool SIPTransaction::ResendCANCEL()
{
  SIP_PDU cancel(Method_CANCEL);
  cancel.InitialiseHeaders(m_uri,
                           m_mime.GetTo(),
                           m_mime.GetFrom(),
                           m_mime.GetCallID(),
                           m_mime.GetCSeqIndex(),
                           m_mime.GetFirstVia()); // Use the topmost via header from the INVITE we cancel as per 9.1. 

  return SendPDU(cancel);
}


PBoolean SIPTransaction::OnReceivedResponse(SIP_PDU & response)
{
  // Stop the timers with asynchronous flag to avoid deadlock
  m_retryTimer.Stop(false);

  PString cseq = response.GetMIME().GetCSeq();

  /* If is the response to a CANCEL we sent, then we stop retransmissions
     and wait for the 487 Request Terminated to come in */
  if (cseq.Find(MethodNames[Method_CANCEL]) != P_MAX_INDEX) {
    m_completionTimer = m_endpoint.GetPduCleanUpTimeout();
    return PFalse;
  }

  // Something wrong here, response is not for the request we made!
  if (cseq.Find(MethodNames[m_method]) == P_MAX_INDEX) {
    PTRACE(2, "SIP\tTransaction " << cseq << " response not for " << *this);
    // Restart timer as haven't finished yet
    m_retryTimer = m_retryTimer.GetResetTime();
    m_completionTimer = m_completionTimer.GetResetTime();
    return PFalse;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return PFalse;

  /* Really need to check if response is actually meant for us. Have a
     temporary cheat in assuming that we are only sending a given CSeq to one
     and one only host, so anything coming back with that CSeq is OK. This has
     "issues" according to the spec but
     */
  if (IsInProgress()) {
    if (response.GetStatusCode()/100 == 1) {
      PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " proceeding.");

      if (m_state == Trying)
        m_state = Proceeding;

      m_retry = 0;
      m_retryTimer = m_retryTimeoutMax;

      int expiry = m_mime.GetExpires();
      if (expiry > 0)
        m_completionTimer.SetInterval(0, expiry);
      else if (m_method == Method_INVITE)
        m_completionTimer = m_endpoint.GetProgressTimeout();
      else
        m_completionTimer = m_endpoint.GetNonInviteTimeout();
    }
    else {
      PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " completed.");
      m_state = Completed;
      m_statusCode = response.GetStatusCode();
      m_completed.Signal();
    }

    if (m_connection != NULL)
      m_connection->OnReceivedResponse(*this, response);
    else
      m_endpoint.OnReceivedResponse(*this, response);

    if (m_state == Completed)
      OnCompleted(response);
  }
  else {
    PTRACE(4, "SIP\tIgnoring duplicate response to " << GetMethod() << " transaction id=" << GetTransactionID());
  }

  if (response.GetStatusCode() >= 200)
    m_completionTimer = m_endpoint.GetPduCleanUpTimeout();

  return true;
}


PBoolean SIPTransaction::OnCompleted(SIP_PDU & /*response*/)
{
  return true;
}


void SIPTransaction::OnRetry(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (!lock.IsLocked() || m_state > Cancelling || (m_state == Proceeding && m_method == Method_INVITE))
    return;

  m_retry++;

  if (m_retry >= m_endpoint.GetMaxRetries()) {
    SetTerminated(Terminated_RetriesExceeded);
    return;
  }

  if (m_state > Trying)
    m_retryTimer = m_retryTimeoutMax;
  else {
    PTimeInterval timeout = m_retryTimeoutMin*(1<<m_retry);
    if (timeout > m_retryTimeoutMax)
      timeout = m_retryTimeoutMax;
    m_retryTimer = timeout;
  }

  PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID()
         << " timeout, making retry " << m_retry << ", timeout " << m_retryTimer);

  if (m_state == Cancelling)
    ResendCANCEL();
  else
    SendPDU(*this);
}


void SIPTransaction::OnTimeout(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (lock.IsLocked()) {
    switch (m_state) {
      case Trying :
        // Sent initial command and got nothin'
        SetTerminated(Terminated_Timeout);
        break;

      case Proceeding :
        /* Got a 100 response and then nothing, give up with a CANCEL
           just in case the other side is still there, and in particular
           in the case of an INVITE where nobody answers */
        Cancel();
        break;

      case Cancelling :
        // We cancelled and finished waiting for retries
        SetTerminated(Terminated_Cancelled);
        break;

      case Completed :
        // We completed and finished waiting for retries
        SetTerminated(Terminated_Success);
        break;

      default :
        // Already terminated in some way
        break;
    }
  }
}


void SIPTransaction::SetTerminated(States newState)
{
  if (!PAssert(newState >= Terminated_Success, PInvalidParameter))
    return;

#if PTRACING
  static const char * const StateNames[NumStates] = {
    "NotStarted",
    "Trying",
    "Proceeding",
    "Cancelling",
    "Completed",
    "Terminated_Success",
    "Terminated_Timeout",
    "Terminated_RetriesExceeded",
    "Terminated_TransportError",
    "Terminated_Cancelled",
    "Terminated_Aborted"
  };
#endif

  // Terminated, so finished with timers
  m_retryTimer.Stop(false);
  m_completionTimer.Stop(false);

  if (m_connection != NULL)
    m_connection->m_pendingTransactions.Remove(this);

  if (m_state >= Terminated_Success) {
    PTRACE_IF(3, newState != Terminated_Success, "SIP\tTried to set state " << StateNames[newState] 
              << " for " << GetMethod() << " transaction id=" << GetTransactionID()
              << " but already terminated ( " << StateNames[m_state] << ')');
    return;
  }

  m_state = newState;
  PTRACE(3, "SIP\tSet state " << StateNames[newState] << " for "
         << GetMethod() << " transaction id=" << GetTransactionID());

  // Transaction failed, tell the endpoint
  if (m_state > Terminated_Success) {
    switch (m_state) {
      case Terminated_Timeout :
      case Terminated_RetriesExceeded:
        m_statusCode = SIP_PDU::Local_Timeout;
        break;

      case Terminated_TransportError :
        m_statusCode = SIP_PDU::Local_TransportError;
        break;

      case Terminated_Aborted :
      case Terminated_Cancelled :
        m_statusCode = SIP_PDU::Failure_RequestTerminated;
        break;

      default :// Prevent GNU warning
        break;
    }

    m_endpoint.OnTransactionFailed(*this);
    if (m_connection != NULL)
      m_connection->OnTransactionFailed(*this);
  }

  m_completed.Signal();
}


PString SIPTransaction::GenerateCallID()
{
  return PGloballyUniqueID().AsString() + '@' + PIPSocket::GetHostName();
}


////////////////////////////////////////////////////////////////////////////////////

SIPParameters::SIPParameters(const PString & aor, const PString & remote)
  : m_remoteAddress(remote)
  , m_addressOfRecord(aor)
  , m_expire(0)
  , m_restoreTime(30)
  , m_minRetryTime(PMaxTimeInterval)
  , m_maxRetryTime(PMaxTimeInterval)
  , m_userData(NULL)
{
}


void SIPParameters::Normalise(const PString & defaultUser, const PTimeInterval & defaultExpire)
{
  /* Try to be intelligent about what we got in the two fields target/remote,
     we have several scenarios depending on which is a partial or full URL.
   */

  SIPURL aor, server;
  PString possibleProxy;

  if (m_addressOfRecord.IsEmpty()) {
    if (m_remoteAddress.IsEmpty())
      aor = server = defaultUser + '@' + PIPSocket::GetHostName();
    else if (m_remoteAddress.Find('@') == P_MAX_INDEX)
      aor = server = defaultUser + '@' + m_remoteAddress;
    else
      aor = server = m_remoteAddress;
  }
  else if (m_addressOfRecord.Find('@') == P_MAX_INDEX) {
    if (m_remoteAddress.IsEmpty())
      aor = server = defaultUser + '@' + m_addressOfRecord;
    else if (m_remoteAddress.Find('@') == P_MAX_INDEX)
      aor = server = m_addressOfRecord + '@' + m_remoteAddress;
    else {
      server = m_remoteAddress;
      aor = m_addressOfRecord + '@' + server.GetHostName();
    }
  }
  else {
    aor = m_addressOfRecord;
    if (m_remoteAddress.IsEmpty())
      server = aor;
    else if (m_remoteAddress.Find('@') != P_MAX_INDEX)
      server = m_remoteAddress; // For third party registrations
    else {
      SIPURL remoteURL = m_remoteAddress;
      server = aor;
      if (!aor.GetHostAddress().IsEquivalent(remoteURL.GetHostAddress())) {
        /* Note this sets the proxy field because the user has given a full AOR
           with a domain for "user" and then specified a specific host name
           which as far as we are concered is the host to talk to. Setting the
           proxy will prevent SRV lookups or other things that might stop us
           from going to that very specific host.
         */
        possibleProxy = m_remoteAddress;
      }
    }
  }

  if (m_proxyAddress.IsEmpty())
    m_proxyAddress = server.GetParamVars()(OPAL_PROXY_PARAM, possibleProxy);
  if (!m_proxyAddress.IsEmpty())
    server.SetParamVar(OPAL_PROXY_PARAM, m_remoteAddress);

  if (!m_localAddress.IsEmpty()) {
    SIPURL local(m_localAddress);
    m_localAddress = local.AsString();
    aor.SetParamVar(OPAL_LOCAL_ID_PARAM, m_localAddress);
  }

  m_remoteAddress = server.AsString();
  m_addressOfRecord = aor.AsString();

  if (m_authID.IsEmpty())
    m_authID = aor.GetUserName();

  if (m_expire == 0)
    m_expire = defaultExpire.GetSeconds();
}


#if PTRACING
ostream & operator<<(ostream & strm, const SIPParameters & params)
{
  strm << "          aor=" << params.m_addressOfRecord << "\n"
          "       remote=" << params.m_remoteAddress << "\n"
          "        local=" << params.m_localAddress << "\n"
          "      contact=" << params.m_contactAddress << "\n"
          "       authID=" << params.m_authID << "\n"
          "        realm=" << params.m_realm << "\n"
          "       expire=" << params.m_expire << "\n"
          "      restore=" << params.m_restoreTime << "\n"
          "     minRetry=";
  if (params.m_minRetryTime != PMaxTimeInterval)
    strm << params.m_minRetryTime;
  else
    strm << "default";
  strm << "\n"
          "     maxRetry=";
  if (params.m_minRetryTime != PMaxTimeInterval)
    strm << params.m_maxRetryTime;
  else
    strm << "default";
  return strm;
}
#endif


////////////////////////////////////////////////////////////////////////////////////

SIPResponse::SIPResponse(SIPEndPoint & endpoint, StatusCodes code)
  : SIPTransaction(NumMethods, endpoint, *(OpalTransport *)NULL)
{
  m_statusCode = code;
}


SIPTransaction * SIPResponse::CreateDuplicate() const
{
  return new SIPResponse(*this);
}


bool SIPResponse::Send(OpalTransport & transport, const SIP_PDU & command)
{
  if (m_state == NotStarted) {
    InitialiseHeaders(command);

    m_endpoint.AddTransaction(this);

    m_state = Completed;
  }

  m_completionTimer = m_endpoint.GetPduCleanUpTimeout();
  PTRACE(4, "SIP\tResponse transaction timer set " << m_completionTimer);

  // Do not use internal m_transport as is dummy
  command.SendResponse(transport, *this, &m_endpoint);
  return true;
}


////////////////////////////////////////////////////////////////////////////////////

SIPInvite::SIPInvite(SIPConnection & connection, const OpalRTPSessionManager & sm)
  : SIPTransaction(Method_INVITE, connection)
  , m_rtpSessions(sm)
{
  //m_mime.SetDate(); // now
  SetAllow(m_endpoint.GetAllowedMethods());

  connection.OnCreatingINVITE(*this);
  
  if (m_SDP != NULL)
    m_SDP->SetSessionName(m_mime.GetUserAgent());
}


SIPTransaction * SIPInvite::CreateDuplicate() const
{
  SIPTransaction * newTransaction = new SIPInvite(*m_connection, GetSessionManager());

  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  // For Asterisk this is not merely SHOULD, but SHALL ....
  newTransaction->GetMIME().SetFrom(m_mime.GetFrom());
  return newTransaction;
}


PBoolean SIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  if (IsTerminated())
    return false;

  if (response.GetMIME().GetCSeq().Find(MethodNames[Method_INVITE]) != P_MAX_INDEX) {
    if (IsInProgress())
      m_connection->OnReceivedResponseToINVITE(*this, response);

    if (response.GetStatusCode() >= 200) {
      PSafeLockReadWrite lock(*this);
      if (!lock.IsLocked())
        return false;

      if (response.GetStatusCode() < 300) {
        // Need to update where the ACK goes to when have 2xx response as per 13.2.2.4
        if (!m_connection->LockReadOnly())
          return false;

        m_remoteAddress = m_connection->GetDialog().GetRemoteTransportAddress();
        m_connection->UnlockReadOnly();

        PTRACE(4, "SIP\tTransaction remote address changed to " << m_remoteAddress);
      }

      // ACK constructed following 13.2.2.4 or 17.1.1.3
      SIPAck ack(*this, response);
      if (!SendPDU(ack))
        return false;
    }
  }

  return SIPTransaction::OnReceivedResponse(response);
}


/////////////////////////////////////////////////////////////////////////

SIPAck::SIPAck(SIPTransaction & invite, SIP_PDU & response)
  : SIP_PDU(Method_ACK)
{
  if (response.GetStatusCode() < 300) {
    InitialiseHeaders(*invite.GetConnection(), invite.GetTransport());
    m_mime.SetCSeq(PString(invite.GetMIME().GetCSeqIndex()) & MethodNames[Method_ACK]);
  }
  else {
    InitialiseHeaders(invite.GetURI(),
                      response.GetMIME().GetTo(),
                      invite.GetMIME().GetFrom(),
                      invite.GetMIME().GetCallID(),
                      invite.GetMIME().GetCSeqIndex(),
                      CreateVia(invite.GetConnection()->GetEndPoint(), invite.GetTransport()));

    // Use the topmost via header from the INVITE we ACK as per 17.1.1.3
    // as well as the initial Route
    PStringList viaList;
    if (invite.GetMIME().GetViaList(viaList))
      m_mime.SetVia(viaList.front());

    if (invite.GetMIME().GetRoute().GetSize() > 0)
      m_mime.SetRoute(invite.GetMIME().GetRoute());
  }

  // Add authentication if had any on INVITE
  if (invite.GetMIME().Contains("Proxy-Authorization") || invite.GetMIME().Contains("Authorization")) {
    SIPAuthenticator auth(*this);
    invite.GetConnection()->GetAuthenticator()->Authorise(auth);
  }
}


SIPTransaction * SIPAck::CreateDuplicate() const
{
  return NULL;
}


/////////////////////////////////////////////////////////////////////////

SIPBye::SIPBye(SIPEndPoint & ep, OpalTransport & trans, SIPDialogContext dialog)
  : SIPTransaction(Method_BYE, ep, trans)
{
  InitialiseHeaders(dialog);
}


SIPBye::SIPBye(SIPConnection & conn)
  : SIPTransaction(Method_BYE, conn)
{
}


SIPTransaction * SIPBye::CreateDuplicate() const
{
  return new SIPBye(*m_connection);
}


////////////////////////////////////////////////////////////////////////////////////

SIPRegister::SIPRegister(SIPEndPoint & ep,
                         OpalTransport & trans,
                         const PString & id,
                         unsigned cseq,
                         const Params & params)
  : SIPTransaction(Method_REGISTER, ep, trans)
{
  InitialiseHeaders(params.m_registrarAddress,
                    params.m_addressOfRecord,
                    params.m_localAddress,
                    id,
                    cseq,
                    CreateVia(ep, trans));

  SetAllow(ep.GetAllowedMethods());

  SetParameters(params);
}


SIPTransaction * SIPRegister::CreateDuplicate() const
{
  return new SIPRegister(*this);
}


#if PTRACING
ostream & operator<<(ostream & strm, SIPRegister::CompatibilityModes mode)
{
  static const char * const Names[] = {
    "FullyCompliant",
    "CannotRegisterMultipleContacts",
    "CannotRegisterPrivateContacts"
  };
  if (mode < PARRAYSIZE(Names) && Names[mode] != NULL)
    strm << Names[mode];
  else
    strm << '<' << (unsigned)mode << '>';
  return strm;
}

ostream & operator<<(ostream & strm, const SIPRegister::Params & params)
{
  return strm << (const SIPParameters &)params << "\n"
                 "compatibility=" << params.m_compatibility;
}
#endif


////////////////////////////////////////////////////////////////////////////////////

static const char * const KnownEventPackage[SIPSubscribe::NumPredefinedPackages] = {
  "message-summary",
  "presence",
  "dialog;sla;ma", // sla is the old version ma is the new for Line Appearance extension
};

SIPSubscribe::EventPackage::EventPackage(PredefinedPackages pkg)
  : PCaselessString((pkg & PackageMask) < NumPredefinedPackages ? KnownEventPackage[(pkg & PackageMask)] : "")
{
  if ((pkg & Watcher) != 0)
    *this += ".winfo";
}


SIPSubscribe::EventPackage & SIPSubscribe::EventPackage::operator=(PredefinedPackages pkg)
{
  if ((pkg & PackageMask) < NumPredefinedPackages) {
    PCaselessString::operator=(KnownEventPackage[(pkg & PackageMask)]);
    if ((pkg & Watcher) != 0)
      *this += ".winfo";
  }
  return *this;
}


PObject::Comparison SIPSubscribe::EventPackage::InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const
{
  // Special rules for comparing event package strings, only up to the ';', if present

  PINDEX idx = 0;
  for (;;) {
    if (length-- == 0)
      return EqualTo;
    if (theArray[idx+offset] == '\0' && cstr[idx] == '\0')
      return EqualTo;
    if (theArray[idx+offset] == ';' || cstr[idx] == ';')
      break;
    Comparison c = PCaselessString::InternalCompare(idx+offset, cstr[idx]);
    if (c != EqualTo)
      return c;
    idx++;
  }

  const char * myIdPtr = strstr(theArray+idx+offset, "id");
  const char * theirIdPtr = strstr(cstr+idx, "id");
  if (myIdPtr == NULL && theirIdPtr == NULL)
    return EqualTo;

  if (myIdPtr == NULL)
    return LessThan;

  if (theirIdPtr == NULL)
    return GreaterThan;

  const char * myIdEnd = strchr(myIdPtr, ';');
  PINDEX myIdLen = myIdEnd != NULL ? myIdEnd - myIdPtr : strlen(myIdPtr);

  const char * theirIdEnd = strchr(theirIdPtr, ';');
  PINDEX theirIdLen = theirIdEnd != NULL ? theirIdEnd - theirIdPtr : strlen(theirIdPtr);

  if (myIdLen < theirIdLen)
    return LessThan;

  if (myIdLen > theirIdLen)
    return GreaterThan;

  return (Comparison)strncmp(myIdPtr, theirIdPtr, theirIdLen);
}


bool SIPSubscribe::EventPackage::IsWatcher() const
{
  static const char Suffix[] = ".winfo";
  return NumCompare(Suffix, sizeof(Suffix)-1, GetLength()-(sizeof(Suffix)-1)) == EqualTo;
}


SIPSubscribe::NotifyCallbackInfo::NotifyCallbackInfo(SIPEndPoint & ep, OpalTransport & trans, SIP_PDU & notify, SIP_PDU & response)
  : m_endpoint(ep)
  , m_transport(trans)
  , m_notify(notify)
  , m_response(response)
  , m_sendResponse(true)
{
}


bool SIPSubscribe::NotifyCallbackInfo::SendResponse(SIP_PDU::StatusCodes status, const char * extra)
{
  // See if already sent response, don't send another.
  if (!m_sendResponse)
    return true;

  m_response.SetStatusCode(status);

  if (extra != NULL)
    m_response.SetInfo(extra);

  if (!m_notify.SendResponse(m_transport, m_response, &m_endpoint))
    return false;

  m_sendResponse = false;
  return true;
}


SIPSubscribe::SIPSubscribe(SIPEndPoint & ep,
                           OpalTransport & trans,
                           SIPDialogContext & dialog,
                           const Params & params)
  : SIPTransaction(Method_SUBSCRIBE, ep, trans)
{
  InitialiseHeaders(dialog, CreateVia(ep, trans));

  // I have no idea why this is necessary, but it is the way OpenSIPS works ....
  if (params.m_eventPackage == SIPSubscribe::Dialog && params.m_contactAddress.IsEmpty())
    m_mime.SetContact(dialog.GetRemoteURI());

  m_mime.SetEvent(params.m_eventPackage);

  PString acceptableContentTypes = params.m_contentType;
  if (acceptableContentTypes.IsEmpty()) {
    SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(params.m_eventPackage);
    if (packageHandler != NULL) {
      acceptableContentTypes = packageHandler->GetContentType();
      delete packageHandler;
    }
  }

  if (params.m_eventList) {
    if (!acceptableContentTypes.IsEmpty())
      acceptableContentTypes += '\n';
    acceptableContentTypes += "multipart/related\napplication/rlmi+xml";
    m_mime.AddSupported("eventlist");
  }

  m_mime.SetAccept(acceptableContentTypes);

  SetAllow(ep.GetAllowedMethods());

  SetParameters(params);
}


SIPTransaction * SIPSubscribe::CreateDuplicate() const
{
  return new SIPSubscribe(*this);
}


#if PTRACING
ostream & operator<<(ostream & strm, const SIPSubscribe::Params & params)
{
  return strm << " eventPackage=" << params.m_eventPackage << '\n' << (const SIPParameters &)params;
}
#endif


////////////////////////////////////////////////////////////////////////////////////

SIPNotify::SIPNotify(SIPEndPoint & ep,
                     OpalTransport & trans,
                     SIPDialogContext & dialog,
                     const SIPEventPackage & eventPackage,
                     const PString & state,
                     const PString & body)
  : SIPTransaction(Method_NOTIFY, ep, trans)
{
  InitialiseHeaders(dialog, CreateVia(ep, trans));

  m_mime.SetEvent(eventPackage);
  m_mime.SetSubscriptionState(state);

  SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(eventPackage);
  if (packageHandler != NULL) {
    m_mime.SetContentType(packageHandler->GetContentType());
    delete packageHandler;
  }

  m_entityBody = body;

  ep.AdjustToRegistration(trans, *this);
}


SIPTransaction * SIPNotify::CreateDuplicate() const
{
  return new SIPNotify(*this);
}


////////////////////////////////////////////////////////////////////////////////////

SIPPublish::SIPPublish(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const PString & id,
                       const PString & sipIfMatch,
                       const SIPSubscribe::Params & params,
                       const PString & body)
  : SIPTransaction(Method_PUBLISH, ep, trans)
{
  SIPURL addr = params.m_addressOfRecord;
  InitialiseHeaders(addr, addr, addr, id, ep.GetNextCSeq(), CreateVia(ep, trans));

  if (!sipIfMatch.IsEmpty())
    m_mime.SetSIPIfMatch(sipIfMatch);

  m_mime.SetEvent(params.m_eventPackage);

  if (!body.IsEmpty()) {
    m_entityBody = body;

    if (!params.m_contentType.IsEmpty())
      m_mime.SetContentType(params.m_contentType);
    else {
      SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(params.m_eventPackage);
      if (packageHandler == NULL)
        m_mime.SetContentType("text/plain");
      else {
        m_mime.SetContentType(packageHandler->GetContentType());
        delete packageHandler;
      }
    }
  }

  SetParameters(params);
}


SIPTransaction * SIPPublish::CreateDuplicate() const
{
  return new SIPPublish(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPRefer::SIPRefer(SIPConnection & connection, const SIPURL & referTo, const SIPURL & referredBy)
  : SIPTransaction(Method_REFER, connection)
{
  m_mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());
  m_mime.SetReferTo(referTo.AsQuotedString());
  if(!referredBy.IsEmpty()) {
    SIPURL adjustedReferredBy = referredBy;
    adjustedReferredBy.Sanitise(SIPURL::RequestURI);
    m_mime.SetReferredBy(adjustedReferredBy.AsQuotedString());
  }
}


SIPTransaction * SIPRefer::CreateDuplicate() const
{
  return new SIPRefer(*m_connection, m_mime.GetReferTo(), m_mime.GetReferredBy());
}


/////////////////////////////////////////////////////////////////////////

SIPReferNotify::SIPReferNotify(SIPConnection & connection, StatusCodes code)
  : SIPTransaction(Method_NOTIFY, connection)
{
  m_mime.SetSubscriptionState(code < Successful_OK ? "active" : "terminated;reason=noresource");
  m_mime.SetEvent("refer");
  m_mime.SetContentType("message/sipfrag");

  PStringStream str;
  str << "SIP/" << m_versionMajor << '.' << m_versionMinor << ' ' << code;
  m_entityBody = str;
}


SIPTransaction * SIPReferNotify::CreateDuplicate() const
{
  return new SIPReferNotify(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPMessage::SIPMessage(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const Params & params)
  : SIPTransaction(Method_MESSAGE, ep, trans)
{
  Construct(params);
}


SIPMessage::SIPMessage(SIPConnection & conn, const Params & params)
  : SIPTransaction(Method_MESSAGE, conn)
{
  Construct(params);
}


void SIPMessage::Construct(const Params & params)
{
  SIPURL addr(params.m_remoteAddress);

  if (!params.m_localAddress.IsEmpty())
    m_localAddress = params.m_localAddress;
  else {
    if (!params.m_addressOfRecord.IsEmpty())
      m_localAddress = params.m_addressOfRecord;
    else
      m_localAddress = m_endpoint.GetRegisteredPartyName(addr, m_transport);
  }

  InitialiseHeaders(addr, addr, m_localAddress, params.m_id, m_endpoint.GetNextCSeq(), CreateVia(m_endpoint, m_transport));

  if (!params.m_contentType.IsEmpty()) {
    m_mime.SetContentType(params.m_contentType);
    m_entityBody = params.m_body;
  }

  SetParameters(params);
}


SIPTransaction * SIPMessage::CreateDuplicate() const
{
  return new SIPMessage(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPOptions::SIPOptions(SIPEndPoint & ep,
                     OpalTransport & trans,
                     const PString & id,
                      const Params & params)
  : SIPTransaction(Method_OPTIONS, ep, trans)
{
  // Build the correct From field
  SIPURL remoteAddress = params.m_remoteAddress;
  SIPURL localAddress = params.m_localAddress;
  if (localAddress.IsEmpty())
    localAddress = ep.GetRegisteredPartyName(remoteAddress.GetHostName(), trans);
  localAddress.SetTag();

  InitialiseHeaders(remoteAddress, remoteAddress, localAddress, id, ep.GetNextCSeq(), CreateVia(ep, trans));

  Construct(params);
}


SIPOptions::SIPOptions(SIPConnection & conn, const Params & params)
  : SIPTransaction(Method_OPTIONS, conn)
{
  Construct(params);
}


void SIPOptions::Construct(const Params & params)
{
  SetAllow(m_endpoint.GetAllowedMethods());
  m_mime.SetAccept(params.m_acceptContent);

  if (!params.m_contentType.IsEmpty()) {
    m_mime.SetContentType(params.m_contentType);
    m_entityBody = params.m_body;
  }

  SetParameters(params);
}


SIPTransaction * SIPOptions::CreateDuplicate() const
{
  return new SIPOptions(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPInfo::SIPInfo(SIPConnection & conn, const Params & params)
  : SIPTransaction(Method_INFO, conn)
{
  if (!params.m_contentType.IsEmpty()) {
    m_mime.SetContentType(params.m_contentType);
    m_entityBody = params.m_body;
  }
}


SIPTransaction * SIPInfo::CreateDuplicate() const
{
  return new SIPInfo(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPPing::SIPPing(SIPEndPoint & ep,
                 OpalTransport & trans,
                 const SIPURL & address)
  : SIPTransaction(Method_PING, ep, trans)
{
  InitialiseHeaders(address,
                    address,
                    "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                    GenerateCallID(),
                    ep.GetNextCSeq(),
                    CreateVia(ep, trans));
  // PING must not have a body
}


SIPTransaction * SIPPing::CreateDuplicate() const
{
  return new SIPPing(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPPrack::SIPPrack(SIPConnection & conn,
                   const PString & rack)
  : SIPTransaction(Method_PRACK, conn)
{
  m_mime.SetAt("RAck", rack);
}


SIPTransaction * SIPPrack::CreateDuplicate() const
{
  return new SIPPrack(*this);
}


#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////

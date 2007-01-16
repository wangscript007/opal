/*
 *
 * Inter Asterisk Exchange 2
 * 
 * The core routine which determines the processing of packets for one call.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 * 
 *  $Log: processor.h,v $
 *  Revision 1.13  2007/01/16 03:17:42  dereksmithies
 *  tidyup of comments. Remove unused variables.
 *  Guarantee that media frames are sent with a monotonically increasing timestamp
 *
 *  Revision 1.12  2007/01/11 03:02:15  dereksmithies
 *  Remove the previous audio buffering code, and switch to using the jitter
 *  buffer provided in Opal. Reduce the verbosity of the log mesasges.
 *
 *  Revision 1.11  2006/09/22 00:33:19  csoutheren
 *  Changed PAtomicInteger to BOOL
 *
 *  Revision 1.10  2006/09/11 03:08:51  dereksmithies
 *  Add fixes from Stephen Cook (sitiveni@gmail.com) for new patches to
 *  improve call handling. Notably, IAX2 call transfer. Many thanks.
 *  Thanks also to the Google summer of code for sponsoring this work.
 *
 *  Revision 1.9  2006/08/09 03:46:39  dereksmithies
 *  Add ability to register to a remote Asterisk box. The iaxProcessor class is split
 *  into a callProcessor and a regProcessor class.
 *  Big thanks to Stephen Cook, (sitiveni@gmail.com) for this work.
 * 
 *  Revision 1.8  2006/06/16 01:47:08  dereksmithies
 *  Get the OnHold features of IAX2 to work correctly.
 *  Thanks to Stephen Cook, (sitiveni@gmail.com) for this work.
 *
 *  Revision 1.7  2005/09/05 01:19:43  dereksmithies
 *  add patches from Adrian Sietsma to avoid multiple hangup packets at call end,
 *  and stop the sending of ping/lagrq packets at call end. Many thanks.
 *
 *  Revision 1.6  2005/08/26 03:07:38  dereksmithies
 *  Change naming convention, so all class names contain the string "IAX2"
 *
 *  Revision 1.5  2005/08/25 03:26:06  dereksmithies
 *  Add patch from Adrian Sietsma to correctly set the packet timestamps under windows.
 *  Many thanks.
 *
 *  Revision 1.4  2005/08/24 04:56:25  dereksmithies
 *  Add code from Adrian Sietsma to send FullFrameTexts and FullFrameDtmfs to
 *  the remote end.  Many Thanks.
 *
 *  Revision 1.3  2005/08/24 01:38:38  dereksmithies
 *  Add encryption, iax2 style. Numerous tidy ups. Use the label iax2, not iax
 *
 *  Revision 1.2  2005/08/04 08:14:17  rjongbloed
 *  Fixed Windows build under DevStudio 2003 of IAX2 code
 *
 *  Revision 1.1  2005/07/30 07:01:32  csoutheren
 *  Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 *  Thanks to Derek Smithies of Indranet Technologies Ltd. for
 *  writing and contributing this code
 *
 *
 *
 *
 *
 *
 *
 */

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <ptlib.h>
#include <opal/connection.h>

#include <iax2/frame.h>
#include <iax2/iedata.h>
#include <iax2/remote.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

class IAX2EndPoint;
class IAX2Connection;
class IAX2ThreadHelper;

////////////////////////////////////////////////////////////////////////////////
/**This class defines what the processor is to do on receiving an ack
   for a full frame.  Thus, the processor sends a full frame (say
   Accept) and defines an action (using this class) to carry out on
   receiving the ack for the Accept.

   Essentially, this class provides us a means of "knowing" the other
   end has acted on the sent full frame. The action to do could be as
   simple as saying, "other end has accepted our Accept packet"

   It also helps us to move on in the call setup phase. On receipt of
   one particular ack packet, we send the next packet in the call
   process.  Thus, on receipt of a ack for a timestamp of X and oseqno
   of Y, we send this packet.
   
   The outgoing IAX2FullFrame has an inSeqNo, which must match the
   outSeqNo of the received ack frame.
   
   The sub classes if appropriate will need to request a call number from the
   endpoint.  This class will automatically check if the source call number
   has been set and if so then will release that number from the endpoint. 
*/
class IAX2WaitingForAck : public PObject
{
  PCLASSINFO(IAX2WaitingForAck, PObject);

 public:
  /**The action to do on receiving the specified ack */
  enum ResponseToAck {
    RingingAcked   = 0,  /*!< Processing acknowledgment to Remote end is ringing  message */ 
    AcceptAcked    = 1,  /*!< Processing acknowledgment to accept  message                */ 
    AuthRepAcked   = 2,  /*!< Processing acknowledgment to AuthRep message                */ 
    AnswerAcked    = 3   /*!< Processing acknowledgment to Answer message                 */ 
  };
  
  /**Construct this response message with values that cannot match an incoming ack packet*/
  IAX2WaitingForAck();
  
  /**Assign this response to wait for the specified coords */
  void Assign(IAX2FullFrame *f, ResponseToAck _response);
  
  /**Return true if the supplied ack frame matches the internal coordinates */
  BOOL MatchingAckPacket(IAX2FullFrame *f);
  
  /**Report the response to carry out */
  ResponseToAck GetResponse() { return response; }
  
  /**Report the internal response code as a string */
  PString GetResponseAsString() const;
  
  /**Pretty print this response to the designated stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Initialise this to no response, and never to match */
  void ZeroValues();
  
 private:
  /**Timestamp of the ack packet we are looking for */
  DWORD timeStamp;
  
  /**OutSeqNo of the ack packet we are looking for */
  PINDEX seqNo;
  
  /**Do this action on finding a matching ACK packet */
  ResponseToAck response;
};

////////////////////////////////////////////////////////////////////////////////
/** This class is an abstract base class for iax2 processors.  This class is
    responsible for handling all the iax2 protocol command messages.
 
    It provides the base structure for two different processor classes,
    a)registration type commands and b)commands specific to one call.
 
    The unique source and destination call number (which are in the IAX2
    frames) are used to determine which processor will handle which incoming
    packet.
 
    Each processor runs in its own thread so as to process incoming packets in
    a timely fashion.
 */
class IAX2Processor : public PThread
{
  PCLASSINFO(IAX2Processor, PObject);
  
 public:
  /**Construct this class */
  IAX2Processor(IAX2EndPoint & ep);

  /**Destructor */
  virtual ~IAX2Processor();
  
  /**Get the sequence number info (inSeqNo and outSeqNo) */
  IAX2SequenceNumbers & GetSequenceInfo() { return sequence; }
  
  /**Get the IAX2 encryption info */
  IAX2Encryption & GetEncryptionInfo() { return encryption; }
  
  /**Handle a received IAX2 frame. This may be a mini frame or full frame */
  void IncomingEthernetFrame (IAX2Frame *frame);
  
  /**A method to cause some of the values in this class to be formatted
     into a printable stream */
  virtual void PrintOn(ostream & strm) const = 0;
  
  /**Access the endpoint class that launched this processor */
  IAX2EndPoint & GetEndPoint() { return endpoint; };
  
   /**Give the call token a value. The call token is the ipaddress of
     the remote node concatented with the remote nodes src
     number. This is guaranteed to be unique.  Sadly, if this
     connection is setting up the call, the callToken is not known
     until receipt of the first packet from the remote node.

     However, if this connection is created in response to a call,
     this connection can determine the callToken on examination of
     that incoming first packet */
  void SetCallToken(const PString & newToken);
  
  /**Return the string that identifies this IAX2Connection instance */
  PString GetCallToken();
  
  /**Get information on IAX2Remote class (remote node address & port + source & dest call number.) */
  IAX2Remote & GetRemoteInfo() { return remote; }
  
  /**Get the call start tick */
  const PTimeInterval & GetCallStartTick() { return callStartTick; }
  
  /**The worker method of this thread. In here, all incoming frames (for this call)
     are handled.
  */
  void Main();
  
  /**Test to see if it is a status query type IAX2 frame (eg lagrq) and handle it. If the frame
     is a status query, and it is handled, return TRUE */
  BOOL IsStatusQueryEthernetFrame(IAX2Frame *frame);
  
  /**Set the flag to indicate if we are handling specialPackets (those
     packets which are not sent to any particular call) */
  void SetSpecialPackets(BOOL newValue) { specialPackets = newValue; }
  
  /**Cause this thread to die immediately */
  void Terminate();
  
  /**Cause this thread to come to life, and process events that are
   * pending at IAX2Connection */
  void Activate();

 protected:
 
  /**Reference to the global variable of this program */
  IAX2EndPoint      & endpoint;
 
  /**Time this connection class was created, which is the call start
     time.  It is reported in Ticks, which is required for millisecond
     accuracy under windows.   */
  PTimeInterval callStartTick;
  
  /**Details on the address of the remote endpoint, and source/dest call numbers */
  IAX2Remote remote;
  
  /** Set the acceptable time (in milliseconds) to wait before giving
      up on this call */
  void StartNoResponseTimer(PINDEX msToWait = 60000);
   
  /** Stop the timer - we have received a reply */
  void StopNoResponseTimer() { noResponseTimer.Stop(); }
  
  /** The timer which is used to test for no reply to our outgoing call setup messages */
  PTimer noResponseTimer;
  
  /**Activate this thread to process all the lists of queued frames */
  void CleanPendingLists() { activate.Signal(); }
  
  /**Action to perform on receiving an ACK packet (which is required
     during call setup phase for receiver */
  IAX2WaitingForAck nextTask;
  
   /**Flag which is used to activate this thread, so all pending tasks/packets are processed */
  PSyncPoint activate;
  
  /**Flag to indicate, end this thread */
  BOOL endThread;
  
  /**Status of encryption for this processor - by default, no encryption */
  IAX2Encryption encryption;
  
  /**Details on the in/out sequence numbers */
  IAX2SequenceNumbers sequence;
  
  /**Array of frames read from the Receiver for this call */
  IAX2FrameList frameList;
  
  /**The call token, which uniquely identifies this IAX2CallProcessor, and the
     associated call */
  SafeString callToken;
  
  /**A threaded pure threaded callback for the sub classes of processor */
  virtual void OnNoResponseTimeout() = 0;
  
  /** A defined value which is the maximum time we will wait for an answer to
     our packets */
  enum DefinedNoResponseTimePeriod {
    NoResponseTimePeriod = 5000 /*!< Time (in milliseconds) we will wait */
  };
  
  /**return the flag to indicate if we are handling special packets,
     which are those packets sent to the endpoint (and not related to
     any particular call). */
  BOOL      IsHandlingSpecialPackets() { return specialPackets; };

  /**Flag to indicate we are handing the special packets, which are
     sent to the endpoint,and not related to any particular call. */
  BOOL       specialPackets;
  
  /**Go through the three lists for incoming data (ethernet/sound/UI
     commands.  */
  virtual void ProcessLists() = 0;  
  
  /**A pure virtual method that is implemented by sub classes to
     process an incoming full frame*/
  virtual void ProcessFullFrame(IAX2FullFrame & fullFrame) = 0;
  
  /**Attempt to process a common frame ie: LagRq, LagRp, Vnak, Ping or Pong.*/
  BOOL ProcessCommonNetworkFrame(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Initial
     message, used to measure the round trip time  */
  virtual void ProcessIaxCmdLagRq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Reply to
     cmdLagrq, which tells us the round trip time  */
  virtual void ProcessIaxCmdLagRp(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is If we
     receive voice before valid first voice frame, send this  */
  virtual void ProcessIaxCmdVnak(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Ping
     request,  */
  virtual void ProcessIaxCmdPing(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is reply to
     a Ping  */
  virtual void ProcessIaxCmdPong(IAX2FullFrameProtocol *src); 
  
  /**remove one frame on the incoming ethernet frame list. If there
     may be more to process, return TRUE. If there are no more to
     process, return FALSE. */
  BOOL ProcessOneIncomingEthernetFrame();
  
  /**Count of the number of control frames sent */
  PAtomicInteger controlFramesSent;
  
  /**Count of the number of control frames received */
  PAtomicInteger controlFramesRcvd;
  
  /**Increment the count of full frames sent*/
  void IncControlFramesSent() { ++controlFramesSent; }
  
  /**Increment the count of full frames received*/
  void IncControlFramesRcvd() { ++controlFramesRcvd; }
  
  /**A pure virtual method that is implmented by to process 
   * an incoming network frame of type  IAX2MiniFrame */
  virtual void ProcessNetworkFrame(IAX2MiniFrame * src) = 0;  
  
  /**Transmit IAX2Frame to remote endpoint, and then increment send
     count. This calls a method in the Transmitter class. .It is only called
     by the this IAX2CallProcessor class.  */
  void TransmitFrameToRemoteEndpoint(IAX2Frame *src);

  /**Transmit IAX2Frame to remote endpoint, and then increment send
     count. This calls a method in the Transmitter class. .It is only
     called by the this IAX2CallProcessor class. The second parameter
     determines what to do when an ack frame is received for the sent
     frame.  */
  void TransmitFrameToRemoteEndpoint(IAX2FullFrame *src,
             IAX2WaitingForAck::ResponseToAck response  ///<action to do on getting Ack
             );

  /**Transmit IAX2Frame to remote endpoint,. This calls a method in the
     Transmitter class. .It is only called by the this Connection
     class. There is no stats change when this method is called. */
  void TransmitFrameNow(IAX2Frame *src);
  
  /**FullFrameProtocol class needs to have the IE's correctly appended prior to transmission */
  void TransmitFrameToRemoteEndpoint(IAX2FullFrameProtocol *src);
  
  /** Do the md5/rsa authentication. Return True if successful. Has the side
      effect of appending the appropriate Ie class to the "reply" parameter.*/
  BOOL Authenticate(IAX2FullFrameProtocol *reply, /*!< this frame contains the result of authenticating the internal data*/
                    PString & password /*!< the password to authenticate with */
        );
        
  /**Hold each of the possible values from an Ie class */
  IAX2IeData ieData;
  
  /**Transmit an IAX2 protocol frame with subclass type ack immediately to
     remote endpoint */
  void SendAckFrame(IAX2FullFrame *inReplyTo);
  
  /**Transmit an unsupported frame to the remote endpoint*/
  void SendUnsupportedFrame(IAX2FullFrame *inReplyTo);
  
 private:
#ifdef DOC_PLUS_PLUS
  /** pwlib constructs to cope with no response to an outgoing
      message. This is used to handle the output of the
      noResponseTimer
       
  This method runs in a separate thread to the heart of the
  Connection.  It is threadsafe, cause each of the elements in
  Connection (that are touched) are thread safe */
  void OnNoResponseTimeoutStart(PTimer &, INT);
#else
  PDECLARE_NOTIFIER(PTimer, IAX2Processor, OnNoResponseTimeoutStart);
#endif 

 protected:
  /**The timestamp we will put on the next mini frame out of here 

     This timestamp is monotonically increasing, and bears "some" relation to
     actuality.  We generate the timestamp uniformly - this instance of an
     iax2 call could be driven from a slightly non uniform packet source. */

  DWORD currentSoundTimeStamp;
};

#endif // PROCESSOR_H

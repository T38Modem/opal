/*
 * handlers.h
 *
 * Session Initiation Protocol endpoint.
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
 * The Initial Developer of the Original Code is Damien Sandras. 
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_SIP_HANDLERS_H
#define OPAL_SIP_HANDLERS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_SIP

#include <opal/pres_ent.h>
#include <opal/connection.h>
#include <sip/sippdu.h>


/* Class to handle SIP REGISTER, SUBSCRIBE, MESSAGE, and renew
 * the 'bindings' before they expire.
 */
class SIPHandler : public PSafeObject 
{
  PCLASSINFO(SIPHandler, PSafeObject);

protected:
  SIPHandler(
    SIP_PDU::Methods method,
    SIPEndPoint & ep,
    const SIPParameters & params
  );

public:
  ~SIPHandler();

  virtual Comparison Compare(const PObject & other) const;

  virtual bool ShutDown();

  enum State {
    Subscribed,       // The registration is active
    Subscribing,      // The registration is in process
    Unavailable,      // The registration is offline and still being attempted
    Refreshing,       // The registration is being refreshed
    Restoring,        // The registration is trying to be restored after being offline
    Unsubscribing,    // The unregistration is in process
    Unsubscribed,     // The registrating is inactive
    NumStates
  };

  void SetState (SIPHandler::State s);

  inline SIPHandler::State GetState () 
  { return m_state; }

  virtual OpalTransport * GetTransport();

  virtual SIPAuthentication * GetAuthentication()
  { return m_authentication; }

  virtual const SIPURL & GetAddressOfRecord()
    { return m_addressOfRecord; }

  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);

  virtual void SetExpire(int e);

  virtual int GetExpire()
    { return m_currentExpireTime; }

  virtual const PString & GetCallID() const
    { return m_callID; }

  virtual void SetBody(const PString & /*body*/) { }

  virtual bool IsDuplicateCSeq(unsigned ) { return false; }

  virtual SIPTransaction * CreateTransaction(OpalTransport & t) = 0;

  SIP_PDU::Methods GetMethod() const { return m_method; }
  virtual SIPSubscribe::EventPackage GetEventPackage() const { return SIPEventPackage(); }

  virtual void OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedIntervalTooBrief(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedTemporarilyUnavailable(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedForbidden(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnTransactionFailed(SIPTransaction & transaction);

  virtual void OnFailed(const SIP_PDU & response);
  virtual void OnFailed(SIP_PDU::StatusCodes);

  bool ActivateState(SIPHandler::State state);
  virtual bool SendNotify(const PObject * /*body*/) { return false; }

  SIPEndPoint & GetEndPoint() const { return endpoint; }

  const OpalProductInfo & GetProductInfo() const { return m_productInfo; }

  const PString & GetUsername() const     { return m_username; }
  const PString & GetPassword() const     { return m_password; }
  const PString & GetRealm() const        { return m_realm; }
  const SIPURL & GetRemoteAddress() const { return m_remoteAddress; }
  const SIPURL & GetProxy() const         { return m_proxy; }

  SIPMIMEInfo m_mime;

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
  void RetryLater(unsigned after);
  PDECLARE_NOTIFIER(PTimer, SIPHandler, OnExpireTimeout);
  static PBoolean WriteSIPHandler(OpalTransport & transport, void * info);
  virtual bool WriteSIPHandler(OpalTransport & transport, bool forked);

  SIPEndPoint               & endpoint;

  SIPAuthentication         * m_authentication;
  unsigned                    m_authenticateErrors;
  PString                     m_username;
  PString                     m_password;
  PString                     m_realm;

  PSafeList<SIPTransaction>   m_transactions;
  OpalTransport             * m_transport;

  SIP_PDU::Methods            m_method;
  SIPURL                      m_addressOfRecord;
  SIPURL                      m_remoteAddress;
  PString                     m_callID;
  unsigned                    m_lastCseq;
  int                         m_currentExpireTime;
  int                         m_originalExpireTime;
  int                         m_offlineExpireTime;
  State                       m_state;
  queue<State>                m_stateQueue;
  bool                        m_receivedResponse;
  PTimer                      m_expireTimer; 
  SIPURL                      m_proxy;
  OpalProductInfo             m_productInfo;
  bool                        m_retry403;

  // Keep a copy of the keys used for easy removal on destruction
  typedef std::map<PString, PSafePtr<SIPHandler> > IndexMap;
  std::pair<IndexMap::iterator, bool> m_byCallID;
  std::pair<IndexMap::iterator, bool> m_byAorAndPackage;
  std::pair<IndexMap::iterator, bool> m_byAuthIdAndRealm;
  std::pair<IndexMap::iterator, bool> m_byAorUserAndRealm;

  friend class SIPHandlersList;
};

#if PTRACING
ostream & operator<<(ostream & strm, SIPHandler::State state);
#endif


class SIPRegisterHandler : public SIPHandler
{
  PCLASSINFO(SIPRegisterHandler, SIPHandler);

public:
  SIPRegisterHandler(
    SIPEndPoint & ep,
    const SIPRegister::Params & params
  );

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);

  virtual void OnFailed(SIP_PDU::StatusCodes r);

  void UpdateParameters(const SIPRegister::Params & params);

  const SIPRegister::Params & GetParams() const { return m_parameters; }

  const SIPURLList & GetContacts() const { return m_contactAddresses; }
  const SIPURLList & GetServiceRoute() const { return m_serviceRoute; }

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
  void SendStatus(SIP_PDU::StatusCodes code, State state);

  SIPRegister::Params  m_parameters;
  unsigned             m_sequenceNumber;
  SIPURLList           m_contactAddresses;
  SIPURLList           m_serviceRoute;
  OpalTransportAddress m_externalAddress;
};


class SIPSubscribeHandler : public SIPHandler
{
  PCLASSINFO(SIPSubscribeHandler, SIPHandler);
public:
  SIPSubscribeHandler(SIPEndPoint & ep, const SIPSubscribe::Params & params);
  ~SIPSubscribeHandler();

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);
  virtual void OnFailed(const SIP_PDU & response);
  virtual SIPEventPackage GetEventPackage() const
    { return m_parameters.m_eventPackage; }

  void UpdateParameters(const SIPSubscribe::Params & params);

  virtual bool IsDuplicateCSeq(unsigned sequenceNumber) { return m_dialog.IsDuplicateCSeq(sequenceNumber); }

  const SIPSubscribe::Params & GetParams() const { return m_parameters; }

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
  virtual bool WriteSIPHandler(OpalTransport & transport, bool forked);
  void SendStatus(SIP_PDU::StatusCodes code, State state);
  bool DispatchNOTIFY(SIP_PDU & request, SIP_PDU & response);

  SIPSubscribe::Params     m_parameters;
  SIPDialogContext         m_dialog;
  bool                     m_unconfirmed;
  SIPEventPackageHandler * m_packageHandler;

  SIP_PDU                  * m_previousResponse;
};


class SIPNotifyHandler : public SIPHandler
{
  PCLASSINFO(SIPNotifyHandler, SIPHandler);
public:
  SIPNotifyHandler(
    SIPEndPoint & ep,
    const PString & targetAddress,
    const SIPEventPackage & eventPackage,
    const SIPDialogContext & dialog
  );
  ~SIPNotifyHandler();

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual SIPEventPackage GetEventPackage() const
    { return m_eventPackage; }

  virtual void SetBody(const PString & body) { m_body = body; }

  virtual bool IsDuplicateCSeq(unsigned sequenceNumber) { return m_dialog.IsDuplicateCSeq(sequenceNumber); }
  virtual bool SendNotify(const PObject * body);

  enum Reasons {
    Deactivated,
    Probation,
    Rejected,
    Timeout,
    GiveUp,
    NoResource
  };

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
  virtual bool WriteSIPHandler(OpalTransport & transport, bool forked);

  SIPEventPackage          m_eventPackage;
  SIPDialogContext         m_dialog;
  Reasons                  m_reason;
  SIPEventPackageHandler * m_packageHandler;
  PString                  m_body;
};


class SIPPublishHandler : public SIPHandler
{
  PCLASSINFO(SIPPublishHandler, SIPHandler);

public:
  SIPPublishHandler(SIPEndPoint & ep, 
                    const SIPSubscribe::Params & params,
                    const PString & body);

  virtual void SetBody(const PString & body) { m_body = body; }

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual SIPEventPackage GetEventPackage() const { return m_parameters.m_eventPackage; }

protected:
  SIPSubscribe::Params m_parameters;
  PString              m_body;
  PString              m_sipETag;
};


class SIPMessageHandler : public SIPHandler
{
  PCLASSINFO(SIPMessageHandler, SIPHandler);
public:
  SIPMessageHandler(SIPEndPoint & ep, const SIPMessage::Params & params);

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnFailed(SIP_PDU::StatusCodes);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);

  void UpdateParameters(const SIPMessage::Params & params);

protected:
  SIPMessage::Params m_parameters;
};


class SIPOptionsHandler : public SIPHandler
{
  PCLASSINFO(SIPOptionsHandler, SIPHandler);
public:
  SIPOptionsHandler(SIPEndPoint & ep, const SIPOptions::Params & params);

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnFailed(SIP_PDU::StatusCodes);
  virtual void OnFailed(const SIP_PDU & response);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);

protected:
  SIPOptions::Params m_parameters;
};


class SIPPingHandler : public SIPHandler
{
  PCLASSINFO(SIPPingHandler, SIPHandler);
public:
  SIPPingHandler(SIPEndPoint & ep, const PURL & to);
  virtual SIPTransaction * CreateTransaction (OpalTransport &);
};


/** This dictionary is used both to contain the active and successful
 * registrations, and subscriptions. 
 */
class SIPHandlersList
{
  public:
    /** Append a new handler to the list
      */
    void Append(SIPHandler * handler);

    /** Remove a handler from the list.
        Handler is not immediately deleted but marked for deletion later by
        DeleteObjectsToBeRemoved() when all references are done with the handler.
      */
    void Remove(SIPHandler * handler);

    /** Update indexes for handler in the list
      */
    void Update(SIPHandler * handler);

    /** Clean up lists of handler.
      */
    bool DeleteObjectsToBeRemoved()
      { return m_handlersList.DeleteObjectsToBeRemoved(); }

    /** Get the first handler in the list. Further enumeration may be done by
        the ++operator on the safe pointer.
     */
    PSafePtr<SIPHandler> GetFirstHandler(PSafetyMode mode = PSafeReference) const
      { return PSafePtr<SIPHandler>(m_handlersList, mode); }

    /**
     * Return the number of registered accounts
     */
    unsigned GetCount(SIP_PDU::Methods meth, const PString & eventPackage = PString::Empty()) const;

    /**
     * Return a list of the active address of records for each handler.
     */
    PStringList GetAddresses(bool includeOffline, SIP_PDU::Methods meth, const PString & eventPackage = PString::Empty()) const;

    /**
     * Find the SIPHandler object with the specified callID
     */
    PSafePtr<SIPHandler> FindSIPHandlerByCallID(const PString & callID, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified authRealm
     */
    PSafePtr<SIPHandler> FindSIPHandlerByAuthRealm(const PString & authRealm, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified authRealm & user
     */
    PSafePtr<SIPHandler> FindSIPHandlerByAuthRealm(const PString & authRealm, const PString & userName, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified URL. The url is
     * the registration address, for example, 6001@sip.seconix.com
     * when registering 6001 to sip.seconix.com with realm seconix.com
     * or 6001@seconix.com when registering 6001@seconix.com to
     * sip.seconix.com
     */
    PSafePtr<SIPHandler> FindSIPHandlerByUrl(const PURL & url, SIP_PDU::Methods meth, PSafetyMode m);
    PSafePtr<SIPHandler> FindSIPHandlerByUrl(const PURL & url, SIP_PDU::Methods meth, const PString & eventPackage, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified registration host.
     * For example, in the above case, the name parameter
     * could be "sip.seconix.com" or "seconix.com".
     */
    PSafePtr <SIPHandler> FindSIPHandlerByDomain(const PString & name, SIP_PDU::Methods meth, PSafetyMode m);

  protected:
    void RemoveIndexes(SIPHandler * handler);

    PMutex m_extraMutex;
    PSafeList<SIPHandler> m_handlersList;

    typedef SIPHandler::IndexMap IndexMap;
    PSafePtr<SIPHandler> FindBy(IndexMap & by, const PString & key, PSafetyMode m);

    IndexMap m_byCallID;
    IndexMap m_byAorAndPackage;
    IndexMap m_byAuthIdAndRealm;
    IndexMap m_byAorUserAndRealm;
};


/** Information for SIP "presence" event package notification messages.
  */
class SIPPresenceInfo : public OpalPresenceInfo
{
public:
  SIPPresenceInfo(
    State state = Unchanged
  );

  // basic presence defined by RFC 3863
  PString m_tupleId;
  PString m_contact;

  // presence extensions defined by RFC 4480
  PStringArray m_activities;  // list of activities, seperated by newline

  // presence agent
  PString m_presenceAgent;

  PString AsXML() const;

#if P_EXPAT
  static bool ParseXML(
    const PString & body,
    list<SIPPresenceInfo> & info,
    PString & error
  );
#endif

  void PrintOn(ostream & strm) const;
  friend ostream & operator<<(ostream & strm, const SIPPresenceInfo & info) { info.PrintOn(strm); return strm; }

  static State FromSIPActivityString(const PString & str);

  static bool AsSIPActivityString(State state, PString & str);
  bool AsSIPActivityString(PString & str) const;

  PString m_personId;
};


/** Information for SIP "dialog" event package notification messages.
  */
struct SIPDialogNotification : public PObject
{
  PCLASSINFO(SIPDialogNotification, PObject);

  enum States {
    Terminated,
    Trying,
    Proceeding,
    Early,
    Confirmed,

    FirstState = Terminated,
    LastState = Confirmed
  };
  friend States operator++(States & state) { return (state = (States)(state+1)); }
  friend States operator--(States & state) { return (state = (States)(state-1)); }
  static PString GetStateName(States state);
  PString GetStateName() const { return GetStateName(m_state); }

  enum Events {
    NoEvent = -1,
    Cancelled,
    Rejected,
    Replaced,
    LocalBye,
    RemoteBye,
    Error,
    Timeout,

    FirstEvent = Cancelled,
    LastEvent = Timeout
  };
  friend Events operator++(Events & evt) { return (evt = (Events)(evt+1)); }
  friend Events operator--(Events & evt) { return (evt = (Events)(evt-1)); }
  static PString GetEventName(Events state);
  PString GetEventName() const { return GetEventName(m_eventType); }

  enum Rendering {
    RenderingUnknown = -1,
    NotRenderingMedia,
    RenderingMedia
  };

  PString  m_entity;
  PString  m_dialogId;
  PString  m_callId;
  bool     m_initiator;
  States   m_state;
  Events   m_eventType;
  unsigned m_eventCode;
  struct Participant {
    Participant() : m_appearance(-1), m_byeless(false), m_rendering(RenderingUnknown) { }
    PString   m_URI;
    PString   m_dialogTag;
    PString   m_identity;
    PString   m_display;
    int       m_appearance;
    bool      m_byeless;
    Rendering m_rendering;
  } m_local, m_remote;

  SIPDialogNotification(const PString & entity = PString::Empty());

  void PrintOn(ostream & strm) const;
};


#endif // OPAL_SIP

#endif // OPAL_SIP_HANDLERS_H

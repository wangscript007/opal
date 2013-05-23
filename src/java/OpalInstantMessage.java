/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalInstantMessage {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalInstantMessage(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalInstantMessage obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalInstantMessage(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setM_from(String value) {
    OPALJNI.OpalInstantMessage_m_from_set(swigCPtr, this, value);
  }

  public String getM_from() {
    return OPALJNI.OpalInstantMessage_m_from_get(swigCPtr, this);
  }

  public void setM_to(String value) {
    OPALJNI.OpalInstantMessage_m_to_set(swigCPtr, this, value);
  }

  public String getM_to() {
    return OPALJNI.OpalInstantMessage_m_to_get(swigCPtr, this);
  }

  public void setM_host(String value) {
    OPALJNI.OpalInstantMessage_m_host_set(swigCPtr, this, value);
  }

  public String getM_host() {
    return OPALJNI.OpalInstantMessage_m_host_get(swigCPtr, this);
  }

  public void setM_conversationId(String value) {
    OPALJNI.OpalInstantMessage_m_conversationId_set(swigCPtr, this, value);
  }

  public String getM_conversationId() {
    return OPALJNI.OpalInstantMessage_m_conversationId_get(swigCPtr, this);
  }

  public void setM_textBody(String value) {
    OPALJNI.OpalInstantMessage_m_textBody_set(swigCPtr, this, value);
  }

  public String getM_textBody() {
    return OPALJNI.OpalInstantMessage_m_textBody_get(swigCPtr, this);
  }

  public void setM_bodyCount(long value) {
    OPALJNI.OpalInstantMessage_m_bodyCount_set(swigCPtr, this, value);
  }

  public long getM_bodyCount() {
    return OPALJNI.OpalInstantMessage_m_bodyCount_get(swigCPtr, this);
  }

  public void setM_mimeType(SWIGTYPE_p_p_char value) {
    OPALJNI.OpalInstantMessage_m_mimeType_set(swigCPtr, this, SWIGTYPE_p_p_char.getCPtr(value));
  }

  public SWIGTYPE_p_p_char getM_mimeType() {
    long cPtr = OPALJNI.OpalInstantMessage_m_mimeType_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_p_char(cPtr, false);
  }

  public void setM_bodies(SWIGTYPE_p_p_char value) {
    OPALJNI.OpalInstantMessage_m_bodies_set(swigCPtr, this, SWIGTYPE_p_p_char.getCPtr(value));
  }

  public SWIGTYPE_p_p_char getM_bodies() {
    long cPtr = OPALJNI.OpalInstantMessage_m_bodies_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_p_char(cPtr, false);
  }

  public void setM_messageId(long value) {
    OPALJNI.OpalInstantMessage_m_messageId_set(swigCPtr, this, value);
  }

  public long getM_messageId() {
    return OPALJNI.OpalInstantMessage_m_messageId_get(swigCPtr, this);
  }

  public OpalInstantMessage() {
    this(OPALJNI.new_OpalInstantMessage(), true);
  }

}
/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip;

public class OpalParamRecording {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalParamRecording(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalParamRecording obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      exampleJNI.delete_OpalParamRecording(swigCPtr);
    }
    swigCPtr = 0;
  }

  public void setM_callToken(String value) {
    exampleJNI.OpalParamRecording_m_callToken_set(swigCPtr, this, value);
  }

  public String getM_callToken() {
    return exampleJNI.OpalParamRecording_m_callToken_get(swigCPtr, this);
  }

  public void setM_file(String value) {
    exampleJNI.OpalParamRecording_m_file_set(swigCPtr, this, value);
  }

  public String getM_file() {
    return exampleJNI.OpalParamRecording_m_file_get(swigCPtr, this);
  }

  public OpalParamRecording() {
    this(exampleJNI.new_OpalParamRecording(), true);
  }

}

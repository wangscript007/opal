/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package -I/home/robertj/opal/include;

public class OpalStatusMediaStream {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalStatusMediaStream(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalStatusMediaStream obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      exampleJNI.delete_OpalStatusMediaStream(swigCPtr);
    }
    swigCPtr = 0;
  }

  public void setM_callToken(String value) {
    exampleJNI.OpalStatusMediaStream_m_callToken_set(swigCPtr, this, value);
  }

  public String getM_callToken() {
    return exampleJNI.OpalStatusMediaStream_m_callToken_get(swigCPtr, this);
  }

  public void setM_identifier(String value) {
    exampleJNI.OpalStatusMediaStream_m_identifier_set(swigCPtr, this, value);
  }

  public String getM_identifier() {
    return exampleJNI.OpalStatusMediaStream_m_identifier_get(swigCPtr, this);
  }

  public void setM_type(String value) {
    exampleJNI.OpalStatusMediaStream_m_type_set(swigCPtr, this, value);
  }

  public String getM_type() {
    return exampleJNI.OpalStatusMediaStream_m_type_get(swigCPtr, this);
  }

  public void setM_format(String value) {
    exampleJNI.OpalStatusMediaStream_m_format_set(swigCPtr, this, value);
  }

  public String getM_format() {
    return exampleJNI.OpalStatusMediaStream_m_format_get(swigCPtr, this);
  }

  public void setM_state(OpalMediaStates value) {
    exampleJNI.OpalStatusMediaStream_m_state_set(swigCPtr, this, value.swigValue());
  }

  public OpalMediaStates getM_state() {
    return OpalMediaStates.swigToEnum(exampleJNI.OpalStatusMediaStream_m_state_get(swigCPtr, this));
  }

  public OpalStatusMediaStream() {
    this(exampleJNI.new_OpalStatusMediaStream(), true);
  }

}

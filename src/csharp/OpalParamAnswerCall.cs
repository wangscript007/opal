//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.7
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class OpalParamAnswerCall : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalParamAnswerCall(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(OpalParamAnswerCall obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalParamAnswerCall() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalParamAnswerCall(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public string callToken {
    set {
      OPALPINVOKE.OpalParamAnswerCall_callToken_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamAnswerCall_callToken_get(swigCPtr);
      return ret;
    } 
  }

  public OpalParamProtocol overrides {
    set {
      OPALPINVOKE.OpalParamAnswerCall_overrides_set(swigCPtr, OpalParamProtocol.getCPtr(value));
    } 
    get {
      global::System.IntPtr cPtr = OPALPINVOKE.OpalParamAnswerCall_overrides_get(swigCPtr);
      OpalParamProtocol ret = (cPtr == global::System.IntPtr.Zero) ? null : new OpalParamProtocol(cPtr, false);
      return ret;
    } 
  }

  public uint withMedia {
    set {
      OPALPINVOKE.OpalParamAnswerCall_withMedia_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalParamAnswerCall_withMedia_get(swigCPtr);
      return ret;
    } 
  }

  public OpalParamAnswerCall() : this(OPALPINVOKE.new_OpalParamAnswerCall(), true) {
  }

}
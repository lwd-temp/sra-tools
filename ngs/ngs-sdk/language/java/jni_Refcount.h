/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class ngs_itf_Refcount */

#ifndef _Included_ngs_itf_Refcount
#define _Included_ngs_itf_Refcount
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     ngs_itf_Refcount
 * Method:    Duplicate
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_Refcount_Duplicate
  (JNIEnv *, jobject, jlong);

/*
 * Class:     ngs_itf_Refcount
 * Method:    Release
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_ngs_itf_Refcount_Release
  (JNIEnv *, jobject, jlong);

/*
 * Class:     ngs_itf_Refcount
 * Method:    ReleaseRef
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_ngs_itf_Refcount_ReleaseRef
  (JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif
#endif

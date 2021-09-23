/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class ngs_itf_ReadCollectionItf */

#ifndef _Included_ngs_itf_ReadCollectionItf
#define _Included_ngs_itf_ReadCollectionItf
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetName
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ngs_itf_ReadCollectionItf_GetName
  (JNIEnv *, jobject, jlong);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetReadGroups
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetReadGroups
  (JNIEnv *, jobject, jlong);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    HasReadGroup
 * Signature: (JLjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_ngs_itf_ReadCollectionItf_HasReadGroup
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetReadGroup
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetReadGroup
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetReferences
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetReferences
  (JNIEnv *, jobject, jlong);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    HasReference
 * Signature: (JLjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_ngs_itf_ReadCollectionItf_HasReference
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetReference
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetReference
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetAlignment
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetAlignment
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetAlignments
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetAlignments
  (JNIEnv *, jobject, jlong, jint);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetAlignmentCount
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetAlignmentCount
  (JNIEnv *, jobject, jlong, jint);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetAlignmentRange
 * Signature: (JJJI)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetAlignmentRange
  (JNIEnv *, jobject, jlong, jlong, jlong, jint);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetRead
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetRead
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetReads
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetReads
  (JNIEnv *, jobject, jlong, jint);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetReadCount
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetReadCount
  (JNIEnv *, jobject, jlong, jint);

/*
 * Class:     ngs_itf_ReadCollectionItf
 * Method:    GetReadRange
 * Signature: (JJJI)J
 */
JNIEXPORT jlong JNICALL Java_ngs_itf_ReadCollectionItf_GetReadRange
  (JNIEnv *, jobject, jlong, jlong, jlong, jint);

#ifdef __cplusplus
}
#endif
#endif

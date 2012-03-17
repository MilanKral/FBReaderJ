/*
 * Copyright (C) 2004-2012 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <AndroidUtil.h>
#include <ZLUnicodeUtil.h>

#include "JavaEncodingConverter.h"

class JavaEncodingConverter : public ZLEncodingConverter {

private:
	JavaEncodingConverter(const std::string &encoding);

public:
	~JavaEncodingConverter();
	void convert(std::string &dst, const char *srcStart, const char *srcEnd);
	void reset();
	bool fillTable(int *map);

private:
	jobject myJavaConverter;
	jbyteArray myInBuffer;
	jbyteArray myOutBuffer;

friend class JavaEncodingConverterProvider;
};

bool JavaEncodingConverterProvider::providesConverter(const std::string &encoding) {
	if (encoding.empty()) {
		return false;
	}
	JNIEnv *env = AndroidUtil::getEnv();
	jclass cls = env->FindClass(AndroidUtil::Class_JavaEncodingCollection);
	jobject collection = env->CallStaticObjectMethod(cls, AndroidUtil::SMID_JavaEncodingCollection_Instance);
	jboolean result = env->CallBooleanMethod(collection, AndroidUtil::MID_JavaEncodingCollection_isEncodingSupported);
	env->DeleteLocalRef(collection);
	env->DeleteLocalRef(cls);
	return result != 0;
}

shared_ptr<ZLEncodingConverter> JavaEncodingConverterProvider::createConverter(const std::string &encoding) {
	return new JavaEncodingConverter(encoding);
}

JavaEncodingConverter::JavaEncodingConverter(const std::string &encoding) {
	JNIEnv *env = AndroidUtil::getEnv();
	jclass cls = env->FindClass(AndroidUtil::Class_JavaEncodingCollection);
	jobject collection = env->CallStaticObjectMethod(cls, AndroidUtil::SMID_JavaEncodingCollection_Instance);
	jstring encodingName = AndroidUtil::createJavaString(env, encoding);
	jobject javaEncoding = env->CallObjectMethod(collection, AndroidUtil::MID_JavaEncodingCollection_getEncoding_String, encodingName);
	myJavaConverter = env->CallObjectMethod(javaEncoding, AndroidUtil::MID_Encoding_createConverter);
	env->DeleteLocalRef(javaEncoding);
	env->DeleteLocalRef(encodingName);
	env->DeleteLocalRef(collection);
	env->DeleteLocalRef(cls);

	myInBuffer = env->NewByteArray(32768);
	myOutBuffer = env->NewByteArray(65536);
}

JavaEncodingConverter::~JavaEncodingConverter() {
	JNIEnv *env = AndroidUtil::getEnv();
	env->DeleteLocalRef(myOutBuffer);
	env->DeleteLocalRef(myInBuffer);
	env->DeleteLocalRef(myJavaConverter);
}

void JavaEncodingConverter::convert(std::string &dst, const char *srcStart, const char *srcEnd) {
	JNIEnv *env = AndroidUtil::getEnv();
	const int srcLen = srcEnd - srcStart;
	env->SetByteArrayRegion(myInBuffer, 0, srcLen, (jbyte*)srcStart);
	const jint dstLen = env->CallIntMethod(myJavaConverter, AndroidUtil::MID_EncodingConverter_convert, myInBuffer, 0, srcLen, myOutBuffer, 0);
	const int origLen = dst.size();
	dst.append(dstLen, '\0');
	env->GetByteArrayRegion(myOutBuffer, 0, dstLen, (jbyte*)dst.data() + origLen);
}

void JavaEncodingConverter::reset() {
	AndroidUtil::getEnv()->CallVoidMethod(myJavaConverter, AndroidUtil::MID_EncodingConverter_reset);
}

bool JavaEncodingConverter::fillTable(int *map) {
	// TODO: implement
	return false;
}

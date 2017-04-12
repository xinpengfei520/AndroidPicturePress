#include "android_backport_webp_WebPFactory.h"
#include "android_backport_webp.h"

#include <stdio.h>
#include <string.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <webp/decode.h>
#include <webp/encode.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     android_backport_webp_WebPFactory
 * Method:    nativeDecodeByteArray
 * Signature: ([BLandroid/graphics/BitmapFactory/Options;)Landroid/graphics/Bitmap;
 */
JNIEXPORT jobject JNICALL Java_android_backport_webp_WebPFactory_nativeDecodeByteArray
  (JNIEnv *jniEnv, jclass, jbyteArray byteArray, jobject options)
{
	// Check if input is valid
	if(!byteArray)
	{
		jniEnv->ThrowNew(jrefs::java::lang::NullPointerException->jclassRef, "Input buffer can not be null");
		return 0;
	}

	// Log what version of WebP is used
	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Using WebP Decoder %08x", WebPGetDecoderVersion());

	// Lock buffer
	jbyte* inputBuffer = jniEnv->GetByteArrayElements(byteArray, NULL);
	size_t inputBufferLen = jniEnv->GetArrayLength(byteArray);

	// Validate image
	int bitmapWidth = 0;
	int bitmapHeight = 0;
	if(!WebPGetInfo((uint8_t*)inputBuffer, inputBufferLen, &bitmapWidth, &bitmapHeight))
	{
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Invalid WebP format");
		return 0;
	}

	// Check if size is all what we were requested to do
	if(options && jniEnv->GetBooleanField(options, jrefs::android::graphics::BitmapFactory->Options.inJustDecodeBounds) == JNI_TRUE)
	{
		// Set values
		jniEnv->SetIntField(options, jrefs::android::graphics::BitmapFactory->Options.outWidth, bitmapWidth);
		jniEnv->SetIntField(options, jrefs::android::graphics::BitmapFactory->Options.outHeight, bitmapHeight);

		// Unlock buffer
		jniEnv->ReleaseByteArrayElements(byteArray, inputBuffer, JNI_ABORT);

		return 0;
	}
	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Decoding %dx%d bitmap", bitmapWidth, bitmapHeight);

	// Create bitmap
	jobject value__ARGB_8888 = jniEnv->GetStaticObjectField(jrefs::android::graphics::Bitmap->Config.jclassRef, jrefs::android::graphics::Bitmap->Config.ARGB_8888);
	jobject outputBitmap = jniEnv->CallStaticObjectMethod(jrefs::android::graphics::Bitmap->jclassRef, jrefs::android::graphics::Bitmap->createBitmap,
		(jint)bitmapWidth, (jint)bitmapHeight,
		value__ARGB_8888);
	if(!outputBitmap)
	{
		jniEnv->ReleaseByteArrayElements(byteArray, inputBuffer, JNI_ABORT);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to allocate Bitmap");
		return 0;
	}
	outputBitmap = jniEnv->NewLocalRef(outputBitmap);

	// Get information about bitmap passed
	AndroidBitmapInfo bitmapInfo;
	if(AndroidBitmap_getInfo(jniEnv, outputBitmap, &bitmapInfo) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
		jniEnv->ReleaseByteArrayElements(byteArray, inputBuffer, JNI_ABORT);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to get Bitmap information");
		return 0;
	}

	// Lock pixels
	void* bitmapPixels = 0;
	if(AndroidBitmap_lockPixels(jniEnv, outputBitmap, &bitmapPixels) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
		jniEnv->ReleaseByteArrayElements(byteArray, inputBuffer, JNI_ABORT);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to lock Bitmap pixels");
		return 0;
	}

	// Decode to ARGB
	if(!WebPDecodeRGBAInto((uint8_t*)inputBuffer, inputBufferLen, (uint8_t*)bitmapPixels, bitmapInfo.height * bitmapInfo.stride, bitmapInfo.stride))
	{
		AndroidBitmap_unlockPixels(jniEnv, outputBitmap);
		jniEnv->ReleaseByteArrayElements(byteArray, inputBuffer, JNI_ABORT);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to unlock Bitmap pixels");
		return 0;
	}

	// pre-multiply the alpha value into the r-g-b channel of each pixel
	uint8_t* pixelsAddr = (uint8_t*)bitmapPixels;
	int index = 0;
	float normalizedAlpha = 1;
	int alpha = 255;
	int end = bitmapInfo.height * bitmapInfo.stride - 4;
	while(index < end)
	{
		alpha = pixelsAddr[index+3];
		normalizedAlpha = alpha / 255.0;
		pixelsAddr[index] = (uint8_t)(pixelsAddr[index]  * normalizedAlpha);
		pixelsAddr[index+1] = (uint8_t)(pixelsAddr[index+1]  * normalizedAlpha);
		pixelsAddr[index+2] = (uint8_t)(pixelsAddr[index+2]  * normalizedAlpha);
		index += 4;
	}

	// Unlock pixels
	if(AndroidBitmap_unlockPixels(jniEnv, outputBitmap) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
		jniEnv->ReleaseByteArrayElements(byteArray, inputBuffer, JNI_ABORT);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to unlock Bitmap pixels");
		return 0;
	}

	// Unlock buffer
	jniEnv->ReleaseByteArrayElements(byteArray, inputBuffer, JNI_ABORT);

	return outputBitmap;
}

/*
 * Class:     android_backport_webp_WebPFactory
 * Method:    nativeDecodeFile
 * Signature: (Ljava/lang/String;Landroid/graphics/BitmapFactory/Options;)Landroid/graphics/Bitmap;
 */
JNIEXPORT jobject JNICALL Java_android_backport_webp_WebPFactory_nativeDecodeFile
  (JNIEnv *jniEnv, jclass, jstring path, jobject options)
{
	// Check if input is valid
	if(!path)
	{
		jniEnv->ThrowNew(jrefs::java::lang::NullPointerException->jclassRef, "path can not be null");
		return 0;
	}

	// Log what version of WebP is used
	//__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Using WebP Decoder %08x", WebPGetDecoderVersion());

    char *inputBuffer;
    size_t inputBufferLen;
    const char* filePath = jniEnv->GetStringUTFChars(path, 0);
    FILE *file = NULL;
    file = fopen(filePath, "rb");
    jniEnv->ReleaseStringUTFChars(path, filePath);
    if(file)
    {
		fseek(file, 0, SEEK_END);
		long file_len = ftell(file);
		fseek(file, 0, SEEK_SET);
        inputBuffer = (char *) malloc(file_len * sizeof(char));
		if (inputBuffer == NULL)
		{
			fclose(file);
			jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "malloc error");
			return 0;
		}
        inputBufferLen = fread(inputBuffer, sizeof(char), file_len, file);
        if (inputBufferLen != file_len)
        {
            free(inputBuffer);
			fclose(file);
			jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Read file error");
			return 0;
        }
		fclose(file);
    } else {
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Can not open file");
		return 0;
	}

	// Validate image
	int bitmapWidth = 0;
	int bitmapHeight = 0;
	if(!WebPGetInfo((uint8_t*)inputBuffer, inputBufferLen, &bitmapWidth, &bitmapHeight))
	{
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Invalid WebP format");
		return 0;
	}

	// Check if size is all what we were requested to do
	if(options && jniEnv->GetBooleanField(options, jrefs::android::graphics::BitmapFactory->Options.inJustDecodeBounds) == JNI_TRUE)
	{
		// Set values
		jniEnv->SetIntField(options, jrefs::android::graphics::BitmapFactory->Options.outWidth, bitmapWidth);
		jniEnv->SetIntField(options, jrefs::android::graphics::BitmapFactory->Options.outHeight, bitmapHeight);

		// Release buffer
        free(inputBuffer);

		return 0;
	}
	//__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Decoding %dx%d bitmap", bitmapWidth, bitmapHeight);

	// Create bitmap
	jobject value__ARGB_8888 = jniEnv->GetStaticObjectField(jrefs::android::graphics::Bitmap->Config.jclassRef, jrefs::android::graphics::Bitmap->Config.ARGB_8888);
	jobject outputBitmap = jniEnv->CallStaticObjectMethod(jrefs::android::graphics::Bitmap->jclassRef, jrefs::android::graphics::Bitmap->createBitmap,
														  (jint)bitmapWidth, (jint)bitmapHeight,
														  value__ARGB_8888);
	if(!outputBitmap)
	{
        free(inputBuffer);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to allocate Bitmap");
		return 0;
	}
	outputBitmap = jniEnv->NewLocalRef(outputBitmap);

	// Get information about bitmap passed
	AndroidBitmapInfo bitmapInfo;
	if(AndroidBitmap_getInfo(jniEnv, outputBitmap, &bitmapInfo) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
        free(inputBuffer);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to get Bitmap information");
		return 0;
	}

	// Lock pixels
	void* bitmapPixels = 0;
	if(AndroidBitmap_lockPixels(jniEnv, outputBitmap, &bitmapPixels) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
        free(inputBuffer);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to lock Bitmap pixels");
		return 0;
	}

	// Decode to ARGB
	if(!WebPDecodeRGBAInto((uint8_t*)inputBuffer, inputBufferLen, (uint8_t*)bitmapPixels, bitmapInfo.height * bitmapInfo.stride, bitmapInfo.stride))
	{
		AndroidBitmap_unlockPixels(jniEnv, outputBitmap);
        free(inputBuffer);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to unlock Bitmap pixels");
		return 0;
	}

	// Unlock pixels
	if(AndroidBitmap_unlockPixels(jniEnv, outputBitmap) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
        free(inputBuffer);
		jniEnv->DeleteLocalRef(outputBitmap);
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to unlock Bitmap pixels");
		return 0;
	}

	// Release buffer
    free(inputBuffer);

	return outputBitmap;
}

#include "skia_portions.h"

typedef void (*ScanlineImporter)(const uint8_t* in, uint8_t* out, int width);

static void ARGB_8888_To_RGBA(const uint8_t* in, uint8_t* rgba, int width)
{
	const uint32_t* src = (const uint32_t*)in;
	for (int i = 0; i < width; ++i)
	{
		const uint32_t c = *src++;
		rgba[0] = SkGetPackedR32(c);
		rgba[1] = SkGetPackedG32(c);
		rgba[2] = SkGetPackedB32(c);
		rgba[3] = SkGetPackedA32(c);
		rgba += 4;
	}
}

static void RGB_565_To_RGB(const uint8_t* in, uint8_t* rgb, int width)
{
	const uint16_t* src = (const uint16_t*)in;
	for (int i = 0; i < width; ++i)
	{
		const uint16_t c = *src++;
		rgb[0] = SkPacked16ToR32(c);
		rgb[1] = SkPacked16ToG32(c);
		rgb[2] = SkPacked16ToB32(c);
		rgb += 3;
	}
}

static ScanlineImporter ChooseImporter(int config)
{
	switch (config) {
	case ANDROID_BITMAP_FORMAT_RGBA_8888:
		return ARGB_8888_To_RGBA;
	case ANDROID_BITMAP_FORMAT_RGB_565:
		return RGB_565_To_RGB;
	default:
		return NULL;
	}
}

static uint32_t GetDestinationScanlinePixelByteSize(int config) 
{
	switch (config) {
	case ANDROID_BITMAP_FORMAT_RGBA_8888:
		return 4;
	case ANDROID_BITMAP_FORMAT_RGB_565:
		return 3;
	default:
		return 0;
	}
}

/*
 * Class:     android_backport_webp_WebPFactory
 * Method:    nativeEncodeBitmap
 * Signature: (Landroid/graphics/Bitmap;I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_android_backport_webp_WebPFactory_nativeEncodeBitmap
  (JNIEnv * jniEnv, jclass, jobject bitmap, jint quality)
{
	// Check if input is valid
	if(!bitmap)
	{
		jniEnv->ThrowNew(jrefs::java::lang::NullPointerException->jclassRef, "Bitmap can not be null");
		return 0;
	}

	// Get information about bitmap passed
	AndroidBitmapInfo bitmapInfo;
	if(AndroidBitmap_getInfo(jniEnv, bitmap, &bitmapInfo) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to get Bitmap information");
		return 0;
	}

	// Check for format
	if(bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888 && bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGB_565)
	{
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Unsupported Bitmap configuration. Currently only RGBA_8888 and RGB_565 are supported");
		return 0;
	}
	
	// Log what version of WebP is used
	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Using WebP Encoder %08x", WebPGetEncoderVersion());

	// Lock pixels
	void* bitmapPixels = 0;
	if(AndroidBitmap_lockPixels(jniEnv, bitmap, &bitmapPixels) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to lock Bitmap pixels");
		return 0;
	}

	// Convert color space
	const uint8_t* src = (uint8_t*)bitmapPixels;
	const uint32_t src_stride = bitmapInfo.stride;
	const uint32_t dst_stride = GetDestinationScanlinePixelByteSize(bitmapInfo.format) * bitmapInfo.width;
	const ScanlineImporter scanline_import = ChooseImporter(bitmapInfo.format);

	uint8_t* dst = new uint8_t[dst_stride * bitmapInfo.height];
	for (int y = 0; y < bitmapInfo.height; ++y)
		scanline_import(src + y * src_stride, dst + y * dst_stride, bitmapInfo.width);

	// Unlock pixels
	if(AndroidBitmap_unlockPixels(jniEnv, bitmap) != ANDROID_BITMAP_RESUT_SUCCESS)
	{
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to unlock Bitmap pixels");
		return 0;
	}
	
	// Encode and save
	size_t encodedImageSize = 0;
	uint8_t* encodedImageData = 0;
	if(bitmapInfo.format == ANDROID_BITMAP_FORMAT_RGBA_8888)
	{
		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Encoding %dx%d image as RGBA_8888", bitmapInfo.width, bitmapInfo.height);
		encodedImageSize = WebPEncodeRGBA(dst, bitmapInfo.width, bitmapInfo.height, dst_stride, quality, &encodedImageData);
	}
	else if(bitmapInfo.format == ANDROID_BITMAP_FORMAT_RGB_565)
	{
		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Encoding %dx%d image as RGBA_565", bitmapInfo.width, bitmapInfo.height);
		encodedImageSize = WebPEncodeRGB(dst, bitmapInfo.width, bitmapInfo.height, dst_stride, quality, &encodedImageData);
	}
	delete[] dst;
	if(encodedImageSize == 0)
	{
		jniEnv->ThrowNew(jrefs::java::lang::RuntimeException->jclassRef, "Failed to encode to WebP");
		return 0;
	}

	// Copy to output buffer
	jbyteArray resultArray = jniEnv->NewByteArray(encodedImageSize);
	jbyte* resultArrayBuffer = jniEnv->GetByteArrayElements(resultArray, NULL);
	memcpy(resultArrayBuffer, encodedImageData, encodedImageSize);
	jniEnv->ReleaseByteArrayElements(resultArray, resultArrayBuffer, 0);

	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "WebP image size %zu bytes", encodedImageSize);

	// Free
	free(encodedImageData);

	return resultArray;
}

#ifdef __cplusplus
}
#endif
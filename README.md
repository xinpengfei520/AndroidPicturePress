
# AndroidPicturePress

Android Picture Press library of picture was base webp project, it can easy set compress rate above 0~100, after compress size only about 10-50 kb.

# webp图片压缩模块集成说明

## 1.打开 ***src*** 目录
	
### 1.1先将 **webp.jar** 文件复制到你主工程module的 **libs**目录下，右键点击此jar文件点击Add As Library
    或者点击Android Studio工具栏里的 **同步** 按钮；

### 1.2然后将 **.so**文件复制到你工程的main目录下的 ***jniLibs** 目录下，如果没有就新建一个dir;

## 2.调用示例

### 2.1压缩bitmap

```byte[] webpImageData = WebPFactory.nativeEncodeBitmap(bitmap, 60);```

@param 1:要压缩的bitmap对象;
@param 2:压缩率由高到低为0~100，压缩率和压缩时间成正比；

### 2.2解压缩bitmap

```
        try {
        	Bitmap webpBitmap = WebPFactory.nativeDecodeByteArray(decode, null);
        } catch (Exception e) {
            e.printStackTrace();
        }
```

@param 1:要解压缩的bitmap对象的byte[];
@param 2:不知道option 可传null;

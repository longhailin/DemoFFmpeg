/*
 * com_tom_ffmpegAPI_MP4player.cpp
 *
 *  Created on: 2016年10月17日
 *      Author: rd0394
 */

#define JAVA_CLASS_PATH "com/tom/ffmpegAPI/Mp4player"
#define LOG_TAG "jni_mp4player"
#include "jni_common.h"
#include "LocalFileDemuxer.h"
#include "H264SWDecoder.h"
#include "SurfaceView.h"
#include "AudioTrack.h"
#include "AACSWDecoder.h"

static struct g_java_fields {
    jfieldID    context; 	// native object pointer
    jmethodID   post_event; // post event to Java Layer/Callback Function
} g_java_field;

struct Mp4player{
	LocalFileDemuxer*  mpDeMuxer ;
	H264SWDecoder*  mpVDecoder ;
	AACSWDecoder* mpADecoder ;
	SurfaceView* mpView ;
	AudioTrack* mpTrack ;
	RenderThread* mpRender ;
};

static void JNICALL native_playmp4 (JNIEnv* env , jobject mp4player , jlong ctx, jstring file_obj , jobject surface , jobject at )
{
	const char* file = NULL;
	JNI_GET_UTF_CHAR( file , file_obj);


	Mp4player* player = new Mp4player();
	player->mpDeMuxer = new LocalFileDemuxer(file);

	AVCodecParameters* para = player->mpDeMuxer->getVideoCodecPara();
	if( para != NULL){
		switch(para->codec_id){
		case AV_CODEC_ID_H264:
			player->mpVDecoder  = new H264SWDecoder(para ,  player->mpDeMuxer->getVideoTimebase() );
			break;
		default:
			ALOGE("don NOT support %d " , para->codec_id );
			assert(para->codec_id == AV_CODEC_ID_H264);
			break;
		}
	}

	para = player->mpDeMuxer->getAudioCodecPara();
	if( para != NULL){
		switch(para->codec_id){
		case AV_CODEC_ID_AAC:
			player->mpADecoder = new AACSWDecoder(para ,  player->mpDeMuxer->getAudioTimebase()  );
			break;
		default:
			ALOGE("don NOT support %d " , para->codec_id );
			assert(para->codec_id == AV_CODEC_ID_AAC);
			break;
		}
	}


	player->mpView = new SurfaceView(ANativeWindow_fromSurface(env,surface), para->width, para->height );
	player->mpTrack = new AudioTrack();
	player->mpRender = new RenderThread(player->mpView, player->mpTrack);


	player->mpDeMuxer->setAudioSinker(player->mpADecoder);
	player->mpDeMuxer->setVideoSinker(player->mpVDecoder);
	player->mpVDecoder->start(player->mpRender);
	player->mpADecoder->start(player->mpRender);
	player->mpDeMuxer->play();


	env->SetLongField(mp4player ,g_java_field.context, (long)player);
	JNI_RELEASE_STR_STR(file , file_obj);

}


static void JNICALL native_stop (JNIEnv* jenv , jobject mp4player, jlong ctx)
{
	Mp4player* player =  (Mp4player*)ctx ;
	jenv->SetLongField(mp4player ,g_java_field.context, 0);
	player->mpDeMuxer->stop();
	player->mpVDecoder->stop();
	player->mpADecoder->stop();
	delete player->mpVDecoder;
	delete player->mpADecoder;
	delete player->mpDeMuxer;
	delete player->mpRender;
	delete player->mpTrack;
	delete player->mpView;

}

jboolean register_com_tom_ffmpegAPI_MP4player(JNIEnv* env )
{
	jclass clazz;
    clazz = env->FindClass(JAVA_CLASS_PATH );
    if (clazz == NULL) {
		ALOGE("%s:Class Not Found" , JAVA_CLASS_PATH );
		return JNI_ERR ;
    }

    JNINativeMethod method_table[] = {
    	{ "native_playmp4", "(JLjava/lang/String;Landroid/view/Surface;Landroid/media/AudioTrack;)V", (void*)native_playmp4 },
    	{ "native_stop","(J)V", (void*)native_stop },
    };

	jniRegisterNativeMethods( env, JAVA_CLASS_PATH ,  method_table, NELEM(method_table)) ;

	// 查找Java对应field属性
    field fields_to_find[] = {
        { JAVA_CLASS_PATH , "mNativeContext",  "J", &g_java_field.context },
    };

    find_fields( env , fields_to_find, NELEM(fields_to_find) );

    return JNI_TRUE;

}

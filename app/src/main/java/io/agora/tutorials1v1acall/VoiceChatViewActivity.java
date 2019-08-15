package io.agora.tutorials1v1acall;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.PorterDuff;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Debug;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;

import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Locale;

import io.agora.rtc.Constants;
import io.agora.rtc.IRtcEngineEventHandler;
import io.agora.rtc.RtcEngine;

public class VoiceChatViewActivity extends AppCompatActivity {
    static {
        System.loadLibrary("apm-audio-lib");
    }

    private static final int PERMISSION_REQ_ID = 22;
    // Recording  Macro
    private static final boolean BEDEBUG = false;

    private static final int CHANNELS = 2;
    //samplesPerCall
    private static final int SAMPLESPERCELL = 480 * CHANNELS; //960

    private static final String TAG = VoiceChatViewActivity.class.getSimpleName();

    // Tutorial Step 1
    private RtcEngine mRtcEngine;
    private final IRtcEngineEventHandler mRtcEventHandler = new IRtcEngineEventHandler() { // Tutorial Step 1

        @Override
        public void onUserOffline(final int uid, final int reason) { // Tutorial Step 4
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    onRemoteUserLeft(uid, reason);
                }
            });
        }

        @Override
        public void onUserMuteAudio(final int uid, final boolean muted) { // Tutorial Step 6
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    onRemoteUserVoiceMuted(uid, muted);
                }
            });
        }
    };

    /* 采集音频相关的变量 */

    //储存AudioRecord录下来的文件

    //true表示正在录音
    private File  recordingFile = null;
    private FileOutputStream dos = null;
    private boolean isRecording = false;
    private AudioRecord audioRecord = null;
    private File parent = null;//文件目录
    private int bufferSize = 0;//最小缓冲区大小
    private int sampleRateInHz = 48000;//采样率
    private int channelConfig = AudioFormat.CHANNEL_IN_STEREO; //双声道
    private int audioFormat = AudioFormat.ENCODING_PCM_16BIT; //量化位数

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_voice_chat_view);

        if (checkSelfPermission(new String[]{Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQ_ID)) {
            initAgoraEngineAndJoinChannel();
            return;
        }else{
            finish();
        }
    }

    private void initAudio() {
        bufferSize = AudioRecord.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);//计算最小缓冲区
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRateInHz, channelConfig, audioFormat, bufferSize);//创建AudioRecorder对象

        parent = new File(Environment.getExternalStorageDirectory().getAbsolutePath() + "/recordTest");
        if (!parent.exists()) {
            parent.mkdirs();//创建文件夹
        }
    }

    private void initSelfRecording()
    {
        // Self Recording
        enableAudioPreProcessing(true);

        initAudio();
        record();
    }

    private void initAgoraEngineAndJoinChannel() {

        initializeAgoraEngine();     // Tutorial Step 1

        initSelfRecording();

        joinChannel();               // Tutorial Step 2
    }

    public boolean checkSelfPermission(String[] permissions, int requestCode) {
        for (int i = 0; i < permissions.length - 1; i++) {
            if (ContextCompat.checkSelfPermission(this,
                    permissions[i])
                    != PackageManager.PERMISSION_GRANTED) {

                ActivityCompat.requestPermissions(this,
                        permissions,
                        requestCode);
                return false;
            }
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String permissions[], @NonNull int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQ_ID: {
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    initAgoraEngineAndJoinChannel();
                } else {
                    showLongToast("No permission for " + Manifest.permission.RECORD_AUDIO + "or " + Manifest.permission.WRITE_EXTERNAL_STORAGE);
                    finish();
                }
                break;
            }
        }
    }

    public final void showLongToast(final String msg) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_LONG).show();
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        stopRecording();
        enableAudioPreProcessing(false);
        leaveChannel();
        RtcEngine.destroy();
        mRtcEngine = null;
    }

    // Tutorial Step 7
    public void onLocalAudioMuteClicked(View view) {
        ImageView iv = (ImageView) view;
        if (iv.isSelected()) {
            iv.setSelected(false);
            iv.clearColorFilter();
        } else {
            iv.setSelected(true);
            iv.setColorFilter(getResources().getColor(R.color.colorPrimary), PorterDuff.Mode.MULTIPLY);
        }

        mRtcEngine.muteLocalAudioStream(iv.isSelected());
//        if (iv.isSelected()){
//            mRtcEngine.startAudioMixing("https://mms.msstatic.com/test/musicCjAjfyKt46.mp3",false,false,1);
//        }else{
//            mRtcEngine.stopAudioMixing();
//        }

    }

    // Tutorial Step 5
    public void onSwitchSpeakerphoneClicked(View view) {
        ImageView iv = (ImageView) view;
        if (iv.isSelected()) {
            iv.setSelected(false);
            iv.clearColorFilter();
        } else {
            iv.setSelected(true);
            iv.setColorFilter(getResources().getColor(R.color.colorPrimary), PorterDuff.Mode.MULTIPLY);
        }

        mRtcEngine.setEnableSpeakerphone(view.isSelected());
    }

    // Tutorial Step 3
    public void onEncCallClicked(View view) {
        finish();
    }

    // Tutorial Step 1
    private void initializeAgoraEngine() {
        try {
            mRtcEngine = RtcEngine.create(getBaseContext(), getString(R.string.agora_app_id), mRtcEventHandler);

            mRtcEngine.setParameters("{\"rtc.log_filter\": 65535}");
            mRtcEngine.setParameters("{\"che.audio.external_capture\":true}");
            //mRtcEngine.setParameters("{\"che.audio.external_render\":false}");

            /*
            * 0.01s : sdk可正常播放的声音最短时长
            * channels :   1:单声道   2:双声道
            * samplesPerCall : sampleRateInHz * channels *0.01 (所选取的，0.01s一帧，0.01s 可以替换为其它时间的)
            */

            // 如果是2048的话，一次回调需要audioFrame里面含有2048个采样点。如果是双声道的话，每个通道1024个点。
            mRtcEngine.setRecordingAudioFrameParameters(sampleRateInHz, CHANNELS, Constants.RAW_AUDIO_FRAME_OP_MODE_READ_WRITE, SAMPLESPERCELL);

        } catch (Exception e) {
            Log.e(TAG, Log.getStackTraceString(e));
            throw new RuntimeException("NEED TO check rtc sdk init fatal error !\n" + Log.getStackTraceString(e));
        }
    }

    // Tutorial Step 2
    private void joinChannel() {
//        mRtcEngine.setParameters("{\"che.audio.start_debug_recording\":\"NoName\"}");
//        mRtcEngine.setParameters("{\"che.audio.farend.record.mixing\":false}");

        mRtcEngine.joinChannel(null, "yy11", "Extra Optional Data", 0);
    }

    // Tutorial Step 3
    private void leaveChannel() {
        if(null != mRtcEngine)
            mRtcEngine.leaveChannel();
    }

    // Tutorial Step 4
    private void onRemoteUserLeft(int uid, int reason) {
        showLongToast(String.format(Locale.US, "user %d left %d", (uid & 0xFFFFFFFFL), reason));
        View tipMsg = findViewById(R.id.quick_tips_when_use_agora_sdk); // optional UI
        tipMsg.setVisibility(View.VISIBLE);
    }

    // Tutorial Step 6
    private void onRemoteUserVoiceMuted(int uid, boolean muted) {
        showLongToast(String.format(Locale.US, "user %d muted or unmuted %b", (uid & 0xFFFFFFFFL), muted));
    }

    //开始录音
    public void record() {
        isRecording = true;
        new Thread(new Runnable() {
            @Override
            public void run() {
                isRecording = true;
                if(BEDEBUG) {
                    recordingFile = new File(parent, "audiotest.pcm");
                    if (recordingFile.exists()) {
                        recordingFile.delete();
                    }

                    try {
                        recordingFile.createNewFile();
                    } catch (IOException e) {
                        e.printStackTrace();
                        Log.e(TAG, "create Audio Saved File Error !");
                    }
                }

                try {
                    if(BEDEBUG) {
                        dos = new FileOutputStream(recordingFile);
                    }
                    byte[] buffer = new byte[bufferSize];
                    audioRecord.startRecording();//开始录音
                    while (isRecording) {
                        int bufferReadResult = audioRecord.read(buffer, 0, bufferSize);
                        pushAudioData(buffer, bufferReadResult, sampleRateInHz, CHANNELS);
                        if(BEDEBUG) {
                            dos.write(buffer, 0, bufferReadResult);
                        }
                    }
                    audioRecord.stop();//停止录音
                    if(BEDEBUG) {
                        dos.flush();
                        dos.close();
                    }

                } catch (Throwable t) {
                    Log.e(TAG, "Recording Failed");
                }

            }
        }).start();
    }

    //停止录音
    public void stopRecording() {
        isRecording = false;
    }

    public native void enableAudioPreProcessing(boolean enable);
    public native void pushAudioData(byte[] buffer, int bufferLength, int sampleRateInHz, int channels);
}

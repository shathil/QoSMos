package nodes.helsinki.mdiaproximity;

import android.app.Service;
import android.content.Intent;
import android.hardware.camera2.CameraManager;
import android.media.AudioManager;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.google.common.collect.Sets;

import nodes.helsinki.metatdata.AudioStat;
import nodes.helsinki.metatdata.MetaMineConstants;
import nodes.helsinki.metatdata.SensorQueue;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Map;
import java.util.Set;

import static android.os.Environment.getExternalStorageDirectory;
//import static com.qoeapps.qoenforce.services.CallTraceker.dataActivityTracker;

/**
 * Created by mohoque on 11/02/2017.
 */

public class MediaContext extends Service {

    static {
        System.loadLibrary("qosmos_native");
    }

    private Boolean isCamoneinUse = null;
    private Boolean isCamtwoinUse = null;
    public String CLASS_NAME = MediaContext.this.getClass().getSimpleName();
    public static boolean FILE_SAMPLER_STATUS = false;
    CameraManager cameraManager = null;
    AudioManager audioManager = null;
    PowerManager powerManager = null;
    TelephonyManager tm = null;


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {


        Log.d(CLASS_NAME,"Device Profiler is also started");
        //socInterrupted.set(0,100,false);
        cameraManager = (CameraManager) getSystemService(CAMERA_SERVICE);
        audioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
        powerManager = (PowerManager) getSystemService(POWER_SERVICE);
        tm = (TelephonyManager)  getSystemService(TELEPHONY_SERVICE);
        if (!FILE_SAMPLER_STATUS) {
            FILE_SAMPLER_STATUS = true;
            this.registerCameraService();
            updateFlowTables();
            //dataActivityTracker(tm,audioManager);
        }

        return super.onStartCommand(intent, flags, startId);

    }

    public boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            return true;
        }
        return false;
    }


    public  void updateFlowTables() {

        final Handler handler = new Handler(Looper.getMainLooper());


        handler.post(new Runnable() {


            private int delayed = 1000;
            private boolean voipState = false;
            private boolean musicState = false;
            private boolean cameraState = false;
            private int mediaState = 0;
            private int timeer = 0;

            //private FlowAppQueue appQueue = FlowAppQueue.getInstance();




            @Override
            public void run() {


                handler.postDelayed(this,delayed);



                // voip State is activated for both the VoIP and GSM calls
                voipState = DeviceState.audioStart(audioManager);
                musicState = audioManager.isMusicActive();
                if(SensorQueue.getQueueInstance().get("Camera") != null)
                    cameraState = SensorQueue.getQueueInstance().pop("Camera");


                //Video conversation
                if (mediaState == 0) {
                    if (voipState && cameraState)
                        mediaState = 7;

                    // Audio conversation
                    if (voipState && !cameraState)
                        mediaState = 3;

                    // live video broadcast
                    if (!voipState && cameraState)
                        mediaState = 6;

                    // Streaming
                    if (musicState)
                        mediaState = 1;

                    if(mediaState>0) {
                        //Set the MediaContext and Signal the JNI
                        setMediaContext(mediaState);
                    }

                }

                // Media context is over.
                if ((timeer > 0)&&(mediaState == 0)){
                    // Set the media context of the device to CS0 and
                    // signal JNI
                    setMediaContext(mediaState);
                }

                // media context to continue
                if(mediaState>0) {
                    timeer += 1;
                }

                // Here we identify the dominating flow
                if (timeer == 7){
                    // find the flow with the maximum



                }





            }







            public Set<String> getCandidateFlows (Set<String> candidates, long time){

                Set<String> candidateFlowSet = Sets.newHashSet();

                for (String flow:candidates
                     ) {

                    if(!flow.contains("null"))
                    {


                        String fiveTuple= flow.split("::")[0];

                        String timeInfo = flow.split("::")[1];

                        Log.d("Flowtime ",timeInfo);

                        long value = Long.parseLong(timeInfo.split(":")[2]);

                        if (Math.abs(value-time)<=1000){


                            candidateFlowSet.add(fiveTuple+"::"+timeInfo);


                        }
                    }
                }
                return candidateFlowSet;
            }


        });



    }




    private void registerCameraService(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            cameraManager.registerAvailabilityCallback(new CameraManager.AvailabilityCallback() {

                //private Bundle bundle = new Bundle();// cameraZeroAvailable = true;

                // We should send this information through MPI to the main service thread.

                @Override
                public void onCameraAvailable(String cameraId) {
                    super.onCameraAvailable(cameraId);
                    if(Integer.parseInt(cameraId) == 0)
                        isCamoneinUse = false;

                    else

                        isCamtwoinUse = false;

                    SensorQueue.getQueueInstance().push("Camera",false);
                }

                @Override
                public void onCameraUnavailable(String cameraId) {

                    super.onCameraUnavailable(cameraId);
                    if(Integer.parseInt(cameraId) == 0)
                        isCamoneinUse = true;

                    else
                        isCamoneinUse = true;

                    SensorQueue.getQueueInstance().push("Camera", true);

                }
            }, null);
        }

    }


    private Boolean backCamerainUse(){
        if (Build.VERSION.SDK_INT>21)
            return isCamoneinUse;
        else
            return DeviceState.camOneinUse();
    }

    private Boolean frontCamerainUse(){
        if (Build.VERSION.SDK_INT>21)
            return isCamtwoinUse;
        else
            return DeviceState.camTwoinUse();
    }



    @Override
    public void onDestroy() {
        super.onDestroy();
    }



    @Override
    public IBinder onBind(Intent intent) {
        // There are Bound an Unbound Services - you should read something about
        // that. This one is an Unbounded Service.
        return null;


    }

    public native void setMediaContext(int mcontext);
    //public native void readContextualFlows(int mcontext);


}

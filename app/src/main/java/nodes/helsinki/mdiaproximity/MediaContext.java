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


            private String callType = null;
            private int delayed = 1000;
            private boolean candidateFound = false;
            private boolean sampling = true;
            private String mstoWrite = null;
            private long audioSession = 0;
            private long audioTerminated = 0;
            private boolean voipState = false;
            private boolean musicState = false;
            private boolean cameraState = false;
            private boolean gsmCallState = false;
            private int inactivityCounter = 0;


            private AudioStat audioStat = null;
            private FileWriter writer = null;
            private String filePath = getExternalStorageDirectory()+"/FlowLogs.txt";


            //private FlowAppQueue appQueue = FlowAppQueue.getInstance();


            public native void setMediaContext(int mcontext);


            @Override
            public void run() {


                handler.postDelayed(this,delayed);



                // voip State is activated for both the VoIP and GSM calls
                voipState = DeviceState.audioStart(audioManager);
                musicState = audioManager.isMusicActive();
                if(SensorQueue.getQueueInstance().get("Camera") != null)
                    cameraState = SensorQueue.getQueueInstance().pop("Camera");

                //Set the MediaContextThroughJNI

                if(audioStat != null){
                    Log.d("Voice QoE", "Voice missed "+musicState+" "+voipState);

                }

                if (audioStat == null){

                    if (tm.getCallState() != TelephonyManager.CALL_STATE_IDLE){
                        long lastAudioTime = System.currentTimeMillis();
                        audioStat = new AudioStat(MetaMineConstants.MetaMineGSM,lastAudioTime);

                    }
                    if (audioStat == null && voipState){

                        if (this.audioSession == 0)
                            this.audioSession = System.currentTimeMillis();


                        /* In the case of outgoing call there could be some delay in updating the proc files.*/
                        Map<String, String> candidates = MetaMineConstants.getCandidateFlow(audioSession,1000);
                        if (candidates.size()>0){
                            audioStat = new AudioStat(MetaMineConstants.MetaMineVoIP,audioSession);
                            /*
                            String msgForWrite ="";
                            for(Map.Entry<String, String> entry: candidates.entrySet()){
                                msgForWrite += audioSession+"::"+entry.getValue() + "::"+entry.getKey()+"\n";
                            }
                            msgForWrite += "\n";
                            logAudioEvents(msgForWrite);
                            //this.delayed = 1000;
                            */

                        }
                    }
                    if (musicState){
                        long lastAudioTime = DeviceState.isMusicOn(audioManager);

                        audioStat = new AudioStat(MetaMineConstants.MetaMineMusic,lastAudioTime);
                        Log.d("AudioManagerVoiceCall", "user in call");

                    }
                }





                // terminating the Voice.
                if (audioStat!=null) {
                    if ((audioStat.getAudioType() == MetaMineConstants.MetaMineGSM) && (tm.getCallState() == TelephonyManager.CALL_STATE_IDLE)) {
                        long lastAudioTime = System.currentTimeMillis();
                        String msgForWrite = lastAudioTime + ":" + "voice call terminated";
                        logAudioEvents(msgForWrite);
                        audioStat = null;
                        sampling = true;

                    }

                }

                if (audioStat!=null){// dummp the deleted flows
                    if(((audioStat.getAudioType()==MetaMineConstants.MetaMineVoIP) && (!voipState)) ||
                            ((audioStat.getAudioType()==MetaMineConstants.MetaMineMusic)&&(!musicState)))
                    {

                        if(audioTerminated == 0)
                            audioTerminated = System.currentTimeMillis();

                        //String msgForWrite ="";

                        Log.d("EventTracker", "Ending event tracking...");

                        /*
                        if(MetaMineConstants.packetInEvents.size()>0){

                            for (Map.Entry<Long,String> mEn : MetaMineConstants.packetInEvents.entrySet()){
                                msgForWrite = mEn.getKey()+":"+mEn.getValue()+"\n";
                                logAudioEvents(msgForWrite);
                            }
                            MetaMineConstants.packetInEvents.clear();

                        }*/






                    }else{


                    }
                }

                //if (sampling)
                //readProcFile();


            }


            private void logAudioEvents(String toWrite){
                //Log.d("logAudioEvents",toWrite);
                String state = Environment.getExternalStorageState();
                FileWriter writer = null;
                if (Environment.MEDIA_MOUNTED.equals(state)) {
                    try{

                        writer = new FileWriter(filePath, true);
                    }catch (IOException ie){
                        Log.d("FileWriter", ie.toString());}
                }
                if(writer !=null){
                    BufferedWriter bw = new BufferedWriter(writer);
                    PrintWriter out = new PrintWriter(bw);
                    out.print(toWrite);
                    out.close();
                    try{
                        bw.close();

                    }catch (IOException ie){
                        Log.d("BufferedWriter", ie.toString());}
                    try{
                        writer.close();
                    }catch (IOException we){
                        Log.d("FileWriter", we.toString());}

                }
            }

            private boolean sessionEnds(AudioStat stat){

                if ((stat.getAudioType()==MetaMineConstants.MetaMineGSM) &&
                    (tm.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK))
                        return true;

                if (stat.getAudioType()==MetaMineConstants.MetaMineVoIP){
                    // Here we check for VoIP call termination
                    /* Very typical solution would be that we maintian a list of the candidate flows and check only the reccently deleted
                     * list of flows */

                }

                if (stat.getAudioType()==MetaMineConstants.MetaMineMusic){
                    //  // Here we check for Streaming  termination
                    /*Should be similar to the above*/
                }

                return false;
            }

            private void insertFlows (String[] newFlows){
               // Set<String> newFlowSet = Sets.newHashSet();

                Long beginTime = System.currentTimeMillis();
                for (String flow: newFlows) {
                    if (flow != null) {
                        flow = beginTime+":"+flow.trim();
                        //appQueue.addData(flow);
                    }
                }

               // return newFlowSet;
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


}

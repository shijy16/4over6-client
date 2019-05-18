package over6.over6client;

import android.content.Intent;
import android.net.VpnService;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Timer;


public class MainActivity extends AppCompatActivity {
    String  ip_name = "/data/data/over6.over6client/ip";
    String  data_name = "/data/data/over6.over6client/data";

    boolean running = false;
    private Thread cThread;

    Button startBtn;
    TextView infoText;

    String recv_ip;
    String recv_route;
    String[] recv_DNS = new String[3];
    Intent vpnService;
    String client_socket;

    String server_ip;
    String server_port;
    int upload_speed = 0;
    int download_speed = 0;
    int pre_upload_speed = 0;
    int pre_download_speed = 0;
    int upload_packet = 0;
    int download_packet = 0;
    EditText ipText;
    EditText portText;
    TextView uploadSpeedText;
    TextView uploadPacketText;
    TextView downloadSpeedText;
    TextView downloadPacketText;


    //读取C管道
    protected String read_file(String name){
        try {
            FileInputStream fileInputStream = new FileInputStream(name);
            BufferedInputStream in = new BufferedInputStream(fileInputStream);
            byte readBuf[] = new byte[1024];
            for(int i = 0;i < 1024;i++){
                readBuf[i] = 0;
            }
            in.read(readBuf);//读取管道
            in.close();
            return new String(readBuf);
        }catch (Exception e){
            e.printStackTrace();
            return null;
        }
    }

    //写入C管道
    protected boolean write_file(String name,String msg){
        try {
            FileOutputStream fileOutputStream = new FileOutputStream(name);
            BufferedOutputStream out = new BufferedOutputStream(fileOutputStream);
            byte msg_byte[] = msg.getBytes();
            //Notify the background C thread
            out.write(msg_byte, 0, msg_byte.length);
            out.flush();
            out.close();
            return true;
        }catch(Exception e){
            Log.d("jni", "write failed" + e.getStackTrace());
            return false;
        }
    }
    Handler handler = new Handler(){
        @Override
        public void handleMessage(Message msg){
            if(msg.what == 0){
                running = false;
                infoText.setText("连接失败");
                startBtn.setText("connect");
                ipText.setFocusable(true);
                portText.setFocusable(true);
                ipText.setFocusableInTouchMode(true);
                portText.setFocusableInTouchMode(true);
                startBtn.setFocusable(true);
                write_file(ip_name, "Bye");
                notify();
            }else{
                uploadPacketText.setText(String.valueOf(upload_packet));
                downloadPacketText.setText(String.valueOf(download_packet));
                if(download_speed - pre_download_speed < 1000){
                    downloadSpeedText.setText(String.valueOf(download_speed - pre_download_speed) + " bytes/s");
                }else if(download_speed - pre_download_speed > 1000000){
                    double d = download_speed - pre_download_speed;
                    d /= 1000000.0;
                    downloadSpeedText.setText(String.valueOf(d) + " MB/s");
                }else{
                    double d = download_speed - pre_download_speed;
                    d /= 1000.0;
                    downloadSpeedText.setText(String.valueOf(d) + " KB/s");
                }
                if(upload_speed - pre_upload_speed < 1000){
                    uploadSpeedText.setText(String.valueOf(upload_speed - pre_upload_speed) + " bytes/s");
                }else if(upload_speed - pre_upload_speed > 1000000){
                    double d = upload_speed - pre_upload_speed;
                    d /= 1000000.0;
                    uploadSpeedText.setText(String.valueOf(d) + " MB/s");
                }else{
                    double d = upload_speed - pre_upload_speed;
                    d /= 1000.0;
                    uploadSpeedText.setText(String.valueOf(d) + " KB/s");
                }
            }
        }
    };
    //创建C线程
    protected boolean startBackground(){
        cThread = new Thread() {
            @Override
            public void run() {
                StringFromJNI();
            }
        };
        try {
            cThread.start();
        }
        catch (Exception e){
            cThread.run();
        }
        Thread timer = new Thread() {
            @Override
            public void run() {
                int t = 0;
                while (running) {
                    synchronized(this) {
                        try{
                            wait(1000);
                        }catch (Exception e){

                        }
                    }

                    t += 1;
                    String a = read_file(data_name);
                    String[] datas = a.split(" ");
                    pre_download_speed = download_speed;
                    pre_upload_speed = upload_speed;
                    download_packet = Integer.parseInt(datas[0]);
                    download_speed = Integer.parseInt(datas[1]);
                    upload_packet = Integer.parseInt(datas[2]);
                    upload_speed = Integer.parseInt(datas[3]);
                    Message msg = new Message();
                    msg.what = 1;
                    handler.sendMessage(msg);
                    String b = read_file(ip_name);
                    if (b.contains("ERROR")) {
                        running = false;
                        msg = new Message();
                        msg.what = 0;
                        handler.sendMessage(msg);
                    }
                }
            }
        };
        timer.start();
        String ip = "";
        int i = 0;
        while(!(ip.contains("0.0") || ip.contains("ERROR"))){
            ip = read_file(ip_name);
//            Log.d("!!!!!!!!" ,ip + cThread.getId());
            if(ip == null) ip="";
            i++;
            if(i == 10000){
                ip="ERROR";
            }
        }
        if(ip.contains("ERROR")){
            running = false;
            infoText.setText("连接失败");
            startBtn.setText("connect");
            ipText.setFocusable(true);
            portText.setFocusable(true);
            ipText.setFocusableInTouchMode(true);
            portText.setFocusableInTouchMode(true);
            startBtn.setFocusable(true);
            write_file(ip_name, "Bye");
            return false;
        }else {
            String[] infos = ip.split(" ");
            recv_ip = infos[0];
            recv_route = infos[1];
            recv_DNS[0] = infos[2];
            recv_DNS[1] = infos[3];
            recv_DNS[2] = infos[4];
            client_socket = infos[5];
            infoText.setText("连接成功，IP:"+recv_ip);
            running = true;

            //开启VPN
            Intent intent = VpnService.prepare(this);
            if (intent != null) {
                startActivityForResult(intent, 0);
            } else {
                onActivityResult(0, RESULT_OK, null);
            }
            vpnService = new Intent(this,Over6VpnService.class);
            vpnService.putExtra("ip",recv_ip);

            vpnService.putExtra("route",recv_route);
            vpnService.putExtra("dns1",recv_DNS[0]);
            vpnService.putExtra("dns2", recv_DNS[1]);
            vpnService.putExtra("dns3", recv_DNS[2]);
            vpnService.putExtra("socket", client_socket);
            startService(vpnService);
            startBtn.setFocusable(true);

            return true;
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        startBtn = (Button)findViewById(R.id.connect_btn);
        infoText = (TextView)findViewById(R.id.info);
        ipText = (EditText)findViewById(R.id.ip);
        portText =(EditText)findViewById(R.id.port);
        uploadSpeedText = (TextView)findViewById(R.id.upload_speed);
        uploadPacketText = (TextView)findViewById(R.id.upload_packet);
        downloadSpeedText = (TextView)findViewById(R.id.download_speed);
        downloadPacketText  = (TextView)findViewById(R.id.download_packet);
        startBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(running){
                    write_file(ip_name, "Bye");
                    infoText.setText("连接断开");
                    startBtn.setText("connect");
                    upload_speed = 0;
                    download_speed = 0;
                    pre_upload_speed = 0;
                    pre_download_speed = 0;
                    upload_packet = 0;
                    download_packet = 0;
                    if(vpnService != null){
                        stopService(vpnService);
                    }
                    ipText.setFocusable(true);
                    portText.setFocusable(true);
                    ipText.setFocusableInTouchMode(true);
                    portText.setFocusableInTouchMode(true);
//                    portText.requestFocus();
//                    ipText.requestFocus();
                    cThread.interrupt();
                    running = false;
                }else{
                    upload_speed = 0;
                    download_speed = 0;
                    pre_upload_speed = 0;
                    pre_download_speed = 0;
                    upload_packet = 0;
                    download_packet = 0;
                    Intent intent = VpnService.prepare(MainActivity.this);
                    server_ip = ipText.getText().toString();
                    server_port = portText.getText().toString();
                    ipText.setFocusable(false);
                    portText.setFocusable(false);
                    write_file(ip_name, server_ip + " " + server_port + " ");
                    infoText.setText("正在连接...");
                    startBtn.setText("disconnect");
                    startBtn.setFocusable(false);
                    running = true;
                    startBackground();
                }
            }
        });
    }

    public native String StringFromJNI();
    static {
        System.loadLibrary("hellojni");
    }

}

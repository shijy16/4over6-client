package over6.over6client;

import android.content.Intent;
import android.net.VpnService;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;


public class MainActivity extends AppCompatActivity {
    String  ip_name = "/data/data/over6.over6client/ip";
    String  data_name = "/data/data/over6.over6client/data";
    int isCStart = 0;
    boolean running = false;
    private Thread cThread;

    Button startBtn;
    TextView infoText;

    String recv_ip;
    String recv_route;
    String[] recv_DNS = new String[3];
    Intent vpnService;

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

    //创建C线程
    protected boolean startBackground(){
        cThread = new Thread() {
            @Override
            public void run() {
                StringFromJNI();
            }
        };
        cThread.start();


        String ip = "";
        int i = 0;
        while(!(ip.contains("0.0") || ip.contains("ERROR"))){
            ip = read_file(ip_name);
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
            write_file(ip_name, "Bye");
            stopCThread();
            return false;
        }else {
            String[] infos = ip.split(" ");
            recv_ip = infos[0];
            recv_route = infos[1];
            recv_DNS[0] = infos[2];
            recv_DNS[1] = infos[3];
            recv_DNS[2] = infos[4];
            infoText.setText(ip);
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
            startService(vpnService);


                return true;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        startBtn = (Button)findViewById(R.id.connect_btn);
        infoText = (TextView)findViewById(R.id.info);
        startBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(running){
                    write_file(ip_name,"Bye");
                    infoText.setText("断开连接");
                    startBtn.setText("connect");
                    stopCThread();
                    stopService(vpnService);
                    running = false;
                }else{
                    infoText.setText("正在连接");
                    startBackground();
                    startBtn.setText("disconnect");
                    running = true;

                }
            }
        });
    }

    public native String StringFromJNI();
    public native void stopCThread();
    static {
        System.loadLibrary("hellojni");
    }

}

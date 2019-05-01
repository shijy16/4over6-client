package over6.over6client;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;


public class MainActivity extends AppCompatActivity {
    String  ip_name = "/data/data/over6.over6client/ip";
    String  data_name = "/data/data/over6.over6client/data";

    //读取C管道
    protected String read_file(String name){
        try {
            FileInputStream fileInputStream = new FileInputStream(name);
            BufferedInputStream in = new BufferedInputStream(fileInputStream);
            byte readBuf[] = new byte[1024];
            int readLen = in.read(readBuf);//读取管道
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
            Log.d("jni","write failed"+e.getStackTrace());
            return false;
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
//        setContentView(R.layout.activity_main);
        TextView textView = new TextView(this);
        StringFromJNI();
        textView.setText(read_file(ip_name));
        setContentView(textView);
        write_file(data_name, "hello from android");
    }
    public native String StringFromJNI();
    static {
        System.loadLibrary("hellojni");
    }
}

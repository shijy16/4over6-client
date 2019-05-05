package over6.over6client;

import android.content.Intent;
import android.net.VpnService;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;

public class Over6VpnService extends VpnService {
    private ParcelFileDescriptor mInterface;
    String  ip_name = "/data/data/over6.over6client/ip";
    //Configure a builder for the interface.
    Builder builder = new Builder();

    // Services interface
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        //Configure the TUN and get the interface.
        String ipAddress = intent.getStringExtra("ip");
        String route = intent.getStringExtra("route");
        String dns1 = intent.getStringExtra("dns1");
        String dns2 = intent.getStringExtra("dns2");
        String dns3 = intent.getStringExtra("dns3");
        ipAddress = ipAddress.replaceAll(" ", "");
        route = route.replaceAll(" ","");
        dns1 = dns1.replaceAll(" ","");
        dns2 = dns2.replaceAll(" ","");
        dns3 = dns3.replaceAll(" ","");
        //Set MTU equal 1000 to avoid incomplete packet from server
        mInterface = builder.setSession("MyVPNService")
                .addAddress(ipAddress, 24)
                .addDnsServer(dns1)
                .addDnsServer(dns2)
                .addDnsServer(dns3)
                .addRoute(route, 0)
                .setMtu(1000)
                .establish();
        Log.d("jni","vpn starting");
        try{
            int fd = mInterface.getFd();
            FileOutputStream fileOutputStream = new FileOutputStream(ip_name);
            BufferedOutputStream out = new BufferedOutputStream(fileOutputStream);
            byte msg_byte[] = Integer.toString(fd).getBytes();
            out.write(msg_byte, 0, msg_byte.length);
            out.flush();
            out.close();
        }catch(Exception e){
            Log.e("jni", e.toString());
            e.printStackTrace();
        }
        Log.e("jni", "VPN started ");
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
    }
}
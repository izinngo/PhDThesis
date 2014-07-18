package io.crowbar.instrumentation.messaging;

import io.crowbar.instrumentation.events.EventListener;
import io.crowbar.instrumentation.messaging.Messages.Message;
import io.crowbar.instrumentation.runtime.Collector;
import io.crowbar.instrumentation.runtime.Probe;
import io.crowbar.instrumentation.runtime.ProbeSet;

import java.io.ObjectOutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Queue;
import java.util.LinkedList;
import java.util.UUID;

public class Client implements EventListener {
    class Dispatcher extends Thread {
        public void run () {
            Message message = getMessage();


            while (message != null) {
                try {
                    if (s == null)
                        s = new Socket(host, port);

                    if (message != null) {
                        ObjectOutputStream out = new ObjectOutputStream(s.getOutputStream());
                        out.writeObject(message);
                        out.flush();
                        System.out.println("Sending " + message);
                    }

                    message = getMessage();
                }
                catch (Exception e) {
                    System.out.println("Exception, reseting socket");
                    e.printStackTrace();

                    s = null;
                    try {
                        Thread.sleep(10000);
                    }
                    catch (Exception e2) {}
                }
            }
        }
    }


    public Client (String host, int port) {
        this.host = host;
        this.port = port;
    }

    private synchronized void postMessage (Messages.Message m) {
        messages.add(m);

        if (t == null) {
            t = new Dispatcher();
            t.start();
        }
    }

    private synchronized Message getMessage () {
        if (messages.size() == 0) {
            t = null;
            return null;
        }

        return messages.poll();
    }

    @Override
    public void register (ProbeSet ps) {
        try {
            postMessage(new Messages.RegisterMessage(ps));
        }
        catch (ProbeSet.NotPreparedException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void startTransaction (Probe p) {
        try {
            postMessage(new Messages.TransactionStartMessage(p));
        }
        catch (ProbeSet.NotPreparedException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void endTransaction (Probe p,
                                boolean[] hit_vector) {
        try {
            postMessage(new Messages.TransactionEndMessage(p, hit_vector));
        }
        catch (ProbeSet.NotPreparedException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void oracle (Probe p,
                        double error,
                        double confidence) {
        try {
            postMessage(new Messages.OracleMessage(p, error, confidence));
        }
        catch (ProbeSet.NotPreparedException e) {
            e.printStackTrace();
        }
    }

    Queue<Message> messages = new LinkedList<Message> ();

    Socket s = null;
    Thread t = null;
    String host;
    int port;
}
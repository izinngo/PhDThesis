package io.crowbar.instrumentation.runtime;

import io.crowbar.instrumentation.runtime.ProbeGroup.Probe;
import io.crowbar.instrumentation.runtime.Tree.RegistrationException;

import io.crowbar.instrumentation.events.EventListener;

public final class Collector {
    private static Collector collector = null;
    private final EventListener listener;
    private final HitVector hitVector = new HitVector();
    private final WritableTree tree;
    private boolean resetOnTransactionStart = true;


    public static Collector instance () {
        assert collector != null;
        return collector;
    }

    public static void start (String name,
                              EventListener listener) {
        collector = new Collector(name, listener);
    }

    private Collector (String name,
                       EventListener listener) {
        this.listener = listener;
        tree = new WritableTree(name);
        registerNode(tree.getRoot());
    }

    public void registerNode (Node n) {
        try {
            listener.registerNode(n);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public Probe registerProbe (String groupName,
                                int nodeId,
                                ProbeType type) throws RegistrationException {
        Probe p = hitVector.registerProbe(groupName,
                                          nodeId,
                                          type);


        System.err.println("Register: " + p);
        try {
            listener.registerProbe(p.getGlobalId(), nodeId, type);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return p;
    }

    public void transactionStart (int probeId) {
        if (resetOnTransactionStart)
            hitVector.reset();

        try {
            listener.startTransaction(probeId);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void transactionEnd (int probeId) {
        try {
            listener.endTransaction(probeId, hitVector.get());
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        hitVector.reset();
    }

    public void oracle (int probeId,
                        double error,
                        double confidence) {
        try {
            listener.oracle(probeId, error, confidence);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public boolean[] getHitVector (String className) {
        return hitVector.get(className);
    }

    public void hit (int globalProbeId) {
        hitVector.hit(globalProbeId);
    }

    public Node getRootNode () {
        return tree.getRoot();
    }

    public Node addNode (String name,
                         Node parent) throws RegistrationException {
        Node n = tree.addNode(name, parent);


        registerNode(n);
        return n;
    }
}
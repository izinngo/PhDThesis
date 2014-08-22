package io.crowbar.instrumentation.spectrum;

import io.crowbar.diagnostic.spectrum.Transaction;
import io.crowbar.diagnostic.spectrum.TransactionFactory;
import io.crowbar.diagnostic.spectrum.activity.Hit;

import java.util.AbstractList;
import java.util.List;


public final class Factory {
    private Factory () {}

    private static class ActivityListAdaptor extends AbstractList<Hit> {
        private final boolean[] hitVector;
        ActivityListAdaptor (boolean[] hitVector) {
            this.hitVector = hitVector;
        }

        @Override
        public final Hit get (int index) {
            return new Hit(hitVector[index]);
        }

        @Override
        public final int size () {
            return hitVector.length;
        }
    }


    public static Transaction<Hit, TrM> createTransaction (int id,
                                                           boolean[] hitVector,
                                                           double error,
                                                           double confidence,
                                                           TrM metadata) {
        List<Hit> adaptor = new ActivityListAdaptor(hitVector);

        TransactionFactory<Hit, TrM> tf =
            new TransactionFactory<Hit, TrM> ();


        return tf.create(id, adaptor, error, confidence, metadata);
    }
}
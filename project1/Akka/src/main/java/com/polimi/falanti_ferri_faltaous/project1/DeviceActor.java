package com.polimi.falanti_ferri_faltaous.project1;

import akka.actor.AbstractActor;
import akka.actor.Props;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class DeviceActor extends AbstractActor {
    public final int id;
    private Set<Integer> contactIds = new HashSet<>();

    public DeviceActor(int id) {
        this.id = id;
    }

    @Override
    public Receive createReceive() {
        return receiveBuilder()
                .match(ContactMessage.class, this::onContactMessage)
                .match(InterestEventMessage.class, this::onInterestEventMessage)
                //.match(NotificationMessage.class, this::onNotificationMessage)
                .build();
    }

    void onContactMessage(ContactMessage msg) {
        contactIds.add(msg.contactId);
        System.out.println(String.format("[%s, id:%d]: added %d to contact list",  getContext().getSelf().path().name(), id, msg.contactId));
    }

    void onInterestEventMessage(InterestEventMessage msg) {
        contactIds.forEach(val -> {
            sender().tell(new NotificationMessage(val), self());
            System.out.println(String.format("[%s, id:%d]: must notify device %d",  getContext().getSelf().path().name(), id, val));
        });
    }

//    void onNotificationMessage(NotificationMessage msg) {
//        // getContext().getSelf().path().name() is used to find the name of the actor, maybe there is a more proper way
//    }

    static Props props(int id) {
        return Props.create(DeviceActor.class, id);
    }
}

package com.polimi.falanti_ferri_faltaous.project1;

import akka.actor.ActorRef;
import akka.actor.ActorSystem;

import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class Server {

    private static final int numThreads = 10;
    private static final int numMessages = 1;

    public static void main(String[] args) {

        final ActorSystem sys = ActorSystem.create("System");
        final ActorRef server = sys.actorOf(ServerActor.props(), "server");

        // Send messages from multiple threads in parallel
        final ExecutorService exec = Executors.newFixedThreadPool(numThreads);

        // TODO: create messages from MQTT topics and use server.tell() to propagate them to actors

        // TODO: just for a quick debug
        for (int i = 0; i < numMessages; i++) {
            exec.submit(() -> server.tell(new ContactMessage(1, 2), ActorRef.noSender()));
            exec.submit(() -> server.tell(new ContactMessage(1, 3), ActorRef.noSender()));
            exec.submit(() -> server.tell(new ContactMessage(2, 1), ActorRef.noSender()));
        }

        for (int i = 0; i < numMessages; i++) {
            exec.submit(() -> server.tell(new InterestEventMessage(1, ""), ActorRef.noSender()));
            exec.submit(() -> server.tell(new InterestEventMessage(2, ""), ActorRef.noSender()));
        }

        // Wait for all messages to be sent and received
        try {
            System.in.read();
        } catch (IOException e) {
            e.printStackTrace();
        }
        exec.shutdown();
        sys.terminate();

    }
}

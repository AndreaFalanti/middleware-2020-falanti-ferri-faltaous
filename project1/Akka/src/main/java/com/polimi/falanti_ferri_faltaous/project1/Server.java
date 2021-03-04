package com.polimi.falanti_ferri_faltaous.project1;

import akka.actor.ActorRef;
import akka.actor.ActorSystem;
import com.google.gson.Gson;
import org.eclipse.paho.client.mqttv3.*;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class Server {
    private static final int numThreads = 10;
    private static final int numMessages = 1;

    public static final String CONTACT_TOPIC = "iot/encounter/json";
    public static final String EVENT_TOPIC = "iot/interest/json";
    public static final String NOTIFICATION_TOPIC = "iot/notification/json";
    public static final int QOS_LEVEL = 0;

    public static void main(String[] args) throws MqttException {
        Gson gson = new Gson();

        // Define connection to MQTT broker
        MqttClient client = new MqttClient(
                "tcp://test.mosquitto.org:1883", //URI
                MqttClient.generateClientId(), //ClientId
                new MemoryPersistence()); //Persistence

        final ActorSystem sys = ActorSystem.create("System");
        final ActorRef server = sys.actorOf(ServerActor.props(client), "server");

        // Define connection options
        MqttConnectOptions options = new MqttConnectOptions();
        options.setMqttVersion(MqttConnectOptions.MQTT_VERSION_3_1);

        // Define how MQTT messages and events are handled
        client.setCallback(new MqttCallback() {
            @Override
            public void connectionLost(Throwable cause) {
                //Called when the client lost the connection to the broker
            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception {
                String jsonString = new String(message.getPayload());
                System.out.println("[MQTT] " + topic + ": " + jsonString);

                // Create message by deserializing the json, with type based on MQTT topic. Send message to server actor
                switch (topic) {
                    case CONTACT_TOPIC:
                        ContactMessage msg = gson.fromJson(jsonString, ContactMessage.class);
                        server.tell(msg, ActorRef.noSender());
                        break;
                    case EVENT_TOPIC:
                        InterestEventMessage msg2 = gson.fromJson(jsonString, InterestEventMessage.class);
                        server.tell(msg2, ActorRef.noSender());
                        break;
                    default:
                        System.out.println("ERROR: topic not recognized");
                        break;
                }
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
                //Called when a outgoing publish is complete
            }
        });

        // Finalize connection to broker and setup subscriptions
        client.connect(options);
        System.out.println("Connected to MQTT broker: tcp://test.mosquitto.org:1883");
        client.subscribe(CONTACT_TOPIC, QOS_LEVEL);
        client.subscribe(EVENT_TOPIC, QOS_LEVEL);

        // TODO: uncomment this section if need a quick test of message handling by actors
//        // Send messages from multiple threads in parallel
//        final ExecutorService exec = Executors.newFixedThreadPool(numThreads);
//
//        for (int i = 0; i < numMessages; i++) {
//            exec.submit(() -> server.tell(new ContactMessage(1, 2), ActorRef.noSender()));
//            exec.submit(() -> server.tell(new ContactMessage(1, 3), ActorRef.noSender()));
//            exec.submit(() -> server.tell(new ContactMessage(2, 1), ActorRef.noSender()));
//        }
//
//        for (int i = 0; i < numMessages; i++) {
//            exec.submit(() -> server.tell(new InterestEventMessage(1), ActorRef.noSender()));
//            exec.submit(() -> server.tell(new InterestEventMessage(2), ActorRef.noSender()));
//        }

        // Terminate the server when "enter" is pressed
        try {
            System.in.read();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

        client.disconnect();
//        exec.shutdown();
        sys.terminate();

    }
}

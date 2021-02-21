package com.polimi.falanti_ferri_faltaous.project1;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.Props;
import com.google.gson.Gson;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;

import java.util.HashMap;
import java.util.Map;

import static java.nio.charset.StandardCharsets.UTF_8;

public class ServerActor extends AbstractActor {
	private MqttClient client;
	private Map<Integer, ActorRef> childRefMap;

	public ServerActor(MqttClient client) {
		this.client = client;
		childRefMap = new HashMap<>();
	}

	@Override
	public Receive createReceive() {
		return receiveBuilder()
				.match(ContactMessage.class, this::onContactMessage)
				.match(InterestEventMessage.class, this::onInterestEventMessage)
				.match(NotificationMessage.class, this::onNotificationMessage)
				.build();
	}

	void onContactMessage(ContactMessage msg) {
		if (!childRefMap.containsKey(msg.senderId)) {
			childRefMap.put(msg.senderId, getContext().actorOf(DeviceActor.props(msg.senderId)));
		}

		childRefMap.get(msg.senderId).tell(msg, self());
	}

	void onInterestEventMessage(InterestEventMessage msg) {
		if (!childRefMap.containsKey(msg.senderId)) {
			childRefMap.put(msg.senderId, getContext().actorOf(DeviceActor.props(msg.senderId)));
		}

		childRefMap.get(msg.senderId).tell(msg, self());
	}

	void onNotificationMessage(NotificationMessage msg) throws MqttException {
		System.out.println(String.format("[%s]: sending notification to %d",  getContext().getSelf().path().name(), msg.targetId));

		// send to MQTT topic
		client.publish(
				Server.NOTIFICATION_TOPIC, // topic
				String.format("{targetId: %d}", msg.targetId).getBytes(UTF_8), // payload
				Server.QOS_LEVEL, // QoS
				false); // retained?
	}

	static Props props(MqttClient client) {
		return Props.create(ServerActor.class, client);
	}

}

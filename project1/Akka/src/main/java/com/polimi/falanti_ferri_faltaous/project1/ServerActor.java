package com.polimi.falanti_ferri_faltaous.project1;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.Props;

import java.util.HashMap;
import java.util.Map;

public class ServerActor extends AbstractActor {
	private Map<Integer, ActorRef> childRefMap;

	public ServerActor() {
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

	void onNotificationMessage(NotificationMessage msg) {
		System.out.println(String.format("[%s]: sending notification to %d",  getContext().getSelf().path().name(), msg.targetId));

		// TODO: send to MQTT topic
	}

	static Props props() {
		return Props.create(ServerActor.class);
	}

}

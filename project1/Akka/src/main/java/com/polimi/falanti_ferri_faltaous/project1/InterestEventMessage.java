package com.polimi.falanti_ferri_faltaous.project1;

public class InterestEventMessage {
    final int senderId;
    final String message;

    public InterestEventMessage(int senderId, String message) {
        this.senderId = senderId;
        this.message = message;
    }
}

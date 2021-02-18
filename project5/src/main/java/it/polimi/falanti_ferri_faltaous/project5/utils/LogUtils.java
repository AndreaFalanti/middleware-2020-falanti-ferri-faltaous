package it.polimi.falanti_ferri_faltaous.project5.utils;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;

public class LogUtils {
    public static void setLogLevel() {
        Logger.getLogger("org").setLevel(Level.OFF);
        Logger.getLogger("akka").setLevel(Level.OFF);
    }
}

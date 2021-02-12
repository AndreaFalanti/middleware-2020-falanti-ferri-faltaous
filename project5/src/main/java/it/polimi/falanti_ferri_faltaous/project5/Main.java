package it.polimi.falanti_ferri_faltaous.project5;

import it.polimi.falanti_ferri_faltaous.project5.utils.LogUtils;
import org.apache.spark.sql.Dataset;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.SparkSession;

import static org.apache.spark.sql.functions.col;


public class Main {
    public static void main(String[] args) {
        LogUtils.setLogLevel();

        final String master = args.length > 0 ? args[0] : "local[4]";

        final SparkSession spark = SparkSession
                .builder()
                .master(master)
                .appName("Project-5")
                .getOrCreate();

        // directly infer the structure from the csv headers
        final Dataset<Row> coronaRecords = spark
                .read()
                .option("header", "true")
                .option("delimiter", ",")
                .csv("data/data.csv");

        // test that the dataset works fine and the structure is correct
        coronaRecords.filter(col("geoId").equalTo("AL")).show();

        spark.close();
    }

}

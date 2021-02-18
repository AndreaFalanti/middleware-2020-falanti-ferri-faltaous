package it.polimi.falanti_ferri_faltaous.project5;

import it.polimi.falanti_ferri_faltaous.project5.utils.LogUtils;
import org.apache.spark.sql.*;
import org.apache.spark.sql.expressions.Window;
import org.apache.spark.sql.expressions.WindowSpec;
import org.apache.spark.sql.types.DataTypes;

import static org.apache.spark.sql.functions.*;


public class Main {
    // HELPER FUNCTIONS USED IN PREPROCESSING
    /**
     * Distribute weekly cases among days as integer values, putting the rest on farther days
     * @param day 0-indexed value
     * @return SQL Column with daily count
     */
    private static Column computeDayCases(int day) {
        return col("cases_weekly").$div(7).$plus(day / 7.0).cast(DataTypes.LongType);
    }

    /**
     * Compute date for row split from week record
     * @param day 0-indexed value
     * @return SQL Column with daily count
     */
    private static Column computeDate(int day) {
        return date_sub(col("date"), 6 - day);
    }

    public static void main(String[] args) {
        LogUtils.setLogLevel();

        final String master = args.length > 0 ? args[0] : "local[4]";
        final String filePath = args.length > 1 ? args[1] : "data/data.csv";
        final boolean logOnConsole = args.length > 2 ? args[2].equals("true") : false;
        final String savePath = args.length > 3 ? args[3] : "./";

        final SparkSession spark = SparkSession
                .builder()
                .master(master)
                .appName("Project-5")
                .getOrCreate();

        // READ INPUT + PREPROCESSING
        // Create the dataframe, directly infer the structure from the csv headers.
        // CSV HEADERS: dateRep, year_week, cases_weekly, deaths_weekly, countriesAndTerritories, geoId,
        // countryterritoryCode, popData2019, continentExp, notification_rate_per_100000_population_14-days
        // Preprocessing:
        // - cast the date to correct type, so that we can order the rows later.
        // - Generate from each row a total of 7 rows -> from weeks to days
        // - Take only the columns required for the project operations.
        final Dataset<Row> coronaRecords = spark
                .read()
                .option("header", "true")
                .option("delimiter", ",")
                .csv(filePath)
                .withColumn("date", to_date(col("dateRep"), "dd/MM/yyyy"))
                .withColumn("cases_day1", computeDayCases(0))
                .withColumn("date_day1", computeDate(0))
                .withColumn("cases_day2", computeDayCases(1))
                .withColumn("date_day2", computeDate(1))
                .withColumn("cases_day3", computeDayCases(2))
                .withColumn("date_day3", computeDate(2))
                .withColumn("cases_day4", computeDayCases(3))
                .withColumn("date_day4", computeDate(3))
                .withColumn("cases_day5", computeDayCases(4))
                .withColumn("date_day5", computeDate(4))
                .withColumn("cases_day6", computeDayCases(5))
                .withColumn("date_day6", computeDate(5))
                .withColumn("cases_day7", computeDayCases(6))
                .withColumn("date_day7", computeDate(6))
                .withColumn("zip_col", explode(arrays_zip(
                        array("cases_day1", "cases_day2", "cases_day3", "cases_day4",
                        "cases_day5", "cases_day6", "cases_day7"), array("date_day1", "date_day2",
                        "date_day3", "date_day4", "date_day5", "date_day6", "date_day7"))))
                .select(col("zip_col").getItem("1").as("date"), col("zip_col").getItem("0").as("cases"),
                        col("countriesAndTerritories"))
                .orderBy("date");

        // DEBUG: test that the dataset works fine and the structure is correct
        //coronaRecords.filter(col("countriesAndTerritories").equalTo("Albania")).show();
        //coronaRecords.printSchema();

        System.out.println("File read correctly");

        // 1st OPERATION: compute the moving average of daily cases
        WindowSpec windowSpec = Window.partitionBy("countriesAndTerritories").orderBy(col("date"));

        final Dataset<Row> movAvg = coronaRecords.withColumn("movingAverage", avg(col("cases"))
                .over(windowSpec.rowsBetween(-6, 0))).cache();

        if (logOnConsole) {
            //movAvg.show(100, false);
            movAvg.select("date", "countriesAndTerritories", "movingAverage").show(100, false);
        }
        else {
            movAvg.select("date", "countriesAndTerritories", "movingAverage")
                    .write().mode(SaveMode.Overwrite)
                    .format("com.databricks.spark.csv")
                    .option("delimiter", ",").format("csv").save(savePath + "/op1.csv");
        }

        // 2nd OPERATION: compute variation percentage between the average of the day and the one of the previous day
        final Dataset<Row> percentageIncrease = movAvg
            .withColumn("variation",
                when((lag("movingAverage", 1).over(windowSpec)).isNull(), 0)
                    .otherwise(col("movingAverage")
                        .$minus(lag("movingAverage", 1).over(windowSpec))))
                .withColumn("variationPercentage",
                        when((lag("variation", 1).over(windowSpec)).isNull(), 0)
                                .otherwise(col("variation")
                                        .$div(when((lag("movingAverage", 1).over(windowSpec)).equalTo(0), Double.MIN_VALUE)
                                                .otherwise(lag("movingAverage", 1).over(windowSpec)))
                                        .$times(100))).cache();

        if (logOnConsole) {
            //percentageIncrease.show(100, false);
            percentageIncrease.select("date", "countriesAndTerritories", "variationPercentage")
                    .show(100, false);
        }
        else {
            percentageIncrease.select("date", "countriesAndTerritories", "variationPercentage")
                    .write().mode(SaveMode.Overwrite)
                    .format("com.databricks.spark.csv")
                    .option("delimiter", ",").format("csv").save(savePath + "/op2.csv");
        }

        movAvg.unpersist();


        //3rd OPERATION: top 10 countries with the greatest percentage increase per day
        WindowSpec windowSpec2 = Window.partitionBy("date").orderBy(col("variationPercentage").desc());

        final Dataset<Row> top10CountriesPerDay = percentageIncrease
                .select(col("*"), row_number().over(windowSpec2).alias("top"))
                .filter(col("top").$less$eq(10))
                .orderBy("date", "top");

        if (logOnConsole) {
            //top10CountriesPerDay.show(100, false);
            //top10CountriesPerDay.where(col("date").$greater(new Date(120, 1, 1))).show(100, false);
            top10CountriesPerDay.select("date", "countriesAndTerritories", "variationPercentage", "top")
                    .show(100, false);
        }
        else {
            top10CountriesPerDay.select("date", "countriesAndTerritories", "variationPercentage", "top")
                    .write().mode(SaveMode.Overwrite)
                    .format("com.databricks.spark.csv")
                    .option("delimiter", ",").save(savePath + "/op3.csv");
        }
        
        percentageIncrease.unpersist();

        spark.close();
    }

}

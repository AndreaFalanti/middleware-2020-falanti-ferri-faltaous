package it.polimi.falanti_ferri_faltaous.project5;

import it.polimi.falanti_ferri_faltaous.project5.utils.LogUtils;
import org.apache.spark.sql.Dataset;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.SparkSession;
import org.apache.spark.sql.expressions.Window;
import org.apache.spark.sql.expressions.WindowSpec;

import static org.apache.spark.sql.functions.*;


public class Main {
//    private static Row[] expandInDays(Row line) {
//        Row[] rows = new Row[7];
//
//        for (int i = 0; i < 7; i++) {
//            rows[i] = RowFactory.create(line.getDate(0), line.getLong(1), line.getString(2));
//        }
//        return rows;
//    }

    public static void main(String[] args) {
        LogUtils.setLogLevel();

        final String master = args.length > 0 ? args[0] : "local[4]";

//        final List<StructField> mySchemaFields = new ArrayList<>();
//        mySchemaFields.add(DataTypes.createStructField("dateRep", DataTypes.DateType, false));
//        mySchemaFields.add(DataTypes.createStructField("year_week", DataTypes.StringType, false));
//        mySchemaFields.add(DataTypes.createStructField("cases_weekly", DataTypes.LongType, false));
//        mySchemaFields.add(DataTypes.createStructField("deaths_weekly", DataTypes.LongType, false));
//        mySchemaFields.add(DataTypes.createStructField("countriesAndTerritories", DataTypes.StringType, false));
//        mySchemaFields.add(DataTypes.createStructField("geoId", DataTypes.StringType, false));
//        mySchemaFields.add(DataTypes.createStructField("countryterritoryCode", DataTypes.StringType, false));
//        mySchemaFields.add(DataTypes.createStructField("popData2019", DataTypes.LongType, false));
//        mySchemaFields.add(DataTypes.createStructField("continentExp", DataTypes.StringType, false));
//        mySchemaFields.add(DataTypes.createStructField("notification_rate_per_100000_population_14-days", DataTypes.FloatType, false));
//        final StructType mySchema = DataTypes.createStructType(mySchemaFields);

        final SparkSession spark = SparkSession
                .builder()
                .master(master)
                .appName("Project-5")
                .getOrCreate();

        // create the dataframe, directly infer the structure from the csv headers
        // headers: dateRep, year_week, cases_weekly, deaths_weekly, countriesAndTerritories, geoId,
        // countryterritoryCode, popData2019, continentExp, notification_rate_per_100000_population_14-days
        // Also cast the date to correct type, so that we can order the rows later. Take only the columns required for
        // the project operations.
        final Dataset<Row> coronaRecords = spark
                .read()
                .option("header", "true")
                .option("delimiter", ",")
                .csv("data/data.csv")
                .withColumn("date", to_date(col("dateRep"), "dd/MM/yyyy"))
                .select("date", "cases_weekly", "countriesAndTerritories");

        // DEBUG: test that the dataset works fine and the structure is correct
        // coronaRecords.filter(col("countriesAndTerritories").equalTo("Albania")).show();

//        coronaRecords = coronaRecords.flatMap(
//                (FlatMapFunction<Row, Row>) line -> Arrays.asList(expandInDays(line)).iterator(),
//                Encoders.tuple(Encoders.DATE(), Encoders.LONG(), Encoders.STRING())
//        );

        final Dataset<Row> movAvg = coronaRecords.withColumn("movingAverage", avg(col("cases_weekly"))
                .over(Window.partitionBy("countriesAndTerritories").orderBy(col("date")).rowsBetween(-6, 0)));

        movAvg.show(100, false);
        //movAvg.printSchema();

        WindowSpec windowSpec = Window.partitionBy("countriesAndTerritories").orderBy(col("date"));

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
                                        .$times(100)));

        percentageIncrease.show(100, false);

//        final Dataset<Row> maxIncreasePerDay = percentageIncrease
//                .groupBy("date")
//                .max("variationPercentage")
//                .orderBy("date");
//
//        maxIncreasePerDay.show();
//
//        final Dataset<Row> topCountriesForDay = percentageIncrease
//                .join(maxIncreasePerDay,
//                    percentageIncrease.col("variationPercentage").equalTo(maxIncreasePerDay.col("max(variationPercentage)"))
//                            .and(percentageIncrease.col("date").equalTo(maxIncreasePerDay.col("date"))),
//                    "leftsemi")
//                .where(col("date").$greater(new Date(120, 1, 1)))
//                .orderBy("date");
//
//        topCountriesForDay.show();

        WindowSpec windowSpec2 = Window.partitionBy("date").orderBy(col("variationPercentage").desc());

        final Dataset<Row> top10CountriesPerDay = percentageIncrease
                .select(col("*"), row_number().over(windowSpec2).alias("top"))
                .filter(col("top").$less$eq(10))
                .orderBy("date", "top");

        top10CountriesPerDay.show(100, false);

        spark.close();
    }

}

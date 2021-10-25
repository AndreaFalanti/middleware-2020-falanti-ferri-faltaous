# middleware-2020-falanti-ferri-faltaous

The project consists in implementing multiple given projects with the middlewares seen during lectures. The middlewares to choose for each project were chosen by students, based on the project needs. Here is a summary of all projects, with the middlewares chosen for implementation.

## Project 1: Contact Tracing with Body-worn IoT Devices
- Main developer: Faltaous
- Middlewares used: Akka + ContikiNG


Description: People roaming in a given location carry IoT devices. The devices use the radio as a proximity sensor. Every time
two such devices are within the same broadcast domain, that is, at 1-hop distance from each other, the two
people wearing the devices are considered to be in contact. The contacts between people’s devices are
periodically reported to the backend on the regular Internet. Whenever one device signals an event of interest,
every other device that was in contact with the former must be informed.

## Project 2: Distributed Node-red Flows
- Main developer: Ferri
- Middlewares used: Node-Red + Apache Kafka


Description: You are to implement an architecture that allows Node-red flows to span multiple devices. Normally, a
Node-red flow executes locally to the machine where it is installed. Instead, consider multiple Node-red
installations that i) register to a central repository that maintains information on all running installations and ii)
can exchange messages among them by logically connecting the output of a node in one installation to the input
of another node in a different installation. Addressing of Node-red installations must be content-based, that is,
the target Node-red installations that receive the messages cannot be determined based on their IP address or
some other form of machine-level identifier. You need to demonstrate that a flow developed to be executed on a
single Node-red installation may be split across two or multiple Node-red machines with a limited set of
modifications to the flow itself.

## Project 4: A Simple Model for Virus Spreading
- Main developer: Falanti
- Middlewares used: MPI


Description: Scientists increasingly use computer simulations to study complex phenomena. In this project, you have to
implement a program that simulates how a virus spreads over time in a population of individuals. The program
considers N individuals that move in a rectangular area with linear motion and velocity v (each individual
following a different direction). Some individuals are initially infected. If an individual remains close to (at least
one) infected individual for more than 10 minutes, it becomes infected. After 10 days, an infected individual
recovers and becomes immune. Immune individuals do not become infected and do not infect others. An
immune individual becomes susceptible again (i.e., it can be infected) after 3 months.

The overall area is split into smaller rectangular sub-areas representing countries. The program outputs, at the
end of each simulated day, the overall number of susceptible, infected, and immune individuals in each country.
An individual belongs to a country if it is in that country at the end of the day.

A performance analysis of the proposed solution is appreciated (but not mandatory). In particular, we are
interested in studies that evaluate (1) how the execution time changes when increasing the number of
individuals and/or the number of countries in the simulation; (2) how the execution time decreases when
adding more processing cores/hosts.

## Project 5: Analysis of COVID-19 Data
- Main developer: Falanti
- Middlewares used: Apache Spark

Description: In this project, you have to implement a program that analyzes open datasets to study the evolution of the
COVID-19 situation worldwide. The program starts from the dataset of new reported cases for each country
daily and computes the following queries:
1. Seven days moving average of new reported cases, for each county and for each day
2. Percentage increase (with respect to the day before) of the seven days moving average, for each country
and for each day
3. Top 10 countries with the highest percentage increase of the seven days moving average, for each day
You can either use real open datasets 1 or synthetic data generated with the simulator developed for Project #4.

A performance analysis of the proposed solution is appreciated (but not mandatory). In particular, we are
interested in studies that evaluate (1) how the execution time changes when increasing the size of the dataset
and/or number of countries; (2) how the execution time decreases when adding more processing cores/hosts.

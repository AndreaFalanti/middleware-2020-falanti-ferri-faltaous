#include "contiki.h"
#include "contiki-net.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "net/ipv6/uip.h"
#include "sys/cc.h"
#include "mqtt.h"

#include "net/ipv6/sicslowpan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_BROAD_PORT 4321
#define UDP_SERVER_PORT	5678


/*---------------------------------------------------------------------------*/
/*
 * Publish to a local MQTT broker (e.g. mosquitto) running on
 * the node that hosts your border router
 */
static const char *broker_ip = MQTT_BROKER_IP_ADDR;
#define DEFAULT_ORG_ID              "mqtt-demo"
/*---------------------------------------------------------------------------*/
/*
 * A timeout used when waiting for something to happen (e.g. to connect or to
 * disconnect)
 */
#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/* Connections and reconnections */
#define RETRY_FOREVER              0xFF
#define RECONNECT_INTERVAL         (CLOCK_SECOND * 2)
/*---------------------------------------------------------------------------*/
/*
 * Number of times to try reconnecting to the broker.
 * Can be a limited number (e.g. 3, 10 etc) or can be set to RETRY_FOREVER
 */
#define RECONNECT_ATTEMPTS         RETRY_FOREVER
#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)
static struct timer connection_life;
static uint8_t connect_attempt;
/*---------------------------------------------------------------------------*/
/* Various states */
static uint8_t state;
#define STATE_INIT            0
#define STATE_REGISTERED      1
#define STATE_CONNECTING      2
#define STATE_CONNECTED       3
#define STATE_PUBLISHING      4
#define STATE_DISCONNECTED    5
#define STATE_NEWCONFIG       6
#define STATE_CONFIG_ERROR 0xFE
#define STATE_ERROR        0xFF
/*---------------------------------------------------------------------------*/
#define CONFIG_ORG_ID_LEN        32
#define CONFIG_TYPE_ID_LEN       32
#define CONFIG_AUTH_TOKEN_LEN    32
#define CONFIG_CMD_TYPE_LEN       8
#define CONFIG_IP_ADDR_STR_LEN   64
/*---------------------------------------------------------------------------*/
/* A timeout used when waiting to connect to a network */
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
/*---------------------------------------------------------------------------*/
/* Default configuration values */
#define DEFAULT_TYPE_ID             "native"
#define DEFAULT_AUTH_TOKEN          "AUTHTOKEN"
#define DEFAULT_SUBSCRIBE_CMD_TYPE  "+"
#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_PUBLISH_INTERVAL    (60 * CLOCK_SECOND)
#define DEFAULT_KEEP_ALIVE_TIMER    60
/*---------------------------------------------------------------------------*/

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		  (15 * CLOCK_SECOND)

//static struct simple_udp_connection udp_conn;
static struct simple_udp_connection broadcast_connection;
/*---------------------------------------------------------------------------*/
/**
 * \brief Data structure declaration for the MQTT client configuration
 */
typedef struct mqtt_client_config {
  	char org_id[CONFIG_ORG_ID_LEN];
	char type_id[CONFIG_TYPE_ID_LEN];
	char auth_token[CONFIG_AUTH_TOKEN_LEN];
	char broker_ip[CONFIG_IP_ADDR_STR_LEN];
	char cmd_type[CONFIG_CMD_TYPE_LEN];
	clock_time_t pub_interval;
	uint16_t broker_port;
} 	mqtt_client_config_t;
/*---------------------------------------------------------------------------*/
/* Maximum TCP segment size for outgoing segments of our socket */
#define MAX_TCP_SEGMENT_SIZE 512
/*---------------------------------------------------------------------------*/
/*
 * Buffers for Client ID and Topic.
 * Make sure they are large enough to hold the entire respective string
 *
 * We also need space for the null termination
 */
#define BUFFER_SIZE 64
static char client_id[BUFFER_SIZE];
static char pub_topic_encounter[BUFFER_SIZE];
static char pub_topic_interest[BUFFER_SIZE];
static char sub_topic_notification[BUFFER_SIZE];
/*---------------------------------------------------------------------------*/
/*
 * The main MQTT buffers.
 * We will need to increase if we start publishing more data.
 */
#define APP_BUFFER_SIZE 512
static struct mqtt_connection conn;
static char app_buffer[APP_BUFFER_SIZE];
/*---------------------------------------------------------------------------*/
static struct mqtt_message *msg_ptr = 0;
static struct etimer publish_periodic_timer;

static char *buf_ptr;
/*---------------------------------------------------------------------------*/
static mqtt_client_config_t conf;
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
PROCESS(mqtt_client_process, "MQTT Client");
AUTOSTART_PROCESSES(&udp_client_process,&mqtt_client_process);
/*---------------------------------------------------------------------------*/
static int
construct_pub_topic(void)
{
	snprintf(pub_topic_encounter, BUFFER_SIZE, MQTT_PUBLISH_TOPIC_ENCOUNTER);
	snprintf(pub_topic_interest, BUFFER_SIZE, MQTT_PUBLISH_TOPIC_INTEREST); 	
	return 1;
}
/*---------------------------------------------------------------------------*/
static int
construct_sub_topic(void)
{
	snprintf(sub_topic_notification, BUFFER_SIZE,"%s/%d/json", MQTT_SUB_TOPIC_NOTIFICATION,node_id);
	return 1;
}
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
	const uip_ipaddr_t *sender_addr,
	uint16_t sender_port,
	const uip_ipaddr_t *receiver_addr,
	uint16_t receiver_port,
	const uint8_t *data,
	uint16_t datalen)
{
	int node_sender  = *(int *)data;
	int remaining = APP_BUFFER_SIZE;
	int len;
	uip_ipaddr_t  dest_ipaddr;
	LOG_INFO("Received BROADCAST %d ", node_sender);
	LOG_INFO_("\n");

	buf_ptr = app_buffer;

	len = snprintf(buf_ptr, remaining,
                 "{\"senderId\":%d,"
		 "\"contactId\":%d"
		 "}",node_sender,node_id
                 );
	
	buf_ptr += len;
	if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
		if(state == STATE_PUBLISHING){

			if(mqtt_ready(&conn)){
			 	mqtt_publish(&conn, NULL, pub_topic_encounter, (uint8_t *)app_buffer,strlen(app_buffer), MQTT_QOS_LEVEL_1, MQTT_RETAIN_OFF);
			}
		}
	}

  	LOG_INFO("Publish encounter sent out!\n");
}
/*---------------------------------------------------------------------------*/
static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
	switch(event) {
	case MQTT_EVENT_CONNECTED: {
		LOG_INFO("Application has a MQTT connection!\n");
		timer_set(&connection_life, CONNECTION_STABLE_TIME);
		state = STATE_CONNECTED;
		break;
  	}
	case MQTT_EVENT_DISCONNECTED: {
		LOG_INFO("MQTT Disconnect: reason %u\n", *((mqtt_event_t *)data));
		state = STATE_DISCONNECTED;
		process_poll(&mqtt_client_process);
		break;
  	}
	case MQTT_EVENT_PUBLISH: {
		msg_ptr = data;
		const char *a = (char *)msg_ptr->payload_chunk;
		
		const int i = strcspn(a,"}");
		char node[25];

		snprintf(node,25,"{\"targetId\": %d}",node_id); 

		if(strncmp(a,node,i)==0){
			if(msg_ptr->first_chunk) {
				msg_ptr->first_chunk = 0;
				LOG_INFO("Application received a publish on topic '%s': %s \n",msg_ptr->topic,node);
			}
		}
		break;
	}
	case MQTT_EVENT_SUBACK: {
		LOG_INFO("Application is subscribed to topic successfully\n");
		break;
	}
	case MQTT_EVENT_UNSUBACK: {
		LOG_INFO("Application is unsubscribed to topic successfully\n");
		break;
	}
	case MQTT_EVENT_PUBACK: {
		LOG_INFO("Publishing complete\n");
		break;
	}
	default:
		LOG_WARN("Application got a unhandled MQTT event: %i\n", event);
		break;
	}
}
/*---------------------------------------------------------------------------*/
static int
construct_client_id(void)
{
	int len = snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x",
		     conf.org_id, conf.type_id,
		     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
		     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
		     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
   
  
	LOG_INFO("Client ID: %s, Buffer %d\n", client_id, BUFFER_SIZE);
  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
	if(len < 0 || len >= BUFFER_SIZE) {
		LOG_INFO("Client ID: %d, Buffer %d\n", len, BUFFER_SIZE);
		return 0;
	}

  	return 1;
}
/*---------------------------------------------------------------------------*/
static void
update_config(void)
{
	if(construct_client_id() == 0) {
	/* Fatal error. Client ID larger than the buffer */
		state = STATE_CONFIG_ERROR;
		return;
	}
	if(construct_pub_topic() == 0) {
	/* Fatal error. Topic larger than the buffer */
		state = STATE_CONFIG_ERROR;
		return;
	}
	if(construct_sub_topic() == 0) {
	/* Fatal error. Topic larger than the buffer */
	state = STATE_CONFIG_ERROR;
	return;
	}
	state = STATE_INIT;

  /*
   * Schedule next timer event ASAP
   *
   * If we entered an error state then we won't do anything when it fires
   *
   * Since the error at this stage is a config error, we will only exit this
   * error state if we get a new config
   */
	etimer_set(&publish_periodic_timer, 0);

	return;
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
static void
subscribe(void)
{
	mqtt_status_t status;

	status = mqtt_subscribe(&conn, NULL, sub_topic_notification, MQTT_QOS_LEVEL_1);
	LOG_INFO("Subscribing\n");
	if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
		LOG_INFO("Tried to subscribe but command queue was full!\n");
	}
	return;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
init_config()
{
	/* Populate configuration with default values */
	memset(&conf, 0, sizeof(mqtt_client_config_t));

	memcpy(conf.org_id, DEFAULT_ORG_ID, strlen(DEFAULT_ORG_ID));
	memcpy(conf.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));
	memcpy(conf.auth_token, DEFAULT_AUTH_TOKEN, strlen(DEFAULT_AUTH_TOKEN));
	memcpy(conf.broker_ip, broker_ip, strlen(broker_ip));
	memcpy(conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);

	conf.broker_port = DEFAULT_BROKER_PORT;
	conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void
publish(void)
{
  /* Publish MQTT topic */
	int len;
	int remaining = APP_BUFFER_SIZE;
	uip_ipaddr_t  dest_ipaddr;


	buf_ptr = app_buffer;

	len = snprintf(buf_ptr, remaining,
			"{\"senderId\":%d}",node_id
                 	); 

	if(len < 0 || len >= remaining) {
		LOG_ERR("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
		return;
	}

	buf_ptr += len;
	if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
		mqtt_publish(&conn, NULL, pub_topic_interest, (uint8_t *)app_buffer,strlen(app_buffer), MQTT_QOS_LEVEL_1, MQTT_RETAIN_OFF);
	}

	LOG_INFO("Publish interest sent out!\n");
}
/*---------------------------------------------------------------------------*/
static void
connect_to_broker(void)
{
	/* Connect to MQTT server */
	mqtt_connect(&conn, conf.broker_ip, conf.broker_port,
	       conf.pub_interval * 3);

	state = STATE_CONNECTING;
}
/*---------------------------------------------------------------------------*/
static void
state_machine(void)
{

	switch(state) {
	case STATE_INIT:
		/* If we have just been configured register MQTT connection */
		mqtt_register(&conn, &mqtt_client_process, client_id, mqtt_event,
			  MAX_TCP_SEGMENT_SIZE);

		mqtt_set_username_password(&conn, MQTT_CLIENT_CONF_USERNAME,
				           conf.auth_token);

		/* _register() will set auto_reconnect; we don't want that */
		conn.auto_reconnect = 0;
		connect_attempt = 1;

		state = STATE_REGISTERED;
		LOG_INFO("Init\n");
		/* Continue */
	case STATE_REGISTERED:
	if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
		/* Registered and with a global IPv6 address, connect! */
		LOG_INFO("Joined network! Connect attempt %u\n", connect_attempt);
		connect_to_broker();
	}
	etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);
	return;
	break;
  	case STATE_CONNECTING:
    	/* Not connected yet. Wait */
    		LOG_INFO("Connecting: retry %u...\n", connect_attempt);
    		break;
  	case STATE_CONNECTED:
  	case STATE_PUBLISHING:
    	/* If the timer expired, the connection is stable */
    	if(timer_expired(&connection_life)) {
	/*
	* Intentionally using 0 here instead of 1: We want RECONNECT_ATTEMPTS
	* attempts if we disconnect after a successful connect
	*/
		connect_attempt = 0;
	}

	if(mqtt_ready(&conn) && conn.out_buffer_sent) {
      	/* Connected; publish */
		if(state == STATE_CONNECTED) {
			subscribe();
			state = STATE_PUBLISHING;
      		} else {  
      			publish();
		
      		}
	      	etimer_set(&publish_periodic_timer,  conf.pub_interval);

	      	LOG_INFO("Publishing\n");
	      /* Return here so we don't end up rescheduling the timer */
	      	return;
    	} else {
      /*
       * Our publish timer fired, but some MQTT packet is already in flight
       * (either not sent at all, or sent but not fully ACKd)
       *
       * This can mean that we have lost connectivity to our broker or that
       * simply there is some network delay. In both cases, we refuse to
       * trigger a new message and we wait for TCP to either ACK the entire
       * packet after retries, or to timeout and notify us
       */

		LOG_INFO("Publishing... (MQTT state=%d, q=%u)\n", conn.state,
        	conn.out_queue_full);

    	}	
   	break;
  	case STATE_DISCONNECTED:
    		LOG_INFO("Disconnected\n");
    		if(connect_attempt < RECONNECT_ATTEMPTS ||
       		RECONNECT_ATTEMPTS == RETRY_FOREVER) {
	      		/* Disconnect and backoff */
	      		clock_time_t interval;
	      		mqtt_disconnect(&conn);
	      		connect_attempt++;

	      		interval = connect_attempt < 3 ? RECONNECT_INTERVAL << connect_attempt :RECONNECT_INTERVAL << 3;

	      		LOG_INFO("Disconnected: attempt %u in %lu ticks\n", connect_attempt, interval);

	      		etimer_set(&publish_periodic_timer, interval);

	      		state = STATE_REGISTERED;
	      		return;
    		} else {
      			/* Max reconnect attempts reached; enter error state */
      			state = STATE_ERROR;
      			LOG_ERR("Aborting connection after %u attempts\n", connect_attempt - 1);
    		}
    		break;
	case STATE_CONFIG_ERROR:
		/* Idle away. The only way out is a new config */
		LOG_ERR("Bad configuration.\n");
		return;
  	case STATE_ERROR:
  	default:
   
		    /*
		     * 'default' should never happen
		     *
		     * If we enter here it's because of some error. Stop timers. The only thing
		     * that can bring us out is a new config event
		     */
		  LOG_INFO("Default case: State=0x%02x\n", state);
		  return;
	}

  		/* If we didn't return so far, reschedule ourselves */
  	etimer_set(&publish_periodic_timer, STATE_MACHINE_PERIODIC);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  	static struct etimer periodic_timer;
  	uip_ipaddr_t  broad_addr,dest_ipaddr;
  
  	PROCESS_BEGIN();
  	PROCESS_PAUSE();

   
  	simple_udp_register(&broadcast_connection,UDP_BROAD_PORT,NULL,
                      UDP_BROAD_PORT,udp_rx_callback);
  
  
  	etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    
    /* 1)join in rpl tree--2)know ip adress of root-- joinare nel tree non significa sapere dove è il root ma significa avere un parent --> avendo le due info puoi inviare un packet*/
    	if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      
		if(state == STATE_PUBLISHING ){
		//creazione link broad
			uip_create_linklocal_allnodes_mcast(&broad_addr);
                	if(mqtt_ready(&conn)){
				simple_udp_sendto(&broadcast_connection, &node_id, sizeof(node_id), &broad_addr);

				LOG_INFO("Sending BROADCAST:  %hi", node_id);
	      			LOG_INFO_("\n");
			}
		} 
    	}

    /* Add some jitter */
    	etimer_set(&periodic_timer, SEND_INTERVAL 
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  	}

  	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mqtt_client_process, ev, data)
{

  	PROCESS_BEGIN();
  
  	LOG_INFO("MQTT Demo Process\n");
  	init_config();
  	update_config();
  

  	// Main loop 
  	while(1) {
    	PROCESS_YIELD();

    		if (ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer ) {
			state_machine();
    		}
  }

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

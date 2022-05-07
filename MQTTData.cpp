/*
   Copyright (C) 2017, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include "MQTTData.h"

#include <assert.h>
#include <iostream>
#include <unistd.h>

void MQTTData::CallbackConnected(struct mosquitto *mosq, void *userdata, int result)
{
    assert(mosq);
	if(result == MOSQ_ERR_SUCCESS)
    {
        assert(userdata);
        ((MQTTData*)userdata)->OnConnected();
	}
    else
    {
		std::cout << "MQTT Error: Failed to connect\n";
	}
}

void MQTTData::CallbackMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    assert(mosq);
    assert(message);
	if(message && message->payloadlen)
    {
        assert(userdata);

        const std::string topic = message->topic;
        const std::string data = ((const char*)message->payload);

        ((MQTTData*)userdata)->mOnData(topic,data);

	}
}

static void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++)
    {
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

static void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    assert(mosq);
	// Print all log messages regardless of level.
#ifdef DEBUG_BUILD
    if( str )
    {
	    std::cout << str << "\n";
    }
#endif
}

MQTTData::MQTTData(const char* pHost,int pPort,
    const std::vector<std::string>& pTopics,
    std::function<void(const std::string &pTopic,const std::string &pData)> pOnData):
    mTopics(pTopics),
    mOnData(pOnData)
{
    assert(pHost && pPort);

	int keepalive = 60;
	bool clean_session = true;

	mosquitto_lib_init();
	mMQTT = mosquitto_new(NULL, clean_session, this);
	if( mMQTT )
    {
        mosquitto_log_callback_set(mMQTT, my_log_callback);
        mosquitto_connect_callback_set(mMQTT, CallbackConnected);
        mosquitto_message_callback_set(mMQTT, CallbackMessage);
        mosquitto_subscribe_callback_set(mMQTT, my_subscribe_callback);

        while( mosquitto_connect(mMQTT, pHost, pPort, keepalive) != MOSQ_ERR_SUCCESS )
        {
            // Try again in a bit.
            sleep(5);
        }

        // Now start the loop.
        if( mosquitto_loop_start(mMQTT) == MOSQ_ERR_SUCCESS )
        {
            mOk = true;
        }
        else
        {
            std::cout << "MQTT Init Error: Failed to start networking loop\n";
        }
    }
    else
    {
		std::cout << "MQTT Init Error: Out of memory\n";
	}
}

MQTTData::~MQTTData()
{
    mosquitto_disconnect(mMQTT);
    if( mosquitto_loop_stop(mMQTT,false) != MOSQ_ERR_SUCCESS )
    {
        mosquitto_loop_stop(mMQTT,true);// Force the close.
    }
	mosquitto_destroy(mMQTT);
	mosquitto_lib_cleanup();
}

void MQTTData::OnConnected()
{
    // Subscribe to broker information topics on successful connect.
    for( auto topic : mTopics )
    {
        Subscribe(topic);
    }
}

void MQTTData::Subscribe(const std::string& pTopic)
{
    assert( pTopic.size() > 0 );

    switch( mosquitto_subscribe(mMQTT, NULL, pTopic.c_str(), MQTT_QOS_AT_LEAST_ONCE) )
    {
    case MOSQ_ERR_SUCCESS:
#ifdef DEBUG_BUILD
         std::cout << "Subscribing too " << pTopic << "\n";
#endif
        break;

    case MOSQ_ERR_INVAL:
        std::cout << "MQTT Subscribe Error: [" << pTopic << "] The input parameters were invalid";
        break;

    case MOSQ_ERR_NOMEM:
        std::cout << "MQTT Subscribe Error: [" << pTopic << "]an out of memory condition occurred\n";
        break;

    case MOSQ_ERR_NO_CONN:
        std::cout << "MQTT Subscribe Error: [" << pTopic << "]the client isn't connected to a broker.\n";
        break;

    case MOSQ_ERR_MALFORMED_UTF8:
        std::cout << "MQTT Subscribe Error: [" << pTopic << "]the topic is not valid UTF-8\n";
        break;
#if LIBMOSQUITTO_REVISION > 7
    case MOSQ_ERR_OVERSIZE_PACKET:
        std::cout << "MQTT Subscribe Error: [" << pTopic << "] Over sized packet\n";
        break;
#endif
    }
}

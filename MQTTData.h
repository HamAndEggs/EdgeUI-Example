/*
   Copyright (C) 2021, Richard e Collins.

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
   
#ifndef MQTTDATA_H
#define MQTTDATA_H

#include <mosquitto.h> // sudo apt install libmosquitto-dev

#define MQTT_QOS_AT_MOST_ONCE   0 // The minimal QoS level is zero. This service level guarantees a best-effort delivery. There is no guarantee of delivery.
#define MQTT_QOS_AT_LEAST_ONCE  1 // QoS level 1 guarantees that a message is delivered at least one time to the receiver. 
#define MQTT_QOS_EXACTLY_ONCE   2 // QoS 2 is the highest level of service in MQTT. This level guarantees that each message is received only once by the intended recipients.



#include <vector>
#include <string>
#include <functional>

class MQTTData
{
public:
    MQTTData(const char* pHost,int pPort,
        const std::vector<std::string>& pTopics,
        std::function<void(const std::string &pTopic,const std::string &pData)> pOnData);

    ~MQTTData();

    bool GetOK()const{return mOk;}


private:
    const std::vector<std::string> mTopics;
    std::function<void(const std::string &pTopic,const std::string &pData)> mOnData;
    struct mosquitto *mMQTT = NULL;
    bool mOk = false;

    static void CallbackConnected(struct mosquitto *mosq, void *userdata, int result);
    static void CallbackMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

    void OnConnected();

    void Subscribe(const std::string& pTopic);
};

#endif //#ifndef MQTTDATA_H
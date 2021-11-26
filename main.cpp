/*
 * Copyright 2013-2020 Automatak, LLC
 *
 * Licensed to Green Energy Corp (www.greenenergycorp.com) and Automatak
 * LLC (www.automatak.com) under one or more contributor license agreements.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership. Green Energy Corp and Automatak LLC license
 * this file to you under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may obtain
 * a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unordered_map>
#include <string>
#include <stdlib.h>
#include <opendnp3/ConsoleLogger.h>
#include <opendnp3/DNP3Manager.h>
#include <opendnp3/channel/PrintingChannelListener.h>
#include <opendnp3/logging/LogLevels.h>
#include <opendnp3/master/DefaultMasterApplication.h>
#include <opendnp3/master/PrintingCommandResultCallback.h>
#include <opendnp3/master/PrintingSOEHandler.h>

#include "lib/include/rapidjson/document.h"
#include "lib/include/rapidjson/writer.h"
#include "lib/include/rapidjson/stringbuffer.h"
#include "lib/include/rapidjson/pointer.h"
#include "lib/include/rapidjson/plugin_api.h"

using namespace std;
using namespace opendnp3;
using namespace rapidjson;

#define DNP3_MAP	QUOTE({					            \
		"values" : [					                \
			    {  					                    \
				    "name"          : "temperature",	\
				    "out-station"   : 1,		        \
				    "assetName"     : "Booth1",	        \
				    "register"      : 102,		        \
				    "scale"         : 0.1,	            \
				    "offset"        : 0.0	        	\
			    },					                    \
			    { 					                    \
				    "name"          : "humidity",	    \
				    "register"      : 109,   	        \
                    "scale"         : 0.1,	            \
				    "offset"        : 0.0	        	\
		       	}					                    \
			  ]					                        \
		})

class TestSOEHandler : public ISOEHandler
{
    public: 
        TestSOEHandler() {
            string def_cfg = QUOTE({
                "plugin" : {
                    "description" : "Modbus TCP and RTU C south plugin",
                    "type" : "string",
                    "default" : "ModbusC",
                    "readonly": "true"
                    },
                "asset" : {
                    "description" : "Default asset name",
                    "type" : "string",
                    "default" : "modbus", 
                    "order": "1",
                    "displayName": "Asset Name",
                    "mandatory": "true"
                    }, 
                "protocol" : {
                    "description" : "Protocol",
                    "type" : "enumeration",
                    "default" : "RTU", 
                    "options" : [ "RTU", "TCP"], 
                    "order": "2",
                    "displayName": "Protocol"
                    }, 
                "address" : {
                    "description" : "Address of Modbus TCP server", 
                    "type" : "string",
                    "default" : "127.0.0.1", 
                    "order": "3",
                    "displayName": "Server Address",
                    "validity": "protocol == \"TCP\""
                    },
                "port" : {
                    "description" : "Port of Modbus TCP server", 
                    "type" : "integer",
                    "default" : "2222", 
                    "order": "4",
                    "displayName": "Port",
                    "validity" : "protocol == \"TCP\""
                    },
                "device" : {
                    "description" : "Device for Modbus RTU",
                    "type" : "string",
                    "default" : "",
                    "order": "5",
                    "displayName": "Device",
                    "validity" : "protocol == \"RTU\""
                    },
                "baud" : {
                    "description" : "Baud rate  of Modbus RTU",
                    "type" : "integer",
                    "default" : "9600",
                    "order": "6",
                    "displayName": "Baud Rate",
                    "validity" : "protocol == \"RTU\""
                    },
                "bits" : {
                    "description" : "Number of data bits for Modbus RTU",
                    "type" : "integer",
                    "default" : "8",
                    "order": "7",
                    "displayName": "Number Of Data Bits",
                    "validity" : "protocol == \"RTU\""
                    },
                "stopbits" : {
                    "description" : "Number of stop bits for Modbus RTU",
                    "type" : "integer",
                    "default" : "1",
                    "order": "8",
                    "displayName": "Number Of Stop Bits",
                    "validity" : "protocol == \"RTU\""
                    },
                "parity" : {
                    "description" : "Parity to use",
                    "type" : "enumeration",
                    "default" : "none",
                    "options" : [ "none", "odd", "even" ],
                    "order": "9",
                    "displayName": "Parity",
                    "validity" : "protocol == \"RTU\""
                    },
                "slave" : {
                    "description" : "The Modbus device default slave ID",
                    "type" : "integer",
                    "default" : "1",
                    "order": "10",
                    "displayName": "Slave ID"
                    },
                "map" : {
                    "description" : "Modbus register map",
                    "order": "11",
                    "displayName": "Register Map", 
                    "type" : "JSON",
                    "default" : DNP3_MAP
                    },
                "timeout" : {
                    "description" : "Modbus request timeout",
                    "type" : "float",
                    "default" : "0.5",
                    "order": "12",
                    "displayName": "Timeout",
                    "validity" : "protocol == \"TCP\""
                    },
                "control" : {
                    "description" : "Modbus request timeout",
                    "type" : "enumeration",
                    "default" : "None",
                    "order": "13",
                    "options" : [ "None", "Use Register Map", "Use Control Map" ],
                    "displayName": "Control"
                    }
                });

            std::unordered_map<std::string, int> readings_name_and_register;
            Document document;

            if (document.Parse(def_cfg.c_str()).HasParseError()){
                cout << "Parse Error" << endl;
                //return 1;
            }

            auto map = document["map"].GetObject();
            assert(map.HasMember("default"));

            auto default_string = map["default"].GetString();
            cout << default_string  << endl;
            Document d2;
            if(d2.Parse(default_string).HasParseError()){
                cout << "Parse error in default" << endl;
                //return 2;
            }

            auto values = d2["values"].GetArray();
            
            // cycle through values and build hasmap of reading name and register
            for (auto& m : values){
                auto reading_name = m["name"].GetString();
                auto register_num = m["register"].GetInt();
                readings_name_and_register[reading_name] = register_num;
            }

            for (auto& tup : readings_name_and_register)
            {
                cout << tup.first << " : " << tup.second << endl;
            }

            auto value1 = d2["values"][0].GetObj();
            auto name = value1["name"].GetString();
                    cout << "SOE Handler Created" << endl;
        }
    private: 

        virtual void BeginFragment(const ResponseInfo& info){};
        virtual void EndFragment(const ResponseInfo& info){};

        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<Binary>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<DoubleBitBinary>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<Analog>>& values) {
            auto print = [](const Indexed<Analog>& pair) {
                double v = pair.value.value;
                //std::cout << "[" << pair.index << "] : " << to_string(pair.value.value) << std::endl;
                if (pair.index == 3) {
                    cout << "[" << pair.index << "] : " << to_string(pair.value.value) << std::endl;
                }
                else {
                    std::cout << "[" << pair.index << "] : " << "Not printed" << std::endl; 
                }
            };

            
            values.ForeachItem(print); 
            //cout << "Values at position 3 " << pair.value.value << endl;
            cout << "Count : " << values.Count() << endl;

            
            //Indexed<Analog>& new_index = [];
            //Indexed<Analog>& new_val = values.ReadOnlyValue();
            //cout << "Value [3] = " << values.value[0].value[3] << endl;
            
        };
        
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<Counter>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<FrozenCounter>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<BinaryOutputStatus>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<AnalogOutputStatus>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<OctetString>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<TimeAndInterval>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<BinaryCommandEvent>>& values) {};
        virtual void Process(const HeaderInfo& info, const ICollection<Indexed<AnalogCommandEvent>>& values) {};    
        virtual void Process(const HeaderInfo& info, const ICollection<DNPTime>& values) {};
};

int main(int argc, char* argv[])
{
    // Specify what log levels to use. NORMAL is warning and above
    // You can add all the comms logging by uncommenting below
    const auto logLevels = levels::NORMAL | levels::ALL_APP_COMMS;

    // This is the main point of interaction with the stack
    DNP3Manager manager(1, ConsoleLogger::Create());

    // Connect via a TCPClient socket to a outstation
    auto channel = manager.AddTCPClient("tcpclient", logLevels, ChannelRetry::Default(), {IPEndpoint("127.0.0.1", 20000)},
                                        "0.0.0.0", PrintingChannelListener::Create());

    // The master config object for a master. The default are
    // useable, but understanding the options are important.
    MasterStackConfig stackConfig;

    // you can override application layer settings for the master here
    // in this example, we've change the application layer timeout to 2 seconds
    stackConfig.master.responseTimeout = TimeDuration::Seconds(2);
    stackConfig.master.disableUnsolOnStartup = true;

    // You can override the default link layer settings here
    // in this example we've changed the default link layer addressing
    stackConfig.link.LocalAddr = 1;
    stackConfig.link.RemoteAddr = 10;

    // Create a new master on a previously declared port, with a
    // name, log level, command acceptor, and config info. This
    // returns a thread-safe interface used for sending commands.
    auto master = channel->AddMaster("master",                           // id for logging
                                     PrintingSOEHandler::Create(),       // callback for data processing
                                     DefaultMasterApplication::Create(), // master application instance
                                     stackConfig                         // stack configuration
    );

    auto test_soe_handler = std::make_shared<TestSOEHandler>();

    // do an integrity poll (Class 3/2/1/0) once per minute
    auto integrityScan = master->AddClassScan(ClassField::AllClasses(), TimeDuration::Minutes(1), test_soe_handler);

    // do a Class 1 exception poll every 5 seconds
    auto exceptionScan = master->AddClassScan(ClassField(ClassField::CLASS_1), TimeDuration::Seconds(2), test_soe_handler);

    // Enable the master. This will start communications.
    master->Enable();

    bool channelCommsLoggingEnabled = true;
    bool masterCommsLoggingEnabled = true;


    while (true)
    {
        std::cout << "Enter a command" << std::endl;
        std::cout << "x - exits program" << std::endl;
        std::cout << "a - performs an ad-hoc range scan" << std::endl;
        std::cout << "i - integrity demand scan" << std::endl;
        std::cout << "e - exception demand scan" << std::endl;
        std::cout << "d - disable unsolicited" << std::endl;
        std::cout << "r - cold restart" << std::endl;
        std::cout << "c - send crob" << std::endl;
        std::cout << "t - toggle channel logging" << std::endl;
        std::cout << "u - toggle master logging" << std::endl;

        char cmd;
        std::cin >> cmd;
        switch (cmd)
        {
        case ('a'):
            master->ScanRange(GroupVariationID(1, 2), 0, 3, test_soe_handler);
            break;
        case ('d'):
            master->PerformFunction("disable unsol", FunctionCode::DISABLE_UNSOLICITED,
                                    {Header::AllObjects(60, 2), Header::AllObjects(60, 3), Header::AllObjects(60, 4)});
            break;
        case ('r'):
        {
            auto print = [](const RestartOperationResult& result) {
                if (result.summary == TaskCompletion::SUCCESS)
                {
                    std::cout << "Success, Time: " << result.restartTime.ToString() << std::endl;
                }
                else
                {
                    std::cout << "Failure: " << TaskCompletionSpec::to_string(result.summary) << std::endl;
                }
            };
            master->Restart(RestartType::COLD, print);
            break;
        }
        case ('x'):
            // C++ destructor on DNP3Manager cleans everything up for you
            return 0;
        case ('i'):
            integrityScan->Demand();
            break;
        case ('e'):
            exceptionScan->Demand();
            break;
        case ('c'):
        {
            ControlRelayOutputBlock crob(OperationType::LATCH_ON);
            master->SelectAndOperate(crob, 0, PrintingCommandResultCallback::Get());
            break;
        }
        case ('t'):
        {
            channelCommsLoggingEnabled = !channelCommsLoggingEnabled;
            auto levels = channelCommsLoggingEnabled ? levels::ALL_COMMS : levels::NORMAL;
            channel->SetLogFilters(levels);
            std::cout << "Channel logging set to: " << levels.get_value() << std::endl;
            break;
        }
        case ('u'):
        {
            masterCommsLoggingEnabled = !masterCommsLoggingEnabled;
            auto levels = masterCommsLoggingEnabled ? levels::ALL_COMMS : levels::NORMAL;
            master->SetLogFilters(levels);
            std::cout << "Master logging set to: " << levels.get_value() << std::endl;
            break;
        }
        default:
            std::cout << "Unknown action: " << cmd << std::endl;
            break;
        }
    }

    return 0;
}

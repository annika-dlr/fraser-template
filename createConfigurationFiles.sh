#!/bin/bash

~/tmp_simulation/models/configuration_server/build/bin/configuration_server --config-file ~/tmp_simulation/hosts_config.xml &
~/tmp_simulation/models/router_0/build/bin/router_0 2>> ~/tmp_simulation/models/router_0/log.txt &
~/tmp_simulation/models/router_1/build/bin/router_1 2>> ~/tmp_simulation/models/router_1/log.txt &
~/tmp_simulation/models/router_2/build/bin/router_2 2>> ~/tmp_simulation/models/router_2/log.txt &
~/tmp_simulation/models/router_3/build/bin/router_3 2>> ~/tmp_simulation/models/router_3/log.txt &
~/tmp_simulation/models/simulation_model/build/bin/simulation_model --create-config-files ../configurations/config_0/
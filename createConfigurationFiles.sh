#!/bin/bash

models/configuration_server/build/bin/configuration_server --config-file hosts-configs/config0.xml &
models/event_queue_1/build/bin/event_queue_1 &
models/router_0/build/bin/router_0 &
models/router_1/build/bin/router_1 &
models/router_2/build/bin/router_2 &
models/router_3/build/bin/router_3 &
models/simulation_model/build/bin/simulation_model --create-config-files configurations/config_0/
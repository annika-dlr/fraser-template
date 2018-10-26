#!/bin/bash

models/configuration_server/build/bin/configuration_server --config-file hosts-configs/config0.xml &
models/router/build/bin/router -n router_0 &
models/router/build/bin/router -n router_1 &
models/router/build/bin/router -n router_2 &
models/router/build/bin/router -n router_3 &
models/processing_element/build/bin/processing_element -n processing_element_1 &
models/processing_element/build/bin/processing_element -n processing_element_2 &
models/processing_element/build/bin/processing_element -n processing_element_3 &
models/systemc_adapter/build/bin/systemc_adapter &
models/simulation_model/build/bin/simulation_model --load-config configurations/config_0/

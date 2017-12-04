#!/bin/bash

../models/configuration_server/build/bin/configuration_server &
../models/event_queue_1/build/bin/event_queue_1 &
../models/model_1/build/bin/model_1 &
../models/model_2/build/bin/model_2 &
../models/simulation_model/build/bin/simulation_model --load-config ../configurations/config_1/

/*
 * Copyright (c) 2018, German Aerospace Center (DLR)
 *
 * This file is part of the development version of FRASER.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2018, Annika Ofenloch (DLR RY-AVS)
 */

#include "SystemcAdapter.h"
#include "proc_elem.h"

SC_MODULE(Top) {
	sc_proc_elem *my_proc_elem;
	SystemcAdapter *sc_fraser_adapter;

	SC_CTOR(Top) {
		// instance of processing-element
		sc_fraser_adapter = new SystemcAdapter("systemc_adapter_0",
						"Forward SystemC events", "sc_wrapper");

		my_proc_elem = new sc_proc_elem("my_proc_elem");

		my_proc_elem->init_socket.bind(sc_fraser_adapter->mTargetSocket);
		sc_fraser_adapter->mInitMemorySocket.bind(my_proc_elem->tgt_memory_socket);
		sc_fraser_adapter->mInitCreditCntSocket.bind(my_proc_elem->tgt_credit_cnt_socket);
		sc_fraser_adapter->mInitInterruptSocket.bind(my_proc_elem->tgt_interrupt_socket);

		std::cout << "After socket bindings ... " << std::endl;
	}
};

int sc_main(int argc, char* argv[]) {

	Top top("top");
	sc_start();

	return 0;
}

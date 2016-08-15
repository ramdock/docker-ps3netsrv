#define SC_GET_IDPS 					(870)
#define SC_GET_PSID 					(872)

uint64_t idps_offset1=0;
uint64_t idps_offset2=0;
uint64_t psid_offset=0;

uint64_t eid0_idps[2];

uint64_t IDPS[2] = {0, 0};
uint64_t PSID[2] = {0, 0};

static void get_idps_psid(void)
{
	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	if(c_firmware<=4.53f)
	{
		{system_call_1(SC_GET_IDPS, (uint64_t) IDPS);}
		{system_call_1(SC_GET_PSID, (uint64_t) PSID);}
	}
	else if(peekq(TOC)==SYSCALLS_UNAVAILABLE)
		return; // do not update IDPS/PSID if syscalls are removed
	else if(idps_offset2 | psid_offset)
	{
			IDPS[0] = peekq(idps_offset2  );
			IDPS[1] = peekq(idps_offset2+8);
			PSID[0] = peekq(psid_offset   );
			PSID[1] = peekq(psid_offset +8);
	}

	{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
}

#ifdef SPOOF_CONSOLEID
static void spoof_idps_psid(void)
{
	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	if(webman_config->spsid)
	{
		uint64_t j, newPSID[2] = {0, 0};

		newPSID[0] = convertH(webman_config->vPSID1);
		newPSID[1] = convertH(webman_config->vPSID2);

		//if(newPSID[0] != 0 && newPSID[1] != 0)
		{
			if(c_firmware<=4.53f)
			{
				{system_call_1(SC_GET_PSID, (uint64_t) PSID);}
				for(j = 0x8000000000300000ULL; j < 0x8000000000600000ULL; j+=4) {
					if((peekq(j) == PSID[0]) && (peekq(j+8) == PSID[1])) {
						pokeq(j, newPSID[0]); j+=8;
						pokeq(j, newPSID[1]); j+=8;
					}
				}
			}
			else if(psid_offset)
			{
				pokeq(psid_offset  , newPSID[0]);
				pokeq(psid_offset+8, newPSID[1]);
			}
		}
	}

	if(webman_config->sidps)
	{
		uint64_t addr, newIDPS[2] = {0, 0};

		newIDPS[0] = convertH(webman_config->vIDPS1);
		newIDPS[1] = convertH(webman_config->vIDPS2);

		if(newIDPS[0] != 0 && newIDPS[1] != 0)
		{
			if(c_firmware<=4.53f)
			{
				{system_call_1(SC_GET_IDPS, (uint64_t) IDPS);}
				for(addr = 0x8000000000300000ULL; addr < 0x8000000000600000ULL; addr+=4)
				{
					if((peekq(addr) == IDPS[0]) && (peekq(addr + 8) == IDPS[1]))
					{
						pokeq(addr, newIDPS[0]); addr+=8;
						pokeq(addr, newIDPS[1]); addr+=8;
					}
				}
			}
			else if(idps_offset1 | idps_offset2)
			{
				pokeq(idps_offset1  , newIDPS[0]);
				pokeq(idps_offset1+8, newIDPS[1]);
				pokeq(idps_offset2  , newIDPS[0]);
				pokeq(idps_offset2+8, newIDPS[1]);
			}
		}
	}

	get_idps_psid();
}
#endif

static void get_eid0_idps(void)
{
	uint64_t buffer[0x40], start_sector;
	uint32_t read;
	sys_device_handle_t source;
	if(sys_storage_open(0x100000000000004ULL, 0, &source, 0) != 0)
	{
		start_sector = 0x204;
		sys_storage_close(source);
		sys_storage_open(0x100000000000001ULL, 0, &source, 0);
	}
	else start_sector = 0x178;
	sys_storage_read(source, 0, start_sector, 1, buffer, &read, 0);
	sys_storage_close(source);

	eid0_idps[0] = buffer[0x0E];
	eid0_idps[1] = buffer[0x0F];
}

static void show_idps(char *msg)
{
	get_eid0_idps();
	get_idps_psid();

	#define SEP "\n                  "
	sprintf((char*) msg, "IDPS EID0 : %016llX%s"
									 "%016llX\n"
						 "IDPS LV2  : %016llX%s"
									 "%016llX\n"
						 "PSID LV2 : %016llX%s"
									"%016llX", eid0_idps[0], SEP, eid0_idps[1], IDPS[0], SEP, IDPS[1], PSID[0], SEP, PSID[1]);
	#undef SEP

	show_msg((char*) msg);
	sys_timer_sleep(2);
}
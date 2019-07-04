////
//// Created by wowa on 14.06.19.
////
////#include <string.h>
//#include "lwipopts.h"
//#include "lwip/apps/snmp.h"
//#include "ubirch_snmp_agent.h"
//
//void init_SNMPAgent(bool debug){
////	u8_t *ocstr = (u8_t*)"test";
////	u16_t ocstrlen;
////	ocstrlen = strlen((const char *) ocstr);
////	u16_t bufsize = 128;
////
////	snmp_mib2_set_syscontact(ocstr, &ocstrlen, bufsize);
////	snmp_mib2_set_syslocation();
////	snmp_set_auth_traps_enabled(1);
////
////	snmp_mib2_set_sysdescr();
////	snmp_set_device_enterprise_oid(NULL);
////	snmp_mib2_set_sysname();
////
////
////	snmp_init();
//}
//
//struct snmp_obj_id sysupid = {9,{1,3,6,1,2,1,1,3,0}};
//struct snmp_obj_id trapoid = {11,{1,3,6,1,6,3,1,1,4,1,0}};
//struct snmp_obj_id pttnotifyoid = {8,{1,3,6,1,4,UBIRCH_PEN,3,18}};
//static unsigned char trapOID[10] = { 0x2b, 6, 1, 4, 1, 0x82, 0xe4, 0x3d, 3, 18};
//struct snmp_varbind *vb1, *vb2, *vb3;
//u32_t *u32ptr, sysuptime;
//
//void vSendTrapTaskDemo( void ){
//	snmp_varbind_list_free(&trap_msg.outvb);
//	vb1 = snmp_varbind_alloc(&sysupid,SNMP_ASN1_TIMETICKS, 4);
//	snmp_get_sysuptime(&sysuptime);
//	vb1->value_len=4;
//	vb1->value_type=0x43;  //Timerticks
//	u32ptr=vb1->value;
//	*u32ptr=sysuptime;
//	snmp_varbind_tail_add(&trap_msg.outvb,vb1);
//
//	vb2 = snmp_varbind_alloc(&trapoid,SNMP_ASN1_OBJ_ID, 11);
//	memcpy (vb2->value, trapOID, 10);
//	snmp_varbind_tail_add(&trap_msg.outvb,vb2);
//
//	vb3 = snmp_varbind_alloc(&pttnotifyoid, SNMP_ASN1_COUNTER, 4);
//	vb3->value_len=4;
//	vb3->value_type=0x02; //Integer32
//	u32ptr=vb3->value;
//	*u32ptr=1;
//	snmp_varbind_tail_add(&trap_msg.outvb,vb3);
//
//	snmp_send_trap(SNMP_GENTRAP_ENTERPRISESPC, &sysupid,18);
//	snmp_varbind_list_free(&trap_msg.outvb);
//}
//void snmp_test(){
//	LWIP_SNMP
//}

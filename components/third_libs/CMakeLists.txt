set(incs monitor third_list tlv_protocol)
set(srcs "monitor/monitor.c"
		 "third_list/utils_list.c"
		 "tlv_protocol/tlv_protocol.c")






idf_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}")
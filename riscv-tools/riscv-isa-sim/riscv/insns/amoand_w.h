require_extension('A');
reg_t v = MMU.load_int32(RS1.data);
MMU.store_uint32(RS1.data, RS2.data & v.data);
WRITE_RD(v);

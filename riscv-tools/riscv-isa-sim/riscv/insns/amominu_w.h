require_extension('A');
reg_t v = MMU.load_int32(RS1.data);
MMU.store_uint32(RS1.data, std::min(uint32_t(RS2.data),(uint32_t)(v.data)));
WRITE_RD(v);

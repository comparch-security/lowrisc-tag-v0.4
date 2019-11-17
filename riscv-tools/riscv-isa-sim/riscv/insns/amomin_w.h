require_extension('A');
reg_t v = MMU.load_int32(RS1.data);
MMU.store_uint32(RS1.data, std::min(int32_t(RS2.data),(int32_t)(v.data)));
WRITE_RD(v);

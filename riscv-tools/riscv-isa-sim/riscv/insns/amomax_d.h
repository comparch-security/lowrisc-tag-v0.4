require_extension('A');
require_rv64;
reg_t v = MMU.load_int64(RS1.data);
MMU.store_uint64(RS1.data, std::max(sword_t(RS2.data),sword_t(v.data)));
WRITE_RD(v);

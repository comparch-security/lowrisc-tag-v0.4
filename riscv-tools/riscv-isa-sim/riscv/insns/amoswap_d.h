require_extension('A');
require_rv64;
reg_t v = MMU.load_uint64(RS1.data);
MMU.store_uint64(RS1.data, RS2.data);
WRITE_RD(v);

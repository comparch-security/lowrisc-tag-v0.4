require_extension('A');
p->get_state()->load_reservation = RS1.data;
WRITE_RD(MMU.load_int32(RS1.data));

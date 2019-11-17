require_extension('A');
require_rv64;
if (RS1.data == p->get_state()->load_reservation)
{
  MMU.store_uint64(RS1.data, RS2);
  WRITE_RD(reg_t(0));
}
else
  WRITE_RD(reg_t(1));

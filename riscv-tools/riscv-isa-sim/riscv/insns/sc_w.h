require_extension('A');
if (RS1.data == p->get_state()->load_reservation)
{
  MMU.store_uint32(RS1.data, RS2);
  WRITE_RD(reg_t(0));
}
else
  WRITE_RD(reg_t(1));

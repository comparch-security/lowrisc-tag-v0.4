require_extension('C');
require_rv64;
WRITE_RVC_RS1S(reg_t(sext32(RVC_RS1S.data - RVC_RS2S.data)));

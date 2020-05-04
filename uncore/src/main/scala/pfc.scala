// See LICENSE for license details.

package uncore

import Chisel._
import cde.{Parameters, Field}

case object PFCL2N extends Field[Int]

trait HasPFCParameters extends HasTagParameters {
  implicit val p: Parameters
  val L2DBanks = p(PFCL2N)
}

abstract class PFCModule(implicit val p: Parameters) extends Module with HasPFCParameters
abstract class PFCBundle(implicit val p: Parameters) extends Bundle with HasPFCParameters

object PerFormanceCounter {
  def apply(cond: Bool, n: Int): UInt = {
    val c = new Counter(n)
    var wrap: Bool = null
    when (cond) { wrap = c.inc() }
    c.value
  }
}

class L1ICachePerform extends Bundle {
  val read = Bool(INPUT)
  val read_miss = Bool(INPUT)
}

class L1DCachePerform extends Bundle {
  val read = Bool(INPUT)
  val read_miss = Bool(INPUT)
  val write =Bool(INPUT)
  val write_miss = Bool(INPUT)
  val write_back = Bool(INPUT)
}

class L2CachePerform extends Bundle {
  val read = Bool(INPUT) //inner.acquire
  val read_miss = Bool(INPUT) //outer.acquire
  val write =Bool(INPUT) //inner.release
  //val write_miss =Bool(INPUT) //
  val write_back = Bool(INPUT) //outer.release
}

class L2CachePerformCounter extends Bundle {
   val read       = UInt(width=64)
   val read_miss  = UInt(width=64)
   val write      = UInt(width=64)
   //val write_miss = UInt(width=64)
   val write_back = UInt(width=64)
}

class TCTAGTrackerPerform extends Bundle {
  val MR  = Bool(INPUT)        //read meta
  val MW  = Bool(INPUT)        //update the meta
  val DR  = Bool(INPUT)        //read tag from data array
  val DW  = Bool(INPUT)        //write tag to data array //equals DWR+DWB
  val WB  = Bool(INPUT)        //write dirty line back to memory
  val F   = Bool(INPUT)        //fetch the target cache line (from memory) // equals DWB
}

class TCTAGTrackerPerformCounter extends Bundle {
  val MR  = UInt(width=64)        //read meta
  val MW  = UInt(width=64)        //update the meta
  val DR  = UInt(width=64)        //read tag from data array
  val DW  = UInt(width=64)        //write tag to data array //equals DWR+DWB
  val WB  = UInt(width=64)        //write dirty line back to memory
  val F   = UInt(width=64)        //fetch the target cache line (from memory) // equals DWB
}

class TCMEMTrackerPerform extends Bundle {
  val readTT           = Bool(INPUT)
  val readTT_miss      = Bool(INPUT)
  val writeTT          = Bool(INPUT)
  val writeTT_miss     = Bool(INPUT)
  val readTM0          = Bool(INPUT)
  val readTM0_miss     = Bool(INPUT)
  val writeTM0         = Bool(INPUT)
  val writeTM0_miss    = Bool(INPUT)
  val readTM1          = Bool(INPUT)
  val readTM1_miss     = Bool(INPUT)
  val writeTM1         = Bool(INPUT)
  val writeTM1_miss    = Bool(INPUT)
}

class TCMEMTrackerPerformCounter extends Bundle {
  val  readTT          = UInt(width=64)
  val  readTT_miss     = UInt(width=64)
  val  writeTT         = UInt(width=64)
  val  writeTT_miss    = UInt(width=64)
  val  readTM0         = UInt(width=64)
  val  readTM0_miss    = UInt(width=64)
  val  writeTM0        = UInt(width=64)
  val  writeTM0_miss   = UInt(width=64)
  val  readTM1         = UInt(width=64)
  val  readTM1_miss    = UInt(width=64)
  val  writeTM1        = UInt(width=64)
  val  writeTM1_miss   = UInt(width=64)
}

class TAGCachePerform extends Bundle {
  val tcttp = new TCTAGTrackerPerform()//
  val tcmtp = new TCMEMTrackerPerform()
}

class PrivatePerform extends Bundle {
  val L1I = new L1ICachePerform()
  val L1D = new L1DCachePerform()
}

//class SharePerform(implicit val p: Parameters) extends PFCBundle { //WRONG
//  val L2D = Vec(L2DBanks, new L2DCachePerform)
class SharePerform(implicit val p: Parameters) extends Bundle {
  val L2D = Vec(p(PFCL2N), new L2CachePerform)
  val TAG = new TAGCachePerform()
}

class PFCReq extends Bundle {
 val cmd      = Bits(width=2) //00:read 01:acquire
 val addr     = Bits(width=6)
 val groupID  = Bits(width=4) //0001:PrivatePFC 0010:SharePFC others:reserve
 val subGroID = Bits(width=4)
 //for PrivatePFC 0001:L1I 0010:L1D
 //for SharePFC   0001:L2  0010:TC
}

class PFCResp extends Bundle {
  val data = UInt(width=64)
}

class PrivatePFC extends Module {
  val io = new Bundle {
    val req = Valid(new PFCReq().asInput) //req from csr
    val resp = Valid(new PFCResp().asOutput) //resp to csr
    val update = new PrivatePerform()
  }

  val groupMatch = io.req.valid && io.req.bits.groupID === UInt(1) //4'b0001
  val groupisL1I = groupMatch && io.req.bits.subGroID === UInt(1) //4'b0001
  val groupisL1D = groupMatch && io.req.bits.subGroID === UInt(2) //4'b0010
  val read = groupMatch && io.req.bits.cmd === UInt(0)
  val acquire = groupMatch && io.req.bits.cmd === UInt(1)

  //import rocket.WideCounter //can't import
  //val L1I_PFC_read = WideCounter(64, io.update.L1I.read)
  //val (L1IPFC_read, full0)     = Counter(io.update.L1I.read,         2^64-1) //wrong why??!!
  val l1ipfc_read           = PerFormanceCounter(io.update.L1I.read,         2^64-1)
  val l1ipfc_readmiss       = PerFormanceCounter(io.update.L1I.read_miss,    2^64-1)
  val l1dpfc_read           = PerFormanceCounter(io.update.L1D.read,         2^64-1)
  val l1dpfc_readmiss       = PerFormanceCounter(io.update.L1D.read_miss,    2^64-1)
  val l1dpfc_write          = PerFormanceCounter(io.update.L1D.write,        2^64-1)
  val l1dpfc_writemiss      = PerFormanceCounter(io.update.L1D.write_miss,   2^64-1)
  val l1dpfc_writeback      = PerFormanceCounter(io.update.L1D.write_back,   2^64-1)

  val Reg_l1ipfc_read       = RegEnable(l1ipfc_read,       acquire)
  val Reg_l1ipfc_readmiss   = RegEnable(l1ipfc_readmiss,   acquire)
  val Reg_l1dpfc_read       = RegEnable(l1dpfc_read,       acquire)
  val Reg_l1dpfc_readmiss   = RegEnable(l1dpfc_readmiss,   acquire)
  val Reg_l1dpfc_write      = RegEnable(l1dpfc_write,      acquire)
  val Reg_l1dpfc_writemiss  = RegEnable(l1dpfc_writemiss,  acquire)
  val Reg_l1dpfc_writeback  = RegEnable(l1dpfc_writeback,  acquire)

  val l1ipfc = Reg_l1ipfc_read
  val l1dpfc = Reg_l1dpfc_read
  io.resp.valid  := read
  io.resp.bits.data := Mux(groupisL1I, l1ipfc, l1dpfc)
  switch(io.req.bits.addr) {
  /*  is(UInt(0)) {
      l1ipfc := Reg_l1ipfc_read
      l1dpfc := Reg_l1dpfc_read
   } */
    is(UInt(1)) {
      l1ipfc := Reg_l1ipfc_readmiss
      l1dpfc := Reg_l1dpfc_readmiss
    }
    is(UInt(2)) {
      l1dpfc := Reg_l1dpfc_write
    }
    is(UInt(3)) {
      l1dpfc := Reg_l1dpfc_writemiss
    }
    is(UInt(4)) {
      l1dpfc := Reg_l1dpfc_writeback
    }
 }
}

class SharePFC(implicit val p: Parameters) extends Module {
  val io = new Bundle {
    val req = Valid(new PFCReq().asInput) //req from csr
    val resp = Valid(new PFCResp().asOutput) //resp to csr
    val update = new SharePerform()
  }

  val L2Banks = p(PFCL2N)
  require(L2Banks<=8)

  val groupMatch = io.req.valid && io.req.bits.groupID === UInt(2) //4'b0010
  val groupisL2 = groupMatch && io.req.bits.subGroID === UInt(1) //4'b0001
  val groupisTC = groupMatch && io.req.bits.subGroID === UInt(2) //4'b0010
  val read = groupMatch && io.req.bits.cmd === UInt(0)
  val acquire = groupMatch && io.req.bits.cmd === UInt(1)

  //L2
  val l2pfcs     = (0 until L2Banks).map(i => Wire(new L2CachePerformCounter))
  val Reg_l2pfcs = (0 until L2Banks).map(i => Reg(new  L2CachePerformCounter))
    (0 until L2Banks).map(i =>{
    Reg_l2pfcs(i)         := RegEnable(l2pfcs(i), acquire)
    l2pfcs(i).read        := PerFormanceCounter(io.update.L2D(i).read.toBool(),        2^64-1)
    l2pfcs(i).read_miss   := PerFormanceCounter(io.update.L2D(i).read_miss.toBool(),   2^64-1)
    l2pfcs(i).write       := PerFormanceCounter(io.update.L2D(i).write.toBool(),       2^64-1)
    l2pfcs(i).write_back  := PerFormanceCounter(io.update.L2D(i).write_back.toBool(),  2^64-1)
  })

  //TC
  val tcttpfc = Wire(new TCTAGTrackerPerformCounter)
  val tcmtpfc = Wire(new TCMEMTrackerPerformCounter)
  val Reg_tcttpfc = RegEnable(tcttpfc, acquire)
  val Reg_tcmtpfc = RegEnable(tcmtpfc, acquire)
  tcttpfc.MR          := PerFormanceCounter(io.update.TAG.tcttp.MR.toBool(),          2^64-1)
  tcttpfc.DR          := PerFormanceCounter(io.update.TAG.tcttp.DR.toBool(),          2^64-1)
  tcttpfc.DW          := PerFormanceCounter(io.update.TAG.tcttp.DW.toBool(),          2^64-1)
  tcttpfc.WB          := PerFormanceCounter(io.update.TAG.tcttp.WB.toBool(),          2^64-1)
  tcttpfc.F           := PerFormanceCounter(io.update.TAG.tcttp.F.toBool(),           2^64-1)
  tcmtpfc.readTT          := PerFormanceCounter(io.update.TAG.tcmtp.readTT.toBool(),         2^64-1)
  tcmtpfc.readTT_miss     := PerFormanceCounter(io.update.TAG.tcmtp.readTT_miss.toBool(),    2^64-1)
  tcmtpfc.writeTT         := PerFormanceCounter(io.update.TAG.tcmtp.writeTT.toBool(),        2^64-1)
  tcmtpfc.writeTT_miss    := PerFormanceCounter(io.update.TAG.tcmtp.writeTT_miss.toBool(),   2^64-1)
  tcmtpfc.readTM0         := PerFormanceCounter(io.update.TAG.tcmtp.readTM0.toBool(),        2^64-1)
  tcmtpfc.readTM0_miss    := PerFormanceCounter(io.update.TAG.tcmtp.readTM0_miss.toBool(),   2^64-1)
  tcmtpfc.writeTM0        := PerFormanceCounter(io.update.TAG.tcmtp.writeTM0.toBool(),       2^64-1)
  tcmtpfc.writeTM0_miss   := PerFormanceCounter(io.update.TAG.tcmtp.writeTM0_miss.toBool(),  2^64-1)
  tcmtpfc.readTM1         := PerFormanceCounter(io.update.TAG.tcmtp.readTM1.toBool(),        2^64-1)
  tcmtpfc.readTM1_miss    := PerFormanceCounter(io.update.TAG.tcmtp.readTM1_miss.toBool(),   2^64-1)
  tcmtpfc.writeTM1        := PerFormanceCounter(io.update.TAG.tcmtp.writeTM1.toBool(),       2^64-1)
  tcmtpfc.writeTM1_miss   := PerFormanceCounter(io.update.TAG.tcmtp.writeTM1_miss.toBool(),  2^64-1)

  val tcpfc = Reg_tcmtpfc.readTT
  val l2pfc = if(L2Banks>0) Reg_l2pfcs(0).read else UInt(0)
  io.resp.valid  := read
  io.resp.bits.data := Mux(groupisL2, l2pfc, tcpfc)
  switch(io.req.bits.addr) {
    /*  is(UInt(0)) {
        tcpfc := Reg_tcmtpfc.readTT
        if(L2Banks>0) l2pfc := Reg_l2pfcs(0).read
     } */
    is(UInt(1)) {
      tcpfc := Reg_tcmtpfc.readTT_miss
      if (L2Banks > 0) l2pfc := Reg_l2pfcs(0).read_miss
    }
    is(UInt(2)) {
      tcpfc := Reg_tcmtpfc.writeTT
      if (L2Banks > 0) l2pfc := Reg_l2pfcs(0).write
    }
    is(UInt(3)) {
      tcpfc := Reg_tcmtpfc.writeTT_miss
      if (L2Banks > 0) l2pfc := Reg_l2pfcs(0).write_back
    }
    is(UInt(4)) {
      tcpfc := Reg_tcmtpfc.readTM0
      if (L2Banks > 1) l2pfc := Reg_l2pfcs(1).read
    }
    is(UInt(5)) {
      tcpfc := Reg_tcmtpfc.readTM0_miss
      if (L2Banks > 1) l2pfc := Reg_l2pfcs(1).read_miss
    }
    is(UInt(6)) {
      tcpfc := Reg_tcmtpfc.writeTM0
      if (L2Banks > 1) l2pfc := Reg_l2pfcs(1).write
    }
    is(UInt(7)) {
      tcpfc := Reg_tcmtpfc.writeTM0_miss
      if (L2Banks > 1) l2pfc := Reg_l2pfcs(1).write_back
    }
    is(UInt(8)) {
      tcpfc := Reg_tcmtpfc.readTM1
      if (L2Banks > 2) l2pfc := Reg_l2pfcs(2).read
    }
    is(UInt(9)) {
      tcpfc := Reg_tcmtpfc.readTM1_miss
      if (L2Banks > 2) l2pfc := Reg_l2pfcs(2).read_miss
    }
    is(UInt(10)) {
      tcpfc := Reg_tcmtpfc.writeTM1
      if (L2Banks > 2) l2pfc := Reg_l2pfcs(2).write
    }
    is(UInt(11)) {
      tcpfc := Reg_tcmtpfc.writeTM1_miss
      if (L2Banks > 2) l2pfc := Reg_l2pfcs(2).write_back
    }
    is(UInt(12)) {
      if (L2Banks > 3) l2pfc := Reg_l2pfcs(3).read
    }
    is(UInt(13)) {
      if (L2Banks > 3) l2pfc := Reg_l2pfcs(3).read_miss
    }
    is(UInt(14)) {
      if (L2Banks > 3) l2pfc := Reg_l2pfcs(3).write
    }
    is(UInt(15)) {
      if (L2Banks > 3) l2pfc := Reg_l2pfcs(3).write_back
    }
    is(UInt(16)) {
      if (L2Banks > 4) l2pfc := Reg_l2pfcs(4).read
    }
    is(UInt(17)) {
      if (L2Banks > 4) l2pfc := Reg_l2pfcs(4).read_miss
    }
    is(UInt(18)) {
      if (L2Banks > 4) l2pfc := Reg_l2pfcs(4).write
    }
    is(UInt(19)) {
      if (L2Banks > 4) l2pfc := Reg_l2pfcs(4).write_back
    }
    is(UInt(20)) {
      if (L2Banks > 5) l2pfc := Reg_l2pfcs(5).read
    }
    is(UInt(21)) {
      if (L2Banks > 5) l2pfc := Reg_l2pfcs(5).read_miss
    }
    is(UInt(22)) {
      if (L2Banks > 5) l2pfc := Reg_l2pfcs(5).write
    }
    is(UInt(23)) {
      if (L2Banks > 5) l2pfc := Reg_l2pfcs(5).write_back
    }
    is(UInt(24)) {
      if (L2Banks > 6) l2pfc := Reg_l2pfcs(6).read
    }
    is(UInt(25)) {
      if (L2Banks > 6) l2pfc := Reg_l2pfcs(6).read_miss
    }
    is(UInt(26)) {
      if (L2Banks > 6) l2pfc := Reg_l2pfcs(6).write
    }
    is(UInt(27)) {
      if (L2Banks > 6) l2pfc := Reg_l2pfcs(6).write_back
    }
    is(UInt(28)) {
      if (L2Banks > 7) l2pfc := Reg_l2pfcs(7).read
    }
    is(UInt(29)) {
      if (L2Banks > 7) l2pfc := Reg_l2pfcs(7).read_miss
    }
    is(UInt(30)) {
      if (L2Banks > 7) l2pfc := Reg_l2pfcs(7).write
    }
    is(UInt(31)) {
      if (L2Banks > 7) l2pfc := Reg_l2pfcs(7).write_back
    }
  }
}
